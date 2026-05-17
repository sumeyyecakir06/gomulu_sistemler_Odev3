/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"

/* USER CODE BEGIN PD */
#define FLASH_BLINK_ADDR  ((uint32_t)0x0800FC00)
/* USER CODE END PD */

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
volatile uint8_t  blink_sayisi        = 4;
volatile uint8_t  blink_done         = 0;
volatile uint8_t  beklemede           = 0;
volatile uint32_t bekleme_sayaci      = 0;
volatile uint8_t  ledin_durumu        = 0;
volatile uint8_t  button_event        = 0;
volatile uint8_t  button_basili       = 0;
volatile uint32_t button_baslama_zamani = 0;
volatile uint8_t  factory_done        = 0;
/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN 0 */
static uint16_t Flash_Read(void)
{
    return *(__IO uint16_t *)FLASH_BLINK_ADDR;
}

static void Flash_Write(uint16_t value)
{
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef erase;
    uint32_t pageError = 0;
    erase.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = FLASH_BLINK_ADDR;
    erase.NbPages     = 1;
    HAL_FLASHEx_Erase(&erase, &pageError);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_BLINK_ADDR, (uint64_t)value);
    HAL_FLASH_Lock();
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) return;

    if (beklemede)
    {
        bekleme_sayaci++;
        if (bekleme_sayaci >= 5)
        {
            beklemede      = 0;
            bekleme_sayaci = 0;
            blink_done     = 0;
            ledin_durumu   = 0;
        }
        return;
    }


    if (ledin_durumu == 0)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); 
        ledin_durumu = 1;
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   
        ledin_durumu = 0;
        blink_done++;

        if (blink_done >= blink_sayisi)
        {
            beklemede      = 1;
            bekleme_sayaci = 0;
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin != GPIO_PIN_0) return;

    static uint32_t last_fall_tick = 0;
    uint32_t now = HAL_GetTick();

   
    if ((now - last_fall_tick) < 50) return;
    last_fall_tick = now;

    if (button_basili) return;

    button_basili         = 1;
    button_baslama_zamani = now;
    factory_done          = 0;
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();

  /* USER CODE BEGIN 2 */

  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
  {
      HAL_Delay(100);
      if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
      {
          uint32_t start_tick = HAL_GetTick();
          while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
          {
              if ((HAL_GetTick() - start_tick) >= 3000)
              {
                  blink_sayisi = 4;
                  Flash_Write(4);
                  break;
              }
          }
          
          while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {}
          HAL_Delay(50);
      }
  }

  
  button_basili = 0;
  button_event  = 0;

  
  uint16_t flash_value = Flash_Read();
  if (flash_value < 4 || flash_value > 7)
  {
      blink_sayisi = 4;
      Flash_Write(4);
  }
  else
  {
      blink_sayisi = (uint8_t)flash_value;
  }


  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

  HAL_TIM_Base_Start_IT(&htim2);
  /* USER CODE END 2 */

  while (1)
  {
     
      if (button_basili && (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET))
      {
          uint32_t sure = HAL_GetTick() - button_baslama_zamani;
          button_basili = 0;

        
          if (!factory_done && sure >= 30 && sure < 3000)
          {
              button_event = 1;
          }
          factory_done = 0;
      }

    
      if (button_basili)
      {
          uint32_t sure = HAL_GetTick() - button_baslama_zamani;
          if (sure >= 3000 && !factory_done)
          {
              factory_done   = 1;
              button_event   = 0;
              blink_sayisi   = 4;
              Flash_Write(4);
             
              blink_done     = 0;
              beklemede      = 0;
              bekleme_sayaci = 0;
              ledin_durumu   = 0;
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); 
          }
      }

      if (button_event)
      {
          button_event = 0;
          uint8_t nc   = (blink_sayisi < 7) ? blink_sayisi + 1 : 4;
          blink_sayisi = nc;
          Flash_Write(nc);
         
          blink_done     = 0;
          beklemede      = 0;
          bekleme_sayaci = 0;
          ledin_durumu   = 0;
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); 
      }
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                   | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig      = {0};

  htim2.Instance               = TIM2;
  htim2.Init.Prescaler         = 7999;          
  htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim2.Init.Period            = 999;          
  htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK) Error_Handler();

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) Error_Handler();

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) Error_Handler();

  HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();


  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
  GPIO_InitStruct.Pin   = GPIO_PIN_13;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


  GPIO_InitStruct.Pin  = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin   = GPIO_PIN_1;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
