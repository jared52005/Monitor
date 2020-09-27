/*******************************************************************************
  * @brief   Module for sending raw KLINE bytes into TCP socket
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 
#include <stdio.h>
#include <stdbool.h>
#include "System_stats.h"
#include "Task_Tcp_KlineRaw.h"
#include "lwip/opt.h"
#include "string.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

// -- Private definitions
#define TCPECHO_THREAD_PRIO  ( tskIDLE_PRIORITY + 4 )
#define TCP_KLINE_BUFFER_ITEMS 255

// -- Private Variables ---------------------------------
static uint8_t tcp_klineFifo[TCP_KLINE_BUFFER_ITEMS]; //FIFO buffer with CAN messages to send to Wireshark
static int  tcp_klineFifo_writePtr = 0;     //Pointer where we are starting with writing
static bool tcp_klineFifo_Overflow = false; //Overflow flag

static void tcpkline_fifo_reset()
{
	tcp_klineFifo_writePtr = 0;
	tcp_klineFifo_Overflow = false;
}

static int tcpkline_fifo_count()
{
  //Check if buffer has overflown
	if (tcp_klineFifo_Overflow == true)
	{
    printf("TCP KLINE buffer overflow");
		tcpkline_fifo_reset();
		return 0;
	}
	return tcp_klineFifo_writePtr;
}

/*-----------------------------------------------------------------------------------*/
static void tcpkline_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err, accept_err;
  int size;
  
  LWIP_UNUSED_ARG(arg);

  /* Create a new connection identifier. */
  conn = netconn_new(NETCONN_TCP);
  
  if (conn!=NULL)
  {  
    /* Bind connection to 19100. */
    err = netconn_bind(conn, NULL, 19100);
    
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
          tcpkline_fifo_reset();
          //Show on LCD that we have connection
          Stats_TCP_KLINE_State_Set(1);
					printf("KLINE RAW Connection established\n");

					//Write kline data into TCP stream in endless loop
          while (netconn_err(newconn) == ERR_OK) 
          {
            size = tcpkline_fifo_count();
            if(size > 0)
            {
              //Write packet data
              netconn_write(newconn, tcp_klineFifo, tcp_klineFifo_writePtr, NETCONN_COPY);

              //Reset buffer
              tcp_klineFifo_writePtr = 0;
              //Send all packets in buffer on TCP
              continue;
            }
            //Mandatory. Give RTOS chance to yield tasks. taskYIELD crashes whole RTOS from some reason
            osDelay(10); 
          }
					printf("KLINE RAW Connection closed\n");
        
          /* Close connection and discard connection identifier. */
          netconn_close(newconn);
          netconn_delete(newconn);
          Stats_TCP_KLINE_State_Set(0);
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

void Task_Tcp_Kline_Init(void)
{
  sys_thread_new("tcpkline_thread", tcpkline_thread, NULL, DEFAULT_THREAD_STACKSIZE, TCPECHO_THREAD_PRIO);
}

void Task_Tcp_Kline_AddNewByte(uint8_t c)
{
  //Write down data into ring buffer for sending
	tcp_klineFifo[tcp_klineFifo_writePtr] = c;
  tcp_klineFifo_writePtr++;

	//If pointer are same after writing, then we made a buffer overflow of RX FIFO
	if (tcp_klineFifo_writePtr >= TCP_KLINE_BUFFER_ITEMS)
	{
    tcp_klineFifo_Overflow = true;
	}
}

/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
