copy ..\..\uefi\Edk2Open\Build\UefiPayloadPkgX64\DEBUG_VS2015x86\FV\UEFIPAYLOAD.fd Platform\QemuBoardPkg\Binaries\UefiPld.fd 
python buildloader.py build qemu -p "OsLoader.efi:LLDR:Lz4;UefiPld.fd:UEFI:Lzma" 
"c:\Program Files\qemu"\qemu-system-x86_64.exe -machine q35 -pflash  Outputs\qemu\SlimBootloader.bin  -serial file:test.log 
