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
  BOOT_PARAMS                *Bp;
  CHAR8                       CmdLineBuf[MAX_CMD_LINE_LEN];
  UINTN                       InitRdLen;
  UINT8                      *InitRdBuf;
  UINTN                       KernelLen;
  UINT8                      *KernelBuf;
  EFI_HOB_GUID_TYPE          *GuidHob;
  PLD_EXTRA_DATA             *PldImgInfo;
  UINT32                      Idx;

  (VOID)PcdSet64S (PcdHobListPtr, (UINT64)(UINTN)Params);
  PlatformHookSerialPortInitialize ();

  DEBUG ((DEBUG_INFO, "Linux Loader ... \n"));

  //
  // Look through the HOB list to find kernel, initrd and command line.
  //
  KernelBuf  = NULL;
  InitRdBuf  = NULL;
  InitRdLen  = 0;
  CmdLineBuf[0] = 0;
  GuidHob = GetNextGuidHob (&gLoadedPayloadImageInfoGuid, GetHobList());
  if (GuidHob != NULL) {
    PldImgInfo  = (PLD_EXTRA_DATA *) GET_GUID_HOB_DATA (GuidHob);
    for (Idx = 0; Idx < PldImgInfo->Count; Idx++) {
      DEBUG ((DEBUG_INFO, "Found loaded image '%a'\n", PldImgInfo->Entry[Idx].Identifier));
      if (AsciiStrCmp (PldImgInfo->Entry[Idx].Identifier, "kernel") == 0) {
        KernelBuf = (VOID *)(UINTN)PldImgInfo->Entry[Idx].Base;
        KernelLen = (UINTN)(UINTN)PldImgInfo->Entry[Idx].Size;
      } else if (AsciiStrCmp (PldImgInfo->Entry[Idx].Identifier, "initrd") == 0) {
        InitRdBuf = (VOID *)(UINTN)PldImgInfo->Entry[Idx].Base;
        InitRdLen = (UINTN)PldImgInfo->Entry[Idx].Size;
      } else if (AsciiStrCmp (PldImgInfo->Entry[Idx].Identifier, "cmdline") == 0) {
        AsciiStrCpyS (CmdLineBuf, MAX_CMD_LINE_LEN, (CHAR8 *)(UINTN)PldImgInfo->Entry[Idx].Base);
      }
    }
  }

  if (KernelBuf != NULL) {
    DEBUG ((EFI_D_ERROR, "Kernel  @ 0x%p:0x%08x\n", KernelBuf, (UINT32)KernelLen));
  }
  if (InitRdBuf != NULL) {
    DEBUG ((EFI_D_ERROR, "InitRd  @ 0x%p:0x%08x\n", InitRdBuf, (UINT32)InitRdLen));
  }
  if (InitRdBuf != NULL) {
    DEBUG ((EFI_D_ERROR, "CmdLine : %a\n", CmdLineBuf));
  }

  if (KernelBuf != NULL) {
    Status = UpldLoadBzImage (KernelBuf, InitRdBuf, (UINT32)InitRdLen, CmdLineBuf, MAX_CMD_LINE_LEN);
    if (!EFI_ERROR(Status)) {
      Bp = (BOOT_PARAMS *)KernelBuf;
      UpldLinuxBoot (GetHobList(), (VOID *)Bp);
    }
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to load kernel image !\n"));
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Failed to locate kernel image !\n"));
  }

  while (1);
}
