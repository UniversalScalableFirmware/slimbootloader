/** @file
  ELF library

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ElfLibInternal.h"

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
  EFI_STATUS    Status;
  Elf32_Ehdr   *Elf32Hdr;
  Elf32_Shdr   *Elf32Shdr;
  CHAR8        *SecName;
  UINT32        Idx;

  Elf32Shdr = NULL;
  Elf32Hdr  = (Elf32_Ehdr *)ElfCt->ImageBase;
  for (Idx = 0; Idx < Elf32Hdr->e_shnum; Idx++) {
    Status = GetElfSectionName (ElfCt, Idx, &SecName);
    if (!EFI_ERROR(Status) && (AsciiStrCmp(Name, SecName) == 0)) {
      Elf32Shdr = GetElf32SectionByIndex (ElfCt, Idx);
      break;
    }
  }
  return Elf32Shdr;
}

Elf32_Phdr *
EFIAPI
GetElf32SegmentByIndex (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SegIdx
)
{
  Elf32_Ehdr        *Elf32Hdr;

  Elf32Hdr  = (Elf32_Ehdr *)ElfCt->ImageBase;
  if (SegIdx >= Elf32Hdr->e_phnum) {
    return NULL;
  }

  return (Elf32_Phdr *)(ElfCt->ImageBase + Elf32Hdr->e_phoff + SegIdx * Elf32Hdr->e_shentsize);
}

/**
  Load ELF image which has 32-bit architecture

  @param[in]  ElfCt               ELF image context pointer.

  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.

**/
EFI_STATUS
LoadElf32Segments (
  IN    ELF_IMAGE_CONTEXT    *ElfCt
  )
{
  Elf32_Ehdr   *Elf32Hdr;
  Elf32_Phdr   *ProgramHdr;
  Elf32_Phdr   *ProgramHdrBase;
  UINT16        Index;
  UINT8        *ImageBase;
  UINTN         Delta;

  if (ElfCt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ImageBase = ElfCt->ImageBase;
  if (ElfCt->NewBase != NULL) {
    Delta = MAX_UINTN;
  } else {
    Delta = 0;
  }

  Elf32Hdr       = (Elf32_Ehdr *)ImageBase;
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

    if (Delta == MAX_UINTN) {
      Delta = (UINTN)ElfCt->NewBase - (ProgramHdr->p_paddr - (ProgramHdr->p_paddr & 0xFFF));
    }
    CopyMem ((VOID *)(UINTN)(ProgramHdr->p_paddr + Delta),
        ImageBase + ProgramHdr->p_offset,
        (UINTN)ProgramHdr->p_filesz);

    if (ProgramHdr->p_memsz > ProgramHdr->p_filesz) {
      ZeroMem ((VOID *)(UINTN)(ProgramHdr->p_paddr + Delta + ProgramHdr->p_filesz),
        (UINTN)(ProgramHdr->p_memsz - ProgramHdr->p_filesz));
    }
  }

  ElfCt->Delta = Delta;
  ElfCt->Entry = Elf32Hdr->e_entry + Delta;

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
      DEBUG ((DEBUG_INFO, "Relocate SEC %d\n", Rel32Shdr->sh_info));
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
            if (ElfCt->Delta == 0) {
              Offset  = Sec32Shdr->sh_offset + (Rel32Entry->r_offset - Sec32Shdr->sh_addr);
              Ptr32   = (UINT32 *)(ElfCt->ImageBase + Offset);
              *Ptr32 += Sec32Shdr->sh_offset + (UINT32)(UINTN)ElfCt->ImageBase - Sec32Shdr->sh_addr;
            } else {
              Ptr32   = (UINT32 *)(Rel32Entry->r_offset + ElfCt->Delta);
              *Ptr32 += (UINT32)ElfCt->Delta;
            }
            break;
          default:
            DEBUG ((DEBUG_INFO, "Unsupported relocation type %02X\n", RelType));
        }
      }

      if (ElfCt->NewBase == NULL) {
        ElfCt->Entry = (UINTN)(Elf32Hdr->e_entry - Sec32Shdr->sh_addr + (UINT32)(UINTN)ElfCt->ImageBase + Sec32Shdr->sh_offset);
      } else {
        ElfCt->Entry = (UINTN)(Elf32Hdr->e_entry + ElfCt->Delta);
      }
    }
  }

  return EFI_SUCCESS;
}
