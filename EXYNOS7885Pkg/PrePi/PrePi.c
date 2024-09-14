/** @file

  Copyright (c) 2011-2017, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/PrePiLib.h>
#include <Library/PrintLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>

#include <Ppi/GuidedSectionExtraction.h>
#include <Ppi/ArmMpCoreInfo.h>
#include <Ppi/SecPerformance.h>

#include "PrePi.h"

VOID EFIAPI ProcessLibraryConstructorList(VOID);

VOID UartInit(VOID)
{
  SerialPortInitialize();
  
  ZeroMem((VOID*)0x67000000, 0x7E9000);

  DEBUG((EFI_D_INFO, "\nPEdeka on sexynos (ARM)\n"));
  DEBUG(
      (EFI_D_INFO, "Firmware version %s built %a %a\n\n",
       (CHAR16 *)PcdGetPtr(PcdFirmwareVersionString), __TIME__, __DATE__));
}

VOID Main (IN  UINT32  StartTimeStamp)
{
  EFI_HOB_HANDOFF_INFO_TABLE  *HobList;
  EFI_STATUS                  Status;

  UINTN MemoryBase     = 0;
  UINTN MemorySize     = 0;
  UINTN UefiMemoryBase = 0;
  UINTN UefiMemorySize = 0;
  UINTN StacksBase     = 0;
  UINTN StacksSize     = 0;
  
  //decon thing
  MmioWrite32 (0x14400070, 0x1281);
  
  // Architecture-specific initialization
  // Enable Floating Point
  ArmEnableVFP();

  /* Enable program flow prediction, if supported */
  ArmEnableBranchPrediction();

  // Declare UEFI region
  MemoryBase     = FixedPcdGet32(PcdSystemMemoryBase);
  MemorySize     = FixedPcdGet32(PcdSystemMemorySize);
  UefiMemoryBase = FixedPcdGet32(PcdUefiMemPoolBase);
  UefiMemorySize = FixedPcdGet32(PcdUefiMemPoolSize);
  StacksSize     = FixedPcdGet32(PcdPrePiStackSize);
  StacksBase     = UefiMemoryBase + UefiMemorySize - StacksSize;

  DEBUG(
      (EFI_D_INFO | EFI_D_LOAD,
       "UEFI Memory Base = 0x%x, Size = 0x%x, Stack Base = 0x%x, Stack "
       "Size = 0x%x\n",
       (VOID *)UefiMemoryBase, UefiMemorySize, (VOID *)StacksBase, StacksSize));

  // Declare the PI/UEFI memory region
  // Set up HOB
  HobList = HobConstructor(
      (VOID *)UefiMemoryBase, UefiMemorySize, (VOID *)UefiMemoryBase,
      (VOID *)StacksBase);
  PrePeiSetHobList (HobList);

  // Invalidate cache
  InvalidateDataCacheRange(
      (VOID *)(UINTN)PcdGet64(PcdFdBaseAddress), PcdGet32(PcdFdSize));

  // Initialize MMU
  Status = MemoryPeim(UefiMemoryBase, UefiMemorySize);

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Failed to configure MMU\n"));
    CpuDeadLoop();
  }

  DEBUG((EFI_D_LOAD | EFI_D_INFO, "MMU configured from device config\n"));

  // Add HOBs
  BuildStackHob ((UINTN)StacksBase, StacksSize);

  // TODO: Call CpuPei as a library
  BuildCpuHob (40, PcdGet8 (PcdPrePiCpuIoSize));

  // Set the Boot Mode
  SetBootMode (BOOT_WITH_FULL_CONFIGURATION);

  // Initialize Platform HOBs (CpuHob and FvHob)
  Status = PlatformPeim ();
  ASSERT_EFI_ERROR (Status);

  // Now, the HOB List has been initialized, we can register performance information
  //PERF_START (NULL, "PEI", NULL, StartTimeStamp);
  
  DEBUG((EFI_D_INFO, "nice im stuck here please reboot me\n"));

  // SEC phase needs to run library constructors by hand.
  ProcessLibraryConstructorList ();
  
  DEBUG((EFI_D_INFO, "not expected to reach here on exynos 5410\n"));

  // Assume the FV that contains the SEC (our code) also contains a compressed FV.
  Status = DecompressFirstFv ();
  ASSERT_EFI_ERROR (Status);
  
  DEBUG((EFI_D_INFO, "and guardian angels sang this strain\n"));

  // Load the DXE Core and transfer control to it
  Status = LoadDxeCoreFromFv (NULL, 0);
  ASSERT_EFI_ERROR (Status);
}

VOID
CEntryPoint ()
{
  UartInit();
  Main(0);
}
