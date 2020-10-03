## @file
# Provides driver and definitions to build payload module.
#
# Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                       = UniversalPayloadPkg
  PLATFORM_GUID                       = A68B69AD-5B06-4FA3-87F9-1E0C020BD575
  PLATFORM_VERSION                    = 0.1
  DSC_SPECIFICATION                   = 0x00010005
  OUTPUT_DIRECTORY                    = Build/UniversalPayloadPkg
  SUPPORTED_ARCHITECTURES             = IA32|X64
  BUILD_TARGETS                       = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER                    = DEFAULT

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################
[LibraryClasses]
  BaseLib        | MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib  | MdePkg/Library/BaseMemoryLibSse2/BaseMemoryLibSse2.inf
  IoLib          | MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PrintLib       | MdePkg/Library/BasePrintLib/BasePrintLib.inf
  PcdLib         | MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DebugLib       | MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  ModuleEntryLib | BootloaderCommonPkg/Library/ModuleEntryLib/ModuleEntryLib.inf
  HobLib         | BootloaderCommonPkg/Library/HobLib/HobLib.inf
  BootloaderLib  | UniversalPayloadPkg/Library/BootloaderLib/BootloaderLib.inf

[Components]
  UniversalPayloadPkg/LinuxLoaderStub/LinuxLoaderStub.inf

[BuildOptions.Common.EDKII]
  # Enable link-time optimization when building with GCC49
  *_GCC49_IA32_CC_FLAGS = -flto
  *_GCC49_IA32_DLINK_FLAGS = -flto
  *_GCC5_IA32_CC_FLAGS = -fno-pic
  *_GCC5_IA32_DLINK_FLAGS = -no-pie
  *_GCC5_IA32_ASLCC_FLAGS = -fno-pic
  *_GCC5_IA32_ASLDLINK_FLAGS = -no-pie
  # Force synchronous PDB writes for parallel builds
  *_VS2015x86_IA32_CC_FLAGS = /FS
  *_XCODE5_IA32_CC_FLAGS = -flto

  *_*_*_CC_FLAGS = -DDISABLE_NEW_DEPRECATED_INTERFACES

!if $(TARGET) == NOOPT
  # GCC: -O0 results in too big size. Override it to -O1 with lto
  *_GCC49_*_CC_FLAGS = -O1
  *_GCC49_*_DLINK_FLAGS = -O1
  *_GCC5_*_CC_FLAGS = -flto -O1
  *_GCC5_*_DLINK_FLAGS = -flto -O1
  # VS: Use default /Od for now
!endif

