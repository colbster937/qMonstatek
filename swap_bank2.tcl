# Run after init+halt
echo "OPTSR_CUR before:"
set val [mrw 0x40022050]
echo [format "0x%08X" $val]

echo "OPTCR before:"
set optcr [mrw 0x4002201C]
echo [format "0x%08X" $optcr]

# Unlock flash - try both address ranges
echo "Unlocking flash..."
catch {mww 0x44022004 0x45670123} err1
catch {mww 0x44022004 0xCDEF89AB} err2
catch {mww 0x40022004 0x45670123} err3
catch {mww 0x40022004 0xCDEF89AB} err4

# Unlock option bytes - try all possible addresses
echo "Unlocking option bytes..."
catch {mww 0x44022008 0x08192A3B} err5
catch {mww 0x44022008 0x4C5D6E7F} err6
catch {mww 0x4402200C 0x08192A3B} err7
catch {mww 0x4402200C 0x4C5D6E7F} err8
catch {mww 0x40022008 0x08192A3B} err9
catch {mww 0x40022008 0x4C5D6E7F} err10
catch {mww 0x4002200C 0x08192A3B} err11
catch {mww 0x4002200C 0x4C5D6E7F} err12

echo "OPTCR after unlock:"
set optcr2 [mrw 0x4002201C]
echo [format "0x%08X" $optcr2]

# Write OPTSR_PRG
echo "Writing OPTSR_PRG..."
catch {mww 0x40022054 0x2d10edf8} err13
catch {mww 0x44022054 0x2d10edf8} err14
catch {mww 0x44022024 0x2d10edf8} err15

echo "OPTSR_PRG after write:"
set prg [mrw 0x40022054]
echo [format "0x%08X" $prg]

# Trigger OPTSTRT
echo "Setting OPTSTRT..."
catch {mww 0x40022020 0x00020000} err16

sleep 500

echo "OPTSR_CUR after OPTSTRT:"
set val2 [mrw 0x40022050]
echo [format "0x%08X" $val2]
