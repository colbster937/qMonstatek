/*
 * dfu_flasher.h — DFU flash support via standalone helper exe
 *
 * Calls stm32_dfu_flash.exe (which loads CubeProgrammer_API.dll) to
 * detect STM32 DFU devices, handle SWAP_BANK, and flash firmware.
 * The helper runs as a separate process to avoid Qt DLL version conflicts.
 */

#ifndef DFU_FLASHER_H
#define DFU_FLASHER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QString>

class DfuFlasher : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool dfuDeviceFound READ isDfuDeviceFound NOTIFY dfuDeviceFoundChanged)
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(bool flashing READ isFlashing NOTIFY flashingChanged)
    Q_PROPERTY(int  progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString dfuDeviceInfo READ dfuDeviceInfo NOTIFY dfuDeviceFoundChanged)

public:
    explicit DfuFlasher(QObject *parent = nullptr);
    ~DfuFlasher() override;

    bool isDfuDeviceFound() const { return m_dfuDeviceFound; }
    bool isScanning()       const { return m_scanning; }
    bool isFlashing()       const { return m_flashing; }
    int  progress()         const { return m_progress; }
    QString statusMessage() const { return m_statusMessage; }
    QString dfuDeviceInfo() const { return m_dfuDeviceInfo; }

    Q_INVOKABLE void startScanning();
    Q_INVOKABLE void stopScanning();
    Q_INVOKABLE void scanOnce();
    Q_INVOKABLE void startFlash(const QString &binFilePath,
                                const QString &target = QStringLiteral("inactive"));
    Q_INVOKABLE void swapBank();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE bool isToolAvailable();

signals:
    void dfuDeviceFoundChanged(bool found);
    void scanningChanged(bool scanning);
    void flashingChanged(bool flashing);
    void progressChanged(int percent);
    void statusMessageChanged(const QString &msg);
    void flashComplete();
    void flashError(const QString &message);
    void swapBankComplete(const QString &message);
    void swapBankError(const QString &message);

private slots:
    void onScanFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onFlashOutput();
    void onFlashFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onSwapBankFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString resolveHelperPath();
    void setStatus(const QString &msg);
    void setDfuDeviceFound(bool found, const QString &info = QString());

    QProcess *m_scanProcess    = nullptr;
    QProcess *m_flashProcess   = nullptr;
    QProcess *m_swapProcess    = nullptr;
    QTimer    m_scanTimer;

    bool    m_dfuDeviceFound = false;
    bool    m_scanning       = false;
    bool    m_flashing       = false;
    bool    m_flashOk        = false;
    int     m_progress       = 0;
    QString m_statusMessage;
    QString m_dfuDeviceInfo;
    QString m_helperPath;
};

#endif // DFU_FLASHER_H
