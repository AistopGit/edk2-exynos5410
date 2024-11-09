/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2014-2018, ARM Ltd. All rights reserved.<BR>
    This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ArmPlatform.h"

DefinitionBlock("DsdtTable.aml", "DSDT", 1, "ARMLTD", "ARM-JUNO", EFI_ACPI_ARM_OEM_REVISION)
{
  Scope(_SB)
  {
      Device(CPU0) 
      {
        Name(_HID, "ACPI0007")
        Name(_UID, 0)
      }

      Device (SDC1) // why
      {
          Name (_HID, "ARMH0180")
          Name (_CID, "ACPI\ARMH0180")
          Name (_UID, 0)
  
          Method (_CRS, 0x0, NotSerialized) {
              Name (RBUF, ResourceTemplate ()
              {
                  // SDCC1 register address space
                  Memory32Fixed (ReadWrite, 0x12200000, 0x00001000)
              })
              Return (RBUF)
          }
 
          Device (EMMC) {
              Method (_ADR) {
                  Return (8)
              }
 
              Method (_RMV) {
                  Return (0)
              }
          }
      }
  } 
}
