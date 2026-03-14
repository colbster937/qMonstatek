# Skip unlocking — flash and option bytes are already unlocked on this crashed chip
# Just write OPTSR_PRG and trigger OPTSTRT

echo "OPTCR (verify unlocked):"
set optcr [mrw 0x4002201C]
echo [format "0x%08X" $optcr]

echo "OPTSR_CUR before:"
set cur [mrw 0x40022050]
echo [format "0x%08X" $cur]

echo "Writing OPTSR_PRG = 0x2D10EDF8 (SWAP_BANK=0)..."
mww 0x40022054 0x2d10edf8

echo "OPTSR_PRG verify:"
set prg [mrw 0x40022054]
echo [format "0x%08X" $prg]

echo "Triggering OPTSTRT (bit 17 in FLASH_NSCR)..."
mww 0x40022020 0x00020000

sleep 1000

echo "FLASH_NSSR (check BSY):"
set nssr [mrw 0x40022028]
echo [format "0x%08X" $nssr]

echo "OPTSR_CUR after:"
set cur2 [mrw 0x40022050]
echo [format "0x%08X" $cur2]

echo "Resetting..."
reset run
