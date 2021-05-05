/** @file
  Linux image load library

  Copyright (c) 2011 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LinuxLoaderStub.h"

// #define  BOOT_PARAMS_BASE        0x90000

#define ACPI_RSDP_CMDLINE_STR    "acpi_rsdp="

/**
  Returns the next instance of a HOB type from the starting HOB.

  This function searches the first instance of a HOB type from the starting HOB pointer.
  If there does not exist such HOB type from the starting HOB pointer, it will return NULL.
  In contrast with macro GET_NEXT_HOB(), this function does not skip the starting HOB pointer
  unconditionally: it returns HobStart back if HobStart itself meets the requirement;
  caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.

  If HobStart is NULL, then ASSERT().

  @param  Type          The HOB type to return.
  @param  HobStart      The starting HOB pointer to search from.

  @return The next instance of a HOB type from the starting HOB.

**/
STATIC
VOID *
EFIAPI
UpldGetNextHob (
  IN UINT16                 Type,
  IN CONST VOID             *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  ASSERT (HobStart != NULL);

  Hob.Raw = (UINT8 *) HobStart;
  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == Type) {
      return Hob.Raw;
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  return NULL;
}

/**
  Returns the next instance of the matched GUID HOB from the starting HOB.

  This function searches the first instance of a HOB from the starting HOB pointer.
  Such HOB should satisfy two conditions:
  its HOB type is EFI_HOB_TYPE_GUID_EXTENSION and its GUID Name equals to the input Guid.
  If there does not exist such HOB from the starting HOB pointer, it will return NULL.
  Caller is required to apply GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE ()
  to extract the data section and its size information, respectively.
  In contrast with macro GET_NEXT_HOB(), this function does not skip the starting HOB pointer
  unconditionally: it returns HobStart back if HobStart itself meets the requirement;
  caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.

  If Guid is NULL, then ASSERT().
  If HobStart is NULL, then ASSERT().

  @param  Guid          The GUID to match with in the HOB list.
  @param  HobStart      A pointer to a Guid.

  @return The next instance of the matched GUID HOB from the starting HOB.

**/
STATIC
VOID *
EFIAPI
UpldGetNextGuidHob (
  IN CONST EFI_GUID         *Guid,
  IN CONST VOID             *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  GuidHob;

  GuidHob.Raw = (UINT8 *) HobStart;
  while ((GuidHob.Raw = UpldGetNextHob (EFI_HOB_TYPE_GUID_EXTENSION, GuidHob.Raw)) != NULL) {
    if (CompareGuid (Guid, &GuidHob.Guid->Name)) {
      break;
    }
    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
  }
  return GuidHob.Raw;
}



/**
  Jump to kernel entry point.

  @param[in] KernelStart       Pointer to kernel entry point.
  @param[in] KernelBootParams  Pointer to boot parameter structure.
 **/
VOID
EFIAPI
JumpToKernel (
  IN VOID   *KernelStart,
  IN VOID   *KernelBootParams
  );

/**
  Jump to kernel 64-bit entry point.

  @param[in] KernelStart       Pointer to kernel 64-bit entry point.
  @param[in] KernelBootParams  Pointer to boot parameter structure.
 **/
VOID
EFIAPI
JumpToKernel64 (
  IN VOID   *KernelStart,
  IN VOID   *KernelBootParams
  );

/**
  Return the memory map info HOB data.

  @retval   Pointer to the memory map info hob.
            NULL if HOB is not found.

**/
STATIC
MEMORY_MAP_INFO *
EFIAPI
UpldGetMemoryMapInfo (
  IN  VOID   *HobList
  )
{
  EFI_HOB_GUID_TYPE             *GuidHob;

  GuidHob = UpldGetNextGuidHob (&gLoaderMemoryMapInfoGuid, HobList);
  if (GuidHob == NULL) {
    ASSERT (GuidHob);
    return NULL;
  }
  return (MEMORY_MAP_INFO *)GET_GUID_HOB_DATA (GuidHob);
}

/**
  Check if the image is a bootable Linux image.

  @param[in]  ImageBase      Memory address of an image

  @retval     TRUE           Image is a bootable kernel image
  @retval     FALSE          Not a bootable kernel image
**/
BOOLEAN
EFIAPI
UpldIsBzImage (
  IN  CONST VOID             *ImageBase
  )
{
  BOOT_PARAMS                *Bp;

  Bp = (BOOT_PARAMS *) ImageBase;
  if (Bp == NULL) {
    return FALSE;
  }

  // Check boot sector Signature
  if ((Bp->Hdr.Signature != 0xAA55) || (Bp->Hdr.Header != SETUP_HDR)) {
    DEBUG ((DEBUG_ERROR, "This image is not in bzimage format\n"));
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "Found bzimage Signature\n"));
  return TRUE;
}

