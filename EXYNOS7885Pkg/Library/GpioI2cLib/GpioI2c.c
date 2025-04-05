#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ExynosGpio.h>
#include <Platform/ArmPlatform.h>
#include <Library/TimerLib.h>
#include <Library/ExynosGpioLib.h>

// GPIO helper functions
VOID
GpioSetOutputHigh (
    IN UINTN Pin
    )
{
    EFI_STATUS Status;
    EXYNOS_GPIO *gSamsungGpioProtocol;
    
    Status = gBS->LocateProtocol(&gSamsungPlatformGpioProtocolGuid, NULL, (VOID *)&gSamsungGpioProtocol);
    if (EFI_ERROR(Status))
    {
        DEBUG((EFI_D_ERROR, "Failed to locate Samsung GPIO protocol! Status = %r\n", Status));
        return;
    }

    gSamsungGpioProtocol->Set(gSamsungGpioProtocol, Pin, GPIO_MODE_OUTPUT_1);
}

VOID
GpioSetOutputLow (
    IN UINTN Pin
    )
{
    EFI_STATUS Status;
    EXYNOS_GPIO *gSamsungGpioProtocol;
    
    Status = gBS->LocateProtocol(&gSamsungPlatformGpioProtocolGuid, NULL, (VOID *)&gSamsungGpioProtocol);
    if (EFI_ERROR(Status))
    {
        DEBUG((EFI_D_ERROR, "Failed to locate Samsung GPIO protocol! Status = %r\n", Status));
        return;
    }

    gSamsungGpioProtocol->Set(gSamsungGpioProtocol, Pin, GPIO_MODE_OUTPUT_0);
}

UINTN
GpioRead (
    IN UINTN Pin
    )
{
    EFI_STATUS Status;
    EXYNOS_GPIO *gSamsungGpioProtocol;
    UINTN Value;
    
    Status = gBS->LocateProtocol(&gSamsungPlatformGpioProtocolGuid, NULL, (VOID *)&gSamsungGpioProtocol);
    if (EFI_ERROR(Status))
    {
        DEBUG((EFI_D_ERROR, "Failed to locate Samsung GPIO protocol! Status = %r\n", Status));
        return 0xFF;
    }

    gSamsungGpioProtocol->Get(gSamsungGpioProtocol, Pin, &Value);
    return Value;
}

VOID 
GpioSetInput (
    IN UINTN Pin
    )
{
    EFI_STATUS Status;
    EXYNOS_GPIO *gSamsungGpioProtocol;
    
    Status = gBS->LocateProtocol(&gSamsungPlatformGpioProtocolGuid, NULL, (VOID *)&gSamsungGpioProtocol);
    if (EFI_ERROR(Status))
    {
        DEBUG((EFI_D_ERROR, "Failed to locate Samsung GPIO protocol! Status = %r\n", Status));
        return;
    }

    gSamsungGpioProtocol->Set(gSamsungGpioProtocol, Pin, GPIO_MODE_INPUT);
}

// I2C Functions
VOID
I2CStart (
    IN UINTN SDA, 
    IN UINTN SCL
    )
{
    GpioSetOutputHigh(SDA);
    GpioSetOutputHigh(SCL);
    MicroSecondDelay(50);
    GpioSetOutputLow(SDA);
    MicroSecondDelay(50);
    GpioSetOutputLow(SCL);
}

VOID
I2CStop (
    IN UINTN SDA, 
    IN UINTN SCL
    )
{
    GpioSetOutputLow(SDA);
    MicroSecondDelay(50);
    GpioSetOutputHigh(SCL);
    MicroSecondDelay(50);
    GpioSetOutputHigh(SDA);
}

