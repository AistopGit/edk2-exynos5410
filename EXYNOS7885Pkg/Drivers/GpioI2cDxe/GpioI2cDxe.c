#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ExynosGpio.h>
#include <Platform/ArmPlatform.h>
#include <Library/ExynosGpioLib.h>
#include <Library/TimerLib.h>

// TODO: gosh PLEASE separate these 3 drivers (GPIO & GPIO I2C & PMIC)
// GPIO Base Address and Offsets
#define GPIO_BASE 0x14000000
#define GPK2_OFFSET 0xE0
#define CON 0x0
#define DAT 0x4

// I2C GPIO Pins
#define I2C_SCL  0  // GPK2_0
#define I2C_SDA  1  // GPK2_1

// I2C Device Address
#define I2C_DEVICE_ADDR 0x66

// PMIC registers
#define MAX77803_MUIC_REG_CDETCTRL1 0x0A
#define MAX77803_CHG_INT_MASK 0xB1
#define MAX77803_CHG_CNFG_00 0xB7
#define MAX77803_CHG_CNFG_11 0xC2

#define CHG_CNFG_00_CHG_MASK (1 << 0)
#define CHG_CNFG_00_OTG_MASK (1 << 1)
#define CHG_CNFG_00_BUCK_MASK (1 << 2)
#define CHG_CNFG_00_BOOST_MASK (1 << 3)
#define CHG_CNFG_00_DIS_MUIC_CTRL_MASK (1 << 5)

// GPIO Helper Functions
VOID GpioSetOutput(UINTN Pin)
{
    UINT32 Value = MmioRead32(GPIO_BASE + GPK2_OFFSET + CON);
    Value &= ~(0b1111 << (Pin * 4)); // Clear pin mode
    Value |= (0b0001 << (Pin * 4));  // Set as output
    MmioWrite32(GPIO_BASE + GPK2_OFFSET + CON, Value);
}

VOID GpioSetHigh(UINTN Pin)
{
    MmioOr32(GPIO_BASE + GPK2_OFFSET + DAT, (1 << Pin));
}

VOID GpioSetLow(UINTN Pin)
{
    MmioAnd32(GPIO_BASE + GPK2_OFFSET + DAT, ~(1 << Pin));
}

UINTN GpioRead(UINTN Pin)
{
    return (MmioRead32(GPIO_BASE + GPK2_OFFSET + DAT) & (1 << Pin)) ? 1 : 0;
}

VOID GpioSetInput(UINTN Pin)
{
    UINT32 Value = MmioRead32(GPIO_BASE + GPK2_OFFSET + CON);
    Value &= ~(0b1111 << (Pin * 4)); // Set pin mode to input (0b0000)
    MmioWrite32(GPIO_BASE + GPK2_OFFSET + CON, Value);
}

// I2C Delay Function (Adjust as needed)
VOID I2C_Delay()
{
    MicroSecondDelay(200);
}

// I2C Functions
VOID I2C_Start()
{
    GpioSetHigh(I2C_SDA);
    GpioSetHigh(I2C_SCL);
    I2C_Delay();
    GpioSetLow(I2C_SDA);
    I2C_Delay();
    GpioSetLow(I2C_SCL);
}

VOID I2C_Stop()
{
    GpioSetLow(I2C_SDA);
    I2C_Delay();
    GpioSetHigh(I2C_SCL);
    I2C_Delay();
    GpioSetHigh(I2C_SDA);
}

BOOLEAN I2C_WriteByte(UINT8 Data)
{
    for (INTN i = 0; i < 8; i++)
    {
        if (Data & 0x80)
        {
            GpioSetHigh(I2C_SDA);
        }
        else
        {
            GpioSetLow(I2C_SDA);
        }
        I2C_Delay();
        GpioSetHigh(I2C_SCL);
        I2C_Delay();
        GpioSetLow(I2C_SCL);
        Data <<= 1;
    }

    // Read ACK bit
    GpioSetInput(I2C_SDA);
    I2C_Delay();
    GpioSetHigh(I2C_SCL);
    I2C_Delay();
    BOOLEAN Ack = !GpioRead(I2C_SDA); // ACK is active low
    GpioSetLow(I2C_SCL);
    GpioSetOutput(I2C_SDA);

    return Ack;
}

