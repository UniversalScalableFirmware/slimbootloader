/** @file
  This file defines the hob structure used for Platform information.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LOADED_PAYLOAD_IMAGE_INFO_GUID_H__
#define __LOADED_PAYLOAD_IMAGE_INFO_GUID_H__

#include <UniversalPayload.h>

extern EFI_GUID gLoadedPayloadImageInfoGuid;

#pragma pack(1)

typedef struct {
  CHAR8                   Identifier[16];
  EFI_PHYSICAL_ADDRESS    Base;
  UINT64                  Size;
} PLD_EXTRA_DATA_ENTRY;

typedef struct {
  PLD_GENERIC_HEADER     PldHeader;
  UINT32                 Count;
  PLD_EXTRA_DATA_ENTRY   Entry[0];
} PLD_EXTRA_DATA;

#pragma pack()

#endif
