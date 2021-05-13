/** @file
  This file defines the hob structure for the serial port device info.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLD_SERIAL_PORT_INFO_GUID_H__
#define __PLD_SERIAL_PORT_INFO_GUID_H__

#include <UniversalPayload.h>

///
/// Serial Port Information GUID
///
extern EFI_GUID gPldSerialPortInfoGuid;

#pragma pack(1)
typedef struct {
  PLD_GENERIC_HEADER   PldHeader;
  UINT16               Revision;
  BOOLEAN              UseMmio;
  UINT8                RegisterWidth;
  UINT32               BaudRate;
  UINT64               RegisterBase;
} PLD_SERIAL_PORT_INFO;
#pragma pack()
#endif
