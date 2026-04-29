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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "MFRC522.h"
#include "i2c_eeprom.h"
#include "display.h"
#include "stdio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef EEPROM_PORT;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
uint8_t write_n_EEPROM(uint16_t addr, char *data, uint8_t size);
uint8_t read_n_EEPROM(uint16_t addr, char *data, uint8_t size);
void Read_All_Data(void);
char* Get_RFID_Data(void);

#define EEPROM_START_ADDR   0x0000
#define TOTAL_BYTES         100
#define PAGE_SIZE           64   // match your EEPROM (EE_PAGE_LEN)
#define ADDR_PTR_LOC  0x01F0
char uart_msg[150];
#define IN1_Pin GPIO_PIN_0
#define IN1_Port GPIOA

#define IN2_Pin GPIO_PIN_1
#define IN2_Port GPIOA

#define IN3_Pin GPIO_PIN_4
#define IN3_Port GPIOC

#define IN4_Pin GPIO_PIN_5
#define IN4_Port GPIOC
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint8_t tx_data[TOTAL_BYTES];
uint8_t rx_data[TOTAL_BYTES];
uint16_t current_addr = 0;
uint8_t reading_mode = 0;
uint8_t rx_char;
char rx_buffer[30];
uint8_t idx = 0;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t uid[4];

MFRC522_t rfID = {&hspi1, GPIOA, GPIO_PIN_4, GPIOC, GPIO_PIN_8};
char buffer[100];
char buffer1[100];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_RTC_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void EEPROM_Write_Buffer(uint16_t addr, uint8_t *data, uint16_t size)
{
    uint16_t bytes_written = 0;
    uint16_t chunk;

    while (bytes_written < size)
    {
        uint16_t page_offset   = addr % PAGE_SIZE;
        uint16_t space_in_page = PAGE_SIZE - page_offset;

        chunk = (size - bytes_written < space_in_page) ?
                (size - bytes_written) : space_in_page;

        if (write_n_EEPROM(addr, (char *)&data[bytes_written], chunk) != HAL_OK)
        {
            // Hard fail â†’ debug here
            Error_Handler();
        }

        /* Poll EEPROM ready (NO blind delay) */
        while (HAL_I2C_IsDeviceReady(&EEPROM_PORT, EEPROM_I2C_ADDR, 1, 100) != HAL_OK);

        addr += chunk;
        bytes_written += chunk;
    }
}
/* =========================
   EEPROM READ
   ========================= */
void EEPROM_Read_Buffer(uint16_t addr, uint8_t *data, uint16_t size)
{
    if (read_n_EEPROM(addr, (char *)data, size) != HAL_OK)
    {
        Error_Handler();
    }
}

