/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"
#include "dma.h"
#include "fatfs.h"
#include "octospi.h"
#include "sdmmc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_dma.h"
#include "doomgeneric.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define frame ((uint16_t*)0x90640000)
FATFS fs;
FIL file;
UINT bytes_read;
//uint16_t color = 0x0000;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void SendCmd(uint8_t cmd){
	LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_5);
	while (!LL_SPI_IsActiveFlag_TXP(SPI1));
	LL_SPI_TransmitData8(SPI1, cmd);
	while (!LL_SPI_IsActiveFlag_TXC(SPI1));

}

void SendData(uint8_t data){
	LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_5);
	while (!LL_SPI_IsActiveFlag_TXP(SPI1));
	LL_SPI_TransmitData8(SPI1, data);
	while (!LL_SPI_IsActiveFlag_TXC(SPI1));

}
void SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){
	SendCmd(0x2A); //отправляем ширину экрана CASET
	SendData(x0>>8); SendData(x0 & 0xFF);
	SendData(x1>>8); SendData(x1 & 0xFF);

	SendCmd(0x2B); //отправляем длину экрана RASET
	SendData(y0>>8); SendData(y0 & 0xFF);
	SendData(y1>>8); SendData(y1 & 0xFF);
	SendCmd(0x2C);


}
void LCD_Init(void){
	LL_SPI_Enable(SPI1);
	LL_SPI_StartMasterTransfer(SPI1);

	LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_4);
	HAL_Delay(20);
	LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_4); //RES
	HAL_Delay(50);

	SendCmd(0x11); //SLPOUT
	HAL_Delay(120);

	SendCmd(0x3A); //COLMOD
	SendData(0x55);

	SendCmd(0x21); // Display Inversion ON (ВКЛЮЧИТЬ ИНВЕРСИЮ IPS!)
	HAL_Delay(10);

	SendCmd(0x13); // Normal Display Mode ON
	HAL_Delay(10);

	SendCmd(0x29);
	HAL_Delay(50);




}

/*void ColorFill(uint16_t color){
	uint8_t highbyte = color >> 8;
	uint8_t lowbyte = color & 0xFF;
	SetWindow(0, 0, 239, 239);
	LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_5);

	for (int i=0; i <240*240; i++){
		while(!LL_SPI_IsActiveFlag_TXP(SPI1));
		LL_SPI_TransmitData8(SPI1, highbyte);

		while(!LL_SPI_IsActiveFlag_TXP(SPI1));
		LL_SPI_TransmitData8(SPI1, lowbyte);
	}
	while(!LL_SPI_IsActiveFlag_TXC(SPI1));
}
*/

void DMA_Init(void){
	SetWindow(0, 0, 239, 239);

	LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_5);

	LL_SPI_Disable(SPI1);

	LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_16BIT);
	LL_SPI_SetTransferSize(SPI1, 0);
	LL_SPI_EnableDMAReq_TX(SPI1);

	LL_SPI_Enable(SPI1);
	LL_SPI_StartMasterTransfer(SPI1);

	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_STREAM_0, (uint32_t)&(SPI1->TXDR));
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_0, (uint32_t)frame);



}

void DrawFrame(void){

	//while (LL_DMA_IsEnabledStream(DMA1, LL_DMA_STREAM_0));
	SCB_CleanDCache_by_Addr((uint32_t*)frame, 240 * 240 * 2);

	/*LL_SPI_Disable(SPI1);

	LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_8BIT);
	LL_SPI_SetTransferSize(SPI1, 0);
	LL_SPI_Enable(SPI1);
	LL_SPI_StartMasterTransfer(SPI1);
	SendCmd(0x2C);*/
	//LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_5);

	/*LL_SPI_Disable(SPI1);
	LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_16BIT);
	LL_SPI_SetTransferSize(SPI1, 240*240);
	LL_SPI_EnableDMAReq_TX(SPI1);
	LL_SPI_Enable(SPI1);*/



	LL_DMA_ClearFlag_TC0(DMA1);
	LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_0, 240*240);

	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_0);
	//LL_SPI_StartMasterTransfer(SPI1);
}

void psram_SendCmd(uint8_t cmd){
	while(OCTOSPI1->SR &OCTOSPI_SR_BUSY);
	OCTOSPI1->CCR = (1 << OCTOSPI_CCR_IMODE_Pos);
	OCTOSPI1->IR = cmd;
	while(!(OCTOSPI1->SR & OCTOSPI_SR_TCF));
	OCTOSPI1->FCR = OCTOSPI_FCR_CTCF;
}


