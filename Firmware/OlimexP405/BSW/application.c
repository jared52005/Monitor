
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
#include "PwmIf.h"

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
static char command[32];

void pars_slcancmd(char *buf, uint16_t bufSize);
void send_canmsg(char *buf, bool rtr, bool ext);
void transfer_can2tty(void);
void slcan_mask(char *buf);
void slcan_filter(char *buf);
void slcan_pwm(char *buf, int pwmChannel);

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
    Can_Mode_Set(CAN_PASSIVE);
    can_speed = CAN_BITRATE_500K;
    Can_Baudrate_Set(can_speed);
    Can_Mask(0);
    Can_Filter(0);

    // Set power pins (Optional)
    GPIO_InitPin(GPIO_VBAT);
    GPIO_InitPin(GPIO_IGN);
    GPIO_InitPin(GPIO_1);
    GPIO_InitPin(GPIO_2);
    GPIO_InitPin(GPIO_3);
    GPIO_InitPin(GPIO_4);

    GPIO_WritePin(GPIO_VBAT, GPIO_PIN_SET_);
    GPIO_WritePin(GPIO_IGN, GPIO_PIN_SET_);
    GPIO_WritePin(GPIO_1, GPIO_PIN_RESET_);
    GPIO_WritePin(GPIO_2, GPIO_PIN_RESET_);
    GPIO_WritePin(GPIO_3, GPIO_PIN_RESET_);
    GPIO_WritePin(GPIO_4, GPIO_PIN_RESET_);

    // blink red LED for test
    GPIO_WritePin(GPIO_LED_RED, GPIO_PIN_SET_);
    Wait_ms(100);
    GPIO_WritePin(GPIO_LED_RED, GPIO_PIN_RESET_);
    Wait_ms(100);
    GPIO_WritePin(GPIO_LED_RED, GPIO_PIN_SET_);
    Wait_ms(100);
    GPIO_WritePin(GPIO_LED_RED, GPIO_PIN_RESET_);
	  Wait_ms(100);
    GPIO_WritePin(GPIO_LED_RED, GPIO_PIN_SET_); //Let red LED on

#ifdef DEBUG_TRACE
    //printf("SLCAN - Boot finished\r\n");
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
            transfer_can2tty();
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
      slcan_filter(buf);
      slcan_ack();
      break;
    case 'm':               // set ACCEPTANCE MASK AMn REG
      slcan_mask(buf);
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
      printf("Gn\t=\tSet GPIO n\n");
      printf("gn\t=\tReset GPIO n\n");
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
      slcan_ack();
      break;
    case 'D':
        //Setup bootflag and request reset
        *((uint32_t*)0x20000000) = 0x44465542; //"DFUB"
        System_Reset();
        break;
    case 'G': //Set pin N
    {
      switch (buf[1]) 
      {
        case '0':
          GPIO_WritePin(GPIO_1, GPIO_PIN_SET_);
          slcan_ack();
          break;
        case '1':
          GPIO_WritePin(GPIO_2, GPIO_PIN_SET_);
          slcan_ack();
          break;
        case '2':
          GPIO_WritePin(GPIO_3, GPIO_PIN_SET_);
          slcan_ack();
          break;
        case '3':
          GPIO_WritePin(GPIO_4, GPIO_PIN_SET_);
          slcan_ack();
          break;
        default:
          slcan_nack();
          break;
      }
      break;
    }
    case 'g': //Reset pin N
    {
      switch (buf[1]) 
      {
        case '0':
          GPIO_WritePin(GPIO_1, GPIO_PIN_RESET_);
          slcan_ack();
          break;
        case '1':
          GPIO_WritePin(GPIO_2, GPIO_PIN_RESET_);
          slcan_ack();
          break;
        case '2':
          GPIO_WritePin(GPIO_3, GPIO_PIN_RESET_);
          slcan_ack();
          break;
        case '3':
          GPIO_WritePin(GPIO_4, GPIO_PIN_RESET_);
          slcan_ack();
          break;
        default:
          slcan_nack();
          break;
      }
      break;
    }
    case 'P': //PWM activation - P1f00005555d44 = PWM Channel 1, 5555Hz, 44% DC
    {
      switch (buf[1]) 
      {
        case '1':
          slcan_pwm(buf, 0);
          slcan_ack();
          break;
        default:
          slcan_nack();
          break;
      }
      break;
    }
    default:
      slcan_nack();
      break;
  }
} // pars_slcancmd()

