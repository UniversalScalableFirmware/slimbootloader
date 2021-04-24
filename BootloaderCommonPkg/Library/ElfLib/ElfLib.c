/** @file
  ELF library

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ElfLibInternal.h"

/**
  Check if the image has ELF Header

  @param[in]  ImageBase       Memory address of an image.

  @retval     TRUE if found ELF Header, otherwise FALSE

**/
STATIC
BOOLEAN
IsElfHeader (
  IN  CONST UINT8             *ImageBase
  )
{
  return ((ImageBase != NULL) &&
          (ImageBase[EI_MAG0] == ELFMAG0) &&
          (ImageBase[EI_MAG1] == ELFMAG1) &&
          (ImageBase[EI_MAG2] == ELFMAG2) &&
          (ImageBase[EI_MAG3] == ELFMAG3));
}

/**
  Check if the image is 32-bit ELF Format

  @param[in]  ImageBase       Memory address of an image.

  @retval     TRUE if ELF32, otherwise FALSE

**/
STATIC
BOOLEAN
IsElfFormat (
  IN  CONST UINT8             *ImageBase
  )
{
  Elf32_Ehdr                  *Elf32Hdr;
  Elf64_Ehdr                  *Elf64Hdr;

  if (ImageBase == NULL) {
    return FALSE;
  }

  Elf32Hdr = (Elf32_Ehdr *)ImageBase;

  //
  // Support little-endian only
  //
  if (Elf32Hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
    return FALSE;
  }

  //
  // Check 32/64-bit architecture
  //
  if (Elf32Hdr->e_ident[EI_CLASS] == ELFCLASS64) {
    Elf64Hdr = (Elf64_Ehdr *)Elf32Hdr;
    Elf32Hdr = NULL;
  } else if (Elf32Hdr->e_ident[EI_CLASS] == ELFCLASS32) {
    Elf64Hdr = NULL;
  } else {
    return FALSE;
  }

  if (Elf64Hdr != NULL) {
    //
    // Support intel architecture only for now
    //
    if (Elf64Hdr->e_machine != EM_X86_64) {
      return FALSE;
    }

    //
    //  Support ELF types: EXEC (Executable file), DYN (Shared object file)
    //
    if ((Elf64Hdr->e_type != ET_EXEC) && (Elf64Hdr->e_type != ET_DYN)) {
      return FALSE;
    }

    //
    // Support current ELF version only
    //
    if (Elf64Hdr->e_version != EV_CURRENT) {
      return FALSE;
    }
  } else {
    //
    // Support intel architecture only for now
    //
    if (Elf32Hdr->e_machine != EM_386) {
      return FALSE;
    }

    //
    //  Support ELF types: EXEC (Executable file), DYN (Shared object file)
    //
    if ((Elf32Hdr->e_type != ET_EXEC) && (Elf32Hdr->e_type != ET_DYN)) {
      return FALSE;
    }

    //
    // Support current ELF version only
    //
    if (Elf32Hdr->e_version != EV_CURRENT) {
      return FALSE;
    }
  }
  return TRUE;
}


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
  )
{
  return ((ImageBase != NULL) &&
          (IsElfHeader (ImageBase)) &&
          (IsElfFormat ((CONST UINT8 *)ImageBase)));
}


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
)
{
  Elf32_Ehdr  *Elf32Hdr;
  Elf64_Ehdr  *Elf64Hdr;
  Elf32_Shdr  *Elf32Shdr;
  Elf64_Shdr  *Elf64Shdr;

  if ((ElfCt == NULL) || (ImageBase == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ElfCt->ImageBase = (UINT8 *)ImageBase;
  if (!IsElfImage (ElfCt->ImageBase)) {
    return EFI_UNSUPPORTED;
  }

  Elf32Hdr = (Elf32_Ehdr *)ElfCt->ImageBase;
  ElfCt->EiClass = Elf32Hdr->e_ident[EI_CLASS];
  if (ElfCt->EiClass == ELFCLASS32) {
    Elf32Shdr = (Elf32_Shdr *)GetElf32SectionByIndex (ElfCt, Elf32Hdr->e_shstrndx);
    if (Elf32Shdr == NULL) {
      return EFI_UNSUPPORTED;
    }
    ElfCt->Entry     = (UINTN)(ElfCt->ImageBase + Elf32Hdr->e_entry);
    ElfCt->ShNum     = Elf32Hdr->e_shnum;
    ElfCt->PhNum     = Elf32Hdr->e_phnum;
    ElfCt->ShStrLen  = Elf32Shdr->sh_size;
    ElfCt->ShStrOff  = Elf32Shdr->sh_offset;
  } else {
    Elf64Hdr  = (Elf64_Ehdr *)Elf32Hdr;
    Elf64Shdr = (Elf64_Shdr *)GetElf64SectionByIndex (ElfCt, Elf64Hdr->e_shstrndx);
    if (Elf64Shdr == NULL) {
      return EFI_UNSUPPORTED;
    }
    ElfCt->Entry     = (UINTN)(ElfCt->ImageBase + Elf64Hdr->e_entry);
    ElfCt->ShNum     = Elf64Hdr->e_shnum;
    ElfCt->PhNum     = Elf64Hdr->e_phnum;
    ElfCt->ShStrLen  = (UINT32)Elf64Shdr->sh_size;
    ElfCt->ShStrOff  = (UINT32)Elf64Shdr->sh_offset;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS    Status;
  ELF_IMAGE_CONTEXT   ElfCt;

  if (ElfBuffer == NULL || EntryPoint == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ParseElfImage (ElfBuffer, &ElfCt);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = LoadElfSegments (&ElfCt);
  if (!EFI_ERROR(Status)) {
    *EntryPoint = (VOID *)ElfCt.Entry;
  }
  return Status;
}

/**
  Load the ELF segments to specified address in ELF header.

  This function loads ELF image segments into memory address specified
  in ELF program header.

  @param[in]  ElfCt               ELF image context pointer.

  @retval EFI_INVALID_PARAMETER   Input parameters are not valid.
  @retval EFI_UNSUPPORTED         Unsupported binary type.
  @retval EFI_LOAD_ERROR          ELF binary loading error.
  @retval EFI_SUCCESS             ELF binary is loaded successfully.
**/
EFI_STATUS
EFIAPI
LoadElfSegments (
  IN  ELF_IMAGE_CONTEXT       *ElfCt
  )
{
  EFI_STATUS          Status;

  if (ElfCt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_UNSUPPORTED;
  if (ElfCt->EiClass == ELFCLASS32) {
    Status = LoadElf32Segments (ElfCt);
  } else if (ElfCt->EiClass == ELFCLASS64) {
    Status = LoadElf64Segments (ElfCt);
  }

  return Status;
}


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
  )
{
  Elf32_Shdr      *Elf32Shdr;
  Elf64_Shdr      *Elf64Shdr;
  CHAR8           *Name;

  if ((ElfCt == NULL) || (SecName == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Name = NULL;
  if (ElfCt->EiClass == ELFCLASS32) {
    Elf32Shdr = GetElf32SectionByIndex (ElfCt, SecIdx);
    if ((Elf32Shdr != NULL) && (Elf32Shdr->sh_name < ElfCt->ShStrLen)) {
      Name = (CHAR8 *)(ElfCt->ImageBase + ElfCt->ShStrOff + Elf32Shdr->sh_name);
    }
  } else if (ElfCt->EiClass == ELFCLASS64) {
    Elf64Shdr = GetElf64SectionByIndex (ElfCt, SecIdx);
    if ((Elf64Shdr != NULL) && (Elf64Shdr->sh_name < ElfCt->ShStrLen)) {
      Name = (CHAR8 *)(ElfCt->ImageBase + ElfCt->ShStrOff + Elf64Shdr->sh_name);
    }
  }

  if (Name == NULL) {
    return EFI_NOT_FOUND;
  }

  *SecName = Name;
  return EFI_SUCCESS;
}


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
  )
{
  Elf32_Shdr      *Elf32Shdr;
  Elf64_Shdr      *Elf64Shdr;
  BOOLEAN          Found;

  if ((ElfCt == NULL) || (SecPos == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Found = FALSE;
  if (ElfCt->EiClass == ELFCLASS32) {
    Elf32Shdr = GetElf32SectionByIndex (ElfCt, SecIdx);
    if ((Elf32Shdr != NULL) && (Elf32Shdr->sh_name < ElfCt->ShStrLen)) {
      SecPos->Offset = (UINTN)Elf32Shdr->sh_offset;
      SecPos->Length = (UINTN)Elf32Shdr->sh_size;
      Found = TRUE;
    }
  } else if (ElfCt->EiClass == ELFCLASS64) {
    Elf64Shdr = GetElf64SectionByIndex (ElfCt, SecIdx);
    if ((Elf64Shdr != NULL) && (Elf64Shdr->sh_name < ElfCt->ShStrLen)) {
      SecPos->Offset = (UINTN)Elf64Shdr->sh_offset;
      SecPos->Length = (UINTN)Elf64Shdr->sh_size;
      Found = TRUE;
    }
  }

  if (!Found) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}


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
  )
{
  EFI_STATUS  Status;

  if (ElfCt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_UNSUPPORTED;
  if (ElfCt->EiClass == ELFCLASS32) {
    Status = RelocateElf32Sections (ElfCt);
  } else if (ElfCt->EiClass == ELFCLASS64) {
    Status = RelocateElf64Sections (ElfCt);
  }
  return Status;
}


/**
  Get a ELF program segment loading info.

  @param[in]  ElfCt               ELF image context pointer.
  @param[in]  SegIdx              ELF segment index.
  @param[out] SegInfo             The pointer to the segment info.

  @retval EFI_INVALID_PARAMETER   ElfCt or SecPos is NULL.
  @retval EFI_NOT_FOUND           Could not find the section.
  @retval EFI_SUCCESS             Section posistion was filled successfully.
**/
EFI_STATUS
EFIAPI
GetElfSegmentInfo (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SegIdx,
  OUT SEGMENT_INFO         *SegInfo
  )
{
  Elf32_Phdr      *Elf32Phdr;
  Elf64_Phdr      *Elf64Phdr;
  BOOLEAN          Found;

  if ((ElfCt == NULL) || (SegInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Found = FALSE;
  if (ElfCt->EiClass == ELFCLASS32) {
    Elf32Phdr = GetElf32SegmentByIndex (ElfCt, SegIdx);
    if (Elf32Phdr != NULL) {
      SegInfo->Offset  = Elf32Phdr->p_offset;
      SegInfo->Length  = Elf32Phdr->p_filesz;
      SegInfo->MemLen  = Elf32Phdr->p_memsz;
      SegInfo->MemAddr = Elf32Phdr->p_paddr;
      Found = TRUE;
    }
  } else if (ElfCt->EiClass == ELFCLASS64) {
    Elf64Phdr = GetElf64SegmentByIndex (ElfCt, SegIdx);
    if (Elf64Phdr != NULL) {
      SegInfo->Offset  = (UINTN)Elf64Phdr->p_offset;
      SegInfo->Length  = (UINTN)Elf64Phdr->p_filesz;
      SegInfo->MemLen  = (UINTN)Elf64Phdr->p_memsz;
      SegInfo->MemAddr = (UINTN)Elf64Phdr->p_paddr;
      Found = TRUE;
    }
  }

  if (!Found) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
