/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
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
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "encoder_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TRACEX_BUFFER_SIZE		    64000
#define ENCODER_THREAD_STACK_SIZE	4096
#define ENCODER_THREAD_PRIORITY		10
#define QUEUE_CAP				    4
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
uint8_t tracex_buffer[TRACEX_BUFFER_SIZE];

TX_THREAD encoder_thread;

leader_state_t current_role = NOT_DETERMINED;

TX_QUEUE q;
uint32_t q_data[QUEUE_CAP] = {0};
extern TX_SEMAPHORE stateDetermined;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void encoder_thread_entry(ULONG init);
/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  /* USER CODE BEGIN App_ThreadX_MEM_POOL */
  TX_BYTE_POOL *bytePool = (TX_BYTE_POOL *) memory_ptr;
  VOID *pointer;

  // stack allocation for ENCODER thread
  ret = tx_byte_allocate(bytePool, &pointer, ENCODER_THREAD_STACK_SIZE, TX_NO_WAIT);

  if (ret != TX_SUCCESS)
    return ret;
  ret = tx_queue_create(&q, "shizz queue", 1, q_data, sizeof(uint32_t) * QUEUE_CAP);

  if (ret != TX_SUCCESS){
	  printf("Error creating queue!\n");
	  return ret;
  }

  // ENCODER thread create
  ret = tx_thread_create(&encoder_thread, "ENCODER thread", encoder_thread_entry, 1234,
	  pointer, ENCODER_THREAD_STACK_SIZE, ENCODER_THREAD_PRIORITY, ENCODER_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

  if (ret != TX_SUCCESS)
    return ret;


  /* USER CODE END App_ThreadX_MEM_POOL */

  /* USER CODE BEGIN App_ThreadX_Init */
  tx_trace_enable(&tracex_buffer, TRACEX_BUFFER_SIZE, 30);
  /* USER CODE END App_ThreadX_Init */

  return ret;
}

  /**
  * @brief  Function that implements the kernel's initialization.
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

  /* USER CODE END  Kernel_Start_Error */
}

/* USER CODE BEGIN 1 */
int queue_push(uint32_t action_id){
	return tx_queue_send(&q, &action_id,TX_NO_WAIT);
}

int queue_poll(){
	int rcv;
	int status = tx_queue_receive(&q, &rcv,TX_NO_WAIT);
	//printf("Status: %d\n", status);
	if (status == TX_QUEUE_EMPTY) rcv = QUEUE_EMPTY;
	return rcv;
}

void encoder_thread_entry(ULONG init)
{
  printf("Entered encoder thread\r\n");
  uint32_t pos = 0;
  tx_semaphore_get(&stateDetermined, TX_WAIT_FOREVER);
  if (current_role == LEADER) {
	  printf("Entering LEADER loop\r\n");
	  uint32_t last_pos = pos;
    while(1) {
      encoder_driver_input(&pos);
      if (last_pos != pos){
		  printf("Encoder position: %ld\n", pos);
		  queue_push(pos);
      }
      last_pos = pos;
      tx_thread_sleep(20);
    }
  }
  else if (current_role == RECEIVER) {
	printf("Entering RECEIVER loop\r\n");
	while(1) {
	  int ret = queue_poll();
	  if(ret != QUEUE_EMPTY) {
	    printf("Queue poll: %d\n", ret);
	    pos = (uint32_t)ret;
	  }
	  printf("Motor to %ld\n", pos);
	  motor_driver_controller(pos);
	  tx_thread_sleep(5);
	}
  }
}

/* USER CODE END 1 */
