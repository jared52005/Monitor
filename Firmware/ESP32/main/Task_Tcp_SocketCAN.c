/*******************************************************************************
 * @brief   Implementation of sending of Socket CAN packets into Wireshark
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 
#include <stdio.h>
#include "System_stats.h"
#include "Task_Tcp_SocketCAN.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

// -- Private definitions
#define TCPECHO_THREAD_PRIO  ( tskIDLE_PRIORITY + 4 )

#define TAG "Task_Tcp_SocketCAN.c"

#define CONFIG_EXAMPLE_IPV4
#define PORT                        19001
#define KEEPALIVE_IDLE              100
#define KEEPALIVE_INTERVAL          100
#define KEEPALIVE_COUNT             100

// -- Private Variables ---------------------------------
static uint8_t fileHeader[] = 
{
  0xD4, 0xC3, 0xB2, 0xA1, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0x00, 0x00, 0xE3, 0x00, 0x00, 0x00
};
static uint8_t packetHeader[] = 
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00
};
static uint8_t packetBody[16];

static QueueHandle_t xCanMessageQueue = NULL;

static void tcpswcan_prepare_header(CanMessage* cmsg, uint8_t* array)
{
  /* 00-00-00-00 00-00-00-00-10-00-00-00-10-00-00-00
   Where:
    00-00-00-00 = Time Stamp seconds.
    00-00-00-00 = Time Stamp micro seconds
    10-00-00-00 = Size of packet saved in a file = 16 bytes of body
    10-00-00-00 = Actual size of packet = 16 bytes of body
  */
  uint32_t timestamp_seconds;
  uint32_t timestamp_microseconds;

  //Prepare timestamp
  timestamp_seconds = (uint32_t)(cmsg->Timestamp / 1000); //Only second part (I know there should be Unix time, but I am too lazy to get RTC or NTP working)
  timestamp_microseconds = (uint32_t)(cmsg->Timestamp % 1000); //Only remainder from seconds
  timestamp_microseconds = timestamp_microseconds * 1000; //Convert milisecond to microseconds
  //Store timestamp
  *(uint32_t*)array = timestamp_seconds;
  *(uint32_t*)(array + 4) = timestamp_microseconds;
}

static void tcpswcan_prepare_body(CanMessage* cmsg, uint8_t* array)
{
  array[0] = (uint8_t)(cmsg->Id >> 24);
  array[1] = (uint8_t)(cmsg->Id >> 16);
  array[2] = (uint8_t)(cmsg->Id >> 8);
  array[3] = (uint8_t)cmsg->Id;
  //DLC
  array[4] = cmsg->Dlc;
  //Data
  memcpy(&array[8], cmsg->Frame, cmsg->Dlc);
}

/*-----------------------------------------------------------------------------------*/
static bool netconn_write(const int sock, uint8_t* tx_buffer, int len)
{
  //ESP_LOGW(TAG, "TCP Transmit");
  //ESP_LOG_BUFFER_HEXDUMP(TAG, tx_buffer, len, ESP_LOG_INFO);
  // send() can return less bytes than supplied length.
  // Walk-around for robust implementation.
  int to_write = len;
  while (to_write > 0) 
  {
    int written = send(sock, tx_buffer + (len - to_write), to_write, 0);
    if (written < 0) 
    {
      ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
      return false;
    }
    to_write -= written;
  }
  return true;
}

static void do_transmit(const int sock)
{
  CanMessage* xcmsg;
  //Erase all data from SWCAN
  xQueueReset(xCanMessageQueue);

  //Show on LCD that we have connection
  Stats_TCP_WS_SocketCAN_State_Set(1);
	ESP_LOGI(TAG, "Connection established");

  //Write file header to init Wireshark
  if(netconn_write(sock, fileHeader, 24) == false)
  {
    return;
  }
  while (1) 
  {
    if(xQueueReceive( xCanMessageQueue, &(xcmsg), ( TickType_t ) 10 ) == pdPASS)
    {
      //Write packet header
      tcpswcan_prepare_header(xcmsg, packetHeader);
      if(netconn_write(sock, packetHeader, 16) == false)
      {
        //Failed to write into TCP, connection probably closed
        return;
      }

      //Write packet data
      tcpswcan_prepare_body(xcmsg, packetBody);
      if(netconn_write(sock, packetBody, 16) == false)
      {
        //Failed to write into TCP, connection probably closed
        return;
      }

      vPortFree(xcmsg);
    }
  }
}

static void tcpwscan_thread(void *pvParameters)
{
  char addr_str[128];
  int addr_family = (int)pvParameters;
  int ip_protocol = 0;
  int keepAlive = 1;
  int keepIdle = KEEPALIVE_IDLE;
  int keepInterval = KEEPALIVE_INTERVAL;
  int keepCount = KEEPALIVE_COUNT;
  struct sockaddr_storage dest_addr;

  //Hold 32 messages max 
  xCanMessageQueue = xQueueCreate( 32, sizeof( CanMessage* ) );

  if (addr_family == AF_INET) 
  {
    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(PORT);
    ip_protocol = IPPROTO_IP;
  }

  int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
  if (listen_sock < 0) 
  {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }
  int opt = 1;
  setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  ESP_LOGI(TAG, "Socket created");

  int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err != 0) 
  {
    ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
    ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
    goto CLEAN_UP;
  }
  ESP_LOGI(TAG, "Socket bound, port %d", PORT);

  err = listen(listen_sock, 1);
  if (err != 0) 
  {
    ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
    goto CLEAN_UP;
  }

  while (1) 
  {
    ESP_LOGI(TAG, "Socket listening");

    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    socklen_t addr_len = sizeof(source_addr);
    int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
    if (sock < 0) 
    {
      ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
      break;
    }

    // Set tcp keepalive option
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

    // Convert ip address to string
    if (source_addr.ss_family == PF_INET) 
    {
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
    }
    
    ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);
    do_transmit(sock);
    ESP_LOGW(TAG, "Socket closed");
    Stats_TCP_WS_SocketCAN_State_Set(0);
    shutdown(sock, 0);
    close(sock);
  }
CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}
/*-----------------------------------------------------------------------------------*/

void Task_Tcp_SocketCAN_Init(void)
{
  xTaskCreate(tcpwscan_thread, "tcp_server_can", 4096, (void*)AF_INET, tskIDLE_PRIORITY + 5, NULL);
}

void Task_Tcp_SocketCAN_AddNewCanMessage(CanMessage cmsg)
{
  if(xCanMessageQueue == NULL)
  {
    return;
  }

  //Copy cmsg into allocated qCanMessage and queue it
  CanMessage* qCanMessage = (CanMessage*)pvPortMalloc(sizeof(CanMessage));
  qCanMessage->Dlc = cmsg.Dlc;
  qCanMessage->Id = cmsg.Id;
  qCanMessage->Timestamp = cmsg.Timestamp;
  memcpy(qCanMessage->Frame, cmsg.Frame, cmsg.Dlc);
  //ESP_LOGW(TAG, "Before queue %x", (uint32_t)qCanMessage);
  //ESP_LOG_BUFFER_HEXDUMP(TAG, (uint8_t*)&cmsg, sizeof(CanMessage), ESP_LOG_INFO);
  xQueueSend(xCanMessageQueue, ( void * ) &qCanMessage, (TickType_t)0);
}
