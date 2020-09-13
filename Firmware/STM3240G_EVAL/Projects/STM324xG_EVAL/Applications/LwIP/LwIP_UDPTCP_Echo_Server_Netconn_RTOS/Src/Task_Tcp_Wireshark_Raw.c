/*******************************************************************************
 * @brief   Implementation of sending of Socket CAN packets into Wireshark
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 
#include <string.h>
#include <stdbool.h>
#include "Task_Tcp_Wireshark_Raw.h"
#include "System_stats.h"
#include "lwip/opt.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

#define TCPECHO_THREAD_PRIO  ( tskIDLE_PRIORITY + 4 )
#define TCP_RAW_BUFFER_ITEMS 4

// -- Private Variables ---------------------------------
static u8_t fileHeader[] = 
{
  0xD4, 0xC3, 0xB2, 0xA1, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00
};
static u8_t packetHeader[16];
static u8_t packetBody[4116]; //4096 of data + 20 bytes of header

static struct RawMessage tcp_rawMessageFifo[TCP_RAW_BUFFER_ITEMS]; //FIFO buffer with CAN messages to send to Wireshark
static int  tcp_rawFifo_readPtr = 0;   //Pointer where we are starting with reading
static int  tcp_rawFifo_writePtr = 0;  //Pointer where we are starting with writing
static bool tcp_rawFifo_Overflow = false; //Overflow flag

static void tcpswraw_prepare_header(RawMessage rmsg, u8_t* array)
{
  /* 00-00-00-00 00-00-00-00-10-00-00-00-10-00-00-00
   Where:
    00-00-00-00 = Time Stamp seconds.
    00-00-00-00 = Time Stamp micro seconds
    10-00-00-00 = Size of packet saved in a file = 20 bytes of header + Frame length
    10-00-00-00 = Actual size of packet = 20 bytes of header + Frame length
  */
  u32_t timestamp_seconds;
  u32_t timestamp_microseconds;

  //Prepare timestamp
  timestamp_seconds = (uint32_t)(rmsg.Timestamp / 1000); //Only second part (I know there should be Unix time, but I am too lazy to get RTC or NTP working)
  timestamp_microseconds = (uint32_t)(rmsg.Timestamp % 1000); //Only remainder from seconds
  timestamp_microseconds = timestamp_microseconds * 1000; //Convert milisecond to microseconds
  //Store timestamp
  *(u32_t*)array = timestamp_seconds;
  *(u32_t*)(array + 4) = timestamp_microseconds;

  //Store length
  *(u32_t*)(array + 8) = rmsg.Length + 20;
  *(u32_t*)(array + 12) = rmsg.Length + 20;
}

static int tcpswraw_prepare_body(RawMessage rmsg, u8_t* array, u16_t sequence)
{
  int totalLength = rmsg.Length + 20;
  array[0] = 0x45; //Version 4, 5 words (5*4=20 bytes)
  array[1] = 0x00; //Differential services
  array[2] = (u8_t)(totalLength >> 8); //Total size
  array[3] = (u8_t)(totalLength);
  array[4] = (u8_t)(sequence >> 8); //Identification, should be unique number
  array[5] = (u8_t)(sequence);
  array[6] = 0x40; //Don't fragment
  array[7] = 0x00;
  array[8] = 0x80; //TTL
  array[9] = (u8_t)(rmsg.MessageType); //Undefined protocol. Used together with datagrams
  array[10] = 0x00; //Header checksum (disabled)
  array[11] = 0x00;
  array[12] = 192; //Source
  array[13] = 168;
  array[14] = 0;
  array[15] = 1;
  array[16] = (u8_t)(rmsg.Id >> 24); //Destination
  array[17] = (u8_t)(rmsg.Id >> 16);
  array[18] = (u8_t)(rmsg.Id >> 8);
  array[19] = (u8_t)(rmsg.Id);
  //Data
  //memcpy(&array[20], rmsg.Frame, rmsg.Length);
  return totalLength;
}

static void tcpswraw_fifo_reset()
{
	tcp_rawFifo_readPtr = 0;
	tcp_rawFifo_writePtr = 0;
	tcp_rawFifo_Overflow = false;
}

