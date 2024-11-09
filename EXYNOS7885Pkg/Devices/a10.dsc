[Defines]
  PLATFORM_NAME                  = EXYNOS7885Pkg
  PLATFORM_GUID                  = 28f1a3bf-193a-47e3-a7b9-5a435eaab2ee
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010019
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = EXYNOS7885Pkg/EXYNOS7885Pkg.fdf

!include EXYNOS7885Pkg/EXYNOS7885Pkg.dsc

[PcdsFixedAtBuild.common]
  # System Memory (2 GB)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x80000000
  gEmbeddedTokenSpaceGuid.PcdPrePiStackBase|0x50201000
  gEmbeddedTokenSpaceGuid.PcdPrePiStackSize|0x00040000      # 256K stack
  gEXYNOS7885PkgTokenSpaceGuid.PcdUefiMemPoolBase|0x50241000         # DXE Heap base address
  gEXYNOS7885PkgTokenSpaceGuid.PcdUefiMemPoolSize|0x07000000         # UefiMemorySize, DXE heap size
  gArmTokenSpaceGuid.PcdCpuVectorBaseAddress|0x50200000

  # Framebuffer (1080x1920)
  gEXYNOS7885PkgTokenSpaceGuid.PcdMipiFrameBufferAddress|0x67000000
  gEXYNOS7885PkgTokenSpaceGuid.PcdMipiFrameBufferWidth|1080
  gEXYNOS7885PkgTokenSpaceGuid.PcdMipiFrameBufferHeight|1920
  gEXYNOS7885PkgTokenSpaceGuid.PcdMipiFrameBufferVisibleWidth|1080
  gEXYNOS7885PkgTokenSpaceGuid.PcdMipiFrameBufferVisibleHeight|1920

  # Timer
  gEXYNOS7885PkgTokenSpaceGuid.PcdPWMTimerBase|0x12DD0000

  # eMMC PCDs
  gEXYNOS7885PkgTokenSpaceGuid.PcdEmmcDMABufferBase|0x40300000
  gEXYNOS7885PkgTokenSpaceGuid.PcdCmuBase|0x10010000
  gEXYNOS7885PkgTokenSpaceGuid.PcdSdMmcCH0Base|0x12200000
  gEXYNOS7885PkgTokenSpaceGuid.PcdSdMmcBase|0x12220000

  # GPIO
  gEXYNOS7885PkgTokenSpaceGuid.PcdGpioPart1Base|0x13400000
  gEXYNOS7885PkgTokenSpaceGuid.PcdGpioPart2Base|0x14000000
  gEXYNOS7885PkgTokenSpaceGuid.PcdGpioPart3Base|0x10D10000
  gEXYNOS7885PkgTokenSpaceGuid.PcdGpioPart4Base|0x03860000
