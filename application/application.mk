USER_APP_C_DEFS = \

USER_APP_C_SOURCES = \
	application/main.c \
	extensions/drivers/led/led.c \
	extensions/drivers/button/button.c \
	extensions/drivers/sht3x/sht3x.c \
	extensions/drivers/lis2dw/lis2dw.c \

USER_APP_C_INCLUDES =  \
	-Iapplication \
	-Iextensions/drivers/led\
	-Iextensions/drivers/button\
	-Iextensions/drivers/sht3x\
	-Iextensions/drivers/lis2dw\

USER_APP_ASM_SOURCES = \
