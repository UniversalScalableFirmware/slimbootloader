/** @file
  ELF library

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ELF_LIB_H__
#define __ELF_LIB_H__

#include <Uefi/UefiBaseType.h>
#include <Standard/ElfCommon.h>
#include <Standard/Elf32.h>
#include <Standard/Elf64.h>

typedef struct {
  UINT8      *ImageBase;
  UINT32      ShStrOff;
  UINT32      ShStrLen;
  UINT8       EiClass;
} ELF_IMAGE_CONTEXT;

/**
  Check if the image is a bootable ELF image.

  @param[in]  ImageBase      Memory address of an image

  @retval     TRUE           Image is a bootable ELF image
  @retval     FALSE          Not a bootable ELF image
**/
BOOLEAN
EFIAPI
IsElfImage (
  IN  CONST VOID             *ImageBase
  );

/**
  Load the ELF image to specified address in ELF header.

  This function loads ELF image segments into memory address specified
  in ELF program header.

  @param[in]  ImageBase           Memory address of an image.
  @param[out] EntryPoint          The entry point of loaded ELF image.

  @retval EFI_INVALID_PARAMETER   Input parameters are not valid.
  @retval EFI_UNSUPPORTED         Unsupported binary type.
  @retval EFI_LOAD_ERROR          ELF binary loading error.
  @retval EFI_SUCCESS             ELF binary is loaded successfully.
**/
EFI_STATUS
LoadElfImage (
  IN        VOID                  *ImageBase,
  OUT       VOID                 **EntryPoint
  );


/**
  Relocated the ELF image at current loaded address.

  This function relocates ELF image sections at current loaded address.

  @param[in]  ImageBase           Memory address of an image.
  @param[out] EntryPoint          The entry point of loaded ELF image.

  @retval EFI_INVALID_PARAMETER   Input parameters are not valid.
  @retval EFI_UNSUPPORTED         Unsupported binary type.
  @retval EFI_LOAD_ERROR          ELF binary loading error.
  @retval EFI_SUCCESS             ELF binary is loaded successfully.
**/
EFI_STATUS
RelocateElfImage (
  IN  CONST VOID                  *ElfBuffer,
  OUT       VOID                 **EntryPoint
  );

EFI_STATUS
EFIAPI
ParseElfImage (
  IN  VOID                 *ImageBase,
  IN  ELF_IMAGE_CONTEXT    *ElfCt
);

CHAR8 *
EFIAPI
GetElfSectionName (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SecIdx
  );

EFI_STATUS
EFIAPI
RelocateElfSections  (
  IN  ELF_IMAGE_CONTEXT    *ElfCt
);

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

#endif /* __ELF_LIB_H__ */
