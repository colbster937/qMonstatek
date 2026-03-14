# swap_bank.tcl — Toggle SWAP_BANK option byte on STM32H5
# Try both possible OPTKEYR addresses

init
halt

echo "=== Current OPTSR_CUR ==="
mdw 0x40022050

echo "=== Current FLASH_OPTCR (OPTLOCK check) ==="
mdw 0x4002201C

# Unlock flash via NSKEYR (try user address 0x44022004)
echo "=== Unlocking flash ==="
catch {mww 0x44022004 0x45670123}
catch {mww 0x44022004 0xCDEF89AB}

# Also try non-secure alias
catch {mww 0x40022004 0x45670123}
catch {mww 0x40022004 0xCDEF89AB}

echo "=== Unlocking option bytes ==="
# Try user address 0x44022008
catch {mww 0x44022008 0x08192A3B}
catch {mww 0x44022008 0x4C5D6E7F}

# Also try 0x4002200C (RM0481 offset 0x0C)
catch {mww 0x4002200C 0x08192A3B}
catch {mww 0x4002200C 0x4C5D6E7F}

# Also try secure alias 0x4402200C
catch {mww 0x4402200C 0x08192A3B}
catch {mww 0x4402200C 0x4C5D6E7F}

echo "=== FLASH_OPTCR after unlock ==="
mdw 0x4002201C

echo "=== Writing OPTSR_PRG with SWAP_BANK=0 ==="
# Current is 0xad10edf8 (bit31=1), target is 0x2d10edf8 (bit31=0)
catch {mww 0x40022054 0x2d10edf8}
catch {mww 0x44022054 0x2d10edf8}

# Also try user's OPTSR_PRG address
catch {mww 0x44022024 0x2d10edf8}

echo "=== Verify OPTSR_PRG ==="
mdw 0x40022054

echo "=== Trigger OPTSTRT (bit 17) in FLASH_NSCR ==="
catch {mww 0x40022020 0x00020000}

sleep 500

echo "=== OPTSR_CUR after ==="
mdw 0x40022050

echo "=== Done ==="
shutdown