void Read_All_Data(void)
{
    uint16_t addr = 0;
    uint8_t len;
    uint8_t buffer[100];

    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\nStored Data:\r\n", 22, HAL_MAX_DELAY);

    while (addr < current_addr)
    {
        EEPROM_Read_Buffer(addr, &len, 1);
        addr += 1;

        // 🚨 IMPORTANT CHECK
        if (len == 0xFF || len == 0 || len > 50)
        {
            char msg[] = "\r\n[Corrupted Data Skipped]\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
            break;
        }

        EEPROM_Read_Buffer(addr, buffer, len);
        addr += len;

        buffer[len] = '\0';  // ✅ make string safe

        HAL_UART_Transmit(&huart2, buffer, len, HAL_MAX_DELAY);
        HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
    }
}
uint8_t RFID_Card_Detected(void)
{
    if (waitcardDetect(&rfID) == STATUS_OK)
    {
        if (MFRC522_ReadUid(&rfID, uid) == STATUS_OK)
        {
            waitcardRemoval(&rfID);
            return 1;   // Card detected
        }
    }

    return 0;   // No card
}
char* Get_RFID_Data(void)
{
    static char id_msg[100];   // ✅ MUST be static

    char n1_msg[] = "Pavan";
    char n2_msg[] = "SANDEEP";
    char n3_msg[] = "INVALID";

    if (uid[0]==0x61 && uid[1]==0xC8 && uid[2]==0x71 && uid[3]==0x06)
    {
        sprintf(id_msg, "%02X %02X %02X %02X  NAME= %s\r\n",
                uid[0], uid[1], uid[2], uid[3], n1_msg);
        HD44780_Clear();
        HD44780_SetCursor(0,0);
        HD44780_PrintStr("CARD ID");
        HD44780_SetCursor(0,1);
        HD44780_PrintStr("Accessed");
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_11,GPIO_PIN_SET);
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_11,GPIO_PIN_RESET);
        // Rotate 90° forward
        rotateSteps(140, 5);   // 64 * 8 = 512 steps
        stopMotor();
        HAL_Delay(100);
        // Rotate 90° backward
        for(int i = 0; i < 140; i++)
        {
            for(int step = 7; step >= 0; step--)
            {
                stepMotor(step);
                HAL_Delay(5);
            }
        }
        stopMotor();
    }
    else if (uid[0]==0xF3 && uid[1]==0xAB && uid[2]==0x50 && uid[3]==0x06)
    {
        sprintf(id_msg, "%02X %02X %02X %02X  NAME= %s\r\n",
                uid[0], uid[1], uid[2], uid[3], n2_msg);
        HD44780_Clear();
        HD44780_SetCursor(0,0);
        HD44780_PrintStr("CARD ID");
        HD44780_SetCursor(0,1);
        HD44780_PrintStr("Accessed");
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_11,GPIO_PIN_SET);
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_11,GPIO_PIN_RESET);
        // Rotate 90° forward
        rotateSteps(140, 5);   // 64 * 8 = 512 steps
        stopMotor();
        HAL_Delay(100);

        // Rotate 90° backward
        for(int i = 0; i < 140; i++)
        {
            for(int step = 7; step >= 0; step--)
            {
                stepMotor(step);
                HAL_Delay(5);
            }
        }
        stopMotor();
    }
    else
    {
        sprintf(id_msg, "%02X %02X %02X %02X  NAME= %s\r\n",
                uid[0], uid[1], uid[2], uid[3],n3_msg);
        HD44780_Clear();
        HD44780_SetCursor(0,0);
        HD44780_PrintStr("CARD ID");
        HD44780_SetCursor(0,1);
        HD44780_PrintStr("Access Denied");
        for (int i = 0; i < 3; i++)
        {
            HAL_GPIO_WritePin(GPIOC,GPIO_PIN_10,GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOC,GPIO_PIN_11,GPIO_PIN_SET);
            HAL_Delay(150);
            HAL_GPIO_WritePin(GPIOC,GPIO_PIN_10,GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOC,GPIO_PIN_11,GPIO_PIN_RESET);
            HAL_Delay(150);
        }
    }

    return id_msg;
}
void EEPROM_Clear_All(void)
{
    uint8_t empty = 0xFF;

    for (uint16_t i = 0; i < 512; i++)
    {
        EEPROM_Write_Buffer(i, &empty, 1);
    }

    uint16_t zero = 0;
    EEPROM_Write_Buffer(ADDR_PTR_LOC, (uint8_t*)&zero, 2);
}
void stepMotor(int step) {
    switch(step) {
        case 0:
            HAL_GPIO_WritePin(IN1_Port, IN1_Pin, 1);
            HAL_GPIO_WritePin(IN2_Port, IN2_Pin, 0);
            HAL_GPIO_WritePin(IN3_Port, IN3_Pin, 0);
            HAL_GPIO_WritePin(IN4_Port, IN4_Pin, 0);
            break;
        case 1:
            HAL_GPIO_WritePin(IN1_Port, IN1_Pin, 1);
            HAL_GPIO_WritePin(IN2_Port, IN2_Pin, 1);
            HAL_GPIO_WritePin(IN3_Port, IN3_Pin, 0);
            HAL_GPIO_WritePin(IN4_Port, IN4_Pin, 0);
            break;
        case 2:
            HAL_GPIO_WritePin(IN1_Port, IN1_Pin, 0);
            HAL_GPIO_WritePin(IN2_Port, IN2_Pin, 1);
            HAL_GPIO_WritePin(IN3_Port, IN3_Pin, 0);
            HAL_GPIO_WritePin(IN4_Port, IN4_Pin, 0);
            break;
        case 3:
            HAL_GPIO_WritePin(IN1_Port, IN1_Pin, 0);
            HAL_GPIO_WritePin(IN2_Port, IN2_Pin, 1);
            HAL_GPIO_WritePin(IN3_Port, IN3_Pin, 1);
            HAL_GPIO_WritePin(IN4_Port, IN4_Pin, 0);
            break;
        case 4:
            HAL_GPIO_WritePin(IN1_Port, IN1_Pin, 0);
            HAL_GPIO_WritePin(IN2_Port, IN2_Pin, 0);
            HAL_GPIO_WritePin(IN3_Port, IN3_Pin, 1);
            HAL_GPIO_WritePin(IN4_Port, IN4_Pin, 0);
            break;
        case 5:
            HAL_GPIO_WritePin(IN1_Port, IN1_Pin, 0);
            HAL_GPIO_WritePin(IN2_Port, IN2_Pin, 0);
            HAL_GPIO_WritePin(IN3_Port, IN3_Pin, 1);
            HAL_GPIO_WritePin(IN4_Port, IN4_Pin, 1);
            break;
        case 6:
            HAL_GPIO_WritePin(IN1_Port, IN1_Pin, 0);
            HAL_GPIO_WritePin(IN2_Port, IN2_Pin, 0);
            HAL_GPIO_WritePin(IN3_Port, IN3_Pin, 0);
            HAL_GPIO_WritePin(IN4_Port, IN4_Pin, 1);
            break;
        case 7:
            HAL_GPIO_WritePin(IN1_Port, IN1_Pin, 1);
            HAL_GPIO_WritePin(IN2_Port, IN2_Pin, 0);
            HAL_GPIO_WritePin(IN3_Port, IN3_Pin, 0);
            HAL_GPIO_WritePin(IN4_Port, IN4_Pin, 1);
            break;
    }
}

