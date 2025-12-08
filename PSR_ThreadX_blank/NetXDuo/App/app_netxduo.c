/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_netxduo.c
  * @author  MCD Application Team
  * @brief   NetXDuo applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "nxd_ftp_server.h"
#include "nx_web_http_server.h"
#include "main.h"
#include "app_threadx.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HTTP_SERVER_PORT						80
#define IP_LINK_CHECK_THREAD_STACK_SIZE			1024
#define IP_LINK_CHECK_THREAD_PRIORITY			12
#define BOARD1_IP_ADDRESS						IP_ADDRESS(192, 168, 1, 11)
#define BOARD2_IP_ADDRESS						IP_ADDRESS(192, 168, 1, 12)
#define PC_IP_ADDRESS 							IP_ADDRESS(192, 168, 1, 10)

#define BOARD1_PORT 							5000
#define BOARD2_PORT 							5001

/* CHANGE HERE TO SET THE OTHER BOARD ADDRESS AND PORT */
#define THIS_BOARD_IP_ADDRESS					BOARD1_IP_ADDRESS
#define OTHER_BOARD_IP_ADDRESS					BOARD2_IP_ADDRESS

#define THIS_BOARD_PORT							BOARD1_PORT
#define OTHER_BOARD_PORT						BOARD2_PORT




/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TX_THREAD      			NxAppThread;
NX_PACKET_POOL 			NxAppPool;
NX_IP          			NetXDuoEthIpInstance;
/* USER CODE BEGIN PV */
TX_THREAD 				ipLinkCheckThread;
NX_UDP_SOCKET 			UDPSocket;
NX_FTP_SERVER 			ftpServer;
CHAR 					*ftpServerStack;

static UCHAR 			data_buffer[256];

extern TX_SEMAPHORE 	sdMountDone;
extern FX_MEDIA        	sdio_disk;

extern leader_state_t 	current_role;
NX_WEB_HTTP_SERVER httpServer;
CHAR *httpServerStack;

UINT prev_switch_state = GPIO_PIN_SET;

