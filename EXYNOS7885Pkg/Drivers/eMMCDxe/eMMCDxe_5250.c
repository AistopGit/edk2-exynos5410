/** @file
  MMC/SD Card driver for Secure Digital Host Controller

  This driver always produces a BlockIo protocol but it starts off with no Media
  present. A TimerCallBack detects when media is inserted or removed and after
  a media change event a call to BlockIo ReadBlocks/WriteBlocks will cause the
  media to be detected (or removed) and the BlockIo Media structure will get
  updated. No MMC/SD Card harward registers are updated until the first BlockIo
  ReadBlocks/WriteBlocks after media has been insterted (booting with a card
  plugged in counts as an insertion event).

  Copyright (c) 2012, Samsung Electronics Co. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/



#include <Library/TimerLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/ExynosGpio.h>
#include <Platform/ArmPlatform.h>
#include <Platform/Exynos5250.h>
#include <Platform/Arndale5250.h>


#include "eMMCDxe.h"

#define DMA_UNCACHE_ALLOC 0
#if DMA_UNCACHE_ALLOC
UINT32 *DMABuffer;  
#endif 
void MSHC_reset_all(void)
{
	int count;
      volatile int ctl_val=0;
      UINT32 SdMmcBaseAddr;
      SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);    

	/* Wait max 100 ms */
	count = 10000;

	/* before reset ciu, it should check DATA0. if when DATA0 is low and
	it resets ciu, it might make a problem */
      while ((MmioRead32 ((SdMmcBaseAddr + MSHCI_STATUS)) & (1<<9))){        
		if (count == 0) {
			DEBUG ((EFI_D_ERROR, "MMC controller never released. \n"));
			return;
		}
		count--;
		MicroSecondDelay(1);
	}

    // Reset CIU
    ctl_val = MmioRead32((SdMmcBaseAddr + MSHCI_CTRL));
    ctl_val |= CTRL_RESET;
    MmioWrite32((SdMmcBaseAddr + MSHCI_CTRL), ctl_val);

    //Reset FIFO
    MSHC_reset_fifo();
 
    //Reset DMA
    ctl_val = MmioRead32((SdMmcBaseAddr + MSHCI_CTRL));
    ctl_val |= DMA_RESET;
    MmioWrite32((SdMmcBaseAddr + MSHCI_CTRL), ctl_val);

    //Set auto stop CMD
    ctl_val = MmioRead32((SdMmcBaseAddr + MSHCI_CTRL));
    ctl_val |= SEND_AS_CCSD;
    MmioWrite32((SdMmcBaseAddr + MSHCI_CTRL), ctl_val);    

}


void MSHC_Set_DDR(int BusMode)
{

    UINT32 clkphase;
    UINT32 SdMmcBaseAddr;
    SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);

    if(BusMode==BUSMODE_DDR)
    {
        DEBUG ((EFI_D_INFO, "MSHC DDR .\n"));
        MmioWrite32((SdMmcBaseAddr + MSHCI_UHS_REG), UHS_DDR);   
        clkphase = 0x03020001;
    }
    else
    {
        DEBUG ((EFI_D_INFO, "MSHC Non DDR .\n"));
        MmioWrite32((SdMmcBaseAddr + MSHCI_UHS_REG), UHS_NON_DDR); 
        clkphase = 0x03030002;
    }
    MmioWrite32((SdMmcBaseAddr + MSHCI_CLKSEL), clkphase);      

}


