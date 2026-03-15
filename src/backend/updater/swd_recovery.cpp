/*
 * swd_recovery.cpp — SWD recovery tools via OpenOCD
 *
 * Locates OpenOCD (bundled with STM32CubeIDE or standalone), builds
 * command strings for each operation, and runs them via QProcess.
 * Output is captured and parsed for progress/status reporting.
 */

#include "swd_recovery.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDebug>

static constexpr int FLASH_BANK_SIZE = 0x100000;  // 1 MB

SwdRecovery::SwdRecovery(QObject *parent)
    : QObject(parent)
{
    m_ocdPath = resolveOpenOcdPath();
    if (!m_ocdPath.isEmpty())
        m_scriptsPath = resolveScriptsPath(m_ocdPath);
}

SwdRecovery::~SwdRecovery()
{
    cancel();
}

// ── Path resolution ──────────────────────────────────────────────

QString SwdRecovery::resolveOpenOcdPath()
{
    // 1. App-local: <app>/openocd/bin/openocd.exe
    QString appDir = QCoreApplication::applicationDirPath();
    QString localPath = QDir(appDir).filePath("openocd/bin/openocd.exe");
    if (QFile::exists(localPath))
        return localPath;

    // 2. STM32CubeIDE installations under C:/ST/
    QDir stDir("C:/ST");
    if (stDir.exists()) {
        // Prefer newest IDE version (reverse sort)
        QStringList ideEntries = stDir.entryList(
            QStringList() << "STM32CubeIDE*", QDir::Dirs, QDir::Name | QDir::Reversed);
        for (const QString &ideEntry : ideEntries) {
            QDir pluginsDir(stDir.filePath(ideEntry) + "/STM32CubeIDE/plugins");
            if (!pluginsDir.exists())
                continue;
            QStringList ocdPlugins = pluginsDir.entryList(
                QStringList() << "com.st.stm32cube.ide.mcu.externaltools.openocd*",
                QDir::Dirs);
            for (const QString &plugin : ocdPlugins) {
                QString ocdPath = pluginsDir.filePath(plugin) + "/tools/bin/openocd.exe";
                if (QFile::exists(ocdPath))
                    return ocdPath;
            }
        }
    }

    // 3. System PATH
    QString pathOcd = QStandardPaths::findExecutable("openocd");
    if (!pathOcd.isEmpty())
        return pathOcd;

    return QString();
}

QString SwdRecovery::resolveScriptsPath(const QString &ocdPath)
{
    QDir binDir = QFileInfo(ocdPath).absoluteDir();  // .../bin/

    // STM32CubeIDE layout (older): tools/bin/ -> tools/resources/openocd/st_scripts/
    QDir toolsDir(binDir);
    toolsDir.cdUp();
    QString stScripts = toolsDir.filePath("resources/openocd/st_scripts");
    if (QDir(stScripts).exists())
        return stScripts;

    // STM32CubeIDE layout (2.1+): scripts are in a separate debug.openocd plugin
    // Binary is in: plugins/com.st.stm32cube.ide.mcu.externaltools.openocd.*/tools/bin/
    // Scripts are in: plugins/com.st.stm32cube.ide.mcu.debug.openocd_*/resources/openocd/st_scripts/
    QDir pluginsDir(binDir);
    // Navigate up: bin/ -> tools/ -> <plugin>/ -> plugins/
    if (pluginsDir.cdUp() && pluginsDir.cdUp() && pluginsDir.cdUp()) {
        QStringList debugPlugins = pluginsDir.entryList(
            QStringList() << "com.st.stm32cube.ide.mcu.debug.openocd*",
            QDir::Dirs);
        for (const QString &dp : debugPlugins) {
            QString dpScripts = pluginsDir.filePath(dp) +
                                "/resources/openocd/st_scripts";
            if (QDir(dpScripts).exists())
                return dpScripts;
        }
    }

    // Standard OpenOCD layout: bin/ -> ../share/openocd/scripts/
    QDir parentDir(binDir);
    parentDir.cdUp();
    QString shareScripts = parentDir.filePath("share/openocd/scripts");
    if (QDir(shareScripts).exists())
        return shareScripts;

    // App-local layout: openocd/bin/ -> openocd/scripts/
    QDir appOcdDir(binDir);
    appOcdDir.cdUp();
    QString localScripts = appOcdDir.filePath("scripts");
    if (QDir(localScripts).exists())
        return localScripts;

    return QString();
}

QString SwdRecovery::interfaceConfig() const
{
    switch (m_probeType) {
    case StLinkV2: return QStringLiteral("interface/stlink.cfg");
    default:       return QStringLiteral("interface/cmsis-dap.cfg");
    }
}