UINT8 I2C_ReadByte(BOOLEAN Ack)
{
    UINT8 Data = 0;
    GpioSetInput(I2C_SDA);

    for (INTN i = 0; i < 8; i++)
    {
        Data <<= 1;
        I2C_Delay();
        GpioSetHigh(I2C_SCL);
        I2C_Delay();
        if (GpioRead(I2C_SDA))
        {
            Data |= 1;
        }
        GpioSetLow(I2C_SCL);
    }

    // Send ACK/NACK
    GpioSetOutput(I2C_SDA);
    if (Ack)
    {
        GpioSetLow(I2C_SDA);
    }
    else
    {
        GpioSetHigh(I2C_SDA);
    }
    I2C_Delay();
    GpioSetHigh(I2C_SCL);
    I2C_Delay();
    GpioSetLow(I2C_SCL);
    GpioSetHigh(I2C_SDA); // Release SDA

    return Data;
}

UINT8
I2C_ReadRegister (IN UINT16 Register)
{
    GpioSetOutput(I2C_SCL);
    GpioSetOutput(I2C_SDA);

    I2C_Start();
    if (!I2C_WriteByte(I2C_DEVICE_ADDR << 1)) // Write mode
    {
        DEBUG((EFI_D_ERROR, "I2C_ReadRegister: Device NACK during address write!\n"));
        return 0xFF;
    }
    I2C_WriteByte(Register);  // Register to read
    I2C_Start();
    I2C_WriteByte((I2C_DEVICE_ADDR << 1) | 1); // Read mode
    UINT8 Value = I2C_ReadByte(FALSE); // NACK to end transfer
    I2C_Stop();

    return Value;
}

VOID
I2C_WriteRegister(IN UINT16 Register, IN UINT8 Value)
{
    GpioSetOutput(I2C_SCL);
    GpioSetOutput(I2C_SDA);

    I2C_Start();
    if (!I2C_WriteByte(I2C_DEVICE_ADDR << 1))  // Write mode
    {
        DEBUG((EFI_D_ERROR, "I2C_WriteRegister: Device NACK during address write!\n"));
        I2C_Stop();
        return;
    }

    if (!I2C_WriteByte(Register))  // Register to write to
    {
        DEBUG((EFI_D_ERROR, "I2C_WriteRegister: Device NACK during register write!\n"));
        I2C_Stop();
        return;
    }

    if (!I2C_WriteByte(Value))  // Data to write
    {
        DEBUG((EFI_D_ERROR, "I2C_WriteRegister: Device NACK during data write!\n"));
        I2C_Stop();
        return;
    }

    I2C_Stop();
}

// Test em
EFI_STATUS 
I2C_Test (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
)
{
    UINT8 IntMask;
    UINT8 ChgDetControl1;
    UINT8 ChargerCfg0;

    // Disable charger interrupt
    IntMask = I2C_ReadRegister(MAX77803_CHG_INT_MASK);
    IntMask |= (1 << 4); // Disable CHGIN INTR (charger interrupt?)
    IntMask |= (1 << 6); // Disable CHG (charger?)
    IntMask &= ~(1 << 0); // Enable BYP INTR (bypass interrupt?)
    I2C_WriteRegister(MAX77803_CHG_INT_MASK, IntMask);

    // Disable charger detection
    ChgDetControl1 = I2C_ReadRegister(MAX77803_MUIC_REG_CDETCTRL1);
    ChgDetControl1 &= ~(1 << 0);
    I2C_WriteRegister(MAX77803_MUIC_REG_CDETCTRL1, ChgDetControl1);

    // Enable OTG, boost and MUIC control
    ChargerCfg0 = I2C_ReadRegister(MAX77803_CHG_CNFG_00);
    ChargerCfg0 &= ~(CHG_CNFG_00_CHG_MASK
				    | CHG_CNFG_00_OTG_MASK
				    | CHG_CNFG_00_BUCK_MASK
				    | CHG_CNFG_00_BOOST_MASK
				    | CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
    ChargerCfg0 |=  (CHG_CNFG_00_OTG_MASK
				    | CHG_CNFG_00_BOOST_MASK
				    | CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
    I2C_WriteRegister(MAX77803_CHG_CNFG_00, ChargerCfg0);

    // Write 0x50 (5V) to CHG_CNFG_11
    I2C_WriteRegister(MAX77803_CHG_CNFG_11, 0x50);
    
    return EFI_SUCCESS;
}

