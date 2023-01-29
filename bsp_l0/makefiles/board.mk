##############################################################################
# Definitions for the STM32 L071 board
##############################################################################


#-----------------------------------------------------------------------------
# Compilation flags
#-----------------------------------------------------------------------------
MCU_FLAGS ?= -mcpu=cortex-m0 -mthumb

BOARD_C_DEFS =  \
	-DUSE_HAL_DRIVER \
	-DSTM32L071xx

BOARD_LDSCRIPT = bsp_l0/mcu_drivers/core/stm32l071rz_flash.ld

JLINKDEV ?= STM32L071RZ

#-----------------------------------------------------------------------------
# Hardware-specific sources
#-----------------------------------------------------------------------------
BOARD_C_SOURCES = \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_i2c.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_i2c_ex.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_lptim.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_rtc.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_rtc_ex.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_spi.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_tim.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_tim_ex.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_uart.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_uart_ex.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_wwdg.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_iwdg.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_rcc.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_rcc_ex.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_flash.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_flash_ex.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_flash_ramfunc.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_gpio.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_dma.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_pwr.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_pwr_ex.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_cortex.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_adc.c \
	bsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_adc_ex.c\
	bsp_l0/smtc_modem_hal/smtc_modem_hal.c\
	bsp_l0/mcu_drivers/core/system_stm32l0xx.c\
	bsp_l0/smtc_hal/smtc_hal_adc.c\
	bsp_l0/smtc_hal/smtc_hal_eeprom.c\
	bsp_l0/smtc_hal/smtc_hal_gpio.c\
	bsp_l0/smtc_hal/smtc_hal_mcu.c\
	bsp_l0/smtc_hal/smtc_hal_rtc.c\
	bsp_l0/smtc_hal/smtc_hal_spi.c\
	bsp_l0/smtc_hal/smtc_hal_lp_timer.c\
	bsp_l0/smtc_hal/smtc_hal_trace.c\
	bsp_l0/smtc_hal/smtc_hal_uart.c\
	bsp_l0/smtc_hal/smtc_hal_watchdog.c\
	bsp_l0/smtc_hal/smtc_hal_rng.c\
	bsp_l0/smtc_hal/smtc_hal_i2c.c\
	extensions/drivers/common/timer.c\
	bsp_l0/common/settings.c

BOARD_ASM_SOURCES =  \
	bsp_l0/mcu_drivers/core/startup_stm32l071xx.s

BOARD_C_INCLUDES =  \
	-Ibsp_l0/mcu_drivers/cmsis/Core/Include\
	-Ibsp_l0/mcu_drivers/cmsis/Device/ST/STM32L0xx/Include\
	-Ibsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Inc\
	-Ibsp_l0/mcu_drivers/STM32L0xx_HAL_Driver/Inc/Legacy\
	-Ibsp_l0\
	-Ibsp_l0/littlefs\
	-Ibsp_l0/mcu_drivers/core\
	-Ibsp_l0/smtc_modem_hal\
	-Ibsp_l0/smtc_hal\
	-Ibsp_l0/common\
	-Iextensions/drivers/common

