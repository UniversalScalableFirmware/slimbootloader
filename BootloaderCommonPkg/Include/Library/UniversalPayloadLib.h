/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UNIVERSAL_PAYLOAD_LIB_H_
#define _UNIVERSAL_PAYLOAD_LIB_H_

#include <Guid/LoadedPayloadImageInfoGuid.h>

typedef  EFI_STATUS  (EFIAPI *UNIVERSAL_PAYLOAD_ENTRYPOINT) (VOID *HobList);

#define PLD_IDENTIFIER                   SIGNATURE_32('U', 'P', 'L', 'D')
#define PLD_INFO_SEC_NAME                ".upld_info"
#define PLD_EXTRA_SEC_NAME_PREFIX        ".upld."
#define PLD_EXTRA_SEC_NAME_PREFIX_LENGTH (sizeof (PLD_EXTRA_SEC_NAME_PREFIX) - 1)

#pragma pack(1)

typedef struct {
  UINT32                          Identifier;
  UINT32                          HeaderLength;
  UINT16                          SpecRevision;
  UINT8                           Reserved[2];
  UINT32                          Revision;
  UINT32                          Attribute;
  UINT32                          Capability;
  CHAR8                           ProducerId[16];
  CHAR8                           ImageId[16];
} PLD_INFO_HEADER;

#define  MAX_PLD_IMAGE_ENTRY      4
typedef struct {
  PLD_INFO_HEADER                 Info;
  UINT32                          ImageCount;
  UINT32                          Machine;
  UINTN                           EntryPoint;
  UINTN                           PayloadBase;
  UINTN                           PayloadSize;
  PAYLOAD_IMAGE_ENTRY             LoadedImage[MAX_PLD_IMAGE_ENTRY];
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
