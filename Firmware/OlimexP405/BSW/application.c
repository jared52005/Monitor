
//SLCAN parser copied from https://github.com/mintynet/esp32-slcan/tree/master
#include "SystemIf.h"
#include "UsbIf.h"
#include "CanIf.h"
#include "UartIf.h"
#include "Usb_Vcp_If.h"
#include "led.h"
#include "application.h"
#include "GpioIf.h"
#include "GptIf.h"

#include "CanDriver.h"

#include "string.h"
#include "stdbool.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "croutine.h"
#include "rtos_utils.h"
//******************************************************************************

#ifdef DEBUG_TRACE
#include "stdio.h"
#endif

char _cmdRxPartialBuffer[128];
char _cmd[2048];
bool working            = false;
bool timestamp          = false;
bool cr                 = false;
bool disp_cnt           = false;
can_bitrate can_speed   = CAN_BITRATE_500K;
uint32_t msg_cnt_in     = 0;
uint32_t msg_cnt_out    = 0;

static uint8_t hexval[17] = "0123456789ABCDEF";

void pars_slcancmd(char *buf, uint16_t bufSize);
void send_canmsg(char *buf, bool rtr, bool ext);

/**
 * @brief Common method which is running main loop for all existing devices
*/
void Application(void)
{
    uint16_t bufferRxPartialSize;
    uint16_t cmdSize;
    bool cmdReceived;
    int i;
    /* Initialize all configured peripherals */
    TM_USB_VCP_Init();
    led_init();

    //Default CAN settings
    Can_Mode_Set(CAN_ACTIVE);
    can_speed = CAN_BITRATE_500K;
    Can_Baudrate_Set(can_speed);

    // blink red LED for test
    GPIO_WritePin(LED_RED, GPIO_PIN_SET_);
    Wait_ms(100);
    GPIO_WritePin(LED_RED, GPIO_PIN_RESET_);
    Wait_ms(100);
    GPIO_WritePin(LED_RED, GPIO_PIN_SET_);
    Wait_ms(100);
    GPIO_WritePin(LED_RED, GPIO_PIN_RESET_);
	Wait_ms(100);
    GPIO_WritePin(LED_RED, GPIO_PIN_SET_); //Let red LED on

#ifdef DEBUG_TRACE
    printf("SLCAN - Boot finished\r\n");
#endif
    cmdSize = 0;
    for(;;)
    {
        taskYIELD();
        //Get partial command buffer from USB
		bufferRxPartialSize = TM_USB_VCP_Gets(_cmdRxPartialBuffer, 128);
        if(bufferRxPartialSize != 0)
        {
            //Copy partial buffer into overal buffer
            memcpy((_cmd + cmdSize), _cmdRxPartialBuffer, bufferRxPartialSize);
            cmdSize += bufferRxPartialSize;
            //Check if we have received all characters
            cmdReceived = false;
            for(i = 0; i<cmdSize; i++)
            {
                if(_cmd[i] == '\n')
                {
                    cmdReceived = true;
                    cmdSize = i;
                }
            }
            if(cmdReceived == true)
            {
                //Loopback
                //TM_USB_VCP_Puts(cmd);
                pars_slcancmd(_cmd, cmdSize);
                cmdSize = 0;
            }
        }
        else
        {
            //Deque RX queue and send data over VCP
        }
    }
}

//----------------------------------------------------------------

void slcan_ack()
{
    printf("\r");
} // slcan_ack()

//----------------------------------------------------------------

void slcan_nack()
{
    printf("\a");
} // slcan_nack()

//----------------------------------------------------------------