//----------------------------------------------------------------

void transfer_can2tty(void)
{
  int i;
  CanMessage rx_frame;
  int cOff;
  long time_now = 0;
  //receive next CAN frame from queue
  if(CanDriver_Receive(&rx_frame)==true) 
  {
    //do stuff!
    if(working) 
    {
        cOff = 0;
        if(rx_frame.ID_Type==CAN_ID_TYPE_EXT) 
        {
            if (rx_frame.RTR==CAN_TYPE_REMOTE) 
            {
                command[cOff++] = 'R';
            }
            else 
            {
                command[cOff++] = 'T';
            }   
            command[cOff++] = hexval[ (rx_frame.Id>>28)&1];
            command[cOff++] = hexval[ (rx_frame.Id>>24)&15];
            command[cOff++] = hexval[ (rx_frame.Id>>20)&15];
            command[cOff++] = hexval[ (rx_frame.Id>>16)&15];
            command[cOff++] = hexval[ (rx_frame.Id>>12)&15];
            command[cOff++] = hexval[ (rx_frame.Id>>8)&15];
            command[cOff++] = hexval[ (rx_frame.Id>>4)&15];
            command[cOff++] = hexval[ rx_frame.Id&15];
            command[cOff++] = hexval[ rx_frame.Dlc];
        }
        else 
        {
            if (rx_frame.RTR==CAN_TYPE_REMOTE) 
            {
            command[cOff++] = 'r';
            } 
            else 
            {
                command[cOff++] = 't';
            }
            command[cOff++] = hexval[ (rx_frame.Id>>8)&15];
            command[cOff++] = hexval[ (rx_frame.Id>>4)&15];
            command[cOff++] = hexval[ rx_frame.Id&15];
            command[cOff++] = hexval[ rx_frame.Dlc ];
        }
        for(i = 0; i < rx_frame.Dlc; i++)
        {
            command[cOff++] = hexval[ rx_frame.Frame[i]>>4 ];
            command[cOff++] = hexval[ rx_frame.Frame[i]&15 ];
        }
        if (timestamp) 
        {
            time_now =  GetTime_ms() % 60000;
            command[cOff++] = hexval[ (time_now>>12)&15 ];
            command[cOff++] = hexval[ (time_now>>8)&15 ];
            command[cOff++] = hexval[ (time_now>>4)&15 ];
            command[cOff++] = hexval[ time_now&15 ];
        }
        command[cOff++] = '\r';
        command[cOff++] = '\0';
        printf(command);
        msg_cnt_in++;
    }
  }
} // transfer_can2tty()

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

void slcan_mask(char *buf)
{
    uint32_t mask;
    uint32_t maskM;
    uint32_t maskL;
    sscanf(&buf[1], "%04x%04x", &maskM, &maskL);
    mask = (maskM << 16) + maskL;
    //printf("Set Mask to %x", mask);
    Can_Mask(mask);
}
void slcan_filter(char *buf)
{
    uint32_t filter;
    uint32_t filterM;
    uint32_t filterL;
    sscanf(&buf[1], "%04x%04x", &filterM, &filterL);
    filter = (filterM << 16) + filterL;
    //printf("Set Filter to %x", filter);
    Can_Filter(filter);
}

void slcan_pwm(char *buf, int pwmChannel)
{
  //PWM activation - P1f00005555d44 = PWM Channel 1, 5555Hz, 44% DC
  uint32_t freq;
  uint32_t dc;
  uint32_t valM;
  uint32_t valL;
  sscanf(&buf[3], "%04d%04d", &valM, &valL);
  freq = (valM * 10000) + valL;
  sscanf(&buf[12], "%02d", &valM);
  dc = valM * 10;
  printf("Set PWM ch%d %dHz %d DC", pwmChannel, freq, dc);
  Pwm_Enable(PWM_CHANNEL_01, freq, dc, dc);
}
//----------------------------------------------------------------
