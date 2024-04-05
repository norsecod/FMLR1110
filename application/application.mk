USER_APP_C_DEFS = \

USER_APP_C_SOURCES = \
	application/main.c \
	extensions/drivers/cayennelpp/cayenne_lpp.c \
		extensions/drivers/cayennelpp/functions.c \


USER_APP_C_INCLUDES =  \
	-Iapplication \
	-extensions/drivers/cayennelpp\

USER_APP_ASM_SOURCES = \
