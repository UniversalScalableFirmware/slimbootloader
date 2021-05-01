/** @file
  This file defines the hob structure for the serial port device info.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLD_SERIAL_PORT_INFO_GUID_H__
#define __PLD_SERIAL_PORT_INFO_GUID_H__

///
/// Serial Port Information GUID
///
extern EFI_GUID gPldSerialPortInfoGuid;

typedef struct {  
  UINT16        Reversion;
  BOOLEAN       UseMmio;
  UINT8         RegisterWidth;
  UINT32        BaudRate;
  UINT64        RegisterBase;
} PLD_SERIAL_PORT_INFO;

#endif
