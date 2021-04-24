/** @file
  ELF library

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_LIB_INTERNAL_H_
#define _EFI_LIB_INTERNAL_H_

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ElfLib.h>
#include <ElfCommon.h>
#include <Elf32.h>
#include <Elf64.h>


Elf32_Shdr *
EFIAPI
GetElf32SectionByIndex (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SecIdx
);

Elf64_Shdr *
EFIAPI
GetElf64SectionByIndex (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SecIdx
);

Elf32_Shdr *
EFIAPI
GetElf32SectionByName (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  CHAR8                *Name
);

Elf64_Shdr *
EFIAPI
GetElf64SectionByName (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  CHAR8                *Name
);

Elf32_Phdr *
EFIAPI
GetElf32SegmentByIndex (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SegIdx
);

Elf64_Phdr *
EFIAPI
GetElf64SegmentByIndex (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SegIdx
);

/**
  Load ELF image which has 32-bit architecture

  @param[in]  ElfCt               ELF image context pointer.

  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.

**/
EFI_STATUS
LoadElf32Segments (
  IN    ELF_IMAGE_CONTEXT    *ElfCt
  );

/**
  Load ELF image which has 64-bit architecture

  @param[in]  ImageBase       Memory address of an image.
  @param[out] EntryPoint      The entry point of loaded ELF image.

  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.

**/
EFI_STATUS
LoadElf64Segments (
  IN    ELF_IMAGE_CONTEXT    *ElfCt
  );


EFI_STATUS
EFIAPI
RelocateElf32Sections  (
  IN    ELF_IMAGE_CONTEXT      *ElfCt
  );


EFI_STATUS
EFIAPI
RelocateElf64Sections  (
  IN    ELF_IMAGE_CONTEXT      *ElfCt
  );

#endif
