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

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

#define TCPECHO_THREAD_PRIO  ( tskIDLE_PRIORITY + 4 )



/*-----------------------------------------------------------------------------------*/
static void tcpwscan_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err, accept_err;
  u8_t sampleData[] = {0x31, 0x32, 0x33, 0x0D, 0x0A};
      
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
          Stats_TCP_WS_SocketCAN_State_Set(1);
					printf("Connection established\n");
					//We are going to write only
          while (netconn_err(newconn) == ERR_OK) 
          {
              netconn_write(newconn, sampleData, 5, NETCONN_COPY);
              osDelay(10); //Mandatory. Give RTOS chance to yield tasks
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
  //Write down data into ring buffer for sending
}

/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