void MSHC_5250_init(void)
{
    UINT32 SdMmcBaseAddr;    
    SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);    
    
    /* Power Enable Register */
    MmioWrite32((SdMmcBaseAddr + MSHCI_PWREN), POWER_ENABLE);    

    MSHC_reset_all();

    // Initialize FIFO
    MmioWrite32((SdMmcBaseAddr + MSHCI_FIFOTH), (MSIZE_8 | TX_WMARK_DEFAULT | RX_WMARK_DEFAULT));  

	/* It clears all pending interrupts */
    MmioWrite32((SdMmcBaseAddr + MSHCI_RINTSTS), INTMSK_ALL);  
	/* It dose not use Interrupt. Disable all */
    MmioWrite32((SdMmcBaseAddr + MSHCI_INTMSK), 0);     

	/* set debounce filter value*/
    MmioWrite32((SdMmcBaseAddr + MSHCI_DEBNCE), (0xfffff));     

	/* clear card type. set 1-bit mode */
    MmioWrite32((SdMmcBaseAddr + MSHCI_CTYPE), 0x0);        

	/* set bus mode register for IDMAC */
    MmioWrite32((SdMmcBaseAddr + MSHCI_BMOD), (BMOD_IDMAC_RESET));     
	
	/* disable all interrupt source of IDMAC */
    MmioWrite32((SdMmcBaseAddr + MSHCI_IDINTEN), (0x0));

	/* set max timeout */
    MmioWrite32((SdMmcBaseAddr + MSHCI_TMOUT), (0xffffffff));

    MmioWrite32((SdMmcBaseAddr + MSHCI_BLKSIZ), BLEN_512BYTES);        
    MmioWrite32((SdMmcBaseAddr + MSHCI_CLKSEL), 0x03030002);        

}

