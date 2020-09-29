/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UNIVERSAL_PAYLOAD_LIB_H_
#define _UNIVERSAL_PAYLOAD_LIB_H_

#include <Standard/UniversalPayload.h>

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
);

#endif
