# Lbm Devboard Example

This project is derived from Semtech's LoRa Basic Modem.
It's ported to Miromico's Evaluation Kit (devboard).
The example code show basic funktionality of the development board.

This project can be compiled for different platforms and their corresponding FMLR modules

||SX1261|LR1110|SX128x|
|-|-|-|-|
|STM32L071|yes|yes|yes|
|STM32L451|yes|yes|yes|

For configuration change the Makefile in the root folder:

||SX1261|LR1110|SX128x|
|-|-|-|-|
|RADIO?=|sx1261|lr1110|sx128x|
|USE_GNSS?=|no|yes|no|
|REGION?=|EU868 or US915|EU868 or US915|comment out|