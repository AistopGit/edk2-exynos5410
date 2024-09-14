#
#  Copyright (c) 2018, Linaro Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

#include <AsmMacroIoLib.h>
#include <Library/ArmLib.h>

ASM_FUNC(ExynosFlushTLBs)
    mcr 	p15, 0, r0, c8, c7, 0	        @ flush I,D TLBs
	mcr	    p15, 0, r0, c7, c5, 4	        @ ISB
	bx lr

ASM_FUNC(ArmPlatformPeiBootAction)
        /*stp     r12, r13, [sp, -16]!
        add     r12, sp, 0
        nop
        ldp     r12, r13, [sp], 16*/
        bx lr

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_FUNC(ArmPlatformIsPrimaryCore)
  /*mov   r1, #FixedPcdGet32(PcdArmPrimaryCoreMask)
  and   r0, r0, r1
  mov   r1, #FixedPcdGet32(PcdArmPrimaryCore)
  cmp   r0, r1
  mov  r0, r1*/
  bx lr

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_FUNC(ArmPlatformGetPrimaryCoreMpId)
  mov r0, #FixedPcdGet32(PcdArmPrimaryCore)
  bx lr

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
// With this function: CorePos = (ClusterId * 4) + CoreId
ASM_FUNC(ArmPlatformGetCorePosition)
  and   r1, r0, #ARM_CORE_MASK
  and   r0, r0, #ARM_CLUSTER_MASK
  add   r0, r1, r0, LSR #6
  bx lr