EFI_STATUS
InitializeMSHC (
  VOID
  )
{

  EFI_STATUS    Status;
  EXYNOS_GPIO *Gpio;
  UINT32 CmuBaseAddr;
  UINT32 i, clock;
  volatile UINT32 ctl_val;


  Status = gBS->LocateProtocol(&gSamsungPlatformGpioProtocolGuid, NULL, (VOID **)&Gpio);
  ASSERT_EFI_ERROR(Status);

  CmuBaseAddr = PcdGet32(PcdCmuBase);

  // Set Clock Source for using MPLL
    ctl_val = MmioRead32((CmuBaseAddr + CLK_SRC_FSYS_OFFSET));
    ctl_val &= ~(0xf);
    ctl_val |= (0x6);
    MmioWrite32((CmuBaseAddr + CLK_SRC_FSYS_OFFSET), ctl_val);

    // CLK mask  
    ctl_val = MmioRead32((CmuBaseAddr + CLK_SRC_MASK_FSYS_OFFSET));
    ctl_val |= (0x1);
    MmioWrite32((CmuBaseAddr + CLK_SRC_MASK_FSYS_OFFSET), ctl_val);

    // CLK gating
    ctl_val = MmioRead32((CmuBaseAddr + CLK_GATE_IP_FSYS_OFFSET));
    ctl_val |= (0x1<<12);
    MmioWrite32((CmuBaseAddr + CLK_GATE_IP_FSYS_OFFSET), ctl_val);


  /* MMC2 clock div */
  clock = 800; //MPLL in MHz
  for(i=0; i<= 0xf; i++)
  {
	  if((clock / (i+1)) <= 90) {
	  	  MmioAndThenOr32 ((CmuBaseAddr + CLK_DIV_FSYS1_OFFSET), ~(0xF << 0), (i << 0));
		  break;
	  }
  }
 

  // 2. GPIO setting
  // Set GPIO for using SD/MMC CH0 for eMMC
  Gpio->Set(Gpio,SD_0_CLK,GPIO_MODE_SPECIAL_FUNCTION_2);
  Gpio->Set(Gpio,SD_0_CMD,GPIO_MODE_SPECIAL_FUNCTION_2);

  //Set CDn as HIGH
  Gpio->Set(Gpio,SD_0_CDn, GPIO_MODE_OUTPUT_1); 


  Gpio->Set(Gpio,SD_0_DATA0,GPIO_MODE_SPECIAL_FUNCTION_2);
  Gpio->Set(Gpio,SD_0_DATA1,GPIO_MODE_SPECIAL_FUNCTION_2);
  Gpio->Set(Gpio,SD_0_DATA2,GPIO_MODE_SPECIAL_FUNCTION_2);
  Gpio->Set(Gpio,SD_0_DATA3,GPIO_MODE_SPECIAL_FUNCTION_2);
  Gpio->Set(Gpio,SD_0_DATA4,GPIO_MODE_SPECIAL_FUNCTION_2);
  Gpio->Set(Gpio,SD_0_DATA5,GPIO_MODE_SPECIAL_FUNCTION_2);
  Gpio->Set(Gpio,SD_0_DATA6,GPIO_MODE_SPECIAL_FUNCTION_2);
  Gpio->Set(Gpio,SD_0_DATA7,GPIO_MODE_SPECIAL_FUNCTION_2);

  Gpio->SetPull(Gpio,SD_0_CLK,GPIO_PULL_NONE);
  Gpio->SetPull(Gpio,SD_0_CMD,GPIO_PULL_NONE);
  Gpio->SetPull(Gpio,SD_0_CDn,GPIO_PULL_UP);
  Gpio->SetPull(Gpio,SD_0_DATA0,GPIO_PULL_UP);
  Gpio->SetPull(Gpio,SD_0_DATA1,GPIO_PULL_UP);
  Gpio->SetPull(Gpio,SD_0_DATA2,GPIO_PULL_UP);
  Gpio->SetPull(Gpio,SD_0_DATA3,GPIO_PULL_UP);
  Gpio->SetPull(Gpio,SD_0_DATA4,GPIO_PULL_UP);
  Gpio->SetPull(Gpio,SD_0_DATA5,GPIO_PULL_UP);
  Gpio->SetPull(Gpio,SD_0_DATA6,GPIO_PULL_UP);
  Gpio->SetPull(Gpio,SD_0_DATA7,GPIO_PULL_UP);

Gpio->SetStrength(Gpio,SD_0_CLK,GPIO_DRV_3X);
Gpio->SetStrength(Gpio,SD_0_CMD,GPIO_DRV_3X);
Gpio->SetStrength(Gpio,SD_0_CDn,GPIO_DRV_3X);
Gpio->SetStrength(Gpio,SD_0_DATA0,GPIO_DRV_3X);
Gpio->SetStrength(Gpio,SD_0_DATA1,GPIO_DRV_3X);
Gpio->SetStrength(Gpio,SD_0_DATA2,GPIO_DRV_3X);
Gpio->SetStrength(Gpio,SD_0_DATA3,GPIO_DRV_3X);
Gpio->SetStrength(Gpio,SD_0_DATA4,GPIO_DRV_3X);
Gpio->SetStrength(Gpio,SD_0_DATA5,GPIO_DRV_3X);
Gpio->SetStrength(Gpio,SD_0_DATA6,GPIO_DRV_3X);
Gpio->SetStrength(Gpio,SD_0_DATA7,GPIO_DRV_3X);

	MSHC_5250_init();
#if DMA_UNCACHE_ALLOC  
  DMABuffer = (UINT32 *)UncachedAllocatePool(EMMC_DMA_PHYSICAL_BUFFER_SIZE/2);
  if(DMABuffer==NULL)
  {
    DEBUG ((EFI_D_ERROR, "MSHC::DMA alloc failed \n"));
  }
#endif   
	return EFI_SUCCESS;
  
}

void MSHC_reset_fifo(void)
{
    volatile int ctl_val=0;
    UINT32 SdMmcBaseAddr;
    SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);    
    //Reset FIFO
    ctl_val = MmioRead32((SdMmcBaseAddr + MSHCI_CTRL));
    ctl_val |= FIFO_RESET;
    MmioWrite32((SdMmcBaseAddr + MSHCI_CTRL), ctl_val);

}


