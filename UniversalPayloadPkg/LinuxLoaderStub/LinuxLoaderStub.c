/** @file

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LinuxLoaderStub.h"


/**
  Linux Loader Stub

**/
VOID
EFIAPI
SecStartup (
  IN VOID  *Params
  )
{
  EFI_STATUS                  Status;
  VOID                       *HobList;
  UINT8                      *Ptr;
  UINT8                      *End;
  BOOT_PARAMS                *Bp;
  CHAR8                       CmdLineBuf[MAX_CMD_LINE_LEN];

  DEBUG ((DEBUG_INFO, "LinuxLoaderStub ... \n"));

  HobList = Params;
  Ptr = (UINT8 *)((UINTN)SecStartup & ~(SIZE_4KB - 1));
  End = Ptr + SIZE_64KB;
  while (Ptr < End) {
    Bp = (BOOT_PARAMS *)Ptr;
    if ((Bp->Hdr.Signature == 0xAA55) && (Bp->Hdr.Header == SETUP_HDR)) {
      break;
    }
    Ptr += SIZE_4KB;
  }
  if (Ptr == End) {
    DEBUG ((DEBUG_INFO, "Could not locate kernel image !\n"));
  } else {
    AsciiStrCpyS (CmdLineBuf, MAX_CMD_LINE_LEN, DEF_CMD_LINE);
    Status = UpldLoadBzImage ((VOID*)Bp, NULL, 0, CmdLineBuf, MAX_CMD_LINE_LEN);
    if (!EFI_ERROR(Status)) {
      UpldLinuxBoot (HobList, (VOID *)Bp);
    } else {
      DEBUG ((DEBUG_INFO, "Failed to setup kernel image !\n"));
    }
  }

  DEBUG ((DEBUG_INFO, "Done\n"));

  while (1);
}
