/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UNIVERSAL_PAYLOAD_LIB_H_
#define _UNIVERSAL_PAYLOAD_LIB_H_

#include <Guid/LoadedPayloadImageInfoGuid.h>
#include <UniversalPayload.h>

#pragma pack(1)

#define  MAX_PLD_IMAGE_ENTRY      4
typedef struct {
  PLD_INFO_HEADER                 Info;
  UINT32                          ImageCount;
  UINT32                          Machine;
  UINTN                           EntryPoint;
  UINTN                           PayloadBase;
  UINTN                           PayloadSize;
  PLD_EXTRA_DATA_ENTRY            LoadedImage[MAX_PLD_IMAGE_ENTRY];
} LOADED_PAYLOAD_INFO;

#pragma pack()

/**
  Load universal payload image into memory.

  @param[in]   ImageBase    The universal payload image base
  @param[out]  PayloadInfo  Pointer to receive payload related info

  @retval     EFI_SUCCESS      The image was loaded successfully
              EFI_ABORTED      The image loading failed
              EFI_UNSUPPORTED  The relocation format is not supported

**/
EFI_STATUS
EFIAPI
LoadElfPayload (
  IN  VOID                     *ImageBase,
  OUT LOADED_PAYLOAD_INFO      *PayloadInfo
);


#endif