bool SwdRecovery::isOpenOcdAvailable()
{
    if (m_ocdPath.isEmpty())
        m_ocdPath = resolveOpenOcdPath();
    if (!m_ocdPath.isEmpty() && m_scriptsPath.isEmpty())
        m_scriptsPath = resolveScriptsPath(m_ocdPath);
    return !m_ocdPath.isEmpty() && !m_scriptsPath.isEmpty();
}

QString SwdRecovery::openOcdLocation()
{
    return m_ocdPath;
}

void SwdRecovery::setProbeType(int type)
{
    if (m_probeType != type) {
        m_probeType = type;
        emit probeTypeChanged(type);
    }
}

// ── Operations ───────────────────────────────────────────────────

void SwdRecovery::recoveryFlash(const QString &binFilePath)
{
    if (m_running) {
        emit operationError("An operation is already in progress.");
        return;
    }

    QFileInfo fi(binFilePath);
    if (!fi.exists()) {
        emit operationError("Firmware file not found: " + binFilePath);
        return;
    }
    if (fi.size() > FLASH_BANK_SIZE + 4) {
        emit operationError(QString("Firmware file is too large (%1 KB). "
                                    "The M1 flash bank is 1 MB.").arg(fi.size() / 1024));
        return;
    }

    // Use forward slashes — OpenOCD's Tcl parser treats \ as escape chars
    QString path = fi.absoluteFilePath().replace('\\', '/');

    // Clear SWAP_BANK first so 0x08000000 always = physical bank 1,
    // then erase and program bank 1. This ensures recovery works
    // regardless of which bank was previously active.
    //
    // STM32H573 flash registers (non-secure base 0x40022000):
    //   NSKEYR     = 0x40022008  (flash unlock keys)
    //   OPTKEYR    = 0x4002200C  (option byte unlock keys)
    //   OPTCR      = 0x4002201C  (OPTSTRT = bit 1)
    //   OPTSR_CUR  = 0x40022050  (SWAP_BANK = bit 31)
    //   OPTSR_PRG  = 0x40022054
    QString cmds = QString(
        "adapter speed 2000; "
        "init; "
        "halt; "
        "mww 0x40022008 0x45670123; "        // NSKEYR unlock key 1
        "mww 0x40022008 0xCDEF89AB; "        // NSKEYR unlock key 2
        "mww 0x4002200C 0x08192A3B; "        // OPTKEYR unlock key 1
        "mww 0x4002200C 0x4C5D6E7F; "        // OPTKEYR unlock key 2
        "set optr [mrw 0x40022050]; "         // read OPTSR_CUR
        "set cleared [expr {$optr & 0x7FFFFFFF}]; " // clear SWAP_BANK bit
        "mww 0x40022054 $cleared; "           // write OPTSR_PRG
        "mww 0x4002201C 0x00000002; "         // OPTCR = OPTSTRT (bit 1)
        "sleep 500; "                         // wait for OBL reset
        "halt; "                              // re-halt after reset
        "stm32h5x mass_erase 0; "
        "program {%1} 0x08000000 verify reset exit"
    ).arg(path);

    runOpenOcd(cmds, "Recovery Flash");
}

void SwdRecovery::swapBank()
{
    if (m_running) {
        emit operationError("An operation is already in progress.");
        return;
    }

    // Unlock flash + option bytes, read OPTR, toggle SWAP_BANK (bit 31), launch, reset
    QString cmds =
        "adapter speed 2000; init; "
        "halt; "
        "mww 0x40022008 0x45670123; "        // NSKEYR unlock key 1
        "mww 0x40022008 0xCDEF89AB; "        // NSKEYR unlock key 2
        "mww 0x4002200C 0x08192A3B; "        // OPTKEYR unlock key 1
        "mww 0x4002200C 0x4C5D6E7F; "        // OPTKEYR unlock key 2
        "set val [mrw 0x40022050]; "          // read OPTSR_CUR
        "set new [expr {$val ^ 0x80000000}]; " // toggle bit 31
        "mww 0x40022054 $new; "               // write OPTSR_PRG
        "mww 0x4002201C 0x00000002; "         // OPTCR = OPTSTRT (bit 1)
        "sleep 500; exit";

    runOpenOcd(cmds, "Swap Bank");
}

