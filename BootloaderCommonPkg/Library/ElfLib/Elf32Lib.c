/** @file
  ELF library

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ElfLib.h>


STATIC
BOOLEAN
IsTextShdr (
  Elf32_Shdr *Shdr
  )
{
  return (BOOLEAN) ((Shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == SHF_ALLOC);
}


STATIC
BOOLEAN
IsDataShdr (
  Elf32_Shdr *Shdr
  )
{
  return (BOOLEAN) (Shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == (SHF_ALLOC | SHF_WRITE);
}


Elf32_Shdr *
EFIAPI
GetElf32SectionByIndex (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SecIdx
)
{
  Elf32_Ehdr        *Elf32Hdr;

  Elf32Hdr  = (Elf32_Ehdr *)ElfCt->ImageBase;
  if (SecIdx >= Elf32Hdr->e_shnum) {
    return NULL;
  }

  return (Elf32_Shdr *)(ElfCt->ImageBase + Elf32Hdr->e_shoff + SecIdx * Elf32Hdr->e_shentsize);
}



Elf32_Shdr *
EFIAPI
GetElf32SectionByName (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  CHAR8                *Name
)
{
  Elf32_Ehdr   *Elf32Hdr;
  Elf32_Shdr   *Elf32Shdr;
  CHAR8        *SecName;
  UINT32        Idx;

  Elf32Shdr = NULL;
  Elf32Hdr  = (Elf32_Ehdr *)ElfCt->ImageBase;
  for (Idx = 0; Idx < Elf32Hdr->e_shnum; Idx++) {
    SecName = GetElfSectionName (ElfCt, Idx);
    if ((SecName != NULL) && (AsciiStrCmp(Name, SecName) == 0)) {
      Elf32Shdr = GetElf32SectionByIndex (ElfCt, Idx);
      break;
    }
  }
  return Elf32Shdr;
}


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
  )
{
  Elf32_Ehdr   *Elf32Hdr;
  Elf32_Phdr   *ProgramHdr;
  Elf32_Phdr   *ProgramHdrBase;
  UINT16        Index;
  UINT8        *ImageBase;

  ImageBase = ElfCt->ImageBase;
  if ((ImageBase == NULL) || (EntryPoint == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Elf32Hdr         = (Elf32_Ehdr *)ImageBase;
  ProgramHdrBase = (Elf32_Phdr *)(ImageBase + Elf32Hdr->e_phoff);
  for (Index = 0; Index < Elf32Hdr->e_phnum; Index++) {
    ProgramHdr = (Elf32_Phdr *)((UINT8 *)ProgramHdrBase + Index * Elf32Hdr->e_phentsize);

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

  *EntryPoint = (VOID *)(UINTN)Elf32Hdr->e_entry;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
RelocateElf32Sections  (
  IN    ELF_IMAGE_CONTEXT      *ElfCt
  )
{
  Elf32_Ehdr      *Elf32Hdr;
  Elf32_Shdr      *Rel32Shdr;
  Elf32_Shdr      *Sec32Shdr;
  Elf32_Rel       *Rel32Entry;
  UINT8           *CurPtr;
  UINT32           Index;
  UINT32           RelIdx;
  UINT32           Offset;
  UINT32          *Ptr32;
  UINT8            RelType;
  CHAR8           *Name;

  Elf32Hdr  = (Elf32_Ehdr *)ElfCt->ImageBase;
  if (Elf32Hdr->e_machine != EM_386) {
    return EFI_UNSUPPORTED;
  }

  CurPtr  = ElfCt->ImageBase + Elf32Hdr->e_shoff;
  for (Index = 0; Index < Elf32Hdr->e_shnum; Index++) {
    Rel32Shdr = (Elf32_Shdr *)CurPtr;
    CurPtr  = CurPtr + Elf32Hdr->e_shentsize;
    if ((Rel32Shdr->sh_type == SHT_REL) || (Rel32Shdr->sh_type == SHT_RELA)) {
      Sec32Shdr = GetElf32SectionByIndex (ElfCt, Rel32Shdr->sh_info);
      if (!IsTextShdr(Sec32Shdr) && !IsDataShdr(Sec32Shdr)) {
        continue;
      }

      for (RelIdx = 0; RelIdx < Rel32Shdr->sh_size; RelIdx += Rel32Shdr->sh_entsize) {
        Rel32Entry = (Elf32_Rel *)((UINT8*)Elf32Hdr + Rel32Shdr->sh_offset + RelIdx);
        RelType = ELF32_R_TYPE(Rel32Entry->r_info);
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
            Offset = Sec32Shdr->sh_offset + (Rel32Entry->r_offset - Sec32Shdr->sh_addr);
            Ptr32  = (UINT32 *)(ElfCt->ImageBase + Offset);
            *Ptr32 = *Ptr32 - Sec32Shdr->sh_addr + Sec32Shdr->sh_offset + (UINT32)(UINTN)ElfCt->ImageBase;
            break;
          default:
            DEBUG ((DEBUG_INFO, "Unsupported relocation type %02X\n", RelType));
        }
      }

      Name = GetElfSectionName (ElfCt, Rel32Shdr->sh_info);
      if ((Name != NULL) && AsciiStrCmp (Name, ".text") == 0) {
        Elf32Hdr->e_entry = Elf32Hdr->e_entry - Sec32Shdr->sh_addr + (UINT32)(UINTN)ElfCt->ImageBase + Sec32Shdr->sh_offset;
      }
    }
  }

  return EFI_SUCCESS;
}