/**
  Load linux kernel image to specified address and setup boot parameters.

  @param[in]  KernelBase     Memory address of an kernel image.
  @param[in]  InitRdBase     Memory address of an InitRd image.
  @param[in]  InitRdLen      InitRd image size.
  @param[in]  CmdLineBase    Memory address of command line buffer.
  @param[in]  CmdLineLen     Command line buffer size.

  @retval EFI_INVALID_PARAMETER   Input parameters are not valid.
  @retval EFI_UNSUPPORTED         Unsupported binary type.
  @retval EFI_SUCCESS             Kernel is loaded successfully.
**/
EFI_STATUS
EFIAPI
UpldLoadBzImage (
  IN  CONST VOID                  *KernelBase,
  IN  CONST VOID                  *InitRdBase,
  IN      UINT32                   InitRdLen,
  IN  CONST VOID                  *CmdLineBase,
  IN      UINT32                   CmdLineLen
  )
{
  BOOT_PARAMS                *Bp;
  UINT32                      BootParamSize;
  UINTN                       KernelSize;
  VOID CONST                 *ImageBase;

  ImageBase = KernelBase;
  if (ImageBase == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!UpldIsBzImage (ImageBase)) {
    return EFI_UNSUPPORTED;
  }

  Bp = (BOOT_PARAMS *) ImageBase;
  if (Bp->Hdr.SetupSectorss != 0) {
    BootParamSize = (Bp->Hdr.SetupSectorss + 1) * 512;
  } else {
    BootParamSize = 5 * 512;
  }

  KernelSize = Bp->Hdr.SysSize * 16;
  CopyMem ((VOID *)(UINTN)Bp->Hdr.PrefAddress, (UINT8 *)ImageBase + BootParamSize, KernelSize);

  //
  // Update boot params
  //
  Bp->Hdr.LoaderId     = 0xff;
  Bp->Hdr.CmdLinePtr   = (UINT32)(UINTN)CmdLineBase;
  Bp->Hdr.CmdlineSize  = CmdLineLen;
  Bp->Hdr.RamDiskStart = (UINT32)(UINTN)InitRdBase;
  Bp->Hdr.RamDisklen   = InitRdLen;

  return EFI_SUCCESS;
}