VOID MSHC_clock_onoff (int value)
{
    volatile UINT32 loop_count = 0x100000;
    UINT32 SdMmcBaseAddr;

    SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);
    
    if(value==CLK_ENABLE)
    {
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CLKENA), (0x1<<0));
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CMD), 0);
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CMD), CMD_ONLY_CLK);
	do {
             if((MmioRead32 (SdMmcBaseAddr + MSHCI_CMD) & CMD_STRT_BIT)==0)
			break;
		loop_count--;
	} while (loop_count);        
    }
    else
    {
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CLKENA), (0x0<<0));
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CMD), 0);
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CMD), CMD_ONLY_CLK);
	do {
        if((MmioRead32 (SdMmcBaseAddr + MSHCI_CMD) & CMD_STRT_BIT)==0)
			break;
		loop_count--;
	} while (loop_count);      
    }

	if (loop_count == 0) {
          DEBUG ((EFI_D_ERROR, "MSHC::clockonoff : Clk = %d\n", value));
	}    
    
}


VOID
UpdateMSHCClkFrequency (
  UINTN NewCLK
  )
{
  UINT32 CmuBaseAddr;
  UINT32 SdMmcBaseAddr;

  SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);
  CmuBaseAddr = PcdGet32(PcdCmuBase);

 // Disable all clocks to not provide the clock to the card
//(CONFIG_CPU_EXYNOS5250_EVT1) 
  MSHC_clock_onoff(CLK_DISABLE);

  //Set new clock frequency.
    if (NewCLK == MSHC_CLK_400)
    {
        DEBUG ((EFI_D_INFO, "MSHC::CLK=400.\n"));
        // MPLL=800, cclk_in=100, 100M/250=400k
        MmioAndThenOr32 ((CmuBaseAddr + CLK_DIV_FSYS1_OFFSET), ~(0xFFFF), 0x1);
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CLKDIV), 125);        
    }
    else if (NewCLK == MSHC_CLK_25M)
    {
        DEBUG ((EFI_D_INFO, "MSHC::CLK=25M.\n"));
        // DDR mode use CLKDIV MPLL = 800, SCLK = 400, cclk_in=100M
        MmioAndThenOr32 ((CmuBaseAddr + CLK_DIV_FSYS1_OFFSET), ~(0xFFFF), 0x1);
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CLKDIV), 2);
    }
    else if(NewCLK == MSHC_CLK_50M)
    {
        DEBUG ((EFI_D_INFO, "MSHC::CLK=50M.\n"));
        // DDR mode use CLKDIV MPLL = 800, SCLK = 400, cclk_in=100M
        MmioAndThenOr32 ((CmuBaseAddr + CLK_DIV_FSYS1_OFFSET), ~(0xFFFF), 0x1);
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CLKDIV), 1);

        MSHC_Set_DDR(BUSMODE_DDR);     
    }

  MSHC_clock_onoff(CLK_ENABLE);

}

extern MSHC_OPERATION_MODE MSHC_operation_mode;
void mshci_set_mdma_desc(UINT8 *desc_vir, UINT8 *desc_phy, 
				UINT32 des0, UINT32 des1, UINT32 des2)
{
	((struct mshci_idmac *)(desc_vir))->des0 = des0;
	((struct mshci_idmac *)(desc_vir))->des1 = des1;
	((struct mshci_idmac *)(desc_vir))->des2 = des2;
	((struct mshci_idmac *)(desc_vir))->des3 = (UINT32)desc_phy +
					sizeof(struct mshci_idmac);
}


VOID
PrepareTransfer (
IN OUT VOID *Buffer, UINTN BlockCount, IN OPERATION_TYPE OperationType
	)
{
  UINT32 SdMmcBaseAddr;
  UINT32 EmmcDMABufferBase;  
  volatile UINT32 MshcRegValue;
  struct mshci_idmac *pdesc_dmac;  
  UINT32 des_flag;
  UINTN i=0;
  UINT32 ByteCnt=0;
  UINT32 BlockCnt=BlockCount;  
  
  SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);
#if DMA_UNCACHE_ALLOC
  EmmcDMABufferBase = (UINT32)DMABuffer;
