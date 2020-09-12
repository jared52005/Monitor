# STM3240G-EVAL
TCP Server code was copied from [https://github.com/STMicroelectronics/STM32CubeF4](https://github.com/STMicroelectronics/STM32CubeF4)

TCP Server is then extended with:
 * CAN driver. Implemented on `CAN2 RX; PB5`
 * **[Missing]** UART (KLINE) drivers. Implemented on `PB11`
 * **[Missing]** Passive protocols `ISO14230`, `KW1281`, `ISO15765`, `VWTP2.0`
 * Wireshark TCP packet formatter
 * CAN statistics on LCD

## How it looks
![STM3240G Example](/STM3240G.jpg)