/**
  Update linux kernel boot parameters.

  @param[in]  Bp             BootParams address to be updated

**/
VOID
EFIAPI
UpldUpdateLinuxBootParams (
  IN  VOID                   *HobList,
  IN  VOID                   *KernelBase
  )
{
  EFI_HOB_GUID_TYPE          *GuidHob;
  EFI_PEI_GRAPHICS_INFO_HOB  *GfxInfoHob;
  UINTN                       MemoryMapSize;
  E820_ENTRY                 *E820Entry;
  MEMORY_MAP_INFO            *MapInfo;
  UINTN                       Index;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GfxMode;
  CHAR8                       ParamValue[64];
  ACPI_TABLE_HOB             *AcpiTable;
  BOOT_PARAMS                *Bp;

  Bp = (BOOT_PARAMS *)KernelBase;
  if (Bp == NULL) {
    return;
  }

  //
  // Get graphics data
  //
  GuidHob = UpldGetNextGuidHob (&gEfiGraphicsInfoHobGuid, HobList);
  if (GuidHob != NULL) {
    GfxInfoHob = (EFI_PEI_GRAPHICS_INFO_HOB *)GET_GUID_HOB_DATA (GuidHob);
    ZeroMem (&Bp->ScreenInfo, sizeof (Bp->ScreenInfo));
    GfxMode = &GfxInfoHob->GraphicsMode;
    if (GfxMode->PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
      Bp->ScreenInfo.RedPos   = 0;
      Bp->ScreenInfo.BluePos  = 16;
    } else if (GfxMode->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
      Bp->ScreenInfo.RedPos   = 16;
      Bp->ScreenInfo.BluePos  = 0;
    } else {
      // Unsupported format
      GfxMode = NULL;
    }
    if (GfxMode != NULL) {
      Bp->ScreenInfo.OrigVideoIsVGA  = VIDEO_TYPE_EFI;
      Bp->ScreenInfo.LfbLinelength   = (UINT16) (GfxMode->PixelsPerScanLine * 4);
      Bp->ScreenInfo.LfbDepth        = 32;
      Bp->ScreenInfo.LfbBase         = (UINT32)(UINTN)GfxInfoHob->FrameBufferBase;
      Bp->ScreenInfo.ExtLfbBase      = (UINT32)RShiftU64 (GfxInfoHob->FrameBufferBase, 32);
      Bp->ScreenInfo.Capabilities   |= VIDEO_CAPABILITY_64BIT_BASE;
      Bp->ScreenInfo.LfbWidth        = (UINT16)GfxMode->HorizontalResolution;
      Bp->ScreenInfo.LfbHeight       = (UINT16)GfxMode->VerticalResolution;
      Bp->ScreenInfo.Pages           = 1;
      Bp->ScreenInfo.RedSize         = 8;
      Bp->ScreenInfo.GreenSize       = 8;
      Bp->ScreenInfo.BlueSize        = 8;
      Bp->ScreenInfo.RsvdSize        = 8;
      Bp->ScreenInfo.GreenPos        = 8;
      Bp->ScreenInfo.RsvdPos         = 24;
    }
  }

  // Get memory map
  E820Entry = &Bp->E820Map[0];
  MemoryMapSize = (UINTN)ARRAY_SIZE (Bp->E820Map);
  MapInfo = UpldGetMemoryMapInfo (HobList);
  for (Index = 0; Index < MapInfo->Count && Index < MemoryMapSize; Index++) {
    E820Entry->Type = (UINT32)MapInfo->Entry[Index].Type;
    E820Entry->Addr = MapInfo->Entry[Index].Base;
    E820Entry->Size = MapInfo->Entry[Index].Size;
    E820Entry++;
  }
  Bp->E820Entries = (UINT8)MapInfo->Count;

  //
  // Append acpi_rsdp only if it does not exist in the kernel command line
  // to allow a user override acpi_rdsp kernel parameter.
  // To check the existence, simply search for "acpi_rsdp=" string since it's
  // case-sensitive with the immediate '=' trailing according to kernel spec.
  //
  if (AsciiStrStr ((CHAR8 *)(UINTN)Bp->Hdr.CmdLinePtr,(CHAR8 *)ACPI_RSDP_CMDLINE_STR) == NULL) {
    GuidHob = GetNextGuidHob (&gEfiAcpi20TableGuid, HobList);
    if (GuidHob != NULL) {
      AcpiTable = (ACPI_TABLE_HOB *)GET_GUID_HOB_DATA (GuidHob);
      AsciiSPrint (ParamValue, sizeof (ParamValue), " %a0x%lx", ACPI_RSDP_CMDLINE_STR, AcpiTable->Rsdp);
      AsciiStrCatS ((CHAR8 *)(UINTN)Bp->Hdr.CmdLinePtr, MAX_CMD_LINE_LEN, ParamValue);
      Bp->Hdr.CmdlineSize = (UINT32)AsciiStrLen ((CHAR8 *)(UINTN)Bp->Hdr.CmdLinePtr);
    }
  }
}

/**
  Whether 64-bit long mode is enabled or not.

  @return   TRUE if long mode has already been enabled, otherwise FALSE

**/
BOOLEAN
EFIAPI
IsLongModeEnabled (
  VOID
  )
{
  UINT64  Msr;

  Msr = AsmReadMsr64 (0xc0000080);
  return ((Msr & BIT8) == BIT8) ? TRUE : FALSE;
}


/**
  Load linux kernel image to specified address and setup boot parameters.

  @param[in]  HobList      HOB list pointer.
  @param[in]  KernelBase   Kernel image base.
**/
VOID
EFIAPI
UpldLinuxBoot (
  IN VOID   *HobList,
  IN VOID   *KernelBase
  )
{
  BOOT_PARAMS   *Bp;
  UINTN          KernelStart;

  Bp = (BOOT_PARAMS *)KernelBase;
  if (Bp != NULL) {
    UpldUpdateLinuxBootParams (HobList, Bp);
    KernelStart = ((UINTN)Bp->Hdr.Code32Start - 0x100000) + (UINTN)Bp->Hdr.PrefAddress;
    if (!IsLongModeEnabled ()) {
      JumpToKernel ((VOID *)KernelStart, Bp);
    } else {
      if ((Bp->Hdr.XloadFlags & BIT0) == BIT0) {
        DEBUG ((DEBUG_INFO, "Jump to 64-bit kernel entrypoint ...\n"));
        KernelStart += 0x200;
        JumpToKernel ((VOID *)KernelStart, Bp);
      } else {
        // In long mode already, but kernel is not 64-bit
        DEBUG ((DEBUG_ERROR, "Unsupported kernel mode !\n"));
      }
    }
  }

  CpuDeadLoop ();
}