#else
  EmmcDMABufferBase = PcdGet32(PcdEmmcDMABufferBase);
#endif 
  //pdesc_dmac = idmac_desc;
  pdesc_dmac = (struct mshci_idmac *)EmmcDMABufferBase;

    if(MSHC_operation_mode==MSHC_FIFO)
    {
        MSHC_reset_fifo();
        
        // 1. enable interrupt mode
        MshcRegValue = MmioRead32(SdMmcBaseAddr + MSHCI_CTRL);
        MshcRegValue |= INT_ENABLE;
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CTRL), MshcRegValue);

        // 2. DDR mode
            
        // 3. set BLKSIZE
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_BLKSIZ), BLEN_512BYTES);
        
         // 4. set BYTCNT
         MmioWrite32 ((SdMmcBaseAddr + MSHCI_BYTCNT), BLEN_512BYTES);
        
        //Setting Data timeout counter value to max value.
          MmioWrite32((SdMmcBaseAddr + MSHCI_TMOUT), (0xffffffff));  

    }
    else if(MSHC_operation_mode==MSHC_IDMA)
    {
        ZeroMem ((VOID *)EmmcDMABufferBase, PHY_BUF_OFFSET);//20120608
        if(OperationType==WRITE)
        {
           CopyMem((VOID *)(EmmcDMABufferBase+PHY_BUF_OFFSET), (VOID *)Buffer, BlockCount*BLEN_512BYTES);
        }
        else
        {
            //DEBUG ((EFI_D_INFO, "MSHC_DMA prepare READ:%d Block \n", BlockCount)); 
        }

        MSHC_reset_fifo();

        //IDMA reset
        MshcRegValue = MmioRead32(SdMmcBaseAddr + MSHCI_BMOD);
        MshcRegValue |= (BMOD_IDMAC_RESET);
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_BMOD), MshcRegValue);

        
        // 1. enable IDMA at CTRL
        MshcRegValue = MmioRead32(SdMmcBaseAddr + MSHCI_CTRL);
        MshcRegValue &= ~(INT_ENABLE);
        MshcRegValue |= (ENABLE_IDMAC);
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_CTRL), MshcRegValue);


        // 2. enable IDMA at BMODE
        //Set Block Size and Block count.
        MshcRegValue = MmioRead32(SdMmcBaseAddr + MSHCI_BMOD);
        MshcRegValue |= (BMOD_IDMAC_ENABLE | BMOD_IDMAC_FB);
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_BMOD), MshcRegValue);
        
        // interrupt enable   
        MmioWrite32((SdMmcBaseAddr + MSHCI_IDINTEN), (0x337));  

    for(i=0; ; i++)
    {
        // set descriptor multichain    
	des_flag = (MSHCI_IDMAC_OWN|MSHCI_IDMAC_CH);
	des_flag |= (i==0) ? MSHCI_IDMAC_FS:0;
        if(BlockCnt<=8)
        {
            des_flag |= (MSHCI_IDMAC_LD);
            mshci_set_mdma_desc((UINT8 *)pdesc_dmac,
                        (UINT8 *)(EmmcDMABufferBase-sizeof(struct mshci_idmac)),
                        des_flag, BlockCnt*BLEN_512BYTES,
                        ((UINT32)((EmmcDMABufferBase + PHY_BUF_OFFSET)+(UINT32)(i*PHY_BUF_SIZE))));
            break;

        }

        mshci_set_mdma_desc((UINT8 *)pdesc_dmac,
                    (UINT8 *)pdesc_dmac,
                    des_flag, BLEN_512BYTES*8,
                    ((UINT32)((EmmcDMABufferBase + PHY_BUF_OFFSET)+(UINT32)(i*PHY_BUF_SIZE))));

        BlockCnt -=8;
        pdesc_dmac++;

    }

    MmioWrite32 ((SdMmcBaseAddr + MSHCI_DBADDR), (UINT32)EmmcDMABufferBase);

        // 3. set BLKSIZE
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_BLKSIZ), BLEN_512BYTES);
        
         // 4. set BYTCNT
         ByteCnt = (BlockCount*BLEN_512BYTES);
         MmioWrite32 ((SdMmcBaseAddr + MSHCI_BYTCNT), ByteCnt);
        
        //Setting Data timeout counter value to max value.
          MmioWrite32((SdMmcBaseAddr + MSHCI_TMOUT), (0xffffffff));  
        DEBUG ((EFI_D_INFO, "Block:%d BYTCNT:0x%x ByteCnt:0x%x\n", BlockCount,MmioRead32(SdMmcBaseAddr + MSHCI_BYTCNT), ByteCnt)); 

    }

}

