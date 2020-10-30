/** @file

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef  _BOOT_FW_H_
#define  _BOOT_FW_H_

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BootloaderCoreLib.h>
#include <Library/TimeStampLib.h>
#include <Library/StageLib.h>
#include <Library/FspApiLib.h>
#include <Library/FspSupportLib.h>
#include <Library/BlMemoryAllocationLib.h>
#include <FspUpd.h>
#include <FsptUpd.h>
#include <FspmUpd.h>
#include <FspsUpd.h>


extern const UINT32 FsptBaseAddr;
extern const UINT32 FspmBaseAddr;
extern const UINT32 FspsBaseAddr;

/**
  Continue Stage 1B execution.

  This function will continue Stage1B execution with a new memory-based stack.

  @param[in]  Context1        Pointer to STAGE1B_PARAM in main memory.
  @param[in]  Context2        Unused.

**/
VOID
EFIAPI
ContinueFunc (
  IN VOID                      *Context1,
  IN VOID                      *Context2
  );

VOID
EFIAPI
JumpToOemEntry (
  IN VOID    *OemEntry,
  IN VOID    *HobList,
  IN UINT32   NemBase,
  IN UINT32   NemSize
  );

#endif