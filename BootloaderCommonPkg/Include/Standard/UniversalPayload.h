/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __UNIVERSAL_PAYLOAD_H__
#define __UNIVERSAL_PAYLOAD_H__

typedef  EFI_STATUS  (EFIAPI *UNIVERSAL_PAYLOAD_ENTRYPOINT) (VOID *HobList);

#define  UPLD_INFO_SEC_NAME           ".upld_info"
#define  UPLD_IMAGE_SEC_NAME_PREFIX   ".upld."
#define  UPLD_IDENTIFIER              SIGNATURE_32('U', 'P', 'L', 'D')

typedef struct {
  UINT32                  Identifier;
  UINT16                  HeaderLength;
  UINT8                   HeaderRevision;
  UINT8                   Reserved;
  UINT64                  Revision;
  UINT64                  Capability;
  CHAR8                   ImageId[16];
  CHAR8                   ProducerId[16];
} UPLD_INFO_HEADER;

#endif
