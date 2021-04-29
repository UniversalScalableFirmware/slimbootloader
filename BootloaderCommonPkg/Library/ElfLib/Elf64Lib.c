/** @file
  ELF library

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ElfLibInternal.h"

STATIC
BOOLEAN
IsTextShdr (
  Elf64_Shdr *Shdr
  )
{
  return (BOOLEAN) ((Shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == SHF_ALLOC);
}


STATIC
BOOLEAN
IsDataShdr (
  Elf64_Shdr *Shdr
  )
{
  return (BOOLEAN) (Shdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == (SHF_ALLOC | SHF_WRITE);
}

Elf64_Shdr *
EFIAPI
GetElf64SectionByIndex (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SecIdx
)
{
  Elf64_Ehdr        *Elf64Hdr;

  Elf64Hdr  = (Elf64_Ehdr *)ElfCt->ImageBase;
  if (SecIdx >= Elf64Hdr->e_shnum) {
    return NULL;
  }

  return (Elf64_Shdr *)(ElfCt->ImageBase + Elf64Hdr->e_shoff + SecIdx * Elf64Hdr->e_shentsize);
}

Elf64_Shdr *
EFIAPI
GetElf64SectionByName (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  CHAR8                *Name
)
{
  EFI_STATUS    Status;
  Elf64_Ehdr   *Elf64Hdr;
  Elf64_Shdr   *Elf64Shdr;
  CHAR8        *SecName;
  UINT32        Idx;

  Elf64Shdr = NULL;
  Elf64Hdr  = (Elf64_Ehdr *)ElfCt->ImageBase;
  for (Idx = 0; Idx < Elf64Hdr->e_shnum; Idx++) {
    Status = GetElfSectionName (ElfCt, Idx, &SecName);
    if (!EFI_ERROR(Status) && (AsciiStrCmp(Name, SecName) == 0)) {
      Elf64Shdr = GetElf64SectionByIndex (ElfCt, Idx);
      break;
    }
  }
  return Elf64Shdr;
}

Elf64_Phdr *
EFIAPI
GetElf64SegmentByIndex (
  IN  ELF_IMAGE_CONTEXT    *ElfCt,
  IN  UINT32                SegIdx
)
{
  Elf64_Ehdr        *Elf64Hdr;

  Elf64Hdr  = (Elf64_Ehdr *)ElfCt->ImageBase;
  if (SegIdx >= Elf64Hdr->e_phnum) {
    return NULL;
  }

  return (Elf64_Phdr *)(ElfCt->ImageBase + Elf64Hdr->e_phoff + SegIdx * Elf64Hdr->e_shentsize);
}


/**
  Load ELF image which has 32-bit architecture

  @param[in]  ElfCt               ELF image context pointer.

  @retval EFI_SUCCESS         ELF binary is loaded successfully.
  @retval Others              Loading ELF binary fails.

**/
EFI_STATUS
EFIAPI
LoadElf64Segments (
  IN    ELF_IMAGE_CONTEXT    *ElfCt
  )
{
  Elf64_Ehdr   *Elf64Hdr;
  Elf64_Phdr   *ProgramHdr;
  Elf64_Phdr   *ProgramHdrBase;
  UINT16        Index;
  UINT8        *ImageBase;
  UINTN         Delta;

  if (ElfCt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ImageBase = ElfCt->ImageBase;
  if (ElfCt->NewBase != NULL) {
    Delta = MAX_UINT32;
  } else {
    Delta = 0;
  }

  Elf64Hdr       = (Elf64_Ehdr *)ImageBase;
  ProgramHdrBase = (Elf64_Phdr *)(ImageBase + Elf64Hdr->e_phoff);
  for (Index = 0; Index < Elf64Hdr->e_phnum; Index++) {
    ProgramHdr = (Elf64_Phdr *)((UINT8 *)ProgramHdrBase + Index * Elf64Hdr->e_phentsize);

    if ((ProgramHdr->p_type != PT_LOAD) ||
        (ProgramHdr->p_memsz == 0) ||
        (ProgramHdr->p_offset == 0)) {
      continue;
    }

    if (ProgramHdr->p_filesz > ProgramHdr->p_memsz) {
      return EFI_LOAD_ERROR;
    }

    if (Delta == MAX_UINTN) {
      Delta = (UINTN)(ElfCt->NewBase - (ProgramHdr->p_paddr - (ProgramHdr->p_paddr & 0xFFF)));
    }
    CopyMem ((VOID *)(UINTN)ProgramHdr->p_paddr,
        ImageBase + ProgramHdr->p_offset,
        (UINTN)ProgramHdr->p_filesz);

    if (ProgramHdr->p_memsz > ProgramHdr->p_filesz) {
      ZeroMem ((VOID *)(UINTN)(ProgramHdr->p_paddr + ProgramHdr->p_filesz),
        (UINTN)(ProgramHdr->p_memsz - ProgramHdr->p_filesz));
    }
  }

  ElfCt->Delta = Delta;
  ElfCt->Entry = (UINTN)Elf64Hdr->e_entry;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RelocateElf64Sections  (
  IN    ELF_IMAGE_CONTEXT      *ElfCt
  )
{
  Elf64_Ehdr      *Elf64Hdr;
  Elf64_Shdr      *Rel64Shdr;
  Elf64_Shdr      *Sec64Shdr;
  Elf64_Rel       *Rel64Entry;
  UINT8           *CurPtr;
  UINT32           Index;
  UINT64           RelIdx;
  UINT64           Offset;
  UINT32          *Ptr32;
  UINT64          *Ptr64;
  UINT32           RelType;

  Elf64Hdr  = (Elf64_Ehdr *)ElfCt->ImageBase;
  if (Elf64Hdr->e_machine != EM_X86_64) {
    return EFI_UNSUPPORTED;
  }

  CurPtr  = ElfCt->ImageBase + Elf64Hdr->e_shoff;
  for (Index = 0; Index < Elf64Hdr->e_shnum; Index++) {
    Rel64Shdr = (Elf64_Shdr *)CurPtr;
    CurPtr  = CurPtr + Elf64Hdr->e_shentsize;
    if ((Rel64Shdr->sh_type == SHT_REL) || (Rel64Shdr->sh_type == SHT_RELA)) {
      Sec64Shdr = GetElf64SectionByIndex (ElfCt, Rel64Shdr->sh_info);
      if (!IsTextShdr(Sec64Shdr) && !IsDataShdr(Sec64Shdr)) {
        continue;
      }

      for (RelIdx = 0; RelIdx < Rel64Shdr->sh_size; RelIdx += Rel64Shdr->sh_entsize) {
        Rel64Entry = (Elf64_Rel *)((UINT8*)Elf64Hdr + Rel64Shdr->sh_offset + RelIdx);
        RelType = ELF64_R_TYPE(Rel64Entry->r_info);
        switch (RelType) {
          case R_X86_64_NONE:
          case R_X86_64_PC32:
          case R_X86_64_PLT32:
          case R_X86_64_GOTPCREL:
          case R_X86_64_GOTPCRELX:
          case R_X86_64_REX_GOTPCRELX:
            break;
          case R_X86_64_64:
            if (ElfCt->Delta == 0) {
              Offset  = Sec64Shdr->sh_offset + (Rel64Entry->r_offset - Sec64Shdr->sh_addr);
              Ptr64   = (UINT64 *)(ElfCt->ImageBase + Offset);
              *Ptr64 += Sec64Shdr->sh_offset + (UINTN)ElfCt->ImageBase - Sec64Shdr->sh_addr;
            } else {
              Ptr64   = (UINT64 *)(UINTN)(Rel64Entry->r_offset + ElfCt->Delta);
              *Ptr64 += ElfCt->Delta;
            }
            break;
          case R_X86_64_32:
            if (ElfCt->Delta == 0) {
              Offset  = Sec64Shdr->sh_offset + (Rel64Entry->r_offset - Sec64Shdr->sh_addr);
              Ptr32   = (UINT32 *)(ElfCt->ImageBase + Offset);
              *Ptr32 += (UINT32)(Sec64Shdr->sh_offset + (UINTN)ElfCt->ImageBase - Sec64Shdr->sh_addr);
            } else {
              Ptr32   = (UINT32 *)(UINTN)(Rel64Entry->r_offset + ElfCt->Delta);
              *Ptr32 += (UINT32)ElfCt->Delta;
            }
            break;
          default:
            DEBUG ((DEBUG_INFO, "Unsupported relocation type %02X\n", RelType));
        }
      }

      if ((Elf64Hdr->e_entry >= Sec64Shdr->sh_addr) && (Elf64Hdr->e_entry < Sec64Shdr->sh_addr + Sec64Shdr->sh_size)) {
        if (ElfCt->NewBase == NULL) {
          ElfCt->Entry = (UINTN)(Elf64Hdr->e_entry - Sec64Shdr->sh_addr + (UINTN)ElfCt->ImageBase + Sec64Shdr->sh_offset);
        } else {
          ElfCt->Entry = (UINTN)(Elf64Hdr->e_entry + ElfCt->Delta);
        }
      }
    }
  }

  return EFI_SUCCESS;
}