#ifndef _EFI_EXYNOS5_USB2_PHY_H_
#define _EFI_EXYNOS5_USB2_PHY_H_

#define USB_XHCI_HCCAPBASE (0x12000000) //Gaia
#define USB_EHCI_HCCAPBASE (0x12110000) //Gaia
#define USB_OHCI_HCCAPBASE (USB_EHCI_HCCAPBASE + 0x10000)

#define EXYNOS5_USB2_PHY_HOST_CTRL0 0x12130000 //Gaia

#define HOST_CTRL0_PHYSWRSTALL			(0x1 << 31)
#define CLKSEL_50M				(0x7 << 16)
#define CLKSEL_24M				(0x5 << 16)
#define CLKSEL_20M				(0x4 << 16)
#define CLKSEL_19200K				(0x3 << 16)
#define CLKSEL_12M				(0x2 << 16)
#define CLKSEL_10M				(0x1 << 16)
#define CLKSEL_9600K				(0x0 << 16)
#define HOST_CTRL0_FSEL_MASK			(0x7 << 16)
#define HOST_CTRL0_COMMONONN			(0x1 << 9)
#define HOST_CTRL0_PHYSWRST			(0x1 << 0)
#define HOST_CTRL0_SIDDQ			(0x1 << 6)
#define HOST_CTRL0_FORCESLEEP			(0x1 << 5)
#define HOST_CTRL0_FORCESUSPEND			(0x1 << 4)
#define HOST_CTRL0_LINKSWRST			(0x1 << 1)
#define HOST_CTRL0_UTMISWRST			(0x1 << 2)
#define HSIC_CTRL_REFCLKDIV(val)		((val&0x7f) << 16)
#define HSIC_CTRL_REFCLKSEL(val)		((val&0x3) << 23)
#define HSIC_CTRL_PHYSWRST			(0x1 << 0)
#define PHY_HSIC_CTRL1				(EXYNOS5_USB2_PHY_HOST_CTRL0 + 0x10)
#define PHY_HSIC_CTRL2				(EXYNOS5_USB2_PHY_HOST_CTRL0 + 0x20)
#define PHY_HOST_EHCICTRL			(EXYNOS5_USB2_PHY_HOST_CTRL0 + 0x30)
#define EHCICTRL_ENAINCRXALIGN			(0x1 << 29)
#define EHCICTRL_ENAINCR4			(0x1 << 28)
#define EHCICTRL_ENAINCR8			(0x1 << 27)
#define EHCICTRL_ENAINCR16			(0x1 << 26)
#define PHY_HOST_OHCICTRL	(EXYNOS5_USB2_PHY_HOST_CTRL0 + 0x34)
#define OHCICTRL_SUSPLGCY			(0x1 << 3)
#define PHY_OTG_SYS			(EXYNOS5_USB2_PHY_HOST_CTRL0 + 0x38)
#define OTG_SYS_PHYLINK_SW_RESET		(0x1 << 14)
#define OTG_SYS_LINK_SW_RST_UOTG		(0x1 << 13)
#define OTG_SYS_PHY0_SW_RST			(0x1 << 12)
#define OTG_SYS_REF_CLK_SEL(val)		((val&0x3) << 9)
#define OTG_SYS_REF_CLK_SEL_MASK		(0x3 << 9)
#define OTG_SYS_IP_PULLUP_UOTG			(0x1 << 8)
#define OTG_SYS_COMMON_ON			(0x1 << 7)
#define OTG_SYS_CLKSEL_SHIFT			(4)
#define OTG_SYS_CTRL0_FSEL_MASK			(0x7 << 4)
#define OTG_SYS_FORCE_SLEEP			(0x1 << 3)
#define OTG_SYS_OTGDISABLE			(0x1 << 2)
#define OTG_SYS_SIDDQ_UOTG			(0x1 << 1)
#define OTG_SYS_FORCE_SUSPEND			(0x1 << 0)

//CMU
#define CLK_GATE_IP_FSYS			0x10020944 //GAIA

// Likely not needed on 5410. This is a WRONG offset currently.
//GPIO
#define ETC6PUD					(0x134002A8) //(0x13400000 + 0x2A8)

//PMU
#define EXYNOS5_USBDEV_PHY_CONTROL		(0x10040000 + 0x0704)
#define EXYNOS5_USBDEV1_PHY_CONTROL		(0x10040000 + 0x0708) // EXYNOS5_USBHOST_PHY_CONTROL on 5250
#define EXYNOS5_USBHOST_PHY_CONTROL		(0x10040000 + 0x070C) // As 5410 isn't 5250, it should be this.

//SYSCTL
#define EXYNOS5_USB_CFG	(0x00100000 + 0x230)

//--------------------- for gaia ----------
#define UHOST_FIN		48000000
#define USBDEV_FIN		12000000

#define USBHOST_AHB_INCR16	0x2000000
#define USBHOST_AHB_INCR8	0x1000000
#define USBHOST_AHB_INCR4	0x0800000
#define USBHOST_AHB_INCRs	0x3800000
#define USBHOST_AHB_INCRx	0x3c00000
#define USBHOST_AHB_SINGLE	0x0000000

typedef enum
{
	REFCLK_XTAL = 0x0,  //XO : form Crystal
	REFCLK_OSC = 0x1,   //XO : OSC 1.8V Clock
	REFCLK_PLL = 0x2,   //PLL form CLKCORE
}USBPHY_REFCLK;

typedef enum
{
	FSEL_9_6M = 0x0,
	FSEL_10M = 0x1,
	FSEL_12M = 0x2,
	FSEL_19_2M = 0x3,
	FSEL_20M = 0x4,
	FSEL_24M = 0x5,
	FSEL_48M = 0x6,   //Reserved
	FSEL_50M = 0x7,
}USBPHY_REFSEL;

#define USBPHY_RETENABLE 1 //Retention Mode Enable == 1, Normal Operation mode must be 1.

void USBPHY_Ctr48MhzClk(UINT8 bEnable_48Mhz);
void usb_host_phy_off(void);

//-------------------------------------------------------
//OTG

enum USBPHY_CON_SFR
{
	rUPHY_USBCTRL0				= EXYNOS5_USB2_PHY_HOST_CTRL0+0x0000,
	rUPHY_USBTUNE0				= EXYNOS5_USB2_PHY_HOST_CTRL0+0x0004,
	rUPHY_HSICCTRL1				= EXYNOS5_USB2_PHY_HOST_CTRL0+0x0010,
	rUPHY_HSICTUNE1				= EXYNOS5_USB2_PHY_HOST_CTRL0+0x0014,
	rUPHY_HSICCTRL2				= EXYNOS5_USB2_PHY_HOST_CTRL0+0x0020,
	rUPHY_HSICTUNE2				= EXYNOS5_USB2_PHY_HOST_CTRL0+0x0024,

	rUPHY_EHCICTRL				= EXYNOS5_USB2_PHY_HOST_CTRL0+0x0030,
	rUPHY_OHCICTRL				= EXYNOS5_USB2_PHY_HOST_CTRL0+0x0034,

	rUPHY_USBOTG_SYS			= EXYNOS5_USB2_PHY_HOST_CTRL0+0x0038,
	rUPHY_USBOTG_TUNE			= EXYNOS5_USB2_PHY_HOST_CTRL0+0x0040,
};

enum usb_clk_type {
	USBOTG_CLK, USBHOST_CLK, USBDRD30_CLK
};

//-------------------------------------------------------
// Functions
//-------------------------------------------------------

void exynos5_usb_phy20_init(void);

#endif


