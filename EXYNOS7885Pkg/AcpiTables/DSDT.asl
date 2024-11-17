DefinitionBlock("DSDT.AML", "DSDT", 0x02, "EXYNO ", "EXY5410 ", 3)
{
    Scope(\_SB_)
    {
        Device(CPU0) 
        {
            Name(_HID, "ACPI0007")
            Name(_UID, 0)
        }

        Device(CPU1) 
        {
            Name(_HID, "ACPI0007")
            Name(_UID, 1)
        }

        Device(CPU2) 
        {
            Name(_HID, "ACPI0007")
            Name(_UID, 2)
        }

        Device(CPU3) 
        {
            Name(_HID, "ACPI0007")
            Name(_UID, 3)
        }

        Device (SDC1)
        {
            Name (_HID, "ARMH0180")
            Name (_CID, "ACPI\ARMH0180")
            Name (_UID, 0)
  
            Method (_CRS, 0x0, NotSerialized) {
                Name (RBUF, ResourceTemplate ()
                {
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
