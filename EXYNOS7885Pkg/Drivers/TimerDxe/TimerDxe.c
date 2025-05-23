/** @file
  Template for Timer Architecture Protocol driver of the ARM flavor

  Copyright (c) 2012, Samsung Electronics Co. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>

#include <Protocol/Timer.h>
#include <Protocol/HardwareInterrupt.h>

#include <Library/ExynosTimerLib.h>
#include <Platform/ArmPlatform.h>

// The notification function to call on every timer interrupt.
volatile EFI_TIMER_NOTIFY      mTimerNotifyFunction   = (EFI_TIMER_NOTIFY)NULL;
EFI_EVENT                       EfiExitBootServicesEvent        = (EFI_EVENT)NULL;

// The current period of the timer interrupt
volatile UINT64 mTimerPeriod = 0;

// Cached copy of the Hardware Interrupt protocol instance
EFI_HARDWARE_INTERRUPT_PROTOCOL *gInterrupt = NULL;

// Cached interrupt vector
UINTN  gVector;

UINT32 mLastTickCount;

/**

  C Interrupt Handler called in the interrupt context when Source interrupt is active.


  @param Source         Source of the interrupt. Hardware routing off a specific platform defines
                        what source means.

  @param SystemContext  Pointer to system register context. Mostly used by debuggers and will
                        update the system context after the return from the interrupt if
                        modified. Don't change these values unless you know what you are doing

**/
VOID
EFIAPI
TimerInterruptHandler (
  IN  HARDWARE_INTERRUPT_SOURCE   Source,
  IN  EFI_SYSTEM_CONTEXT          SystemContext
  )
{
  EFI_TPL OriginalTPL;
  UINT32 Tmp;
  UINT32	PWMTimerBase;


  PWMTimerBase=PcdGet32(PcdPWMTimerBase);
  //DEBUG ((EFI_D_ERROR, "PWM Timer INT Occur\n"));
  //
  // DXE core uses this callback for the EFI timer tick. The DXE core uses locks
  // that raise to TPL_HIGH and then restore back to current level. Thus we need
  // to make sure TPL level is set to TPL_HIGH while we are handling the timer tick.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  // clear the periodic interrupt
  Tmp = MmioRead32 (PWMTimerBase + PWM_TINT_CSTAT_OFFSET);
  MmioWrite32 ((PWMTimerBase + PWM_TINT_CSTAT_OFFSET), Tmp);

  // signal end of interrupt early to help avoid losing subsequent ticks from long duration handlers
  gInterrupt->EndOfInterrupt (gInterrupt, Source);

  if (mTimerNotifyFunction) {
    mTimerNotifyFunction (mTimerPeriod);
  }

  gBS->RestoreTPL (OriginalTPL);
}

