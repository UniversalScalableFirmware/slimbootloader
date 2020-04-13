@echo off
set IFWI_PATH=.\Outputs\qemu\SlimBootloader.bin
if not exist %IFWI_PATH% (
  python BuildLoader.py build qemu -a x64
)
set QEMU_PATH="C:\Program Files\qemu\qemu-system-x86_64.exe"
%QEMU_PATH% -m 256M -cpu max -machine q35,accel=tcg -drive if=pflash,format=raw,file=%IFWI_PATH% -drive id=mydisk,if=none,file=fat:rw:TestImage\MinQemu,format=raw -device ide-hd,drive=mydisk -boot order=d  -serial stdio
