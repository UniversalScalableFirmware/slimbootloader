/** @file
  This file defines the hob structure for SMBIOS table.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMBIOS_TABLE_GUID_H__
#define __SMBIOS_TABLE_GUID_H__

#include <UniversalPayload.h>

///
/// SMBIOS TABLE HOB GUID
///
extern GUID     gPldSmbios3TableGuid;
extern GUID     gPldSmbiosTableGuid;

#pragma pack(1)
///
/// Bootloader SMBIOS table hob
///
typedef struct {
  PLD_GENERIC_HEADER   PldHeader;
  EFI_PHYSICAL_ADDRESS SmBiosEntryPoint;
} PLD_SMBIOS_TABLE;

#pragma pack()

#endif