static NX_WEB_HTTP_SERVER_MIME_MAP app_mime_maps[] =
{
  {"css", "text/css"},
  {"svg", "image/svg+xml"},
  {"png", "image/png"},
  {"jpg", "image/jpg"},
  {"ico", "image/x-icon"},
  {"js", "text/javascript"}
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static VOID nx_app_thread_entry (ULONG thread_input);
/* USER CODE BEGIN PFP */
UINT ftpLogin(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info);
UINT ftpLogout(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info);
UINT http_request_notify(NX_WEB_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr);
VOID ipLinkCheckEntry(ULONG ini);
static VOID check_switch_and_send(VOID);
static VOID process_udp_command(UCHAR *data, UINT length);
static VOID send_packet(ULONG ipAddress, ULONG port, UCHAR* message, UINT message_len);
static VOID handle_udp_receive(NX_UDP_SOCKET* socket);
static VOID poll_queue_and_send(VOID);
/* USER CODE END PFP */

/**
  * @brief  Application NetXDuo Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_NetXDuo_Init(VOID *memory_ptr)
{
  UINT ret = NX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;
  CHAR *pointer;

  /* USER CODE BEGIN MX_NetXDuo_MEM_POOL */
  /* USER CODE END MX_NetXDuo_MEM_POOL */

  /* USER CODE BEGIN 0 */

  /* USER CODE END 0 */

  /* Initialize the NetXDuo system. */
  nx_system_initialize();

    /* Allocate the memory for packet_pool.  */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the Packet pool to be used for packet allocation,
   * If extra NX_PACKET are to be used the NX_APP_PACKET_POOL_SIZE should be increased
   */
  ret = nx_packet_pool_create(&NxAppPool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, pointer, NX_APP_PACKET_POOL_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_POOL_ERROR;
  }

    /* Allocate the memory for Ip_Instance */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, Nx_IP_INSTANCE_THREAD_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

   /* Create the main NX_IP instance */
  ret = nx_ip_create(&NetXDuoEthIpInstance, "NetX Ip instance", THIS_BOARD_IP_ADDRESS, NX_APP_DEFAULT_NET_MASK, &NxAppPool, nx_stm32_eth_driver,
                     pointer, Nx_IP_INSTANCE_THREAD_SIZE, NX_APP_INSTANCE_PRIORITY);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

    /* Allocate the memory for ARP */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Enable the ARP protocol and provide the ARP cache size for the IP instance */

  /* USER CODE BEGIN ARP_Protocol_Initialization */

  /* USER CODE END ARP_Protocol_Initialization */

  ret = nx_arp_enable(&NetXDuoEthIpInstance, (VOID *)pointer, DEFAULT_ARP_CACHE_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the ICMP */

  /* USER CODE BEGIN ICMP_Protocol_Initialization */

  /* USER CODE END ICMP_Protocol_Initialization */

  ret = nx_icmp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable TCP Protocol */

  /* USER CODE BEGIN TCP_Protocol_Initialization */

  /* USER CODE END TCP_Protocol_Initialization */

  ret = nx_tcp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

  /* Enable the UDP protocol required for  DHCP communication */

  /* USER CODE BEGIN UDP_Protocol_Initialization */

  /* USER CODE END UDP_Protocol_Initialization */

  ret = nx_udp_enable(&NetXDuoEthIpInstance);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_SUCCESSFUL;
  }

   /* Allocate the memory for main thread   */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS) // possibly switch to IP_LINK_CHECK_THREAD_STACK_SIZE
  {
    return TX_POOL_ERROR;
  }

  /* Create the main thread */
  ret = tx_thread_create(&NxAppThread, "NetXDuo App thread", nx_app_thread_entry , 0, pointer, NX_APP_THREAD_STACK_SIZE,
                         NX_APP_THREAD_PRIORITY, NX_APP_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

  if (ret != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* USER CODE BEGIN MX_NetXDuo_Init */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, 2*NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {

	  return TX_POOL_ERROR;
  }
  ftpServerStack = pointer;

  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, 2*NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {

      return TX_POOL_ERROR;
  }
  httpServerStack = pointer;

  /* Allocate the memory for IP link check thread   */
    if (tx_byte_allocate(byte_pool, (VOID **) &pointer, NX_APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
      return TX_POOL_ERROR;
    }

    /* Create the IP link check thread */
	ret = tx_thread_create(&ipLinkCheckThread, "IP link check thread", ipLinkCheckEntry , 0, pointer, IP_LINK_CHECK_THREAD_STACK_SIZE,
						 IP_LINK_CHECK_THREAD_PRIORITY, IP_LINK_CHECK_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

	if (ret != TX_SUCCESS)
	{
	return TX_THREAD_ERROR;
	}


  /* USER CODE END MX_NetXDuo_Init */

  return ret;
}

/**
* @brief  Main thread entry.
* @param thread_input: ULONG user argument used by the thread entry
* @retval none
*/
static VOID nx_app_thread_entry (ULONG thread_input)
{
  /* USER CODE BEGIN Nx_App_Thread_Entry 0 */

	UINT ret;
	/* CREATE UDP SOCKET */
	ret = nx_udp_socket_create(&NetXDuoEthIpInstance, &UDPSocket, "UDP Server Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 2);
	if (ret != NX_SUCCESS)
	{
		printf("UDP server create error. %02X\r\n", ret);
		while(1)
		{
			tx_thread_sleep(100);
		}
	}

	//bind the socket to board's port
	ret = nx_udp_socket_bind(&UDPSocket, THIS_BOARD_PORT, TX_WAIT_FOREVER);
	if (ret != NX_SUCCESS)
	{
		printf("Binding error. %02X\r\n", ret);
		while(1)
		{
			tx_thread_sleep(100);
		}
	}
	else
	{
		printf("UDP Server listening on PORT 5000.\r\n");
	}
	/* DETERMINE LEADER */
	while (current_role == NOT_DETERMINED){
		check_switch_and_send();
		handle_udp_receive(&UDPSocket);
	}

	/* CREATE FTP AND HTTP SERVERS IF RECEIVER*/
	// waiting for SD card mount and then start the FTP server
	if (current_role == RECEIVER){
		ret = tx_semaphore_get(&sdMountDone, TX_WAIT_FOREVER);
		if (ret == TX_SUCCESS)
		{
			// create the FTP server
			ret =  nx_ftp_server_create(&ftpServer, "FTP Server Instance", &NetXDuoEthIpInstance,
											  &sdio_disk, ftpServerStack, 2*NX_APP_THREAD_STACK_SIZE, &NxAppPool,
											  ftpLogin, ftpLogout);

			if (ret != TX_SUCCESS)
			{
			  printf("FTP server create error. %02X\r\n", ret);

			}

			// start the ftp server
			if (nx_ftp_server_start(&ftpServer) == NX_SUCCESS)
			{
				printf("FTP server started.\r\n");
			}

			// create webserver
			ret = nx_web_http_server_create(&httpServer, "HTTP server", &NetXDuoEthIpInstance, HTTP_SERVER_PORT,
					&sdio_disk, (VOID *) httpServerStack, 2*NX_APP_THREAD_STACK_SIZE, &NxAppPool,
					NULL, http_request_notify);

			if (ret != NX_SUCCESS)
			{
				printf("HTTP server create error. %02X\r\n", ret);
			}

			ret = nx_web_http_server_mime_maps_additional_set(&httpServer,&app_mime_maps[0], 6);

			if (nx_web_http_server_start(&httpServer) == NX_SUCCESS)
			{
				printf("HTTP server started.\r\n");
			}
		}
	}

	// start the loop
	while (1)
	{
		// wait for one second or until the UDP is received
		if (current_role == LEADER) poll_queue_and_send();
		check_switch_and_send();
		handle_udp_receive(&UDPSocket);
	}

  /* USER CODE END Nx_App_Thread_Entry 0 */

}

/* USER CODE BEGIN 1 */
UINT ftpLogin(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info)
{
	// we will accept all login attempts regardless the login and password
	return NX_SUCCESS;
}

UINT ftpLogout(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, ULONG client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info)
{
	return NX_SUCCESS;
}

UINT http_request_notify(NX_WEB_HTTP_SERVER *server_ptr,
                       UINT request_type,
                       CHAR *resource,
                       NX_PACKET *packet_ptr)
{
    NX_PARAMETER_NOT_USED(packet_ptr);
    NX_PARAMETER_NOT_USED(server_ptr);

     if (!(request_type == NX_WEB_HTTP_SERVER_GET_REQUEST || request_type == NX_WEB_HTTP_SERVER_POST_REQUEST))
    {
    	 return NX_SUCCESS;
    }

    if ((strcmp(resource, "/") == 0) || (strcmp(resource, "/index.htm") == 0))
    {
        strcpy(resource, "/index.html");
        return NX_SUCCESS;
    }

    if (strcmp(resource, "/data.json") == 0)
    {
        return NX_WEB_HTTP_CALLBACK_COMPLETED;
    }

    if (memcmp(resource, "/LED", 4) == 0)
    {
    	if (resource[4] == '1')
    	{
    		// button LED1 pressed
    	}

    	if (resource[4] == '2')
    	{
    		// button LED2 pressed
    	}

    	if (resource[4] == '3')
		{
    		// button LED3 pressed
		}

    	return NX_SUCCESS;
    }

    return NX_SUCCESS;
}

VOID ipLinkCheckEntry(ULONG ini)
{
	while (1)
	{
		ULONG actual_status;
		UINT ret;

		ret = nx_ip_interface_status_check(&NetXDuoEthIpInstance, 0, NX_IP_LINK_ENABLED, &actual_status, 10);

		if (ret == TX_SUCCESS && actual_status == NX_IP_LINK_ENABLED)
		{
			nx_ip_driver_direct_command(&NetXDuoEthIpInstance, NX_LINK_ENABLE, &actual_status);
		}


		tx_thread_sleep(100);
	}
}

static VOID send_packet(ULONG ipAddress, ULONG port, UCHAR* message, UINT message_len){
	UINT ret;
	NX_PACKET *outcoming_packet;

	// Allocate packet
	ret = nx_packet_allocate(&NxAppPool, &outcoming_packet, NX_UDP_PACKET, 100);
	if (ret != NX_SUCCESS)
	{
		printf("Packet allocate error: %02X\n", ret);
		return;
	}

	// Append data to packet
	ret = nx_packet_data_append(outcoming_packet, message, message_len, &NxAppPool, 100);
	if (ret != NX_SUCCESS)
	{
		printf("Packet append error: %02X\n", ret);
		nx_packet_release(outcoming_packet);
		return;
	}
	// send the data to the IP address and port which has been extracted from the incoming packet
	ret = nx_udp_socket_send(&UDPSocket, outcoming_packet,	ipAddress, port);
	if (ret != NX_SUCCESS)
	{
		// in the case of socket send failure we MUST release the outcoming packet!
		printf("UDP send error %02x\r\n", ret);
		nx_packet_release(outcoming_packet);
	}
	else
	{
		// in the case of socket success we MUST NOT release the outcoming packet!
		printf("UDP send successfully\r\n");
	}
}
static VOID handle_udp_receive(NX_UDP_SOCKET* socket){
	NX_PACKET *pkt;
	UINT ret;
	ULONG ip;
	UINT port;
	ULONG bytes_read = 0;

	ret = nx_udp_socket_receive(socket, &pkt, 100);
	if (ret != NX_SUCCESS)
		return;

	ret = nx_packet_data_retrieve(pkt, data_buffer, &bytes_read);
	if (ret == NX_SUCCESS)
	{
		if (bytes_read < sizeof(data_buffer))
			data_buffer[bytes_read] = '\0';
		else
			data_buffer[sizeof(data_buffer) - 1] = '\0';

		nx_udp_source_extract(pkt, &ip, &port);

		printf("Socket received %lu bytes from %lu.%lu.%lu.%lu:%u\r\n",
			   bytes_read,
			   (ip >> 24) & 0xFF,
			   (ip >> 16) & 0xFF,
			   (ip >> 8) & 0xFF,
			   ip & 0xFF,
			   port);

		process_udp_command(data_buffer, bytes_read);
		//echo back
		//send_packet(ip, port, data_buffer, bytes_read);
	}

	nx_packet_release(pkt);
	printf("Packets available %d\r\n\n", (int) NxAppPool.nx_packet_pool_available);
}

static VOID check_switch_and_send(VOID){
	UINT current_switch_state;

	current_switch_state = HAL_GPIO_ReadPin(GPIOB, SW1_Pin);
	// Check if state has changed
	if (current_switch_state != prev_switch_state) {
		prev_switch_state = current_switch_state;
		printf("Switch state: %u", current_switch_state);
		UCHAR message[16];
		UINT message_len;
		if (current_role != NOT_DETERMINED){
			if (current_switch_state == GPIO_PIN_RESET){
				message_len = sprintf((char*)message, "SW1=1");
				printf("Switch pressed - sending: %s\r\n", message);
			}
			else
			{
				message_len = sprintf((char*)message, "SW1=0");
				printf("Switch released - sending: %s\r\n", message);
			}
		} else
			if (current_switch_state == GPIO_PIN_SET){ // only register button push after the button's release
				current_role = LEADER;
				message_len = sprintf((char*)message, "LDR");
				printf("Recorded switch push while leader not yet determined, becoming LEADER r\n");
			}

		// Send to PC (or other board)
		send_packet(OTHER_BOARD_IP_ADDRESS, OTHER_BOARD_PORT, message, message_len);
	}
}

static VOID poll_queue_and_send(VOID) {
    int rcv = queue_poll();
    if (rcv == QUEUE_EMPTY) return;

    const char *fmt = NULL;

	//handle encoder value
    if (rcv < 1000) {
        fmt = "ENC=%d";
    } else {
		//handle rest of the states
        switch (rcv) {
            case LED1_ON:  fmt = "LED1G=1"; break;
            case LED1_OFF: fmt = "LED1G=0"; break;
            case LED2_ON:  fmt = "LED2R=1"; break;
            case LED2_OFF: fmt = "LED2R=0"; break;
            default: return;  // unknown code
        }
    }

    char *data;
    UINT data_len;

    if (rcv < 1000) {
        // need formatting
        int needed = snprintf(NULL, 0, fmt, rcv) + 1;
        data = malloc(needed);
        if (!data) return;
        snprintf(data, needed, fmt, rcv);
        data_len = needed - 1;
    } else {
        // literal string from table, doesn’t need formatting
        data_len = strlen(fmt);
        data = malloc(data_len + 1);
        if (!data) return;
        memcpy(data, fmt, data_len + 1);
    }

    send_packet(OTHER_BOARD_IP_ADDRESS, OTHER_BOARD_PORT, (UCHAR*) data, data_len);

    free(data);
}


static VOID process_udp_command(UCHAR *data, UINT length) {

	/* LEADER SELECTION */
	if ((strncmp((char*)data, "LDR", 3) == 0) && (current_role == NOT_DETERMINED))
	{
		current_role = RECEIVER;
		printf("Leader packet received, becoming receiver\r\n");
	}
	/* CHECK LED */
	else if (strncmp((char*)data, "LED1G=1", 7) == 0)
	{
		//HAL_GPIO_WritePin(LED1_G_GPIO_Port, LED1_G_Pin, GPIO_PIN_RESET);
		queue_push(LED1_ON);
		printf("LED1 Green ON\r\n");
	}
	else if (strncmp((char*)data, "LED1G=0", 7) == 0)
	{
		//HAL_GPIO_WritePin(LED1_G_GPIO_Port, LED1_G_Pin, GPIO_PIN_SET);
		queue_push(LED1_OFF);
		printf("LED1 Green OFF\r\n");
	}
	// Check for LED2 (if available)
	else if (strncmp((char*)data, "LED2R=1", 7) == 0)
	{
		queue_push(LED2_ON);
		//HAL_GPIO_WritePin(LED2_R_GPIO_Port, LED2_R_Pin, GPIO_PIN_RESET);
		printf("LED2 Red ON\r\n");
	}
	else if (strncmp((char*)data, "LED2R=0", 7) == 0)
	{
		//HAL_GPIO_WritePin(LED2_R_GPIO_Port, LED2_R_Pin, GPIO_PIN_SET);
		queue_push(LED2_OFF);
		printf("LED2 Red OFF\r\n");
	}

	else if (strncmp((char*)data, "ENC=", 4) == 0) //read encoder value
	{
		char *p = (char*) data + 4;
		char *end;
		uint32_t val = strtoul(p, &end, 10);

		if (end == p) {
		    printf("ERROR: Incorrect encoder message formatting (No value set)\r\n");
			return;
		}
		if (*end != '\0') {
		    printf("ERROR: Incorrect encoder message formatting (Invalid characters)\r\n");
			return;
		}
		else if (val > ENCODER_MAX) {
			printf("Warning: max encoder value reached: %d, setting to %d", val, ENCODER_MAX);
			val = ENCODER_MAX;
		}
		queue_push(val);
	}
	else
	{
		printf("Unknown command: %s\r\n", data);
	}
}

/* USER CODE END 1 */
