/*
 * stm32_dfu_flash.c — Standalone STM32 DFU flash helper
 *
 * Loads CubeProgrammer_API.dll at runtime to detect STM32 DFU devices,
 * handle SWAP_BANK option byte, and flash firmware.
 *
 * This is a separate process so its CubeProgrammer DLL dependencies
 * (including Qt 6.6) don't conflict with qMonstatek's Qt 6.4.
 *
 * Stdout protocol (line-based, flushed after each line):
 *   DEVICE:<usb_index>|<serial>|<product>
 *   DEVICE:none
 *   SWAP_BANK:<0|1|-1>
 *   STATUS:<message>
 *   PROGRESS:<0-100>
 *   LOG:<level>:<message>
 *   OK
 *   ERROR:<message>
 *
 * Usage:
 *   stm32_dfu_flash.exe scan
 *   stm32_dfu_flash.exe flash <firmware.bin>
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Unbuffered output ── */
#undef OUT
#define OUT(fmt, ...) do { printf(fmt "\n", ##__VA_ARGS__); fflush(stdout); } while(0)

/* ════════════════════════════════════════════════════════════════════════════
 * CubeProgrammer API types — layout-compatible copies of the structs from
 * CubeProgrammer_API.h / DeviceDataStructure.h.  Defined locally so this
 * exe has zero compile-time dependency on the CubeProgrammer installation.
 * ════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    void (*initProgressBar)(void);
    void (*logMessage)(int msgType, const wchar_t *str);
    void (*loadBar)(int current, int total);
} CP_DisplayCallBacks;

typedef struct {
    char usbIndex[10];
    int  busNumber;
    int  addressNumber;
    char productId[100];
    char serialNumber[100];
    unsigned int dfuVersion;
} CP_DfuDeviceInfo;

typedef struct {
    unsigned short deviceId;
    int  flashSize;
    int  bootloaderVersion;
    char type[4];
    char cpu[20];
    char name[100];
    char series[100];
    char description[150];
    char revisionId[8];
    char board[100];
} CP_GeneralInf;

/* Option byte tree: peripheral → banks → categories → bits */
typedef struct { unsigned int multiplier; unsigned int offset; } CP_BitCoeff;
typedef struct { unsigned int value; char description[1000]; }   CP_BitValue;

typedef struct {
    char          name[64];
    char          description[1000];
    unsigned int  wordOffset;
    unsigned int  bitOffset;
    unsigned int  bitWidth;
    unsigned char access;
    unsigned int  valuesNbr;
    CP_BitValue **values;
    CP_BitCoeff   equation;
    unsigned char *reference;
    unsigned int  bitValue;
    unsigned int  valLine;
} CP_Bit;

typedef struct {
    char          name[100];
    unsigned int  bitsNbr;
    CP_Bit      **bits;
} CP_Category;

typedef struct {
    unsigned int   size;
    unsigned int   address;
    unsigned char  access;
    unsigned int   categoriesNbr;
    CP_Category  **categories;
} CP_Bank;

typedef struct {
    char          name[64];
    char          description[1000];
    unsigned int  banksNbr;
    CP_Bank     **banks;
} CP_Peripheral;

/* ════════════════════════════════════════════════════════════════════════════
 * Function pointer types
 * ════════════════════════════════════════════════════════════════════════════ */

typedef void          (*fn_setDisplayCallbacks_t)(CP_DisplayCallBacks);
typedef void          (*fn_setVerbosityLevel_t)(int);
typedef void          (*fn_setLoadersPath_t)(const char *);
typedef int           (*fn_getDfuDeviceList_t)(CP_DfuDeviceInfo **, int, int);
typedef int           (*fn_connectDfuBootloader_t)(char *);
typedef void          (*fn_disconnect_t)(void);
typedef void          (*fn_deleteInterfaceList_t)(void);
typedef int           (*fn_downloadFile_t)(const wchar_t *, unsigned int,
                                          unsigned int, unsigned int,
                                          const wchar_t *);
typedef int           (*fn_execute_t)(unsigned int);
typedef int           (*fn_sendOptionBytesCmd_t)(char *);
typedef CP_Peripheral*(*fn_initOptionBytesInterface_t)(void);
typedef CP_GeneralInf*(*fn_getDeviceGeneralInf_t)(void);

/* ── Resolved pointers ── */
static fn_setDisplayCallbacks_t      p_setDisplayCallbacks;
static fn_setVerbosityLevel_t        p_setVerbosityLevel;
static fn_setLoadersPath_t           p_setLoadersPath;
static fn_getDfuDeviceList_t         p_getDfuDeviceList;
static fn_connectDfuBootloader_t     p_connectDfuBootloader;
static fn_disconnect_t               p_disconnect;
static fn_deleteInterfaceList_t      p_deleteInterfaceList;
static fn_downloadFile_t             p_downloadFile;
static fn_execute_t                  p_execute;
static fn_sendOptionBytesCmd_t       p_sendOptionBytesCmd;
static fn_initOptionBytesInterface_t p_initOptionBytesInterface;
static fn_getDeviceGeneralInf_t      p_getDeviceGeneralInf;

