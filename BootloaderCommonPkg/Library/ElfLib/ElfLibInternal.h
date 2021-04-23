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

/**
  Load ELF image which has 32-bit architecture

  @param[in]  ImageBase       Memory address of an image.
  @param[out] EntryPoint      The entry point of loaded ELF image.

  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.

**/
EFI_STATUS
LoadElf32Segments (
  IN    ELF_IMAGE_CONTEXT    *ElfCt,
  OUT       VOID            **EntryPoint
  );


/**
  Load ELF image which has 32-bit architecture

  @param[in]  ImageBase       Memory address of an image.
  @param[out] EntryPoint      The entry point of loaded ELF image.

  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.

**/
EFI_STATUS
LoadElf64Segments (
  IN    ELF_IMAGE_CONTEXT    *ElfCt,
  OUT       VOID            **EntryPoint
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