static int tcpwsraw_fifo_count()
{
  //Check if buffer has overflown
	if (tcp_rawFifo_Overflow == true)
	{
    printf("TCP RAW buffer overflow");
		tcpswraw_fifo_reset();
		return 0;
	}
  //this setup is being used as a ring buffer
	if(tcp_rawFifo_readPtr > tcp_rawFifo_writePtr)
	{
		//If read data has higher index than write data
		//this means that write data index has jumped on start
		return TCP_RAW_BUFFER_ITEMS - tcp_rawFifo_readPtr + tcp_rawFifo_writePtr;
	}
	else
	{
		return tcp_rawFifo_writePtr - tcp_rawFifo_readPtr;
	}
}


/*-----------------------------------------------------------------------------------*/
static void tcpwsraw_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err, accept_err;
  int packetBody_Lenght;
	int sequence;
	RawMessage rmsg;
      
  LWIP_UNUSED_ARG(arg);

  /* Create a new connection identifier. */
  conn = netconn_new(NETCONN_TCP);
  
  if (conn!=NULL)
  {  
    /* Bind connection to 19000. */
    err = netconn_bind(conn, NULL, 19000);
    
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
          tcpswraw_fifo_reset();
					sequence = 0;
          //Show on LCD that we have connection
          Stats_TCP_WS_RAW_State_Set(1);
					printf("RAW Connection established\n");

          //Write file header to init Wireshark
          netconn_write(newconn, fileHeader, 24, NETCONN_COPY);

					//Write RAW packets into TCP stream in endless loop
          while (netconn_err(newconn) == ERR_OK) 
          {
            if(tcpwsraw_fifo_count() > 0)
            {
							printf("Get Raw Message");
              rmsg = tcp_rawMessageFifo[tcp_rawFifo_readPtr];
              //Write packet header
							printf("Create and send raw header\n");
              tcpswraw_prepare_header(rmsg, packetHeader);
              netconn_write(newconn, packetHeader, 16, NETCONN_COPY);
							
              //Write packet data
							printf("Create and send raw body\n");
              packetBody_Lenght = tcpswraw_prepare_body(rmsg, packetBody, sequence);
							printf("Send Raw packet %x\n", packetBody_Lenght);
              netconn_write(newconn, packetBody, packetBody_Lenght, NETCONN_COPY);

              //Move to next packet
              tcp_rawFifo_readPtr++;
              if (tcp_rawFifo_readPtr >= TCP_RAW_BUFFER_ITEMS)
              {
                tcp_rawFifo_readPtr = 0;
              }
              //Send all packets in buffer on TCP
              continue;
            }
            //Mandatory. Give RTOS chance to yield tasks. taskYIELD crashes whole RTOS from some reason
            osDelay(10); 
          }
					printf("RAW Connection closed\n");
        
          /* Close connection and discard connection identifier. */
          netconn_close(newconn);
          netconn_delete(newconn);
          Stats_TCP_WS_RAW_State_Set(0);
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

void Task_Tcp_Wireshark_Raw_Init(void)
{
  sys_thread_new("tcpwsraw_thread", tcpwsraw_thread, NULL, DEFAULT_THREAD_STACKSIZE, TCPECHO_THREAD_PRIO);
}

void Task_Tcp_Wireshark_Raw_AddNewRawMessage(uint8_t* frame, uint32_t length, uint32_t id, uint32_t timestamp, RawMessageType msgType)
{
	int i;
  //Write down data into ring buffer for sending
	tcp_rawMessageFifo[tcp_rawFifo_writePtr].MessageType = msgType;
	tcp_rawMessageFifo[tcp_rawFifo_writePtr].Id = id;
	tcp_rawMessageFifo[tcp_rawFifo_writePtr].Timestamp = timestamp;
  tcp_rawMessageFifo[tcp_rawFifo_writePtr].Length = length;
	
	for (i = 0; i < length; i++)
	{
		tcp_rawMessageFifo[tcp_rawFifo_writePtr].Frame[i] = frame[i];
	}

	//Move write pointer, if we are on the end of ring buffer, reset pointer
	tcp_rawFifo_writePtr++;
	if (tcp_rawFifo_writePtr >= TCP_RAW_BUFFER_ITEMS)
	{
		tcp_rawFifo_writePtr = 0;
	}

	//If pointer are same after writing, then we made a buffer overflow of RX FIFO
	if (tcp_rawFifo_writePtr == tcp_rawFifo_readPtr)
	{
    tcp_rawFifo_Overflow = true;
	}
}
/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
