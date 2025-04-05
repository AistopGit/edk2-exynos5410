#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GpioI2c.h>
#include <Platform/ArmPlatform.h>
#include <Library/TimerLib.h>
#include <Library/GpioI2cLib.h>

EFI_STATUS
I2CReadByte (
    IN GPIO_I2C *This,
    IN UINTN SDA, 
    IN UINTN SCL, 
    IN BOOLEAN Ack,
    OUT UINT8 *Byte
)
{
    UINT8 Data = 0;
    GpioSetInput(SDA);

    for (INTN i = 0; i < 8; i++)
    {
        Data <<= 1;
        MicroSecondDelay(50);
        GpioSetOutputHigh(SCL);
        MicroSecondDelay(50);
        if (GpioRead(SDA))
        {
            Data |= 1;
        }
        GpioSetOutputLow(SCL);
    }

    // Send ACK/NACK
    if (Ack)
    {
        GpioSetOutputLow(SDA);
    }
    else
    {
        GpioSetOutputHigh(SDA);
    }
    MicroSecondDelay(50);
    GpioSetOutputHigh(SCL);
    MicroSecondDelay(50);
    GpioSetOutputLow(SCL);
    GpioSetOutputHigh(SDA); // Release SDA

    *Byte = Data;

    return EFI_SUCCESS;
}

EFI_STATUS
I2CWriteByte (
    IN GPIO_I2C *This,
    IN UINTN SDA, 
    IN UINTN SCL, 
    IN UINT8 Data
)
{
    for (INTN i = 0; i < 8; i++)
    {
        if (Data & 0x80)
        {
            GpioSetOutputHigh(SDA);
        }
        else
        {
            GpioSetOutputLow(SDA);
        }
        MicroSecondDelay(50);
        GpioSetOutputHigh(SCL);
        MicroSecondDelay(50);
        GpioSetOutputLow(SCL);
        Data <<= 1;
    }

    // Read ACK bit
    GpioSetInput(SDA);
    MicroSecondDelay(50);
    GpioSetOutputHigh(SCL);
    MicroSecondDelay(50);
    GpioSetOutputLow(SCL);

    return (GpioRead(SDA) == 0) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

EFI_STATUS
I2CReadRegister (
    IN GPIO_I2C *This,
    IN UINTN SDA, 
    IN UINTN SCL, 
    IN UINT16 DevAddr,
    IN UINT16 Reg,
    OUT UINT8 *Value
)
{
    UINT8 Data = 0;

    I2CStart(SDA, SCL);
    if (EFI_ERROR(I2CWriteByte(This, SDA, SCL, DevAddr << 1))) // Write mode
    {
        DEBUG((EFI_D_ERROR, "I2CReadRegister: Device NACK during address write!\n"));
        return EFI_DEVICE_ERROR;
    }
    I2CWriteByte(This, SDA, SCL, Reg);  // Register to read
    I2CStart(SDA, SCL);
    I2CWriteByte(This, SDA, SCL, (DevAddr << 1) | 1); // Read mode
    I2CReadByte(This, SDA, SCL, FALSE, &Data); // NACK to end transfer
    I2CStop(SDA, SCL);

    *Value = Data;

    return EFI_SUCCESS;
}

EFI_STATUS
I2CWriteRegister (
    IN GPIO_I2C *This,
    IN UINTN SDA, 
    IN UINTN SCL, 
    IN UINT16 DevAddr,
    IN UINT16 Reg, 
    IN UINT8 Value
)
{
    I2CStart(SDA, SCL);
    if (EFI_ERROR(I2CWriteByte(This, SDA, SCL, DevAddr << 1)))  // Write mode
    {
        DEBUG((EFI_D_ERROR, "I2CWriteRegister: Device NACK during address write!\n"));
        I2CStop(SDA, SCL);
        return EFI_DEVICE_ERROR;
    }

    if (EFI_ERROR(I2CWriteByte(This, SDA, SCL, Reg)))  // Register to write to
    {
        DEBUG((EFI_D_ERROR, "I2CWriteRegister: Device NACK during register write!\n"));
        I2CStop(SDA, SCL);
        return EFI_DEVICE_ERROR;
    }

    if (EFI_ERROR(I2CWriteByte(This, SDA, SCL, Value)))  // Data to write
    {
        DEBUG((EFI_D_ERROR, "I2CWriteRegister: Device NACK during data write!\n"));
        I2CStop(SDA, SCL);
        return EFI_DEVICE_ERROR;
    }

    I2CStop(SDA, SCL);

    return EFI_SUCCESS;
}

GPIO_I2C GpioI2c = {
  I2CReadByte,
  I2CWriteByte,
  I2CReadRegister,
  I2CWriteRegister
};

EFI_STATUS 
GpioI2cInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
)
{
    EFI_STATUS  Status;

    Status = gBS->InstallMultipleProtocolInterfaces(&ImageHandle, &gGpioI2cProtocolGuid, &GpioI2c, NULL);
    return Status;
}

