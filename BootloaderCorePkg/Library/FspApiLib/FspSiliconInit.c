/** @file

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <FspApiLibInternal.h>
#include <Library/BoardInitLib.h>
#include <Library/BlMemoryAllocationLib.h>


/**
  This FSP API is called after TempRamExit API.
  FspMemoryInit, TempRamExit and FspSiliconInit APIs provide an alternate method to complete the
  silicon initialization.

  @retval EFI_SUCCESS                 FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER       Input parameters are invalid.
  @retval EFI_UNSUPPORTED             The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR            FSP initialization failed.
  @retval FSP_STATUS_RESET_REQUIREDx  A reset is reuired. These status codes will not be returned during S3.
**/
EFI_STATUS
EFIAPI
CallFspSiliconInit (
  VOID
  )
{
  VOID                      *FspsUpdptr;
  UINT8                      *DefaultSiliconInitUpd;
  FSP_INFO_HEADER            *FspHeader;
  FSP_SILICON_INIT            FspSiliconInit;
  EFI_STATUS                  Status;

  FspHeader = (FSP_INFO_HEADER *)(UINTN)(PcdGet32 (PcdFSPSBase) + FSP_INFO_HEADER_OFF);

  ASSERT (FspHeader->Signature == FSP_INFO_HEADER_SIGNATURE);
  ASSERT (FspHeader->ImageBase == PcdGet32 (PcdFSPSBase));
  FspsUpdptr = AllocatePool (FspHeader->CfgRegionSize);
  if (FspsUpdptr == NULL) {
      return EFI_OUT_OF_RESOURCES;
  }
  (VOID) PcdSet32S (PcdFspsUpdPtr,(UINT32)(UINTN)FspsUpdptr);
  // Copy default UPD data
  DefaultSiliconInitUpd = (UINT8 *)(UINTN)(FspHeader->ImageBase + FspHeader->CfgRegionOffset);
  CopyMem (FspsUpdptr, DefaultSiliconInitUpd, FspHeader->CfgRegionSize);

  /* Update architectural UPD fields */
  UpdateFspConfig (FspsUpdptr);

  ASSERT (FspHeader->FspSiliconInitEntryOffset != 0);
  FspSiliconInit = (FSP_SILICON_INIT)(UINTN)(FspHeader->ImageBase + \
                                             FspHeader->FspSiliconInitEntryOffset);

  DEBUG ((DEBUG_INFO, "Call FspSiliconInit "));
  if (IS_X64 && !(FspHeader->ComponentAttribute & BIT2)) {
    DEBUG ((DEBUG_INFO, "(x86)"));
    Status = Execute32BitCode ((UINTN)FspSiliconInit, (UINTN)FspsUpdptr, (UINTN)0, FALSE);
    Status = (UINTN)LShiftU64 (Status & ((UINTN)MAX_INT32 + 1), 32) | (Status & MAX_INT32);
  } else {
    if (IS_X64) {
      DEBUG ((DEBUG_INFO, "(x64)"));
    } else {
      DEBUG ((DEBUG_INFO, "(x86)"));
    }
    Status = FspSiliconInit (FspsUpdptr);
  }
  DEBUG ((DEBUG_INFO, " ... %r\n", Status));

  return Status;
}

/**
  This FSP API is called after FspSiliconInit API.
  FspMemoryInit, TempRamExit and FspSiliconInit APIs provide an alternate method to complete the
  silicon initialization.


  @retval EFI_SUCCESS                 FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER       Input parameters are invalid.
  @retval EFI_UNSUPPORTED             The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR            FSP initialization failed.
  @retval FSP_STATUS_RESET_REQUIREDx  A reset is reuired. These status codes will not be returned during S3.
**/
EFI_STATUS
EFIAPI
CallFspSmmInit (
  VOID
  )
{
  FSP_INFO_HEADER            *FspHeader;
  FSP_SMM_INIT                FspSmmInit;
  EFI_STATUS                  Status;

  FspHeader = (FSP_INFO_HEADER *)(UINTN)(PcdGet32 (PcdFSPIBase) + FSP_INFO_HEADER_OFF);
  DEBUG ((DEBUG_INFO, "FSP-I... %x\n", FspHeader));
  DEBUG ((DEBUG_INFO, "FSP-I... %x\n", FspHeader->ComponentAttribute));

  ASSERT (FspHeader->Signature == FSP_INFO_HEADER_SIGNATURE);
  ASSERT (FspHeader->ImageBase == PcdGet32 (PcdFSPIBase));

  ASSERT (FspHeader->FspSiliconInitEntryOffset != 0);
  FspSmmInit = (FSP_SMM_INIT)(UINTN)(FspHeader->ImageBase + \
                                             FspHeader->FspSiliconInitEntryOffset);

  DEBUG ((DEBUG_INFO, "Call FspSmmInit "));
  Status = FspSmmInit ((UINTN)(0));
  DEBUG ((DEBUG_INFO, " ... %r\n", Status));

  return Status;
}
