/** @file

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LINUX_LOADER_STUB_
#define _LINUX_LOADER_STUB_

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/LinuxLib.h>
#include <Library/PrintLib.h>
#include <Guid/GraphicsInfoHob.h>
#include <Guid/MemoryMapInfoGuid.h>
#include <Guid/AcpiTableGuid.h>

#define MAX_CMD_LINE_LEN  0x200

#define DEF_CMD_LINE      "init=/init root=/dev/ram0 rw 3 console=ttyS0,115200 console=tty0"

/**
  Load linux kernel image to specified address and setup boot parameters.

  @param[in]  HobList      HOB list pointer.
  @param[in]  KernelBase   Kernel image base.
**/
VOID
EFIAPI
UpldLinuxBoot (
  IN VOID   *HobList,
  IN VOID   *KernelBase
  );

/**
  Load linux kernel image to specified address and setup boot parameters.

  @param[in]  KernelBase     Memory address of an kernel image.
  @param[in]  InitRdBase     Memory address of an InitRd image.
  @param[in]  InitRdLen      InitRd image size.
  @param[in]  CmdLineBase    Memory address of command line buffer.
  @param[in]  CmdLineLen     Command line buffer size.

  @retval EFI_INVALID_PARAMETER   Input parameters are not valid.
  @retval EFI_UNSUPPORTED         Unsupported binary type.
  @retval EFI_SUCCESS             Kernel is loaded successfully.
**/
EFI_STATUS
EFIAPI
UpldLoadBzImage (
  IN  CONST VOID                  *KernelBase,
  IN  CONST VOID                  *InitRdBase,
  IN      UINT32                   InitRdLen,
  IN  CONST VOID                  *CmdLineBase,
  IN      UINT32                   CmdLineLen
  );

#endif
