#ifndef __GPIO_I2C_H__
#define __GPIO_I2C_H__

//
// Protocol interface structure
//
typedef struct _GPIO_I2C   GPIO_I2C;

//
// Function Prototypes
//
typedef
EFI_STATUS
(EFIAPI *I2C_READ_BYTE) (
    IN  GPIO_I2C    *This,
    IN  UINTN       SDA, 
    IN  UINTN       SCL, 
    IN  BOOLEAN     Ack,
    OUT UINT8       *Byte
    );
/*++

Routine Description:

  Reads a byte from the emulated I2C device

Arguments:

  This  - pointer to protocol
  SDA   - the SDA pin of the emulated I2C
  SCL   - the SCL pin of the emulated I2C
  Ack   - defines whether an ACK should be sent or not
  Byte  - the returned byte

Returns:

  EFI_SUCCESS - the byte is returned in Byte

--*/


typedef
EFI_STATUS
(EFIAPI *I2C_WRITE_BYTE) (
    IN  GPIO_I2C    *This,
    IN  UINTN       SDA, 
    IN  UINTN       SCL, 
    IN  UINT8       Data
    );
/*++

Routine Description:

  Writes a byte to the emulated I2C device

Arguments:

  This  - pointer to protocol
  SDA   - the SDA pin of the emulated I2C
  SCL   - the SCL pin of the emulated I2C
  Data  - the value to write

Returns:

  EFI_SUCCESS      - ACK was received
  EFI_DEVICE_ERROR - ACK wasn't received

--*/


typedef
EFI_STATUS
(EFIAPI *I2C_READ_REGISTER) (
    IN  GPIO_I2C    *This,
    IN  UINTN       SDA, 
    IN  UINTN       SCL, 
    IN  UINT16      DevAddr,
    IN  UINT16      Reg,
    OUT UINT8       *Value
    );
/*++

Routine Description:

  Reads a register from the emulated I2C device

Arguments:

  This     - pointer to protocol
  SDA      - the SDA pin of the emulated I2C
  SCL      - the SCL pin of the emulated I2C
  DevAddr  - I2C-connected device address
  Reg      - the register to be read
  Value    - the returned value

Returns:

  EFI_SUCCESS      - the value is returned in Value
  EFI_DEVICE_ERROR - device sent an NACK when writing the register value

--*/


typedef
EFI_STATUS
(EFIAPI *I2C_WRITE_REGISTER) (
    IN  GPIO_I2C    *This,
    IN  UINTN       SDA, 
    IN  UINTN       SCL, 
    IN  UINT16      DevAddr,
    IN  UINT16      Reg,
    IN  UINT8       Value
    );
/*++

Routine Description:

  Writes to a register on the emulated I2C device

Arguments:

  This     - pointer to protocol
  SDA      - the SDA pin of the emulated I2C
  SCL      - the SCL pin of the emulated I2C
  DevAddr  - I2C-connected device address
  Reg      - the register to be written
  Value    - the value to be written

Returns:

  EFI_SUCCESS      - the value is returned in Value
  EFI_DEVICE_ERROR - device sent an NACK when writing the address/register/data value

--*/

struct _GPIO_I2C {
  I2C_READ_BYTE         I2CReadByte;
  I2C_WRITE_BYTE        I2CWriteByte;
  I2C_READ_REGISTER     I2CReadRegister;
  I2C_WRITE_REGISTER    I2CWriteRegister;
};

extern EFI_GUID gGpioI2cProtocolGuid;

#endif
