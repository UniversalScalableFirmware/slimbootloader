/** @file
  ELF library

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ElfLib.h>

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
  Elf_Ehdr                  *ElfHdr;

  if (ImageBase == NULL) {
    return FALSE;
  }

  ElfHdr = (Elf_Ehdr *)ImageBase;

  //
  // Check 32/64-bit architecture
  //
  if (ElfHdr->e_ident[EI_CLASS] != ELFCLASS) {
    return FALSE;
  }

  //
  // Support little-endian only
  //
  if (ElfHdr->e_ident[EI_DATA] != ELFDATA2LSB) {
    return FALSE;
  }

  //
  // Support intel architecture only for now
  //
  if (ElfHdr->e_machine != ELF_EM) {
    return FALSE;
  }

  //
  //  Support ELF types: EXEC (Executable file), DYN (Shared object file)
  //
  if ((ElfHdr->e_type != ET_EXEC) && (ElfHdr->e_type != ET_DYN)) {
    return FALSE;
  }

  //
  // Support current ELF version only
  //
  if (ElfHdr->e_version != EV_CURRENT) {
    return FALSE;
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


CHAR8 *
EFIAPI
GetSectionName (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SecIdx
)
{
  Elf32_Shdr      *ElfShdr;
  CHAR8           *Name;

  Name = NULL;
  ElfShdr = GetSectionByIndex (ElfCt, SecIdx);
  if ((ElfShdr != NULL) && (ElfShdr->sh_name < ElfCt->ShStrLen)) {
    Name = (CHAR8 *)(ElfCt->ImageBase + ElfCt->ShStrOff + ElfShdr->sh_name);
  }

  return Name;
}


Elf32_Shdr *
EFIAPI
GetSectionByIndex (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SecIdx
)
{
  Elf_Ehdr        *ElfHdr;

  ElfHdr  = (Elf_Ehdr *)ElfCt->ImageBase;
  if (SecIdx >= ElfHdr->e_shnum) {
    return NULL;
  }

  return (Elf32_Shdr *)(ElfCt->ImageBase + ElfHdr->e_shoff + SecIdx * ElfHdr->e_shentsize);
}


Elf32_Shdr *
EFIAPI
GetSectionByName (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  CHAR8                *Name
)
{
  Elf_Ehdr   *ElfHdr;
  Elf32_Shdr *ElfShdr;
  CHAR8      *SecName;
  UINT32      Idx;

  ElfShdr = NULL;
  ElfHdr  = (Elf_Ehdr *)ElfCt->ImageBase;
  for (Idx = 0; Idx < ElfHdr->e_shnum; Idx++) {
    SecName = GetSectionName (ElfCt, Idx);
    if ((SecName != NULL) && (AsciiStrCmp(Name, SecName) == 0)) {
      ElfShdr = GetSectionByIndex (ElfCt, Idx);
      break;
    }
  }
  return ElfShdr;
}


EFI_STATUS
EFIAPI
ParseElfImage (
  IN  VOID                 *ImageBase,
  IN  ELF_IMAGE_CONTEXT    *ElfCt
)
{
  Elf_Ehdr   *ElfHdr;
  Elf32_Shdr *ElfShdr;

  if ((ElfCt == NULL) || (ImageBase == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ElfCt->ImageBase = (UINT8 *)ImageBase;
  if (!IsElfImage (ElfCt->ImageBase)) {
    return EFI_UNSUPPORTED;
  }

  ElfHdr = (Elf_Ehdr *)ElfCt->ImageBase;
  ElfShdr = GetSectionByIndex (ElfCt, ElfHdr->e_shstrndx);
  if (ElfShdr == NULL) {
    return EFI_UNSUPPORTED;
  }

  ElfCt->ShStrLen  = ElfShdr->sh_size;
  ElfCt->ShStrOff  = ElfShdr->sh_offset;

  return EFI_SUCCESS;
}


STATIC
BOOLEAN
IsTextShdr (
  Elf_Shdr *Shdr
  )
{
  return (BOOLEAN) ((Shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == SHF_ALLOC);
}


STATIC
BOOLEAN
IsDataShdr (
  Elf_Shdr *Shdr
  )
{
  return (BOOLEAN) (Shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == (SHF_ALLOC | SHF_WRITE);
}


EFI_STATUS
EFIAPI
RelocateElfSections  (
  IN    ELF_IMAGE_CONTEXT      *ElfCt
  )
{
  Elf_Ehdr        *ElfHdr;
  Elf32_Shdr      *RelShdr;
  UINT8           *CurPtr;
  UINT32           Index;
  UINT32           RelIdx;
  UINT32           Offset;
  UINT32          *Ptr32;
  UINT8            RelType;
  Elf_Shdr        *SecShdr;
  Elf32_Rel       *RelEntry;
  CHAR8           *Name;

  ElfHdr  = (Elf_Ehdr *)ElfCt->ImageBase;
  if (ElfHdr->e_machine != EM_386) {
    return EFI_UNSUPPORTED;
  }

  CurPtr  = ElfCt->ImageBase + ElfHdr->e_shoff;
  for (Index = 0; Index < ElfHdr->e_shnum; Index++) {
    RelShdr = (Elf32_Shdr *)CurPtr;
    CurPtr  = CurPtr + ElfHdr->e_shentsize;
    if ((RelShdr->sh_type == SHT_REL) || (RelShdr->sh_type == SHT_RELA)) {
      SecShdr = GetSectionByIndex (ElfCt, RelShdr->sh_info);
      if (!IsTextShdr(SecShdr) && !IsDataShdr(SecShdr)) {
        continue;
      }

      for (RelIdx = 0; RelIdx < RelShdr->sh_size; RelIdx += RelShdr->sh_entsize) {
        RelEntry = (Elf_Rel *)((UINT8*)ElfHdr + RelShdr->sh_offset + RelIdx);
        RelType = ELF_R_TYPE(RelEntry->r_info);
        switch (RelType) {
          case R_386_NONE:
          case R_386_PC32:
            //
            // No fixup entry required.
            //
            break;
          case R_386_32:
            //
            // Creates a relative relocation entry from the absolute entry.
            //
            Offset = SecShdr->sh_offset + (RelEntry->r_offset - SecShdr->sh_addr);
            Ptr32  = (UINT32 *)(ElfCt->ImageBase + Offset);
            *Ptr32 = *Ptr32 - SecShdr->sh_addr + SecShdr->sh_offset + (UINT32)ElfCt->ImageBase;
            break;
          default:
            DEBUG ((DEBUG_INFO, "Unsupported relocation type %02X\n", RelType));
        }
      }

      Name = GetSectionName (ElfCt, RelShdr->sh_info);
      if ((Name != NULL) && AsciiStrCmp (Name, ".text") == 0) {
        ElfHdr->e_entry = ElfHdr->e_entry - SecShdr->sh_addr + (UINT32)ElfCt->ImageBase + SecShdr->sh_offset;
      }
    }
  }

  return EFI_SUCCESS;
}


/**
  Load ELF image which has 32-bit architecture

  @param[in]  ImageBase       Memory address of an image.
  @param[out] EntryPoint      The entry point of loaded ELF image.

  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.

**/
STATIC
EFI_STATUS
LoadElfSegments (
  IN    ELF_IMAGE_CONTEXT      *ElfCt,
  OUT       VOID        **EntryPoint
  )
{
  Elf_Ehdr   *ElfHdr;
  Elf_Phdr   *ProgramHdr;
  Elf_Phdr   *ProgramHdrBase;
  UINT16      Index;
  UINT8      *ImageBase;

  ImageBase = ElfCt->ImageBase;
  if ((ImageBase == NULL) || (EntryPoint == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ElfHdr         = (Elf_Ehdr *)ImageBase;
  ProgramHdrBase = (Elf_Phdr *)(ImageBase + ElfHdr->e_phoff);
  for (Index = 0; Index < ElfHdr->e_phnum; Index++) {
    ProgramHdr = (Elf_Phdr *)((UINT8 *)ProgramHdrBase + Index * ElfHdr->e_phentsize);

    if ((ProgramHdr->p_type != PT_LOAD) ||
        (ProgramHdr->p_memsz == 0) ||
        (ProgramHdr->p_offset == 0)) {
      continue;
    }

    if (ProgramHdr->p_filesz > ProgramHdr->p_memsz) {
      return EFI_LOAD_ERROR;
    }

    CopyMem ((VOID *)(UINTN)ProgramHdr->p_paddr,
        ImageBase + ProgramHdr->p_offset,
        (UINTN)ProgramHdr->p_filesz);

    if (ProgramHdr->p_memsz > ProgramHdr->p_filesz) {
      ZeroMem ((VOID *)(UINTN)(ProgramHdr->p_paddr + ProgramHdr->p_filesz),
        (UINTN)(ProgramHdr->p_memsz - ProgramHdr->p_filesz));
    }
  }

  *EntryPoint = (VOID *)(UINTN)ElfHdr->e_entry;

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

  Status = LoadElfSegments (&ElfCt, EntryPoint);
  return Status;
}