/**
  This function registers the handler NotifyFunction so it is called every time
  the timer interrupt fires.  It also passes the amount of time since the last
  handler call to the NotifyFunction.  If NotifyFunction is NULL, then the
  handler is unregistered.  If the handler is registered, then EFI_SUCCESS is
  returned.  If the CPU does not support registering a timer interrupt handler,
  then EFI_UNSUPPORTED is returned.  If an attempt is made to register a handler
  when a handler is already registered, then EFI_ALREADY_STARTED is returned.
  If an attempt is made to unregister a handler when a handler is not registered,
  then EFI_INVALID_PARAMETER is returned.  If an error occurs attempting to
  register the NotifyFunction with the timer interrupt, then EFI_DEVICE_ERROR
  is returned.

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  NotifyFunction   The function to call when a timer interrupt fires. This
                           function executes at TPL_HIGH_LEVEL. The DXE Core will
                           register a handler for the timer interrupt, so it can know
                           how much time has passed. This information is used to
                           signal timer based events. NULL will unregister the handler.
  @retval EFI_SUCCESS           The timer handler was registered.
  @retval EFI_UNSUPPORTED       The platform does not support timer interrupts.
  @retval EFI_ALREADY_STARTED   NotifyFunction is not NULL, and a handler is already
                                registered.
  @retval EFI_INVALID_PARAMETER NotifyFunction is NULL, and a handler was not
                                previously registered.
  @retval EFI_DEVICE_ERROR      The timer handler could not be registered.

**/
EFI_STATUS
EFIAPI
TimerDriverRegisterHandler (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_TIMER_NOTIFY         NotifyFunction
  )
{
  if ((NotifyFunction == NULL) && (mTimerNotifyFunction == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((NotifyFunction != NULL) && (mTimerNotifyFunction != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  DEBUG ((EFI_D_INFO, "++TimerDriverRegisterHandler\n"));
  mTimerNotifyFunction = NotifyFunction;

  return EFI_SUCCESS;
}

/**
    Make sure all ArrmVe Timers are disabled
**/
VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINT32	PWMTimerBase;

  PWMTimerBase=PcdGet32(PcdPWMTimerBase);
  // All PWM timer is off
  MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), 0);
}

/**

  This function adjusts the period of timer interrupts to the value specified
  by TimerPeriod.  If the timer period is updated, then the selected timer
  period is stored in EFI_TIMER.TimerPeriod, and EFI_SUCCESS is returned.  If
  the timer hardware is not programmable, then EFI_UNSUPPORTED is returned.
  If an error occurs while attempting to update the timer period, then the
  timer hardware will be put back in its state prior to this call, and
  EFI_DEVICE_ERROR is returned.  If TimerPeriod is 0, then the timer interrupt
  is disabled.  This is not the same as disabling the CPU's interrupts.
  Instead, it must either turn off the timer hardware, or it must adjust the
  interrupt controller so that a CPU interrupt is not generated when the timer
  interrupt fires.

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod      The rate to program the timer interrupt in 100 nS units. If
                           the timer hardware is not programmable, then EFI_UNSUPPORTED is
                           returned. If the timer is programmable, then the timer period
                           will be rounded up to the nearest timer period that is supported
                           by the timer hardware. If TimerPeriod is set to 0, then the
                           timer interrupts will be disabled.


  @retval EFI_SUCCESS           The timer period was changed.
  @retval EFI_UNSUPPORTED       The platform cannot change the period of the timer interrupt.
  @retval EFI_DEVICE_ERROR      The timer period could not be changed due to a device error.

**/
EFI_STATUS
EFIAPI
TimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod
  )
{
  EFI_STATUS  Status;
  UINT64      TimerTicks;
  UINT32			Tmp;
  UINT32	PWMTimerBase;

  PWMTimerBase=PcdGet32(PcdPWMTimerBase);
  // Stop PWM timer 0
  Tmp = MmioRead32 (PWMTimerBase + PWM_TCON_OFFSET);
  Tmp &= ~(0x1F << 0);
  MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET) ,Tmp);

  if (TimerPeriod == 0) {
    // leave timer disabled from above, and...
	Tmp = MmioRead32 (PWMTimerBase + PWM_TINT_CSTAT_OFFSET);
	Tmp &= ~(1 << 0);
	MmioWrite32 ((PWMTimerBase + PWM_TINT_CSTAT_OFFSET), Tmp);
    // disable timer 0/1 interrupt for a TimerPeriod of 0
	Status = gInterrupt->DisableInterruptSource (gInterrupt, gVector);
  } else {
	// Convert TimerPeriod into 1MHz clock counts (us units = 100ns units / 10)
	TimerTicks = DivU64x32 (TimerPeriod, 10);
	// if it's larger than 32-bits, pin to highest value
	if (TimerTicks > 0xffffffff) {
		TimerTicks = 0xffffffff;
	}

	// PWM Timer 0 used by Period counter with Auto re-load mode
	MmioWrite32 ((PWMTimerBase + PWM_TCNTB0_OFFSET), TimerTicks);
	// Set and Clear PWM Manually update for Timer 0
	Tmp = MmioRead32 (PWMTimerBase + PWM_TCON_OFFSET);
	Tmp |= (0x2 << 0);
	MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);
	Tmp &= ~(0x2 << 0);
	MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);
	// Set Auto re-load and start Timer
	Tmp |= (0x9 << 0);
	MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);

	// enable PWM Timer 0 interrupts
	Tmp = MmioRead32 (PWMTimerBase + PWM_TINT_CSTAT_OFFSET);
	Tmp |= (1 << 0);
	MmioWrite32 ((PWMTimerBase + PWM_TINT_CSTAT_OFFSET), Tmp);

	Status = gInterrupt->EnableInterruptSource (gInterrupt, gVector);
	}

  // Save the new timer period
  mTimerPeriod = TimerPeriod;
  return Status;
}

