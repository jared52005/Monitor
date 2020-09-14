# STM3240G-EVAL
Base code for TCP Server code was copied from [https://github.com/STMicroelectronics/STM32CubeF4](https://github.com/STMicroelectronics/STM32CubeF4)

TCP Server is then extended with:
 * CAN driver. Implemented on `CAN2 RX @ PB5`
 * UART (KLINE) drivers. Implemented on `UART3 RX @ PB11`
 * Passive protocols `ISO14230`, `KW1281`, `ISO15765`, `VWTP2.0`
 * Wireshark RAW protocol TCP packet formatter
 * Wireshark SocketCAN protocol TCP packet formatter
 * Device statistics on LCD

## How it looks
![STM3240G Example](STM3240G.jpg)
