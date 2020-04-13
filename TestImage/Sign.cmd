@echo off

set SBL_ROOT=..\
set KEYFILE=%SBL_ROOT%\BootloaderCorePkg\Tools\Keys\TestSigningPrivateKey.pem
set ITYPE=-cl CMDL:MinQemu\config.cfg KRNL:MinQemu\vmlinuz INRD:MinQemu\initrd

if not exist Out  mkdir Out

echo Using key file: %KEYFILE%
python %SBL_ROOT%\BootloaderCorePkg\Tools\GenContainer.py create ^
       %ITYPE% -t  CLASSIC  -o Out\IasImage.bin  -k %KEYFILE% -a RSA2048_PKCS1_SHA2_256
if not "%ERRORLEVEL%" == "0" goto :EOF
copy Out\IasImage.bin  MinQemu\IasImage.bin  /y

