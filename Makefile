
PROJECT=test
RADIO ?= lr1110
#RADIO ?= sx1261
#RADIO ?= sx128x

BSP=bsp_l0
BOOTLOADER=bootloader_l0

BYPASS=no

PERF_TEST=no

# CRYPTO Management
CRYPTO ?= SOFT

# Compile for debugging
DEBUG ?= no

# Compile with coverage analysis support
COVERAGE ?= no

# Use multithreaded build (make -j)
MULTITHREAD ?= yes

# Print each object file size
SIZE ?= no

# Save memory usage to log file
LOG_MEM ?= no

# Flash board present on DRIVE
DRIVE ?= nc

# Tranceiver
USE_LR11XX_CRC_SPI ?= no

# Application
MODEM_APP ?= nc

#TRACE
MODEM_TRACE ?= yes
MODEM_DEEP_TRACE ?= yes
MODEM_HAL_TRACE ?= yes
APP_TRACE ?= yes

# GNSS
USE_GNSS ?= yes

REGION ?= EU_868

-include lbm/makefiles/printing.mk
-include extensions/makefiles/common.mk
-include extensions/makefiles/tools.mk