EFI_STATUS MSHC_ReadFIFO(IN UINTN Size32, OUT VOID *Buffer)
{
    EFI_STATUS Status;
    UINT32 SdMmcBaseAddr;
    UINTN *DataBuffer = Buffer;    
    UINTN BufSize=Size32;
    UINTN FifoCount=0;
    UINTN Count=0;

    SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);        
    //Check controller status to make sure there is no error.
    
    while (BufSize) 
    {
        if((MmioRead32 (SdMmcBaseAddr + MSHCI_RINTSTS)) & INTMSK_RXDR) 
        {
            
            //Read block worth of data.
            FifoCount = GET_FIFO_COUNT(MmioRead32 (SdMmcBaseAddr + MSHCI_STATUS));
            if (FifoCount > BufSize)
            {
                FifoCount = BufSize;
            }
            DEBUG ((EFI_D_INFO, "MSHC::ReadBlock RXDR FIFO:%d\n", FifoCount));            
            for (Count = 0; Count < FifoCount; Count++) 
            {
                *DataBuffer++ = MmioRead32 (SdMmcBaseAddr + MSHCI_FIFO);
            }
            MmioWrite32((SdMmcBaseAddr + MSHCI_RINTSTS), INTMSK_RXDR);
            BufSize -= FifoCount;
        }
        
        else if((MmioRead32 (SdMmcBaseAddr + MSHCI_RINTSTS)) & INTMSK_DTO) 
        {
            
            //Read block worth of data.
            FifoCount = GET_FIFO_COUNT(MmioRead32 (SdMmcBaseAddr + MSHCI_STATUS));
            if (FifoCount > BufSize)
            {
                FifoCount = BufSize;
            }
            DEBUG ((EFI_D_INFO, "MSHC::ReadBlock DTO FIFO:%d\n", FifoCount));            
            for (Count = 0; Count < FifoCount; Count++) 
            {
              *DataBuffer++ = MmioRead32 (SdMmcBaseAddr + MSHCI_FIFO);              
            }
            MmioWrite32((SdMmcBaseAddr + MSHCI_RINTSTS), INTMSK_DTO);
            BufSize -= FifoCount;
        }    

        else if((MmioRead32 (SdMmcBaseAddr + MSHCI_RINTSTS)) & (INTMSK_DCRC|INTMSK_DRTO|INTMSK_HTO|INTMSK_FRUN))
        {
            DEBUG ((EFI_D_INFO, "MSHC::ReadBlock Error RINT:0x%x\n", MmioRead32 (SdMmcBaseAddr + MSHCI_RINTSTS)));
            return EFI_DEVICE_ERROR;
        }
    }

    if(BufSize==0)  
    {
        Status = EFI_SUCCESS;
    }
    else
    {
        Status = EFI_BAD_BUFFER_SIZE;
    }
    return Status;
}

EFI_STATUS MSHC_WriteFIFO(IN UINTN Size32, IN VOID *Buffer)
{
    UINT32 SdMmcBaseAddr;
    UINTN *DataBuffer = Buffer;    
    UINTN BufSize=Size32;
    UINTN Count=0;

    SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);

    if(BufSize>FIFO_SIZE)
    {
        DEBUG ((EFI_D_INFO, "MSHC::Error MSHC_WriteFIFO Bad buffer size\n"));            
        return EFI_BAD_BUFFER_SIZE;
    }
        
    //Write block worth of data.
    for (Count = 0; Count < BufSize; Count++) 
    {
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_FIFO), *DataBuffer++);
    }
    return EFI_SUCCESS;
    
}

