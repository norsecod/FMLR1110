##############################################################################
# Definitions for the SX128x tranceiver
##############################################################################
ifeq ($(RADIO),sx1261)
TARGET = sx1261
endif
ifeq ($(RADIO),sx1262)
TARGET = sx1262
endif
ifeq ($(RADIO),sx1268)
TARGET = sx1268
endif

#-----------------------------------------------------------------------------
# Common sources
#-----------------------------------------------------------------------------

RADIO_HAL_C_SOURCES += \
	$(BSP)/radio_hal/sx126x_hal.c \
	$(BSP)/radio_hal/ral_sx126x_bsp.c

#-----------------------------------------------------------------------------
# Includes
#-----------------------------------------------------------------------------
MODEM_C_INCLUDES =  \
	-I$(LORA_BASICS_MODEM)/smtc_modem_core/radio_drivers/sx126x_driver/src\
	-I$(LORA_BASICS_MODEM)/smtc_modem_core/smtc_ralf/src \
	-I$(LORA_BASICS_MODEM)/smtc_modem_core/smtc_ral/src

#-----------------------------------------------------------------------------
# Region
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Radio specific compilation flags
#-----------------------------------------------------------------------------
MODEM_C_DEFS += \
	-DSX126X 

ifeq ($(RADIO),sx1262)
MODEM_C_DEFS += \
    -DSX1262
endif

ifeq ($(RADIO),sx1268)
MODEM_C_DEFS += \
    -DSX1268
endif