/**
  This function retrieves the period of timer interrupts in 100 ns units,
  returns that value in TimerPeriod, and returns EFI_SUCCESS.  If TimerPeriod
  is NULL, then EFI_INVALID_PARAMETER is returned.  If a TimerPeriod of 0 is
  returned, then the timer is currently disabled.

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod      A pointer to the timer period to retrieve in 100 ns units. If
                           0 is returned, then the timer is currently disabled.


  @retval EFI_SUCCESS           The timer period was returned in TimerPeriod.
  @retval EFI_INVALID_PARAMETER TimerPeriod is NULL.

**/
EFI_STATUS
EFIAPI
TimerDriverGetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL   *This,
  OUT UINT64                   *TimerPeriod
  )
{
  if (TimerPeriod == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerPeriod = mTimerPeriod;
  return EFI_SUCCESS;
}

/**
  This function generates a soft timer interrupt. If the platform does not support soft
  timer interrupts, then EFI_UNSUPPORTED is returned. Otherwise, EFI_SUCCESS is returned.
  If a handler has been registered through the EFI_TIMER_ARCH_PROTOCOL.RegisterHandler()
  service, then a soft timer interrupt will be generated. If the timer interrupt is
  enabled when this service is called, then the registered handler will be invoked. The
  registered handler should not be able to distinguish a hardware-generated timer
  interrupt from a software-generated timer interrupt.

  @param  This             The EFI_TIMER_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS           The soft timer interrupt was generated.
  @retval EFI_UNSUPPORTED       The platform does not support the generation of soft timer interrupts.

**/
EFI_STATUS
EFIAPI
TimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Interface structure for the Timer Architectural Protocol.

  @par Protocol Description:
  This protocol provides the services to initialize a periodic timer
  interrupt, and to register a handler that is called each time the timer
  interrupt fires.  It may also provide a service to adjust the rate of the
  periodic timer interrupt.  When a timer interrupt occurs, the handler is
  passed the amount of time that has passed since the previous timer
  interrupt.

  @param RegisterHandler
  Registers a handler that will be called each time the
  timer interrupt fires.  TimerPeriod defines the minimum
  time between timer interrupts, so TimerPeriod will also
  be the minimum time between calls to the registered
  handler.

  @param SetTimerPeriod
  Sets the period of the timer interrupt in 100 nS units.
  This function is optional, and may return EFI_UNSUPPORTED.
  If this function is supported, then the timer period will
  be rounded up to the nearest supported timer period.


  @param GetTimerPeriod
  Retrieves the period of the timer interrupt in 100 nS units.

  @param GenerateSoftInterrupt
  Generates a soft timer interrupt that simulates the firing of
  the timer interrupt. This service can be used to invoke the   registered handler if the timer interrupt has been masked for
  a period of time.

**/
EFI_TIMER_ARCH_PROTOCOL   gTimer = {
  TimerDriverRegisterHandler,
  TimerDriverSetTimerPeriod,
  TimerDriverGetTimerPeriod,
  TimerDriverGenerateSoftInterrupt
};


/**
  Initialize the state information for the Timer Architectural Protocol and
  the Timer Debug support protocol that allows the debugger to break into a
  running program.

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
EFIAPI
TimerInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_HANDLE  Handle = NULL;
  EFI_STATUS  Status;

  UINT32	Tmp;
  UINT32	PWMTimerBase;

  PWMTimerBase=PcdGet32(PcdPWMTimerBase);
  // Find the interrupt controller protocol.  ASSERT if not found.
  Status = gBS->LocateProtocol (&gHardwareInterruptProtocolGuid, NULL, (VOID **)&gInterrupt);
  ASSERT_EFI_ERROR (Status);

        // Timer Source : SCLK_MPLL(800Mhz)
        Tmp = MmioRead32(0x10020250);
        Tmp &= ~(0xF << 24);
        Tmp |= (0x6 << 24);
        MmioWrite32(0x10020250, Tmp);
        // Timer 0,1,2 prescale:0x02(/(0x02+1)) => 266666666hz (at least 0x01+1)
        Tmp = MmioRead32(PWMTimerBase + PWM_TCFG0_OFFSET);
        Tmp &= ~((0xFF << 8) + (0xFF << 0));
        Tmp |= (0x02 << 8) + (0x02 << 0);
        MmioWrite32 ((PWMTimerBase + PWM_TCFG0_OFFSET), Tmp);
        // Timer 0,1,2 divider:0x2(/4) => 66666666hz
        MmioWrite32 ((PWMTimerBase + PWM_TCFG1_OFFSET), (0x2 << 2) + (0x2 << 1) + (0x2 << 0));
/*
  // PWM Input source clock is 100Mhz and Configure 1Mhz for PWM Timer
  Tmp = MmioRead32 (PWMTimerBase + PWM_TCFG0_OFFSET);
  Tmp &= ~(0xFF << 0);
  Tmp |= (0x63 << 0);
  MmioWrite32 ((PWMTimerBase + PWM_TCFG0_OFFSET), Tmp);
  MmioWrite32 ((PWMTimerBase + PWM_TCFG1_OFFSET), 0x0);
*/
	//Timer 1 INT disable
  Tmp = MmioRead32 (PWMTimerBase + PWM_TINT_CSTAT_OFFSET);
  Tmp &= ~(1 << 1);
  MmioWrite32 ((PWMTimerBase + PWM_TINT_CSTAT_OFFSET), Tmp);

  // configure timer 1 Stop
  Tmp = MmioRead32 (PWMTimerBase + PWM_TCON_OFFSET);
  Tmp &= ~(0xF << 8);
  MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);

  // PWM Timer 1 used by Free running counter with Auto re-load mode
  MmioWrite32 ((PWMTimerBase + PWM_TCNTB1_OFFSET), 0xFFFFFFFF);
  // Set and Clear PWM Manually update for Timer 1
  Tmp = MmioRead32 (PWMTimerBase + PWM_TCON_OFFSET);
  Tmp |= (0x2 << 8);
  MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);
  Tmp &= ~(0x2 << 8);
  MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);
  // Set Auto re-load and start Timer
  Tmp |= (0x9 << 8);
  MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);

  // Install interrupt handler
  gVector = PWM_TIMER0_INTERRUPT_NUM;
  Status = gInterrupt->RegisterInterruptSource (gInterrupt, gVector, TimerInterruptHandler);
  ASSERT_EFI_ERROR (Status);

  // Disable the timer
  Status = TimerDriverSetTimerPeriod (&gTimer, 0);
  ASSERT_EFI_ERROR (Status);

  // PWM Timer 0 make to stop
  Tmp = MmioRead32 (PWMTimerBase + PWM_TCON_OFFSET);
  Tmp &= ~(0x1F << 0);
  MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);

  // PWM Timer 0 INT disable
  Tmp = MmioRead32 (PWMTimerBase + PWM_TINT_CSTAT_OFFSET);
  Tmp &= ~(1 << 0);
  MmioWrite32 ((PWMTimerBase + PWM_TINT_CSTAT_OFFSET), Tmp);

  // PWM Timer 0 used by Period counter with Auto re-load mode
  MmioWrite32 ((PWMTimerBase + PWM_TCNTB0_OFFSET), FixedPcdGet32(PcdTimerPeriod));
  Status = TimerDriverSetTimerPeriod (&gTimer, FixedPcdGet32(PcdTimerPeriod));
  ASSERT_EFI_ERROR (Status);

  // Set and Clear PWM Manually update for Timer 0
  Tmp = MmioRead32 (PWMTimerBase + PWM_TCON_OFFSET);
  Tmp |= (0x2 << 0);
  MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);
  Tmp &= ~(0x2 << 0);
  MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);
  // Set Auto re-load and start Timer
  Tmp |= (0x9 << 0);
  MmioWrite32 ((PWMTimerBase + PWM_TCON_OFFSET), Tmp);

  //PWM Timer0 INT enable
  Tmp = MmioRead32 (PWMTimerBase + PWM_TINT_CSTAT_OFFSET);
  Tmp |= (1 << 0);
  MmioWrite32 ((PWMTimerBase + PWM_TINT_CSTAT_OFFSET), Tmp);

  // Install the Timer Architectural Protocol onto a new handle
  Status = gBS->InstallMultipleProtocolInterfaces(
                  &Handle,
                  &gEfiTimerArchProtocolGuid,      &gTimer,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  // Register for an ExitBootServicesEvent
  Status = gBS->CreateEvent (EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY, ExitBootServicesEvent, NULL, &EfiExitBootServicesEvent);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