void pars_slcancmd(char *buf, uint16_t bufSize)
{                           // LAWICEL PROTOCOL
  switch (buf[0]) 
  {
    case 'O':               // OPEN CAN
      working=true;
      CanDriver_Start();
      msg_cnt_in = 0;
      msg_cnt_out = 0;
      slcan_ack();
      break;
    case 'C':               // CLOSE CAN
      working=false;
      CanDriver_Stop();
      slcan_ack();
      break;
    case 't':               // SEND STD FRAME - i.e. t12323E00 = 0x123 [3E 00]
      send_canmsg(buf,false,false);
      slcan_ack();
      break;
    case 'T':               // SEND EXT FRAME
      send_canmsg(buf,false,true);
      slcan_ack();
      break;
    case 'r':               // SEND STD RTR FRAME
      send_canmsg(buf,true,false);
      slcan_ack();
      break;
    case 'R':               // SEND EXT RTR FRAME
      send_canmsg(buf,true,true);
      slcan_ack();
      break;
    case 'Z':               // ENABLE TIMESTAMPS
      switch (buf[1]) {
        case '0':           // TIMESTAMP OFF  
          timestamp = false;
          slcan_ack();
          break;
        case '1':           // TIMESTAMP ON
          timestamp = true;
          slcan_ack();
          break;
        default:
          slcan_nack();
          break;
      }
      break;
    case 'M':               ///set ACCEPTANCE CODE ACn REG
      slcan_ack();
      break;
    case 'm':               // set ACCEPTANCE CODE AMn REG
      slcan_ack();
      break;
    case 's':               // CUSTOM CAN bit-rate
      slcan_nack();
      break;
    case 'S':               // CAN bit-rate
      if (working) 
      {
        slcan_nack();
        break;
      }
      switch (buf[1]) 
      {
        case '0':           // 10k  
          can_speed = CAN_BITRATE_10K;
          Can_Baudrate_Set(can_speed);
          slcan_nack();
          break;
        case '1':           // 20k
          can_speed = CAN_BITRATE_20K;
          Can_Baudrate_Set(can_speed);
          slcan_nack();
          break;
        case '2':           // 50k
          can_speed = CAN_BITRATE_50K;
          Can_Baudrate_Set(can_speed);
          slcan_nack();
          break;
        case '3':           // 100k
          can_speed = CAN_BITRATE_100K;
          Can_Baudrate_Set(can_speed);
          slcan_ack();
          break;
        case '4':           // 125k
          can_speed = CAN_BITRATE_125K;
          Can_Baudrate_Set(can_speed);
          slcan_ack();
          break;
        case '5':           // 250k
          can_speed = CAN_BITRATE_250K;
          Can_Baudrate_Set(can_speed);
          slcan_ack();
          break;
        case '6':           // 500k
          can_speed = CAN_BITRATE_500K;
          Can_Baudrate_Set(can_speed);
          slcan_ack();
          break;
        case '7':           // 800k
          slcan_nack();
          break;
        case '8':           // 1000k
          can_speed = CAN_BITRATE_1000K;
	      Can_Baudrate_Set(can_speed);
          slcan_ack();
          break;
        default:
          slcan_nack();
          break;
      }
      break;
    case 'F':               // STATUS FLAGS
      printf("F00");
      slcan_ack();
      break;
    case 'V':               // VERSION NUMBER
      printf("V1234");
      slcan_ack();
      break;
    case 'N':               // SERIAL NUMBER
      printf("N2208");
      slcan_ack();
      break;
    case 'h':               // (NOT SPEC) HELP SERIAL
      printf("\n");
      printf("mintynet.com - slcan stm32\n");
      printf("\n");
      printf("O\t=\tStart slcan\n");
      printf("C\t=\tStop slcan\n");
      printf("t\t=\tSend std frame\n");
      printf("r\t=\tSend std rtr frame\n");
      printf("T\t=\tSend ext frame\n");
      printf("R\t=\tSend ext rtr frame\n");
      printf("Z0\t=\tTimestamp Off\n");
      printf("Z1\t=\tTimestamp On\n");
      printf("snn\t=\tSpeed 0xnnk N/A\n");
      printf("S0\t=\tSpeed 10k N/A\n");
      printf("S1\t=\tSpeed 20k N/A\n");
      printf("S2\t=\tSpeed 50k N/A\n");
      printf("S3\t=\tSpeed 100k\n");
      printf("S4\t=\tSpeed 125k\n");
      printf("S5\t=\tSpeed 250k\n");
      printf("S6\t=\tSpeed 500k\n");
      printf("S7\t=\tSpeed 800k N/A\n");
      printf("S8\t=\tSpeed 1000k\n");
      printf("f\t=\tCAN ID filter\n");
      printf("F\t=\tFlags N/A\n");
      printf("N\t=\tSerial No\n");
      printf("V\t=\tVersion\n");
      printf("-----NOT SPEC-----\n");
      printf("h\t=\tHelp\n");
      printf("CAN_SPEED:\t");
      switch(can_speed) 
      {
        case CAN_BITRATE_100K:
          printf("100");
          break;
        case CAN_BITRATE_125K:
          printf("125");
          break;
        case CAN_BITRATE_250K:
          printf("250");
          break;
        case CAN_BITRATE_500K:
          printf("500");
          break;
        case CAN_BITRATE_1000K:
          printf("1000");
          break;
        default:
          break;
      }
      printf("kbps");
      if (timestamp) 
      {
        printf("\tT");
      }
      if (working) 
      {
        printf("\tON");
      } else {
        printf("\tOFF");
      }
      slcan_nack();
      break;
    case 'D':
        //Setup bootflag and request reset
        *((uint32_t*)0x20000000) = 0x44465542; //"DFUB"
        System_Reset();
    default:
      slcan_nack();
      break;
  }
} // pars_slcancmd()

//----------------------------------------------------------------
/*
void transfer_tty2can()
{
  int ser_length;
  static char cmdbuf[32];
  static int cmdidx = 0;
  if (bluetooth) {
    if ((ser_length = SerialBT.available()) > 0) {
      for (int i = 0; i < ser_length; i++) {
        char val = SerialBT.read();
        cmdbuf[cmdidx++] = val;
        if (cmdidx == 32)
        {
          slcan_nack();
          cmdidx = 0;
        } else if (val == '\r')
        {
          cmdbuf[cmdidx] = '\0';
          pars_slcancmd(cmdbuf);
          cmdidx = 0;
        }
      }
    }
  } else {
    if ((ser_length = Serial.available()) > 0) {
      for (int i = 0; i < ser_length; i++) {
        char val = Serial.read();
        cmdbuf[cmdidx++] = val;
        if (cmdidx == 32)
        {
          slcan_nack();
          cmdidx = 0;
        } else if (val == '\r')
        {
          cmdbuf[cmdidx] = '\0';
          pars_slcancmd(cmdbuf);
          cmdidx = 0;
        }
      }
    }
  }
} // transfer_tty2can()
*/
//----------------------------------------------------------------

