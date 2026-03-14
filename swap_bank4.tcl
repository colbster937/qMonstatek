# Try writing OPTKEYR through AP0 (mem_ap) instead of CPU
# The KEYR registers are write-only — OpenOCD mww through CPU may fail

echo "=== Reset to clean state ==="
reset halt
sleep 200

echo "OPTCR:"
set optcr [mrw 0x4002201C]
echo [format "0x%08X" $optcr]

echo "NSCR:"
set nscr [mrw 0x40022020]
echo [format "0x%08X" $nscr]

# Try unlocking via AP0 (access port 0 = direct bus access)
echo "=== Unlock flash via AP0 ==="
catch {stm32h5.ap0 mww 0x40022004 0x45670123} e1
echo "NSKEYR key1: $e1"
catch {stm32h5.ap0 mww 0x40022004 0xCDEF89AB} e2
echo "NSKEYR key2: $e2"

echo "NSCR after flash unlock:"
set nscr2 [mrw 0x40022020]
echo [format "0x%08X" $nscr2]

echo "=== Unlock option bytes via AP0 ==="
catch {stm32h5.ap0 mww 0x4002200C 0x08192A3B} e3
echo "OPTKEYR key1: $e3"
catch {stm32h5.ap0 mww 0x4002200C 0x4C5D6E7F} e4
echo "OPTKEYR key2: $e4"

echo "OPTCR after opt unlock:"
set optcr2 [mrw 0x4002201C]
echo [format "0x%08X" $optcr2]

# If OPTLOCK is clear, write OPTSR_PRG and OPTSTRT
echo "=== Write OPTSR_PRG ==="
mww 0x40022054 0x2d10edf8
set prg [mrw 0x40022054]
echo [format "OPTSR_PRG: 0x%08X" $prg]

echo "=== Trigger OPTSTRT ==="
mww 0x40022020 0x00020000
sleep 1000

echo "OPTSR_CUR:"
set cur [mrw 0x40022050]
echo [format "0x%08X" $cur]

echo "=== Reset ==="
reset run
