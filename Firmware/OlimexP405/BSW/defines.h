#ifdef STM32F40GEVAL
#define BOARD_TYPE_STRING " STM32F40G-Eval "
#endif

#ifdef OLIMEXP405_Cv10
#define BOARD_TYPE_STRING " Olimex P405 Cv10 "
#endif

#ifdef BOARD_STM32F405_Cv20
#define BOARD_TYPE_STRING " STM32F405 Cv20 "
#endif

#ifndef BOARD_TYPE_STRING
#error "Define type of board to build this firmware for"
#endif

