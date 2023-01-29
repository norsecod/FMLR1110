##############################################################################
# Definitions for the STM32 L451 board
##############################################################################


#-----------------------------------------------------------------------------
# Compilation flags
#-----------------------------------------------------------------------------

CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU_FLAGS ?= $(CPU) -mthumb $(FPU) $(FLOAT-ABI)
JLINKDEV ?= STM32L451RE

BOARD_C_DEFS =  \
	-DUSE_HAL_DRIVER \
	-DSTM32L451xx

BOARD_LDSCRIPT = bsp_l4/mcu_drivers/core/stm32l451xx_flash.ld

#-----------------------------------------------------------------------------
# Hardware-specific sources
#-----------------------------------------------------------------------------
BOARD_C_SOURCES = \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rng.c\
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c_ex.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_lptim.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rtc.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rtc_ex.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_spi.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_tim.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_tim_ex.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart_ex.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_wwdg.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_iwdg.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc_ex.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ex.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ramfunc.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_gpio.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_dma.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr_ex.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_cortex.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_adc.c \
	bsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_adc_ex.c\
	bsp_l4/smtc_modem_hal/smtc_modem_hal.c\
	bsp_l4/mcu_drivers/core/system_stm32l4xx.c\
	bsp_l4/smtc_hal/smtc_hal_adc.c\
	bsp_l4/smtc_hal/smtc_hal_flash.c\
	bsp_l4/smtc_hal/smtc_hal_gpio.c\
	bsp_l4/smtc_hal/smtc_hal_mcu.c\
	bsp_l4/smtc_hal/smtc_hal_rtc.c\
	bsp_l4/smtc_hal/smtc_hal_rng.c\
	bsp_l4/smtc_hal/smtc_hal_spi.c\
	bsp_l4/smtc_hal/smtc_hal_lp_timer.c\
	bsp_l4/smtc_hal/smtc_hal_trace.c\
	bsp_l4/smtc_hal/smtc_hal_uart.c\
	bsp_l4/smtc_hal/smtc_hal_watchdog.c\
	bsp_l4/smtc_hal/smtc_hal_i2c.c\
	extensions/drivers/common/timer.c\
	bsp_l4/common/settings.c

BOARD_ASM_SOURCES =  \
	bsp_l4/mcu_drivers/core/startup_stm32l451xx.s

BOARD_C_INCLUDES =  \
	-Ibsp_l4/mcu_drivers/cmsis/Core/Include\
	-Ibsp_l4/mcu_drivers/cmsis/Device/ST/STM32L4xx/Include\
	-Ibsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Inc\
	-Ibsp_l4/mcu_drivers/STM32L4xx_HAL_Driver/Inc/Legacy\
	-Ibsp_l4\
	-Ibsp_l4/littlefs\
	-Ibsp_l4/mcu_drivers/core\
	-Ibsp_l4/smtc_modem_hal\
	-Ibsp_l4/smtc_hal \
	-Ibsp_l4/common \
	-Iextensions/drivers/common \