EFI_STATUS MSHC_ReadDMA(OUT VOID *Buffer, IN UINTN BlockCount)
{
    EFI_STATUS Status;
    UINT32 SdMmcBaseAddr;
    UINT32 EmmcDMABufferBase;
    UINTN Count=MAX_RETRY_COUNT;

    SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);   
#if DMA_UNCACHE_ALLOC
    EmmcDMABufferBase = (UINT32)DMABuffer;
#else
    EmmcDMABufferBase = PcdGet32(PcdEmmcDMABufferBase);
#endif 
    //Check controller status to make sure there is no error.
    
    while (Count) 
    {
        if((MmioRead32 (SdMmcBaseAddr + MSHCI_RINTSTS)) & INTMSK_DTO) 
        {
            CopyMem((VOID *)Buffer, (VOID *)(EmmcDMABufferBase+PHY_BUF_OFFSET), BlockCount*BLEN_512BYTES);
            DEBUG ((EFI_D_INFO, "MSHC_ReadDMA Over %d Blocks\n", BlockCount));                        
            break;
        }
        else if((MmioRead32 (SdMmcBaseAddr + MSHCI_RINTSTS)) & (DATA_ERR|DATA_TOUT)) 
        {
            DEBUG ((EFI_D_ERROR, "MSHC_ReadDMA Err RINT:0x%x\n", MmioRead32 (SdMmcBaseAddr + MSHCI_RINTSTS)));            
            return EFI_DEVICE_ERROR;
        }
        
        else
        {
            Count--;
            MicroSecondDelay(1);                      
        }

    }
    if(Count!=0)  
    {
        DEBUG ((EFI_D_INFO, "\n"));
        Status = EFI_SUCCESS;
    }
    else
    {
        DEBUG ((EFI_D_ERROR, "\nMSHC_ReadDMA bad buffer size RINT:0x%x\n", MmioRead32 (SdMmcBaseAddr + MSHCI_RINTSTS)));            
        Status = EFI_BAD_BUFFER_SIZE;
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_PLDMND), 7);

    }
    return Status;
}

EFI_STATUS MSHC_WriteDMA(IN VOID *Buffer, IN UINTN BlockCount)
{
    EFI_STATUS Status;
    UINT32 SdMmcBaseAddr;
    UINT32 EmmcDMABufferBase;
    UINTN Count=MAX_RETRY_COUNT;

    SdMmcBaseAddr = PcdGet32(PcdSdMmcCH0Base);   
    EmmcDMABufferBase = PcdGet32(PcdEmmcDMABufferBase);     
    
    while (Count) 
    {
        if((MmioRead32 (SdMmcBaseAddr + MSHCI_RINTSTS)) & INTMSK_DTO) 
        {
            DEBUG ((EFI_D_INFO, "MSHC_writeDMA Over %d blocks\n", BlockCount));    
            CopyMem((VOID *)(EmmcDMABufferBase+PHY_BUF_OFFSET), (VOID *)Buffer, BlockCount*BLEN_512BYTES);                           
            break;
        }
        else
        {
            MicroSecondDelay(1);
            Count--;
        }

    }

    if(Count!=0)  
    {
        Status = EFI_SUCCESS;
    }
    else
    {
        Status = EFI_BAD_BUFFER_SIZE;
        MmioWrite32 ((SdMmcBaseAddr + MSHCI_PLDMND), 7);
        DEBUG ((EFI_D_ERROR, "MSHC_writeDMA bad buffer size RINT:0x%x \n", MmioRead32 (SdMmcBaseAddr + MSHCI_RINTSTS)));            
    }
    return Status;
}        
