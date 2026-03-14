/*
 * dfu_flasher.cpp — DFU flash support via standalone helper exe
 *
 * Calls stm32prog/stm32_dfu_flash.exe via QProcess and parses its
 * structured stdout output (DEVICE, SWAP_BANK, STATUS, PROGRESS, OK, ERROR).
 */

#include "dfu_flasher.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>

static constexpr int SCAN_INTERVAL_MS = 2000;
static constexpr int FLASH_BANK_SIZE  = 0x100000;  // 1 MB

DfuFlasher::DfuFlasher(QObject *parent)
    : QObject(parent)
{
    m_helperPath = resolveHelperPath();

    m_scanTimer.setInterval(SCAN_INTERVAL_MS);
    connect(&m_scanTimer, &QTimer::timeout, this, &DfuFlasher::scanOnce);
}

DfuFlasher::~DfuFlasher()
{
    stopScanning();
    cancel();
}

// ── Path resolution ──

QString DfuFlasher::resolveHelperPath()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString path = QDir(appDir).filePath("stm32prog/stm32_dfu_flash.exe");
    if (QFile::exists(path))
        return path;

    return QString();
}

bool DfuFlasher::isToolAvailable()
{
    if (m_helperPath.isEmpty())
        m_helperPath = resolveHelperPath();
    return !m_helperPath.isEmpty();
}

// ── Scanning ──

void DfuFlasher::startScanning()
{
    if (m_scanning)
        return;
    m_scanning = true;
    emit scanningChanged(true);

    scanOnce();
    m_scanTimer.start();
}

void DfuFlasher::stopScanning()
{
    m_scanTimer.stop();
    if (m_scanProcess) {
        m_scanProcess->kill();
        m_scanProcess->waitForFinished(1000);
        m_scanProcess->deleteLater();
        m_scanProcess = nullptr;
    }
    if (m_scanning) {
        m_scanning = false;
        emit scanningChanged(false);
    }
}

void DfuFlasher::scanOnce()
{
    if (!isToolAvailable()) {
        setStatus("DFU flash tool not found. Check that stm32prog/ folder "
                  "exists next to qmonstatek.exe.");
        setDfuDeviceFound(false);
        return;
    }

    // Don't overlap scans
    if (m_scanProcess)
        return;

    m_scanProcess = new QProcess(this);
    connect(m_scanProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DfuFlasher::onScanFinished);

    m_scanProcess->start(m_helperPath, QStringList() << "scan");
}

void DfuFlasher::onScanFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);

    if (!m_scanProcess)
        return;

    QString output = QString::fromUtf8(m_scanProcess->readAllStandardOutput());
    m_scanProcess->deleteLater();
    m_scanProcess = nullptr;

    // Parse DEVICE line
    // Format: DEVICE:<usb_index>|<serial>|<product>  or  DEVICE:none
    for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith("DEVICE:none")) {
            setDfuDeviceFound(false);
            if (!m_flashing)
                setStatus("No DFU device found. Follow the steps above to enter DFU mode.");
            return;
        }

        if (trimmed.startsWith("DEVICE:")) {
            QString info = trimmed.mid(7);  // after "DEVICE:"
            QStringList parts = info.split('|');
            QString display = "STM32 DFU device detected";
            if (parts.size() >= 2)
                display = QString("STM32 DFU — USB: %1, SN: %2")
                              .arg(parts[0], parts[1]);

            setDfuDeviceFound(true, display);
            setStatus("STM32 DFU device detected.");
            return;
        }

        if (trimmed.startsWith("ERROR:")) {
            setDfuDeviceFound(false);
            setStatus(trimmed.mid(6));
            return;
        }
    }

    // No DEVICE line found
    setDfuDeviceFound(false);
    if (!m_flashing)
        setStatus("No DFU device found.");
}

// ── Flashing ──

void DfuFlasher::startFlash(const QString &binFilePath, const QString &target)
{
    if (m_flashing) {
        emit flashError("A flash operation is already in progress.");
        return;
    }

    if (!isToolAvailable()) {
        emit flashError("DFU flash tool not found. Check that stm32prog/ folder "
                        "exists next to qmonstatek.exe.");
        return;
    }

    if (!m_dfuDeviceFound) {
        emit flashError("No DFU device detected. Enter DFU mode first.");
        return;
    }

    // Validate file
    QFileInfo fi(binFilePath);
    if (!fi.exists() || !fi.isFile()) {
        emit flashError("Firmware file not found: " + binFilePath);
        return;
    }

    qint64 fileSize = fi.size();
    if (fileSize > FLASH_BANK_SIZE + 4) {
        emit flashError(QString("Firmware file is too large (%1 KB). "
                                "The M1 flash bank is 1 MB.")
                            .arg(fileSize / 1024));
        return;
    }

    if (fileSize < 1024) {
        emit flashError("Firmware file is suspiciously small. Please verify the file.");
        return;
    }

    // Stop scanning during flash
    stopScanning();

    m_flashing = true;
    m_flashOk  = false;
    m_progress = 0;
    emit flashingChanged(true);
    emit progressChanged(0);
    setStatus("Starting DFU flash...");

    m_flashProcess = new QProcess(this);
    m_flashProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_flashProcess, &QProcess::readyRead,
            this, &DfuFlasher::onFlashOutput);
    connect(m_flashProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DfuFlasher::onFlashFinished);

    QStringList args;
    args << "flash" << QDir::toNativeSeparators(binFilePath) << target;

    qInfo() << "DFU flash command:" << m_helperPath << args;
    m_flashProcess->start(m_helperPath, args);
}

