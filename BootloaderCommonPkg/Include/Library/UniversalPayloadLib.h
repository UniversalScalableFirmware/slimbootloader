/** @file

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UNIVERSAL_PAYLOAD_LIB_H_
#define _UNIVERSAL_PAYLOAD_LIB_H_

#include <Standard/UniversalPayload.h>

/**
  Load universal payload image into memory.

  @param[in]   ImageBase    The universal payload image base
  @param[out]  PldEntry     Pointer to receive payload entry point
  @param[out]  PldBase      Pointer to receive payload image base
  @param[out]  PldMachine   Pointer to receive payload image machine type

  @retval     EFI_SUCCESS      The image was loaded successfully
              EFI_ABORTED      The image loading failed
              EFI_UNSUPPORTED  The relocation format is not supported

**/
EFI_STATUS
EFIAPI
LoadUniversalPayload (
  IN  UINTN                    ImageBase,
  OUT UNIVERSAL_PAYLOAD_ENTRY  *PldEntry,
  OUT UINT32                   *PldBase,
  OUT UINT16                   *PldMachine
);

/**
  Authenticate a universal payload image.

  @param[in]  ImageBase    The universal payload image base

  @retval     EFI_SUCCESS      The image was authenticated successfully
              EFI_ABORTED      The image loading failed
              EFI_UNSUPPORTED  The relocation format is not supported
              EFI_SECURITY_VIOLATION  The image does not contain auth info

**/
EFI_STATUS
EFIAPI
AuthenticateUniversalPayload (
  IN  UINT32                    ImageBase
);

#endif
