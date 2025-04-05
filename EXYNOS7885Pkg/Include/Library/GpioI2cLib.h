#ifndef __GPIO_I2C_LIB_H__
#define __GPIO_I2C_LIB_H__

/*===========================================================================
  MACRO DECLARATIONS
===========================================================================*/

/*===========================================================================
  EXTERNAL VARIABLES
===========================================================================*/

VOID
EFIAPI
GpioSetOutputHigh (
    IN UINTN Pin
    );

VOID
EFIAPI
GpioSetOutputLow (
    IN UINTN Pin
    );

UINTN
EFIAPI
GpioRead (
    IN UINTN Pin
    );

VOID 
EFIAPI
GpioSetInput (
    IN UINTN Pin
    );

VOID
EFIAPI
I2CStart (
    IN UINTN SDA, 
    IN UINTN SCL
    );

VOID
EFIAPI
I2CStop (
    IN UINTN SDA, 
    IN UINTN SCL
    );

#endif // __GPIO_I2C_LIB_H__
