/*******************************************************************************
 * @brief   Implementation of sending of Socket CAN packets into Wireshark
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 
#include <stdio.h>
#include "System_stats.h"
#include "Task_Tcp_Wireshark_SocketCAN.h"
#include "lwip/opt.h"
#include "string.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

// -- Private definitions
#define TCPECHO_THREAD_PRIO  ( tskIDLE_PRIORITY + 4 )
#define TCP_CAN_BUFFER_ITEMS 255

// -- Private Variables ---------------------------------
static u8_t fileHeader[] = 
{
  0xD4, 0xC3, 0xB2, 0xA1, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0x00, 0x00, 0xE3, 0x00, 0x00, 0x00
};
static u8_t packetHeader[] = 
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00
};
static u8_t packetBody[16];

static struct CanMessage tcp_canMessageFifo[TCP_CAN_BUFFER_ITEMS]; //FIFO buffer with CAN messages to send to Wireshark
static int tcp_canFifo_readPtr = 0;   //Pointer where we are starting with reading
static int tcp_canFifo_writePtr = 0;  //Pointer where we are starting with writing
static bool tcp_canFifo_Overflow = false; //Overflow flag

static void tcpswcan_prepare_header(CanMessage cmsg, u8_t* array)
{
  /* 00-00-00-00 00-00-00-00-10-00-00-00-10-00-00-00
   Where:
    00-00-00-00 = Time Stamp seconds.
    00-00-00-00 = Time Stamp micro seconds
    10-00-00-00 = Size of packet saved in a file = 16 bytes of body
    10-00-00-00 = Actual size of packet = 16 bytes of body
  */
  u32_t timestamp_seconds;
  u32_t timestamp_microseconds;

  //Prepare timestamp
  timestamp_seconds = (uint32_t)(cmsg.Timestamp / 1000); //Only second part (I know there should be Unix time, but I am too lazy to get RTC or NTP working)
  timestamp_microseconds = (uint32_t)(cmsg.Timestamp % 1000); //Only remainder from seconds
  timestamp_microseconds = timestamp_microseconds * 1000; //Convert milisecond to microseconds
  //Store timestamp
  *(u32_t*)array = timestamp_seconds;
  *(u32_t*)(array + 4) = timestamp_microseconds;
}

static void tcpswcan_prepare_body(CanMessage cmsg, u8_t* array)
{
  array[0] = (u8_t)(cmsg.Id >> 24);
  array[1] = (u8_t)(cmsg.Id >> 16);
  array[2] = (u8_t)(cmsg.Id >> 8);
  array[3] = (u8_t)cmsg.Id;
  //DLC
  array[4] = cmsg.Dlc;
  //Data
  memcpy(&array[8], cmsg.Frame, cmsg.Dlc);
}

static void tcpswcan_fifo_reset()
{
	tcp_canFifo_readPtr = 0;
	tcp_canFifo_writePtr = 0;
	tcp_canFifo_Overflow = false;
}

static int tcpwscan_fifo_count()
{
  //Check if buffer has overflown
	if (tcp_canFifo_Overflow == true)
	{
    printf("TCP CAN buffer overflow");
		tcpswcan_fifo_reset();
		return 0;
	}
  //this setup is being used as a ring buffer
	if(tcp_canFifo_readPtr > tcp_canFifo_writePtr)
	{
		//If read data has higher index than write data
		//this means that write data index has jumped on start
		return TCP_CAN_BUFFER_ITEMS - tcp_canFifo_readPtr + tcp_canFifo_writePtr;
	}
	else
	{
		return tcp_canFifo_writePtr - tcp_canFifo_readPtr;
	}
}

/*-----------------------------------------------------------------------------------*/
static void tcpwscan_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err, accept_err;
  CanMessage cmsg;
  
  LWIP_UNUSED_ARG(arg);

  /* Create a new connection identifier. */
  conn = netconn_new(NETCONN_TCP);
  
  if (conn!=NULL)
  {  
    /* Bind connection to 19001. */
    err = netconn_bind(conn, NULL, 19001);
    
    if (err == ERR_OK)
    {
      /* Tell connection to go into listening mode. */
      netconn_listen(conn);
    
      while (1) 
      {
        /* Grab new connection. */
        accept_err = netconn_accept(conn, &newconn);

        /* Process the new connection. */
        if (accept_err == ERR_OK) 
        {
          //Erase all data from SWCAN
          tcpswcan_fifo_reset();
          //Show on LCD that we have connection
          Stats_TCP_WS_SocketCAN_State_Set(1);
					printf("Connection established\n");

          //Write file header to init Wireshark
          netconn_write(newconn, fileHeader, 24, NETCONN_COPY);

					//Write CAN packets into TCP stream in endless loop
          while (netconn_err(newconn) == ERR_OK) 
          {
            if(tcpwscan_fifo_count() > 0)
            {
              cmsg = tcp_canMessageFifo[tcp_canFifo_readPtr];
              //Write packet header
              tcpswcan_prepare_header(cmsg, packetHeader);
              netconn_write(newconn, packetHeader, 16, NETCONN_COPY);

              //Write packet data
              tcpswcan_prepare_body(cmsg, packetBody);
              netconn_write(newconn, packetBody, 16, NETCONN_COPY);

              //Move to next packet
              tcp_canFifo_readPtr++;
              if (tcp_canFifo_readPtr >= TCP_CAN_BUFFER_ITEMS)
              {
                tcp_canFifo_readPtr = 0;
              }
              //Send all packets in buffer on TCP
              continue;
            }
            //Mandatory. Give RTOS chance to yield tasks. taskYIELD crashes whole RTOS from some reason
            osDelay(10); 
          }
					printf("Connection closed\n");
        
          /* Close connection and discard connection identifier. */
          netconn_close(newconn);
          netconn_delete(newconn);
          Stats_TCP_WS_SocketCAN_State_Set(0);
        }
      }
    }
    else
    {
      netconn_delete(newconn);
    }
  }
}
/*-----------------------------------------------------------------------------------*/

void Task_Tcp_Wireshark_SocketCAN_Init(void)
{
  sys_thread_new("tcpwscan_thread", tcpwscan_thread, NULL, DEFAULT_THREAD_STACKSIZE, TCPECHO_THREAD_PRIO);
}

void Task_Tcp_Wireshark_SocketCAN_AddNewCanMessage(CanMessage cmsg)
{
	int i;
  //Write down data into ring buffer for sending
	tcp_canMessageFifo[tcp_canFifo_writePtr].Dlc = cmsg.Dlc;
	tcp_canMessageFifo[tcp_canFifo_writePtr].Id = cmsg.Id;
	tcp_canMessageFifo[tcp_canFifo_writePtr].Timestamp = cmsg.Timestamp;
	
	for (i = 0; i < cmsg.Dlc; i++)
	{
		tcp_canMessageFifo[tcp_canFifo_writePtr].Frame[i] = cmsg.Frame[i];
	}

	//Move write pointer, if we are on the end of ring buffer, reset pointer
	tcp_canFifo_writePtr++;
	if (tcp_canFifo_writePtr >= TCP_CAN_BUFFER_ITEMS)
	{
		tcp_canFifo_writePtr = 0;
	}

	//If pointer are same after writing, then we made a buffer overflow of RX FIFO
	if (tcp_canFifo_writePtr == tcp_canFifo_readPtr)
	{
    tcp_canFifo_Overflow = true;
	}
}

/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
