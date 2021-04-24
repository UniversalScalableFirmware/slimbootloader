/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UNIVERSAL_PAYLOAD_LIB_H_
#define _UNIVERSAL_PAYLOAD_LIB_H_

#include <Guid/LoadedPayloadImageInfoGuid.h>

typedef  EFI_STATUS  (EFIAPI *UNIVERSAL_PAYLOAD_ENTRYPOINT) (VOID *HobList);

#define  UPLD_INFO_SEC_NAME           ".upld_info"
#define  UPLD_IMAGE_SEC_NAME_PREFIX   ".upld."
#define  UPLD_IDENTIFIER              SIGNATURE_32('U', 'P', 'L', 'D')

typedef struct {
  UINT32                          Identifier;
  UINT16                          HeaderLength;
  UINT8                           HeaderRevision;
  UINT8                           Reserved;
  UINT64                          Revision;
  UINT64                          Capability;
  CHAR8                           ImageId[16];
  CHAR8                           ProducerId[16];
} UPLD_INFO_HEADER;

#define  MAX_PLD_IMAGE_ENTRY      4
typedef struct {
  UPLD_INFO_HEADER                Info;
  UINT32                          Machine;
  UINT32                          ImageCount;
  UNIVERSAL_PAYLOAD_ENTRYPOINT    EntryPoint;
  PAYLOAD_IMAGE_ENTRY             LoadedImage[MAX_PLD_IMAGE_ENTRY];
} LOADED_PAYLOAD_INFO;


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
LoadUniversalPayload (
  IN  VOID                     *ImageBase,
  OUT LOADED_PAYLOAD_INFO      *PayloadInfo
);


#endif
