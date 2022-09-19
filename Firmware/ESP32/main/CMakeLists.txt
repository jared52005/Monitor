idf_component_register(
        SRCS 
        "main.c"
        "CanIf.c"
        "Passive_Iso15765.c"
        "Passive_Kline.c"
        "Passive_Vwtp20.c"
        "rtos_utils.c"
        "System_stats.c"
        "Task_CanReconstruct.c"
        "Task_Tcp_SocketCAN.c"
        "uart.c" 
        "wifi.c"
        INCLUDE_DIRS ".")