void psram_init(void){
	LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_3, LL_GPIO_SPEED_FREQ_MEDIUM);
	OCTOSPI1->CR &= ~OCTOSPI_CR_EN;
	OCTOSPI1->CR |= OCTOSPI_CR_EN;
	psram_SendCmd(0x66); // Res Enable
	psram_SendCmd(0x99); // Res
	HAL_Delay(1);
	psram_SendCmd(0x35); // Quad mode enter
	 OCTOSPI1->CR &= ~OCTOSPI_CR_EN;
	 while(OCTOSPI1->SR & OCTOSPI_SR_BUSY);
	 OCTOSPI1->CR &= ~OCTOSPI_CR_EN;
	 OCTOSPI1->DCR2 &= ~0xFF;
	 OCTOSPI1->DCR2 |= 31;
	 OCTOSPI1->DCR1 &= ~OCTOSPI_DCR1_CSHT_Msk;
	    OCTOSPI1->DCR1 |= (3 << OCTOSPI_DCR1_CSHT_Pos);

	OCTOSPI1->TCR = (6 << OCTOSPI_TCR_DCYC_Pos)| OCTOSPI_TCR_SSHIFT;
	OCTOSPI1->CCR = (3<< OCTOSPI_CCR_DMODE_Pos) | (2 << OCTOSPI_CCR_ADSIZE_Pos) | (3<< OCTOSPI_CCR_ADMODE_Pos) | (3<< OCTOSPI_CCR_IMODE_Pos);
	OCTOSPI1->IR = 0xEB;
	OCTOSPI1->WTCR = 0;
	OCTOSPI1->WCCR = OCTOSPI_WCCR_DQSE | (3<< OCTOSPI_WCCR_DMODE_Pos) | (2 << OCTOSPI_WCCR_ADSIZE_Pos) | (3<< OCTOSPI_WCCR_ADMODE_Pos) | (3<< OCTOSPI_WCCR_IMODE_Pos);
	OCTOSPI1->WIR = 0x38;


	OCTOSPI1->CR &= ~OCTOSPI_CR_FMODE_Msk;
	OCTOSPI1->CR |= (3<<OCTOSPI_CR_FMODE_Pos);
	  OCTOSPI1->CR |= OCTOSPI_CR_EN;

}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */


  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SDMMC1_SD_Init();
  MX_SPI1_Init();
  MX_OCTOSPI1_Init();
  MX_UART4_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  SCB_EnableICache();
  SCB_EnableDCache();
  __HAL_RCC_SYSCFG_CLK_ENABLE();
   HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PC3, SYSCFG_SWITCH_PC3_CLOSE);

   LCD_Init();
   DMA_Init();
   psram_init();

  f_mount(&fs, "", 1);

  // === СТРЕСС-ТЕСТ PSRAM НА 64 КИЛОБАЙТА ===
   /* volatile uint32_t *ram = (volatile uint32_t *)0x90000000;
    const uint32_t test_words = 16384; // 16384 * 4 байта = 64 КБ!

    for (uint32_t i = 0; i < test_words; i++) {
        ram[i] = 0xAA000000U ^ (i * 0x01010101U);
    }

    // Сбрасываем и инвалидируем ВСЕ 64 КБ!
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)ram, test_words * sizeof(uint32_t));
    __DSB();

    int stress_error = -1;
    for (uint32_t i = 0; i < test_words; i++) {
        uint32_t expected = 0xAA000000U ^ (i * 0x01010101U);
        if (ram[i] != expected) {
            stress_error = i;
            break;
        }
    }

    if (stress_error == -1) {
        LL_GPIO_SetOutputPin(GPIOE, LL_GPIO_PIN_3); // ВСЕ 64 КБ ПРОШЛИ! СВЕТОДИОД ГОРИТ!
    } else {
        while(1) { // Сбой
            LL_GPIO_TogglePin(GPIOE, LL_GPIO_PIN_3);
            HAL_Delay(100);
        }
    }*/
  doomgeneric_Create(0, NULL);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  //uint16_t test_colors[] = {0xF800, 0x07E0, 0x001F, 0xFFE0, 0xF81F, 0x07FF};
   //int color_index = 0;
  while (1)
  {
	  doomgeneric_Tick();

	/*  if ( test_ram[0] == 0xDEADBEEF && test_ram[1] == 0xCAFEBABE)
	  {
		  LL_GPIO_SetOutputPin(GPIOE, LL_GPIO_PIN_3);
	  }
	  else {
		  while(1) {
		           LL_GPIO_TogglePin(GPIOE, LL_GPIO_PIN_3);
		           HAL_Delay(100);
		       }
	  }
*/
	//  while (LL_DMA_IsEnabledStream(DMA1, LL_DMA_STREAM_0));

	 /* LL_GPIO_SetOutputPin(GPIOE, LL_GPIO_PIN_3);
	  HAL_Delay(500);
	  LL_GPIO_ResetOutputPin(GPIOE, LL_GPIO_PIN_3);
	  HAL_Delay(500);*/
	 /* uint16_t c = test_colors[color_index];

	  for(uint16_t i=0; i<57600;i++){
	    	  	  frame[i] = c;
	    	    }
	  color_index = (color_index + 1) % 6;*/
	 // f_read(&file, frame, 240*240*2, &bytes_read);
	 // DrawFrame();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 44;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8MB;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
