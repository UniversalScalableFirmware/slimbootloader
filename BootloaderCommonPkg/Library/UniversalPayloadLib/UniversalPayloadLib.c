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
  EFI_STATUS           Status;
  ELF_IMAGE_CONTEXT    ElfCt;
  Elf32_Shdr          *ElfShdr;
  UPLD_INFO_HEADER    *UpldInfo;
  Elf_Ehdr            *ElfHdr;
  UINT32               Idx;
  BOOLEAN              Found;
  UINT16               ImgIdx;
  CHAR8               *SecName;


  if ((ImageBase == NULL) || (PayloadInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Found  = FALSE;
  Status = ParseElfImage (ImageBase, &ElfCt);
  if (!EFI_ERROR(Status)) {
    ElfShdr = GetSectionByName (&ElfCt, UPLD_INFO_SEC_NAME);
    if (ElfShdr != NULL) {
      UpldInfo = (UPLD_INFO_HEADER *)(ElfCt.ImageBase + ElfShdr->sh_offset);
      if (UpldInfo->Identifier == UPLD_IDENTIFIER) {
        Found  = TRUE;
      }
    }
  }
  if (!Found) {
    return EFI_NOT_FOUND;
  }

  // Relocate image
  Status = RelocateElfSections (&ElfCt);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  // Fill in payload image info
  ImgIdx = 0;
  ElfHdr = (Elf_Ehdr *)ElfCt.ImageBase;
  ZeroMem (PayloadInfo, sizeof(LOADED_PAYLOAD_INFO));
  for (Idx = 0; Idx < ElfHdr->e_shnum; Idx++) {
    SecName = GetSectionName (&ElfCt, Idx);
    if ((SecName != NULL) && (AsciiStrnCmp(SecName, UPLD_IMAGE_SEC_NAME_PREFIX, 6) == 0)) {
      ElfShdr = GetSectionByIndex (&ElfCt, Idx);
      AsciiStrCpyS (PayloadInfo->LoadedImage[ImgIdx].Name, sizeof(PayloadInfo->LoadedImage[ImgIdx].Name), SecName);
      PayloadInfo->LoadedImage[ImgIdx].Base = (UINTN)(ElfCt.ImageBase + ElfShdr->sh_offset);
      PayloadInfo->LoadedImage[ImgIdx].Size = ElfShdr->sh_size;
      ImgIdx++;
      if (ImgIdx >= ARRAY_SIZE(PayloadInfo->LoadedImage)) {
        break;
      }
    }
  }
  CopyMem (&PayloadInfo->Info, UpldInfo, sizeof(UPLD_INFO_HEADER));
  PayloadInfo->Machine    = ElfHdr->e_machine;
  PayloadInfo->ImageCount = ImgIdx;
  PayloadInfo->EntryPoint = (UNIVERSAL_PAYLOAD_ENTRYPOINT)ElfHdr->e_entry;
  return EFI_SUCCESS;
}