void rotateSteps(int steps, int delay_ms) {
    for(int i = 0; i < steps; i++) {
        for(int step = 0; step < 8; step++) {
            stepMotor(step);
            HAL_Delay(delay_ms);
        }
    }
}
void stopMotor() {
    HAL_GPIO_WritePin(IN1_Port, IN1_Pin, 0);
    HAL_GPIO_WritePin(IN2_Port, IN2_Pin, 0);
    HAL_GPIO_WritePin(IN3_Port, IN3_Pin, 0);
    HAL_GPIO_WritePin(IN4_Port, IN4_Pin, 0);
}
RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;

void Get_Time_Date(void)
{
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // Time
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
             sTime.Hours, sTime.Minutes, sTime.Seconds);

    // Date
    snprintf(buffer1, sizeof(buffer1), "%02d-%02d-%04d",
             sDate.Date, sDate.Month, sDate.Year + 2000);
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (rx_char == '\r' || rx_char == '\n')
        {
            rx_buffer[idx] = '\0';

            if (strcmp(rx_buffer, "read data") == 0)
            {
                char msg[] = "\r\nReading EEPROM Data...\r\n";
                HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

                Read_All_Data();   // ✅ YOUR FUNCTION
            }

            idx = 0; // reset buffer
        }
        else
        {
            if (idx < sizeof(rx_buffer) - 1)
            {
                rx_buffer[idx++] = rx_char;
            }
        }

        // 🔁 RE-ENABLE INTERRUPT (VERY IMPORTANT)
        HAL_UART_Receive_IT(&huart2, &rx_char, 1);
    }
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
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart2, &rx_char, 1);
  MFRC522_Init(&rfID);
  HD44780_Init(2);

  EEPROM_Clear_All();
  EEPROM_Read_Buffer(ADDR_PTR_LOC, (uint8_t*)&current_addr, 2);
  if (current_addr == 0xFFFF)
  {
      current_addr = 0;
  }
  uint8_t last_state_card1 = 0; // 0=ENTRY, 1=EXIT
  uint8_t last_state_card2 = 0;
  char msg[]="START\r\n";
  HAL_UART_Transmit(&huart2,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if (RFID_Card_Detected())
      {
          Get_Time_Date();     // ✅ update time
          Get_RFID_Data();     // ✅ run display + motor

          char log_msg[120];

          if (uid[0]==0x61 && uid[1]==0xC8 && uid[2]==0x71 && uid[3]==0x06)
          {
              if (last_state_card1 == 0)
              {
                  sprintf(log_msg,
                  "CARD: %02X %02X %02X %02X ENTRY %s %s",
                  uid[0], uid[1], uid[2], uid[3], buffer, buffer1);

                  last_state_card1 = 1;
              }
              else
              {
                  sprintf(log_msg,
                  "CARD: %02X %02X %02X %02X EXIT %s %s",
                  uid[0], uid[1], uid[2], uid[3], buffer, buffer1);

                  last_state_card1 = 0;
              }
          }
          else if (uid[0]==0xF3 && uid[1]==0xAB && uid[2]==0x50 && uid[3]==0x06)
          {
              if (last_state_card2 == 0)
              {
                  sprintf(log_msg,
                  "CARD: %02X %02X %02X %02X ENTRY %s %s",
                  uid[0], uid[1], uid[2], uid[3], buffer, buffer1);

                  last_state_card2 = 1;
              }
              else
              {
                  sprintf(log_msg,
                  "CARD: %02X %02X %02X %02X EXIT %s %s",
                  uid[0], uid[1], uid[2], uid[3], buffer, buffer1);

                  last_state_card2 = 0;
              }
          }
          else
          {
              // ✅ STORE INVALID ALSO
              sprintf(log_msg,
              "CARD: %02X %02X %02X %02X INVALID %s %s",
              uid[0], uid[1], uid[2], uid[3], buffer, buffer1);
          }
          uint8_t len = strlen(log_msg);

          // Write length
          EEPROM_Write_Buffer(current_addr, &len, 1);
          current_addr += 1;

          // Write data
          EEPROM_Write_Buffer(current_addr, (uint8_t*)log_msg, len);
          current_addr += len;

          // Save pointer
          EEPROM_Write_Buffer(ADDR_PTR_LOC, (uint8_t*)&current_addr, 2);

          // UART print
          char success_msg[] = "Card Stored Successfully\r\n";
          HAL_UART_Transmit(&huart2, (uint8_t*)success_msg, strlen(success_msg), HAL_MAX_DELAY);
          HAL_Delay(1000); // debounce
      }
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 8;
  sTime.Minutes = 30;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_APRIL;
  sDate.Date = 20;
  sDate.Year = 26;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_9
                          |GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PC4 PC5 PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC9 PC10 PC11 */
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