/* ── Exe directory ── */
static char g_exeDir[MAX_PATH];

/* ════════════════════════════════════════════════════════════════════════════
 * Display callbacks — forward to stdout protocol
 * ════════════════════════════════════════════════════════════════════════════ */

static void cb_initProgressBar(void) { /* no-op */ }

static void cb_logMessage(int msgType, const wchar_t *str)
{
    char buf[4096];
    WideCharToMultiByte(CP_UTF8, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    /* trim trailing whitespace */
    int len = (int)strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r' || buf[len-1] == ' '))
        buf[--len] = '\0';
    if (len > 0)
        OUT("LOG:%d:%s", msgType, buf);
}

static void cb_loadBar(int current, int total)
{
    int pct = (total > 0) ? (current * 100 / total) : 0;
    OUT("PROGRESS:%d", pct);
}

/* ════════════════════════════════════════════════════════════════════════════
 * Library loading
 * ════════════════════════════════════════════════════════════════════════════ */

static void getExeDir(void)
{
    GetModuleFileNameA(NULL, g_exeDir, MAX_PATH);
    char *last = strrchr(g_exeDir, '\\');
    if (last) *last = '\0';
}

static HMODULE loadCubeProgAPI(void)
{
    char dllPath[MAX_PATH];
    snprintf(dllPath, MAX_PATH, "%s\\CubeProgrammer_API.dll", g_exeDir);

    /* Tell Windows to search our directory for dependency DLLs */
    SetDllDirectoryA(g_exeDir);

    HMODULE h = LoadLibraryA(dllPath);
    if (!h) {
        OUT("ERROR:Failed to load CubeProgrammer_API.dll (Windows error %lu). "
            "Make sure the CubeProgrammer DLLs are in: %s", GetLastError(), g_exeDir);
        return NULL;
    }

#define RESOLVE(name)                                                         \
    p_##name = (fn_##name##_t)GetProcAddress(h, #name);                       \
    if (!p_##name) {                                                          \
        OUT("ERROR:CubeProgrammer_API.dll missing function: " #name);         \
        FreeLibrary(h);                                                       \
        return NULL;                                                          \
    }

    RESOLVE(setDisplayCallbacks);
    RESOLVE(setVerbosityLevel);
    RESOLVE(setLoadersPath);
    RESOLVE(getDfuDeviceList);
    RESOLVE(connectDfuBootloader);
    RESOLVE(disconnect);
    RESOLVE(deleteInterfaceList);
    RESOLVE(downloadFile);
    RESOLVE(execute);
    RESOLVE(sendOptionBytesCmd);
    RESOLVE(initOptionBytesInterface);
    RESOLVE(getDeviceGeneralInf);

#undef RESOLVE

    /* Initialize the API */
    CP_DisplayCallBacks cbs;
    cbs.initProgressBar = cb_initProgressBar;
    cbs.logMessage      = cb_logMessage;
    cbs.loadBar         = cb_loadBar;
    p_setDisplayCallbacks(cbs);
    p_setVerbosityLevel(1);
    p_setLoadersPath(g_exeDir);

    return h;
}

/* ════════════════════════════════════════════════════════════════════════════
 * SWAP_BANK detection — walk the option byte tree
 * ════════════════════════════════════════════════════════════════════════════ */

static int readSwapBank(void)
{
    CP_Peripheral *ob = p_initOptionBytesInterface();
    if (!ob) return -1;

    unsigned int i, j, k;
    for (i = 0; i < ob->banksNbr; i++) {
        CP_Bank *bank = ob->banks[i];
        for (j = 0; j < bank->categoriesNbr; j++) {
            CP_Category *cat = bank->categories[j];
            for (k = 0; k < cat->bitsNbr; k++) {
                CP_Bit *bit = cat->bits[k];
                if (strcmp(bit->name, "SWAP_BANK") == 0)
                    return (int)bit->bitValue;
            }
        }
    }
    return -1;  /* not found */
}

/* ════════════════════════════════════════════════════════════════════════════
 * Commands
 * ════════════════════════════════════════════════════════════════════════════ */

static int cmdScan(void)
{
    CP_DfuDeviceInfo *list = NULL;
    int count = p_getDfuDeviceList(&list, 0xDF11, 0x0483);

    if (count > 0 && list)
        OUT("DEVICE:%s|%s|%s", list[0].usbIndex, list[0].serialNumber,
            list[0].productId);
    else
        OUT("DEVICE:none");

    p_deleteInterfaceList();
    return (count > 0) ? 0 : 1;
}

/*
 * Target options:
 *   "inactive" — flash to whichever bank is NOT currently active, then swap
 *   "bank1"    — flash to physical bank 1, boot from bank 1
 *   "bank2"    — flash to physical bank 2, boot from bank 2
 *
 * Strategy: always clear SWAP_BANK to 0 first so addresses are predictable:
 *   0x08000000 = physical bank 1
 *   0x08100000 = physical bank 2
 * Then flash to the right address, and set SWAP_BANK afterward if needed.
 */
/*
 * Flash-only — NEVER touches SWAP_BANK.
 *
 * Reads SWAP_BANK to determine the current address mapping, calculates
 * the correct logical address for the target physical bank, writes the
 * firmware, and disconnects.  The user swaps banks separately if needed.
 *
 * Address mapping:
 *   SWAP_BANK=0: 0x08000000 = phys bank 1, 0x08100000 = phys bank 2
 *   SWAP_BANK=1: 0x08000000 = phys bank 2, 0x08100000 = phys bank 1
 */
static int cmdFlash(const char *binPath, const char *target)
{
    int rc;
    int targetBank;  /* 1 or 2 — which PHYSICAL bank to flash */

    /* ── Step 1: Find device ── */
    OUT("STATUS:Scanning for DFU device...");

    CP_DfuDeviceInfo *list = NULL;
    int count = p_getDfuDeviceList(&list, 0xDF11, 0x0483);
    if (count <= 0 || !list) {
        p_deleteInterfaceList();
        OUT("ERROR:No STM32 DFU device found. Enter DFU mode first "
            "(hold Up + OK for 5 seconds while powered off).");
        return 1;
    }

    char usbIndex[16];
    strncpy(usbIndex, list[0].usbIndex, sizeof(usbIndex) - 1);
    usbIndex[sizeof(usbIndex) - 1] = '\0';
    OUT("DEVICE:%s|%s|%s", list[0].usbIndex, list[0].serialNumber,
        list[0].productId);

    /* ── Step 2: Connect ── */
    OUT("STATUS:Connecting to DFU device...");
    rc = p_connectDfuBootloader(usbIndex);
    if (rc != 0) {
        p_deleteInterfaceList();
        OUT("ERROR:Failed to connect to DFU device (code %d). "
            "Check USB connection and ensure device is in DFU mode.", rc);
        return 1;
    }

    /* ── Step 3: Read SWAP_BANK and determine target bank ── */
    OUT("STATUS:Reading SWAP_BANK option byte...");
    int swapBank = readSwapBank();
    OUT("SWAP_BANK:%d", swapBank);

    if (swapBank < 0) {
        OUT("ERROR:Could not read SWAP_BANK option byte. Cannot safely "
            "determine flash bank mapping.");
        p_disconnect();
        p_deleteInterfaceList();
        return 1;
    }

    if (strcmp(target, "bank1") == 0) {
        targetBank = 1;
    } else if (strcmp(target, "bank2") == 0) {
        targetBank = 2;
    } else {
        /* "inactive" — flash to whichever physical bank is NOT active */
        if (swapBank == 1)
            targetBank = 1;  /* bank 2 is active, so bank 1 is inactive */
        else
            targetBank = 2;  /* bank 1 is active, so bank 2 is inactive */
    }

    /* Logical address for the target physical bank under current mapping */
    unsigned int flashAddr;
    if (targetBank == 1)
        flashAddr = (swapBank == 0) ? 0x08000000 : 0x08100000;
    else
        flashAddr = (swapBank == 0) ? 0x08100000 : 0x08000000;

    OUT("STATUS:Flashing to physical bank %d (logical 0x%08X, SWAP_BANK=%d)",
        targetBank, flashAddr, swapBank);

    /* ── Step 4: Flash firmware ── */
    OUT("STATUS:Erasing and writing firmware to bank %d...", targetBank);
    OUT("PROGRESS:0");

    wchar_t wPath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, binPath, -1, wPath, MAX_PATH);

    rc = p_downloadFile(wPath, flashAddr, 0, 1, L"");
    if (rc != 0) {
        OUT("ERROR:Flash failed (code %d). Your existing firmware and bank "
            "configuration are unchanged.", rc);
        p_disconnect();
        p_deleteInterfaceList();
        return 1;
    }

    OUT("STATUS:Flash write complete.");

    /* ── Step 5: Set SWAP_BANK after successful flash if needed ── */
    int finalSwapBank = (targetBank == 1) ? 0 : 1;

    if (finalSwapBank != swapBank) {
        /* SWAP_BANK change needed to boot from the newly flashed bank.
         * Safe: firmware is already written. Even if this fails, both
         * banks have valid firmware — user can swap manually via DFU. */
        OUT("STATUS:Setting SWAP_BANK=%d to boot from bank %d...",
            finalSwapBank, targetBank);

        char obCmd[32];
        snprintf(obCmd, sizeof(obCmd), "-ob SWAP_BANK=%d", finalSwapBank);
        p_sendOptionBytesCmd(obCmd);
        /* Device resets on OB change — connection drops, expected */
        p_disconnect();
    } else {
        /* SWAP_BANK already correct — just start the new firmware */
        OUT("STATUS:Starting firmware from bank %d...", targetBank);
        p_execute(flashAddr);
        p_disconnect();
    }
    p_deleteInterfaceList();

    OUT("PROGRESS:100");
    OUT("OK");
    return 0;
}

static int cmdSwapBank(void)
{
    int rc;

    /* ── Find device ── */
    OUT("STATUS:Scanning for DFU device...");

    CP_DfuDeviceInfo *list = NULL;
    int count = p_getDfuDeviceList(&list, 0xDF11, 0x0483);
    if (count <= 0 || !list) {
        p_deleteInterfaceList();
        OUT("ERROR:No STM32 DFU device found. Enter DFU mode first "
            "(hold Up + OK for 5 seconds while powered off).");
        return 1;
    }

    char usbIndex[16];
    strncpy(usbIndex, list[0].usbIndex, sizeof(usbIndex) - 1);
    usbIndex[sizeof(usbIndex) - 1] = '\0';
    OUT("DEVICE:%s|%s|%s", list[0].usbIndex, list[0].serialNumber,
        list[0].productId);
    /* NOTE: do NOT call deleteInterfaceList here — connectDfuBootloader
     * needs the internal device list to remain valid. */

    /* ── Connect ── */
    OUT("STATUS:Connecting to DFU device...");
    rc = p_connectDfuBootloader(usbIndex);
    if (rc != 0) {
        p_deleteInterfaceList();
        OUT("ERROR:Failed to connect to DFU device (code %d). "
            "Check USB connection and ensure device is in DFU mode.", rc);
        return 1;
    }

    /* ── Read current SWAP_BANK ── */
    OUT("STATUS:Reading SWAP_BANK option byte...");
    int swapBank = readSwapBank();
    OUT("SWAP_BANK:%d", swapBank);

    if (swapBank < 0) {
        OUT("ERROR:Could not read SWAP_BANK option byte.");
        p_disconnect();
        return 1;
    }

    /* ── Toggle it ── */
    int newVal = (swapBank == 0) ? 1 : 0;
    OUT("STATUS:Switching SWAP_BANK from %d to %d...", swapBank, newVal);

    char obCmd[32];
    snprintf(obCmd, sizeof(obCmd), "-ob SWAP_BANK=%d", newVal);
    rc = p_sendOptionBytesCmd(obCmd);
    /* rc may be non-zero if device resets during OB change — expected */

    p_disconnect();
    p_deleteInterfaceList();

    OUT("STATUS:Bank swap complete. Device will reboot into bank %d.",
        newVal == 0 ? 1 : 2);
    OUT("SWAP_BANK:%d", newVal);
    OUT("OK");
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 * Main
 * ════════════════════════════════════════════════════════════════════════════ */

int main(int argc, char *argv[])
{
    /* Disable stdout buffering so parent process gets lines immediately */
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 2) {
        fprintf(stderr, "STM32 DFU Flash Helper (CubeProgrammer API)\n\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s scan                             Detect STM32 DFU devices\n", argv[0]);
        fprintf(stderr, "  %s flash <file.bin> [target]        Flash firmware via DFU\n", argv[0]);
        fprintf(stderr, "  %s swapbank                         Toggle SWAP_BANK option byte\n", argv[0]);
        fprintf(stderr, "\nFlash targets:\n");
        fprintf(stderr, "  inactive   Flash to inactive bank, swap to it (default)\n");
        fprintf(stderr, "  bank1      Flash to bank 1, boot from bank 1\n");
        fprintf(stderr, "  bank2      Flash to bank 2, boot from bank 2\n");
        return 1;
    }

    getExeDir();

    HMODULE hLib = loadCubeProgAPI();
    if (!hLib)
        return 1;

    int result;

    if (strcmp(argv[1], "scan") == 0) {
        result = cmdScan();
    } else if (strcmp(argv[1], "flash") == 0) {
        if (argc < 3) {
            OUT("ERROR:No firmware file specified");
            result = 1;
        } else {
            const char *target = (argc >= 4) ? argv[3] : "inactive";
            result = cmdFlash(argv[2], target);
        }
    } else if (strcmp(argv[1], "swapbank") == 0) {
        result = cmdSwapBank();
    } else {
        OUT("ERROR:Unknown command: %s", argv[1]);
        result = 1;
    }

    FreeLibrary(hLib);
    return result;
}
