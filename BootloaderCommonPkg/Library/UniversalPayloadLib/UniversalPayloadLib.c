/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BootloaderCommonLib.h>
#include <Library/LitePeCoffLib.h>
#include <Library/UniversalPayloadLib.h>


#undef   DEBUG_VERBOSE
#define  DEBUG_VERBOSE   DEBUG_INFO


/**
  Performs an specific relocation fpr PECOFF images. The caller needs to
  allocate enough buffer at the PreferedImageBase

  @param  ImageBase        Pointer to the current image base.

  @return Status code.

**/
RETURN_STATUS
EFIAPI
RelocateUniversalPayload (
  IN UPLD_RELOC_HEADER      *UpldRelocHdr,
  IN UINTN                   ActPldBase,
  IN UINTN                   FixupDelta
  )
{
  RETURN_STATUS                   Status;
  UINT32                          RelocSectionSize;
  UINT16                         *RelocDataPtr;
  UINT32                          PageRva;
  UINT32                          BlockSize;
  UINTN                           Index;
  UINT8                           Type;
  UINT32                         *DataPtr;
  UINT16                          Offset;
  UINT16                          TypeOffset;
  UINT32                          ImgOffset;
  UINT32                          Adjust;
  UINT64                          PeBase;
  PE_RELOC_BLOCK_HEADER          *RelocBlkHdr;

  Status = RETURN_SUCCESS;

  if (UpldRelocHdr->RelocFmt == UPLD_RELOC_FMT_RAW) {
    RelocDataPtr      =  (UINT16 *)(&UpldRelocHdr[1]);
    RelocSectionSize  = UpldRelocHdr->CommonHeader.HeaderLength - sizeof(UPLD_RELOC_HEADER);
  } else if (UpldRelocHdr->RelocFmt == UPLD_RELOC_FMT_PTR) {
    RelocBlkHdr       = (PE_RELOC_BLOCK_HEADER *)(&UpldRelocHdr[1]);
    RelocDataPtr      = (UINT16 *)(RelocBlkHdr->PageRva + ActPldBase);
    RelocSectionSize  = RelocBlkHdr->BlockSize;
  } else {
    // Not support yet
    DEBUG ((DEBUG_ERROR, "Relocation format is not supported yet !\n"));
    return EFI_UNSUPPORTED;
  }

  PeBase             = ActPldBase + UpldRelocHdr->RelocImgOffset;
  Adjust             = UpldRelocHdr->RelocImgStripped;
  DEBUG ((DEBUG_VERBOSE, "BaseDelta: 0x%x  Adjust: 0x%x\n", FixupDelta, Adjust));

  // This seems to be a bug in the way MS generates the reloc fixup blocks.
  // After we have gone thru all the fixup blocks in the .reloc section, the
  // variable RelocSectionSize should ideally go to zero. But I have found some orphan
  // data after all the fixup blocks that don't quite fit anywhere. So, I have
  // changed the check to a greater-than-eight. It should be at least eight
  // because the PageRva and the BlockSize together take eight bytes. If less
  // than 8 are remaining, then those are the orphans and we need to disregard them.
  while (RelocSectionSize >= 8) {
    // Read the Page RVA and Block Size for the current fixup block.
    PageRva   = * (UINT32 *) (RelocDataPtr + 0);
    BlockSize = * (UINT32 *) (RelocDataPtr + 2);
    DEBUG ((DEBUG_VERBOSE, "PageRva = %04X  BlockSize = %04X\n", PageRva, BlockSize));

    RelocDataPtr += 4;
    if (BlockSize == 0) {
      break;
    }

    RelocSectionSize -= sizeof (UINT32) * 2;

    // Extract the correct number of Type/Offset entries. This is given by:
    // Loop count = Number of relocation items =
    // (Block Size - 4 bytes (Page RVA field) - 4 bytes (Block Size field)) divided
    // by 2 (each Type/Offset entry takes 2 bytes).
    DEBUG ((DEBUG_VERBOSE, "LoopCount = %04x\n", ((BlockSize - 2 * sizeof(UINT32)) / sizeof(UINT16))));
    for (Index = 0; Index < ((BlockSize - 2 * sizeof (UINT32)) / sizeof (UINT16)); Index++) {
      TypeOffset = *RelocDataPtr++;
      Type   = (UINT8) ((TypeOffset & 0xf000) >> 12);
      Offset = (UINT16) ((UINT16)TypeOffset & 0x0fff);
      RelocSectionSize -= sizeof (UINT16);
      ImgOffset = PageRva + Offset - Adjust;
      DEBUG ((DEBUG_VERBOSE, "%d: PageRva: %08x Offset: %04x Type: %x \n", Index, PageRva, ImgOffset, Type));
      DataPtr = (UINT32 *)(UINTN)(PeBase + ImgOffset);
      switch (Type) {
      case 0:
        break;
      case 1:
        *DataPtr += (((UINT32)FixupDelta >> 16) & 0x0000ffff);
        break;
      case 2:
        *DataPtr += ((UINT32)FixupDelta & 0x0000ffff);
        break;
      case 3:
        *DataPtr += (UINT32)FixupDelta;
        break;
      case 10:
        *(UINT64 *)DataPtr += FixupDelta;
        break;
      default:
        DEBUG ((DEBUG_ERROR, "Unknown RELOC type: %d\n", Type));
        break;
      }
    }
  }

  return Status;
}


/**
  Load universal payload image into memory.

  @param[in]  ImageBase    The universal payload image base
  @param[in]  PldEntry     The payload image entry point

  @retval     EFI_SUCCESS      The image was loaded successfully
              EFI_ABORTED      The image loading failed
              EFI_UNSUPPORTED  The relocation format is not supported

**/
EFI_STATUS
EFIAPI
LoadUniversalPayload (
  IN  UINT32                    ImageBase,
  IN  UNIVERSAL_PAYLOAD_ENTRY  *PldEntry
)
{
  EFI_STATUS              Status;
  UPLD_INFO_HEADER       *UpldInfoHdr;
  UPLD_RELOC_HEADER      *UpldRelocHdr;
  UINT32                  PldImgBase;

  UpldInfoHdr = (UPLD_INFO_HEADER *)ImageBase;
  if ((UpldInfoHdr->Capability & UPLD_IMAGE_CAP_RELOC) != 0) {
    DEBUG ((DEBUG_VERBOSE, "Found relocation table\n"));
    UpldRelocHdr = (UPLD_RELOC_HEADER *)&UpldInfoHdr[1];
    if (UpldRelocHdr->CommonHeader.Identifier != UPLD_RELOC_ID) {
      DEBUG ((DEBUG_ERROR, "Relocation table is invalid !\n"));
      return EFI_ABORTED;
    }

    PldImgBase = ImageBase + UpldInfoHdr->ImageOffset;
    Status = RelocateUniversalPayload (UpldRelocHdr, PldImgBase, PldImgBase - (UINT32)UpldInfoHdr->ImageBase);
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "Payload image relocation failed - %r\n", Status));
    } else {
      DEBUG ((DEBUG_VERBOSE, "Image was relocated successfully\n"));
      if (PldEntry != NULL) {
        *PldEntry = (UNIVERSAL_PAYLOAD_ENTRY)(PldImgBase + UpldInfoHdr->EntryPointOffset);
        DEBUG ((DEBUG_VERBOSE, "Image entry point is at %p\n", *PldEntry));
      }
    }
  }

  return Status;
}