/*
void transfer_can2tty()
{
  CAN_frame_t rx_frame;
  String command = "";
  long time_now = 0;
  //receive next CAN frame from queue
  if(xQueueReceive(CAN_cfg.rx_queue,&rx_frame, 3*portTICK_PERIOD_MS)==pdTRUE) {
    //do stuff!
    if(working) {
      if(rx_frame.FIR.B.FF==CAN_frame_ext) {
        if (rx_frame.FIR.B.RTR==CAN_RTR) {
          command = command + "R";
        } else {
          command = command + "T";
        }
        command = command + char(hexval[ (rx_frame.MsgID>>28)&1]);
        command = command + char(hexval[ (rx_frame.MsgID>>24)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>20)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>16)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>12)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>8)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>4)&15]);
        command = command + char(hexval[ rx_frame.MsgID&15]);
        command = command + char(hexval[ rx_frame.FIR.B.DLC ]);
      } else {
        if (rx_frame.FIR.B.RTR==CAN_RTR) {
          command = command + "r";
        } else {
          command = command + "t";
        }
        command = command + char(hexval[ (rx_frame.MsgID>>8)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>4)&15]);
        command = command + char(hexval[ rx_frame.MsgID&15]);
        command = command + char(hexval[ rx_frame.FIR.B.DLC ]);
      }
      for(int i = 0; i < rx_frame.FIR.B.DLC; i++){
        command = command + char(hexval[ rx_frame.data.u8[i]>>4 ]);
        command = command + char(hexval[ rx_frame.data.u8[i]&15 ]);
        //printf("%c\t", (char)rx_frame.data.u8[i]);
      }
    if (timestamp) {
      time_now = millis() % 60000;
      command = command + char(hexval[ (time_now>>12)&15 ]);
      command = command + char(hexval[ (time_now>>8)&15 ]);
      command = command + char(hexval[ (time_now>>4)&15 ]);
      command = command + char(hexval[ time_now&15 ]);
    }
    command = command + '\r';
    if (bluetooth) SerialBT.print(command);
    else printf(command);
    if (cr) printfln("");
    }
    msg_cnt_in++;
  }
} // transfer_can2tty()
*/

//----------------------------------------------------------------

void send_canmsg(char *buf, bool rtr, bool ext) 
{
    int i;
    uint32_t msg_id = 0;
    uint32_t msg_ide = 0;
    uint32_t msg_len = 0;
    uint32_t candata = 0;
    CanMessage tx_frame;
    if (working) 
    {
        if (rtr) 
        {
            if (ext) 
            {
                sscanf(&buf[1], "%04x%04x", &msg_ide, &msg_id);
                tx_frame.RTR = CAN_TYPE_REMOTE;
                tx_frame.ID_Type = CAN_ID_TYPE_EXT;
            }
            else 
            {
                sscanf(&buf[1], "%03x", &msg_id);
                tx_frame.RTR = CAN_TYPE_REMOTE;
                tx_frame.ID_Type = CAN_ID_TYPE_STD;
            }
        }
        else 
        {
            if (ext) 
            {
                sscanf(&buf[1], "%04x%04x", &msg_ide, &msg_id);

                tx_frame.RTR = CAN_TYPE_DATA;
                tx_frame.ID_Type = CAN_ID_TYPE_EXT;
            }
            else 
            {
                sscanf(&buf[1], "%03x", &msg_id);
                tx_frame.RTR = CAN_TYPE_DATA;
                tx_frame.ID_Type = CAN_ID_TYPE_STD;
            }
        }
        tx_frame.Id = (msg_ide << 16) + msg_id;
        if (ext) 
        {
            sscanf(&buf[9], "%01x", &msg_len);
        }
        else 
        {
            sscanf(&buf[4], "%01x", &msg_len);
        }
        tx_frame.Dlc = msg_len;        
        if (ext) 
        {
            for (i = 0; i < msg_len; i++) 
            {
                sscanf(&buf[10 + (i*2)], "%02x", &candata);
                tx_frame.Frame[i] = (uint8_t)candata;
            }
        }
        else 
        {
            for (i = 0; i < msg_len; i++) 
            {
                sscanf(&buf[5 + (i*2)], "%02x", &candata);
                tx_frame.Frame[i] = (uint8_t)candata;
            }
        }
        CanDriver_Transmit(tx_frame);
        msg_cnt_out++;
    }
} // send_canmsg()

//----------------------------------------------------------------
