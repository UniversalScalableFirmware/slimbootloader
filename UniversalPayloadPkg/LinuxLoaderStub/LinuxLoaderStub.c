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
  LINUX_COMP_HDR             *LnxHdr;
  UINT32                      InitRdLen;
  UINT8                      *InitRdBuf;

  DEBUG ((DEBUG_INFO, "LinuxLoaderStub ... \n"));

  Ptr = (UINT8 *)((UINTN)SecStartup & ~(SIZE_4KB - 1));

  HobList    = Params;
  Status     = EFI_NOT_FOUND;
  InitRdLen  = 0;
  InitRdBuf  = NULL;

  End = Ptr + SIZE_64KB;
  while (Ptr < End) {
    Bp = (BOOT_PARAMS *)Ptr;
    if ((Bp->Hdr.Signature == 0xAA55) && (Bp->Hdr.Header == SETUP_HDR)) {
      break;
    }
    LnxHdr     = (LINUX_COMP_HDR *)Ptr;
    if (LnxHdr->Signature == LINUX_HDR_SIGNATURE) {
      break;
    }
    Ptr += SIZE_4KB;
  }

  if (Ptr == End) {
    DEBUG ((DEBUG_INFO, "Could not locate kernel image !\n"));
  } else {
    if (Bp->Hdr.Signature == 0xAA55) {
      AsciiStrCpyS (CmdLineBuf, MAX_CMD_LINE_LEN, CmdLineBuf);
      Status = EFI_SUCCESS;
    } else {
      if (LnxHdr->InitRd.Len > 0) {
        InitRdLen = LnxHdr->InitRd.Len;
        InitRdBuf = Ptr + LnxHdr->InitRd.Off;
      }
      if (LnxHdr->CmdLine.Len > 0) {
        AsciiStrCpyS (CmdLineBuf, MAX_CMD_LINE_LEN, Ptr + LnxHdr->CmdLine.Off);
      }
      if (LnxHdr->Kernel.Len > 0) {
        Bp = (BOOT_PARAMS *)(Ptr + LnxHdr->Kernel.Off);
        Status = EFI_SUCCESS;
      }
    }

    if (!EFI_ERROR(Status)) {
      Status = UpldLoadBzImage ((VOID*)Bp, InitRdBuf, InitRdLen, CmdLineBuf, MAX_CMD_LINE_LEN);
      if (!EFI_ERROR(Status)) {
        UpldLinuxBoot (HobList, (VOID *)Bp);
      }
    }

    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_INFO, "Failed to load kernel image !\n"));
    }
  }

  while (1);
}
