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
  Elf32_Ehdr                    *Elf32Hdr;
  Elf32_Shdr                    *Elf32Shdr;
  Elf64_Ehdr                    *Elf64Hdr;
  Elf64_Shdr                    *Elf64Shdr;
  UPLD_INFO_HEADER              *UpldInfo;
  UINT32                         Idx;
  BOOLEAN                        Found;
  UINT16                         ImgIdx;
  CHAR8                         *SecName;
  UINT32                         ShOff;
  UINT32                         ShLen;
  UINT32                         ShNum;
  UINT32                         Machine;
  UNIVERSAL_PAYLOAD_ENTRYPOINT   Entry;

  if ((ImageBase == NULL) || (PayloadInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Found  = FALSE;
  Status = ParseElfImage (ImageBase, &ElfCt);
  if (!EFI_ERROR(Status)) {
    UpldInfo = NULL;
    if (ElfCt.EiClass == ELFCLASS32) {
      Elf32Hdr  = (Elf32_Ehdr *)ImageBase;
      Elf32Shdr = GetElf32SectionByName (&ElfCt, UPLD_INFO_SEC_NAME);
      if (Elf32Shdr != NULL) {
        UpldInfo = (UPLD_INFO_HEADER *)(ElfCt.ImageBase + Elf32Shdr->sh_offset);
        ShNum = Elf32Hdr->e_shnum;
      }
    } else {
      Elf64Hdr  = (Elf64_Ehdr *)ImageBase;
      Elf64Shdr = GetElf64SectionByName (&ElfCt, UPLD_INFO_SEC_NAME);
      if (Elf64Shdr != NULL) {
        UpldInfo = (UPLD_INFO_HEADER *)(ElfCt.ImageBase + Elf64Shdr->sh_offset);
        ShNum = Elf64Hdr->e_shnum;
      }
    }
    if ((UpldInfo != NULL) && (UpldInfo->Identifier == UPLD_IDENTIFIER)) {
      Found  = TRUE;
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
  if (ElfCt.EiClass == ELFCLASS32) {
    Elf32Hdr  = (Elf32_Ehdr *)ElfCt.ImageBase;
    Entry     = (UNIVERSAL_PAYLOAD_ENTRYPOINT)(UINTN)Elf32Hdr->e_entry;
    Machine   = Elf32Hdr->e_machine;
  } else {
    Elf64Hdr  = (Elf64_Ehdr *)ElfCt.ImageBase;
    Entry     = (UNIVERSAL_PAYLOAD_ENTRYPOINT)(UINTN)Elf64Hdr->e_entry;
    Machine   = Elf64Hdr->e_machine;
  }

  ZeroMem (PayloadInfo, sizeof(LOADED_PAYLOAD_INFO));
  for (Idx = 0; Idx < ShNum; Idx++) {
    SecName = GetElfSectionName (&ElfCt, Idx);
    if ((SecName != NULL) && (AsciiStrnCmp(SecName, UPLD_IMAGE_SEC_NAME_PREFIX, 6) == 0)) {
      if (ElfCt.EiClass == ELFCLASS32) {
        Elf32Shdr = GetElf32SectionByIndex (&ElfCt, Idx);
        ShOff   = Elf32Shdr->sh_offset;
        ShLen   = Elf32Shdr->sh_size;
      } else {
        Elf64Shdr = GetElf64SectionByIndex (&ElfCt, Idx);
        ShOff   = (UINT32)Elf64Shdr->sh_offset;
        ShLen   = (UINT32)Elf64Shdr->sh_size;
      }
      AsciiStrCpyS (PayloadInfo->LoadedImage[ImgIdx].Name, sizeof(PayloadInfo->LoadedImage[ImgIdx].Name), SecName);
      PayloadInfo->LoadedImage[ImgIdx].Base = (UINTN)(ElfCt.ImageBase + ShOff);
      PayloadInfo->LoadedImage[ImgIdx].Size = ShLen;
      ImgIdx++;
      if (ImgIdx >= ARRAY_SIZE(PayloadInfo->LoadedImage)) {
        break;
      }
    }
  }

  CopyMem (&PayloadInfo->Info, UpldInfo, sizeof(UPLD_INFO_HEADER));
  PayloadInfo->Machine    = Machine == EM_386 ? IMAGE_FILE_MACHINE_I386 : IMAGE_FILE_MACHINE_X64;
  PayloadInfo->ImageCount = ImgIdx;
  PayloadInfo->EntryPoint = Entry;
  return EFI_SUCCESS;
}