void SwdRecovery::verifyBank2(const QString &binFilePath)
{
    if (m_running) {
        emit operationError("An operation is already in progress.");
        return;
    }

    QFileInfo fi(binFilePath);
    if (!fi.exists()) {
        emit operationError("Firmware file not found: " + binFilePath);
        return;
    }

    QString path = fi.absoluteFilePath().replace('\\', '/');

    // Check SWAP_BANK to find physical bank 2's address:
    //   SWAP_BANK=0: bank 2 at 0x08100000
    //   SWAP_BANK=1: bank 2 at 0x08000000 (swapped)
    QString cmds = QString(
        "adapter speed 2000; init; "
        "halt; "
        "set optr [mrw 0x40022050]; "
        "if {$optr & 0x80000000} {"
        "  verify_image {%1} 0x08000000"
        "} else {"
        "  verify_image {%1} 0x08100000"
        "}; "
        "exit"
    ).arg(path);

    runOpenOcd(cmds, "Verify Bank 2");
}

void SwdRecovery::cloneBank1ToBank2()
{
    if (m_running) {
        emit operationError("An operation is already in progress.");
        return;
    }

    QString tempFile = QString(QDir::tempPath() + "/m1_swd_bank1_backup.bin")
                           .replace('\\', '/');

    // Check SWAP_BANK to find correct addresses:
    //   SWAP_BANK=0: bank 1 at 0x08000000, bank 2 at 0x08100000
    //   SWAP_BANK=1: bank 1 at 0x08100000, bank 2 at 0x08000000
    // Use reset halt (not just halt) to ensure all peripherals/DMA are
    // stopped before the 1MB flash read — prevents bus access conflicts.
    QString cmds = QString(
        "adapter speed 2000; init; "
        "reset halt; "
        "set optr [mrw 0x40022050]; "
        "if {$optr & 0x80000000} {"
        "  set bank1 0x08100000; set bank2 0x08000000"
        "} else {"
        "  set bank1 0x08000000; set bank2 0x08100000"
        "}; "
        "dump_image {%1} $bank1 0x100000; "
        "program {%1} $bank2 verify; "
        "reset; exit"
    ).arg(tempFile);

    runOpenOcd(cmds, "Clone Bank 1 to Bank 2");
}

void SwdRecovery::readStatus()
{
    if (m_running) {
        emit operationError("An operation is already in progress.");
        return;
    }

    runOpenOcd("adapter speed 2000; init; "
               "halt; "
               "mdw 0x40022050; exit",  // OPTSR_CUR at 0x40022050
               "Read Status");
}

// ── Process management ───────────────────────────────────────────

void SwdRecovery::runOpenOcd(const QString &commands, const QString &opName)
{
    if (!isOpenOcdAvailable()) {
        emit operationError(
            "OpenOCD not found. Install STM32CubeIDE or place OpenOCD "
            "in the openocd/ folder next to qmonstatek.exe.");
        return;
    }

    clearLog();
    m_currentOp = opName;
    m_running   = true;
    m_progress  = 0;
    emit runningChanged(true);
    emit progressChanged(0);
    setStatus(opName + "...");

    appendLog("=== " + opName + " ===\n");
    appendLog("Probe: " +
              QString(m_probeType == StLinkV2 ? "ST-Link V2" : "Pico CMSIS-DAP") + "\n");
    appendLog("OpenOCD: " + m_ocdPath + "\n\n");

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    // Run OpenOCD from its own bin directory so it finds libusb DLLs
    m_process->setWorkingDirectory(QFileInfo(m_ocdPath).absolutePath());

    connect(m_process, &QProcess::readyRead,
            this, &SwdRecovery::onProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SwdRecovery::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, [this](QProcess::ProcessError err) {
        QString msg;
        switch (err) {
        case QProcess::FailedToStart:
            msg = "OpenOCD failed to start. Check that the executable exists and "
                  "required DLLs (libusb-1.0.dll) are present.";
            break;
        case QProcess::Timedout:
            msg = "OpenOCD timed out.";
            break;
        default:
            msg = "OpenOCD process error: " + QString::number(err);
            break;
        }
        appendLog("\nERROR: " + msg + "\n");
        if (m_running) {
            m_running = false;
            emit runningChanged(false);
            setStatus(m_currentOp + " -- " + msg);
            emit operationError(msg);
        }
    });

    // Use forward slashes — OpenOCD's Tcl parser treats \ as escape
    QString scriptsPathFwd = QString(m_scriptsPath).replace('\\', '/');

    QStringList args;
    args << "-s" << scriptsPathFwd
         << "-f" << interfaceConfig()
         << "-f" << "target/stm32h5x.cfg"
         << "-c" << commands;

    // Log the full command for debugging
    appendLog("Command: " + m_ocdPath + "\n");
    for (const QString &arg : args)
        appendLog("  " + arg + "\n");
    appendLog("\n");

    qInfo() << "SWD:" << m_ocdPath << args;
    m_process->start(m_ocdPath, args);
}

