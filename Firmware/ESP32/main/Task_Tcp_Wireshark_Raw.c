/*******************************************************************************
 * @brief   Implementation of sending of Socket CAN packets into Wireshark
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 
#include <stdio.h>
#include "System_stats.h"
#include "Task_Tcp_Wireshark_Raw.h"
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

// -- Private Definitions -------------------------------
#define TAG "Task_Tcp_Wireshark_Raw.c"

#define CONFIG_EXAMPLE_IPV4
#define PORT                        19000
#define KEEPALIVE_IDLE              100
#define KEEPALIVE_INTERVAL          100
#define KEEPALIVE_COUNT             100

// -- Private Variables ---------------------------------
static uint8_t fileHeader[] = 
{
  0xD4, 0xC3, 0xB2, 0xA1, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00
};
static uint8_t packetHeader[16];
static uint8_t packetBody[4116]; //4096 of data + 20 bytes of header

static QueueHandle_t xRawMessageQueue = NULL;

static void tcpswraw_prepare_header(RawMessage* rmsg, uint8_t* array)
{
  /* 00-00-00-00 00-00-00-00-10-00-00-00-10-00-00-00
   Where:
    00-00-00-00 = Time Stamp seconds.
    00-00-00-00 = Time Stamp micro seconds
    10-00-00-00 = Size of packet saved in a file = 20 bytes of header + Frame length
    10-00-00-00 = Actual size of packet = 20 bytes of header + Frame length
  */
  uint32_t timestamp_seconds;
  uint32_t timestamp_microseconds;

  //Prepare timestamp
  timestamp_seconds = (uint32_t)(rmsg->Timestamp / 1000); //Only second part (I know there should be Unix time, but I am too lazy to get RTC or NTP working)
  timestamp_microseconds = (uint32_t)(rmsg->Timestamp % 1000); //Only remainder from seconds
  timestamp_microseconds = timestamp_microseconds * 1000; //Convert milisecond to microseconds
  //Store timestamp
  *(uint32_t*)array = timestamp_seconds;
  *(uint32_t*)(array + 4) = timestamp_microseconds;

  //Store length
  *(uint32_t*)(array + 8) = rmsg->Length + 20;
  *(uint32_t*)(array + 12) = rmsg->Length + 20;
}

static int tcpswraw_prepare_body(RawMessage* rmsg, uint8_t* array, uint16_t sequence)
{
  int totalLength = rmsg->Length + 20;
  array[0] = 0x45; //Version 4, 5 words (5*4=20 bytes)
  array[1] = 0x00; //Differential services
  array[2] = (uint8_t)(totalLength >> 8); //Total size
  array[3] = (uint8_t)(totalLength);
  array[4] = (uint8_t)(sequence >> 8); //Identification, should be unique number
  array[5] = (uint8_t)(sequence);
  array[6] = 0x40; //Don't fragment
  array[7] = 0x00;
  array[8] = 0x80; //TTL
  array[9] = (uint8_t)(rmsg->MessageType); //Undefined protocol. Used together with datagrams
  array[10] = 0x00; //Header checksum (disabled)
  array[11] = 0x00;
  array[12] = 192; //Source
  array[13] = 168;
  array[14] = 0;
  array[15] = 1;
  array[16] = (uint8_t)(rmsg->Id >> 24); //Destination
  array[17] = (uint8_t)(rmsg->Id >> 16);
  array[18] = (uint8_t)(rmsg->Id >> 8);
  array[19] = (uint8_t)(rmsg->Id);
  //Data
  memcpy(&array[20], rmsg->Frame, rmsg->Length);
  return totalLength;
}

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
  RawMessage* xrmsg;
  int packetBody_Lenght;
  //Erase all data from SWCAN
  xQueueReset(xRawMessageQueue);

  //Show on LCD that we have connection
  Stats_TCP_WS_RAW_State_Set(1);
	ESP_LOGI(TAG, "Connection established");

  //Write file header to init Wireshark
  if(netconn_write(sock, fileHeader, 24) == false)
  {
    return;
  }
  while (1) 
  {
    if(xQueueReceive( xRawMessageQueue, &(xrmsg), ( TickType_t ) 10 ) == pdPASS)
    {
      //Write packet header
      tcpswraw_prepare_header(xrmsg, packetHeader);
      if(netconn_write(sock, packetHeader, 16) == false)
      {
        //Failed to write into TCP, connection probably closed
        return;
      }

      //Write packet data
      packetBody_Lenght = tcpswraw_prepare_body(xrmsg, packetBody, 0);
      if(netconn_write(sock, packetBody, packetBody_Lenght) == false)
      {
        //Failed to write into TCP, connection probably closed
        return;
      }
      vPortFree(xrmsg->Frame);
      vPortFree(xrmsg);
    }
  }
}

static void tcpwsraw_thread(void *pvParameters)
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
  xRawMessageQueue = xQueueCreate( 32, sizeof( RawMessage* ) );

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
    Stats_TCP_WS_RAW_State_Set(0);
    shutdown(sock, 0);
    close(sock);
  }
CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

/*-----------------------------------------------------------------------------------*/

void Task_Tcp_Wireshark_Raw_Init(void)
{
  xTaskCreate(tcpwsraw_thread, "tcpwsraw_thread", 4096, (void*)AF_INET, 5, NULL);
}

void Task_Tcp_Wireshark_Raw_AddNewRawMessage(uint8_t* frame, uint32_t length, uint32_t id, uint32_t timestamp, RawMessageType msgType)
{
	if(xRawMessageQueue == NULL)
  {
    return;
  }
  //Alocate data for message
  RawMessage* qRawMessage = (RawMessage*)pvPortMalloc(sizeof(RawMessage));
  qRawMessage->Frame = pvPortMalloc(sizeof(length));

  //Write down data into ring buffer for sending
	qRawMessage->MessageType = msgType;
	qRawMessage->Id = id;
	qRawMessage->Timestamp = timestamp;
  qRawMessage->Length = length;
	
  memcpy(qRawMessage->Frame, frame, length);
	
  //ESP_LOGW(TAG, "Before queue %x", (uint32_t)qRawMessage);
  //ESP_LOG_BUFFER_HEXDUMP(TAG, (uint8_t*)&qRawMessage, sizeof(RawMessage), ESP_LOG_INFO);
  xQueueSend(xRawMessageQueue, ( void * ) &qRawMessage, (TickType_t)0);
}
