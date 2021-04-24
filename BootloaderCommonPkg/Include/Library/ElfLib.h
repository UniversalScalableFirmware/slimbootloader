/** @file
  ELF library

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ELF_LIB_H__
#define __ELF_LIB_H__

#include <PiPei.h>

#define  ELF_CLASS32   1
#define  ELF_CLASS64   2

typedef struct {
  UINT8      *ImageBase;
  UINT32      EiClass;
  UINT32      ShNum;
  UINTN       ShStrOff;
  UINTN       ShStrLen;
  UINTN       Entry;
} ELF_IMAGE_CONTEXT;

typedef struct {
  UINTN       Offset;
  UINTN       Length;
} SECTION_POS;


/**
  Check if the image is a bootable ELF image.

  @param[in]  ImageBase      Memory address of an image

  @retval     TRUE           Image is a bootable ELF image
  @retval     FALSE          Not a bootable ELF image
**/
BOOLEAN
EFIAPI
IsElfImage (
  IN  CONST VOID            *ImageBase
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
EFIAPI
LoadElfImage (
  IN        VOID                  *ImageBase,
  OUT       VOID                 **EntryPoint
  );


/**
  Parse the ELF image info.

  @param[in]  ImageBase      Memory address of an image.
  @param[out] ElfCt          The EFL image context pointer.

  @retval EFI_INVALID_PARAMETER   Input parameters are not valid.
  @retval EFI_UNSUPPORTED         Unsupported binary type.
  @retval EFI_LOAD_ERROR          ELF binary loading error.
  @retval EFI_SUCCESS             ELF binary is loaded successfully.
**/
EFI_STATUS
EFIAPI
ParseElfImage (
  IN  VOID                 *ImageBase,
  OUT ELF_IMAGE_CONTEXT    *ElfCt
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
EFIAPI
LoadElfImage (
  IN        VOID                  *ElfBuffer,
  OUT       VOID                 **EntryPoint
  );


/**
  Get a ELF section name from its index.

  @param[in]  ElfCt               ELF image context pointer.
  @param[in]  SecIdx              ELF section index.
  @param[out] SecName             The pointer to the section name.

  @retval EFI_INVALID_PARAMETER   ElfCt or SecName is NULL.
  @retval EFI_NOT_FOUND           Could not find the section.
  @retval EFI_SUCCESS             Section name was filled successfully.
**/
EFI_STATUS
EFIAPI
GetElfSectionName (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SecIdx,
  OUT CHAR8               **SecName
  );


/**
  Get a ELF section name from its index.

  @param[in]  ElfCt               ELF image context pointer.
  @param[in]  SecIdx              ELF section index.
  @param[out] SecPos              The pointer to the section postion.

  @retval EFI_INVALID_PARAMETER   ElfCt or SecPos is NULL.
  @retval EFI_NOT_FOUND           Could not find the section.
  @retval EFI_SUCCESS             Section posistion was filled successfully.
**/
EFI_STATUS
EFIAPI
GetElfSectionPos (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SecIdx,
  OUT SECTION_POS          *SecPos
  );


/**
  Relocate all sections in a ELF image to current location.

  @param[in]  ElfCt               ELF image context pointer.

  @retval EFI_INVALID_PARAMETER   ElfCt is NULL.
  @retval EFI_UNSUPPORTED         Relocation is not supported.
  @retval EFI_SUCCESS             ELF image was relocated successfully.
**/
EFI_STATUS
EFIAPI
RelocateElfSections  (
  IN    ELF_IMAGE_CONTEXT      *ElfCt
  );


#endif /* __ELF_LIB_H__ */