void DfuFlasher::onFlashOutput()
{
    if (!m_flashProcess)
        return;

    QByteArray raw = m_flashProcess->readAll();
    QString chunk = QString::fromUtf8(raw);

    for (const QString &part : chunk.split('\n', Qt::SkipEmptyParts)) {
        QString line = part.trimmed();
        if (line.isEmpty())
            continue;

        if (line.startsWith("PROGRESS:")) {
            int pct = line.mid(9).toInt();
            if (pct != m_progress) {
                m_progress = pct;
                emit progressChanged(m_progress);
            }
        } else if (line.startsWith("STATUS:")) {
            setStatus(line.mid(7));
        } else if (line.startsWith("SWAP_BANK:")) {
            int val = line.mid(10).toInt();
            if (val == 1)
                setStatus("SWAP_BANK is active — clearing for safe flash...");
        } else if (line == "OK") {
            m_flashOk = true;
        } else if (line.startsWith("LOG:")) {
            qDebug() << "CubeProg:" << line.mid(4);
        } else if (line.startsWith("ERROR:")) {
            // Will be handled in onFlashFinished
            qWarning() << "DFU helper:" << line;
        }
    }
}

void DfuFlasher::onFlashFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Read any remaining output
    QString remaining;
    if (m_flashProcess) {
        remaining = QString::fromUtf8(m_flashProcess->readAll());
        m_flashProcess->deleteLater();
        m_flashProcess = nullptr;
    }

    m_flashing = false;
    emit flashingChanged(false);

    if (exitStatus == QProcess::CrashExit) {
        setStatus("DFU flash helper crashed.");
        emit flashError("The DFU flash process crashed. Check USB connection and try again.");
        startScanning();
        return;
    }

    // Check for OK or ERROR in remaining output (may already be caught by onFlashOutput)
    QString allOutput = remaining;
    bool success = m_flashOk;
    QString errorMsg;

    for (const QString &part : allOutput.split('\n', Qt::SkipEmptyParts)) {
        QString line = part.trimmed();
        if (line == "OK")
            success = true;
        else if (line.startsWith("ERROR:"))
            errorMsg = line.mid(6);
    }

    if (exitCode == 0 && success) {
        m_progress = 100;
        emit progressChanged(100);
        setStatus("Flash complete! Your M1 will reboot into the new firmware.");
        setDfuDeviceFound(false);
        emit flashComplete();
    } else {
        if (errorMsg.isEmpty())
            errorMsg = "Flash failed (exit code " + QString::number(exitCode) + ").";
        setStatus(errorMsg);
        emit flashError(errorMsg);
    }

    startScanning();
}

// ── Swap Bank ──

void DfuFlasher::swapBank()
{
    if (m_flashing || m_swapProcess) {
        emit swapBankError("Another operation is in progress.");
        return;
    }

    if (!isToolAvailable()) {
        emit swapBankError("DFU flash tool not found.");
        return;
    }

    if (!m_dfuDeviceFound) {
        emit swapBankError("No DFU device detected. Enter DFU mode first.");
        return;
    }

    stopScanning();

    m_swapProcess = new QProcess(this);
    m_swapProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_swapProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DfuFlasher::onSwapBankFinished);

    setStatus("Swapping flash banks...");

    qInfo() << "Swap bank command:" << m_helperPath << "swapbank";
    m_swapProcess->start(m_helperPath, QStringList() << "swapbank");
}

void DfuFlasher::onSwapBankFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString output;
    if (m_swapProcess) {
        output = QString::fromUtf8(m_swapProcess->readAll());
        m_swapProcess->deleteLater();
        m_swapProcess = nullptr;
    }

    if (exitStatus == QProcess::CrashExit) {
        setStatus("Swap bank process crashed.");
        emit swapBankError("The swap bank process crashed.");
        startScanning();
        return;
    }

    bool success = false;
    QString errorMsg;
    QString statusMsg;

    for (const QString &part : output.split('\n', Qt::SkipEmptyParts)) {
        QString line = part.trimmed();
        if (line == "OK")
            success = true;
        else if (line.startsWith("ERROR:"))
            errorMsg = line.mid(6);
        else if (line.startsWith("STATUS:"))
            statusMsg = line.mid(7);
    }

    if (exitCode == 0 && success) {
        QString msg = statusMsg.isEmpty()
                          ? "Bank swap complete! Device will reboot."
                          : statusMsg;
        setStatus(msg);
        setDfuDeviceFound(false);
        emit swapBankComplete(msg);
    } else {
        if (errorMsg.isEmpty())
            errorMsg = "Bank swap failed (exit code " + QString::number(exitCode) + ").";
        setStatus(errorMsg);
        emit swapBankError(errorMsg);
    }

    startScanning();
}

// ── Cancel ──

void DfuFlasher::cancel()
{
    if (m_flashProcess) {
        m_flashProcess->kill();
        m_flashProcess->waitForFinished(2000);
        m_flashProcess->deleteLater();
        m_flashProcess = nullptr;
    }

    if (m_flashing) {
        m_flashing = false;
        m_progress = 0;
        emit flashingChanged(false);
        emit progressChanged(0);
        setStatus("Flash cancelled.");
        startScanning();
    }
}

// ── Helpers ──

void DfuFlasher::setStatus(const QString &msg)
{
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged(msg);
    }
}

void DfuFlasher::setDfuDeviceFound(bool found, const QString &info)
{
    bool changed = (m_dfuDeviceFound != found);
    m_dfuDeviceFound = found;
    if (!info.isEmpty())
        m_dfuDeviceInfo = info;
    else if (!found)
        m_dfuDeviceInfo.clear();

    if (changed)
        emit dfuDeviceFoundChanged(found);
}
