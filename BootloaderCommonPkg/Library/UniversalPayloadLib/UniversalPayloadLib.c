/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BootloaderCommonLib.h>
#include <Library/UniversalPayloadLib.h>
#include <Library/ElfLib.h>

//#undef   DEBUG_VERBOSE
//#define  DEBUG_VERBOSE   DEBUG_INFO


/**
  Load universal payload image into memory.

  @param[in]   ImageBase    The universal payload image base
  @param[out]  PayloadInfo  Pointer to receive payload related info

  @retval     EFI_SUCCESS      The image was loaded successfully
              EFI_ABORTED      The image loading failed
              EFI_UNSUPPORTED  The relocation format is not supported

**/
EFI_STATUS
EFIAPI
LoadUniversalPayload (
  IN  VOID                     *ImageBase,
  OUT LOADED_PAYLOAD_INFO      *PayloadInfo
  )
{
  EFI_STATUS                     Status;
  ELF_IMAGE_CONTEXT              ElfCt;
  UPLD_INFO_HEADER              *UpldInfo;
  UINT32                         Idx;
  UINT16                         ImgIdx;
  CHAR8                         *SecName;
  SECTION_POS                    SecPos;
  SEGMENT_INFO                   SegInfo;
  UINTN                          ImageSize;
  BOOLEAN                        Xip;

  if ((ImageBase == NULL) || (PayloadInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ParseElfImage (ImageBase, &ElfCt);
  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }

  ImgIdx   = 0;
  UpldInfo = NULL;
  ZeroMem (PayloadInfo, sizeof(LOADED_PAYLOAD_INFO));
  for (Idx = 0; Idx < ElfCt.ShNum; Idx++) {
    Status = GetElfSectionName (&ElfCt, Idx, &SecName);
    if (EFI_ERROR(Status)) {
      continue;
    }
    if (AsciiStrCmp(SecName, UPLD_INFO_SEC_NAME) == 0) {
      Status = GetElfSectionPos (&ElfCt, Idx, &SecPos);
      if (!EFI_ERROR(Status)) {
        UpldInfo = (UPLD_INFO_HEADER *)(ElfCt.ImageBase + SecPos.Offset);
      }
    } else if (AsciiStrnCmp(SecName, UPLD_IMAGE_SEC_NAME_PREFIX, 6) == 0) {
      Status = GetElfSectionPos (&ElfCt, Idx, &SecPos);
      if (!EFI_ERROR(Status) && (ImgIdx < ARRAY_SIZE(PayloadInfo->LoadedImage))) {
        AsciiStrCpyS (PayloadInfo->LoadedImage[ImgIdx].Name, sizeof(PayloadInfo->LoadedImage[ImgIdx].Name), SecName);
        PayloadInfo->LoadedImage[ImgIdx].Base = (UINTN)(ElfCt.ImageBase + SecPos.Offset);
        PayloadInfo->LoadedImage[ImgIdx].Size = SecPos.Length;
        ImgIdx++;
      }
    }
  }

  if ((UpldInfo == NULL) || (UpldInfo->Identifier != UPLD_IDENTIFIER)) {
    return EFI_UNSUPPORTED;
  }

  Status = CalculateElfImageSize (&ElfCt, &ImageSize);
  if (EFI_ERROR(Status)) {
    return EFI_ABORTED;
  }
  DEBUG ((DEBUG_INFO, "UPLD Image Size: 0x%08X\n", ImageSize));

  // Determine if it can be executed in place
  Xip = TRUE;
  for (Idx = 0; Idx < ElfCt.PhNum; Idx++) {
    Status = GetElfSegmentInfo (&ElfCt, Idx, &SegInfo);
    if (!EFI_ERROR(Status)) {
      if ((SegInfo.PtType == ELF_PT_LOAD) && (SegInfo.MemLen != SegInfo.Length)) {
        Xip = FALSE;
        break;
      }
    }
  }

  if (Xip) {
    DEBUG ((DEBUG_INFO, "UPLD Relocate ELF\n"));
    Status = RelocateElfSections (&ElfCt);
    if (EFI_ERROR(Status)) {
      return EFI_ABORTED;
    }
  } else {
    DEBUG ((DEBUG_INFO, "UPLD Load ELF\n"));
    Status = LoadElfSegments (&ElfCt);
    if (EFI_ERROR(Status)) {
      return EFI_ABORTED;
    }
  }

  CopyMem (&PayloadInfo->Info, UpldInfo, sizeof(UPLD_INFO_HEADER));
  PayloadInfo->Machine     = (ElfCt.EiClass == ELF_CLASS32) ? IMAGE_FILE_MACHINE_I386 : IMAGE_FILE_MACHINE_X64;
  PayloadInfo->ImageCount  = ImgIdx;
  PayloadInfo->EntryPoint  = (UNIVERSAL_PAYLOAD_ENTRYPOINT)ElfCt.Entry;
  PayloadInfo->PayloadSize = ImageSize;
  PayloadInfo->PayloadBase = (UINT32)(UINTN)ImageBase;

  return EFI_SUCCESS;
}