void SwdRecovery::onProcessOutput()
{
    if (!m_process)
        return;

    QString text = QString::fromUtf8(m_process->readAll());
    appendLog(text);

    // Parse progress indicators from OpenOCD output
    for (const QString &part : text.split('\n', Qt::SkipEmptyParts)) {
        QString line = part.trimmed();

        if (line.contains("Programming Started")) {
            m_progress = 25;
            emit progressChanged(m_progress);
            setStatus(m_currentOp + " -- programming...");
        } else if (line.contains("Programming Finished")) {
            m_progress = 50;
            emit progressChanged(m_progress);
            setStatus(m_currentOp + " -- verifying...");
        } else if (line.contains("Verify Started")) {
            m_progress = 60;
            emit progressChanged(m_progress);
        } else if (line.contains("Verified OK") || line.contains("verified")) {
            m_progress = 90;
            emit progressChanged(m_progress);
            setStatus(m_currentOp + " -- verified OK");
        } else if (line.contains("Resetting Target")) {
            m_progress = 95;
            emit progressChanged(m_progress);
        } else if (line.contains("dumped")) {
            m_progress = 40;
            emit progressChanged(m_progress);
            setStatus(m_currentOp + " -- bank 1 dumped, programming bank 2...");
        }

        // Parse OPTSR_CUR register value for Read Status
        if (m_currentOp == "Read Status" && line.contains("0x40022050")) {
            static QRegularExpression re("0x40022050\\s*:?\\s*([0-9a-fA-F]{8})");
            auto match = re.match(line);
            if (match.hasMatch()) {
                bool ok;
                quint32 optr = match.captured(1).toUInt(&ok, 16);
                if (ok) {
                    bool swapBankSet = (optr & 0x80000000) != 0;
                    QString bankInfo = swapBankSet
                        ? "SWAP_BANK = 1 : Bank 2 is active (mapped to 0x08000000)"
                        : "SWAP_BANK = 0 : Bank 1 is active (mapped to 0x08000000)";
                    appendLog("\n" + bankInfo + "\n");
                    setStatus(bankInfo);
                }
            }
        }
    }
}

void SwdRecovery::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Read any remaining output
    if (m_process) {
        QString remaining = QString::fromUtf8(m_process->readAll());
        if (!remaining.isEmpty())
            appendLog(remaining);
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_running = false;
    emit runningChanged(false);

    if (exitStatus == QProcess::CrashExit) {
        setStatus(m_currentOp + " -- OpenOCD crashed.");
        emit operationError("OpenOCD process crashed. Check probe connection and try again.");
        return;
    }

    if (exitCode == 0) {
        m_progress = 100;
        emit progressChanged(100);
        QString msg = m_currentOp + " -- complete.";
        setStatus(msg);
        appendLog("\n" + msg + "\n");
        emit operationComplete(msg);
    } else {
        QString msg = m_currentOp + " failed (exit code " + QString::number(exitCode) + ").";

        // Add hints for common errors
        if (m_outputLog.contains("unable to find") ||
            m_outputLog.contains("Error connecting DP")) {
            msg += " Check that the probe is connected and the M1 is powered.";
        }
        if (m_outputLog.contains("No CMSIS-DAP") ||
            m_outputLog.contains("no device found")) {
            msg += " No debug probe detected. Verify USB connection.";
        }
        if (m_outputLog.contains("LIBUSB_ERROR")) {
            msg += " USB driver issue. Try reinstalling the probe driver.";
        }

        setStatus(msg);
        appendLog("\n" + msg + "\n");
        emit operationError(msg);
    }
}

// ── Cancel ───────────────────────────────────────────────────────

void SwdRecovery::cancel()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
        m_process->deleteLater();
        m_process = nullptr;
    }

    if (m_running) {
        m_running  = false;
        m_progress = 0;
        emit runningChanged(false);
        emit progressChanged(0);
        setStatus("Operation cancelled.");
        appendLog("\nOperation cancelled.\n");
    }
}

// ── Helpers ──────────────────────────────────────────────────────

void SwdRecovery::setStatus(const QString &msg)
{
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged(msg);
    }
}

void SwdRecovery::appendLog(const QString &text)
{
    m_outputLog += text;
    emit outputLogChanged();
}

void SwdRecovery::clearLog()
{
    m_outputLog.clear();
    emit outputLogChanged();
}
