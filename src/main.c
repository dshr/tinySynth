#include "main.h"

uint8_t state = 0x00;
uint8_t val = 0x00;

int main(){
  setupClocks();
  setupGPIO();
  setupTimer2();
  setupI2S();
  setupI2C();
  setupCS32L22();
  __enable_irq();

  while(1)
  {
    val++;
    if (val > 0x0F){
      val = 0x0F;
    }
    // if the flag is set, we can send our data
    if (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE))
    {
      if (state == 0x00)
      {
        // state is 0, send unshifted slope
        SPI_I2S_SendData(SPI3, val);
      }
      else
      {
        // statse is 1, send shifted slope
        SPI_I2S_SendData(SPI3, 0xFF - val);
      }
    }
  }
  return 0;
}

void setupClocks() {
  // enable GPIO ports A, B, C and D
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | //I^2S WS signal
   RCC_AHB1Periph_GPIOB | // I2C_SDA & I2C_SCL
   RCC_AHB1Periph_GPIOC | // I2S_MCK, I2S_SCK, I2S_SD
   RCC_AHB1Periph_GPIOD, ENABLE); // reset pin on the DAC

  // enable the serial peripherals
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1 |
   RCC_APB1Periph_SPI3, ENABLE);

  // enable the PLL for the I^2S
  RCC_PLLI2SCmd(ENABLE);

  // enable timer2
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
}

void setupGPIO() {
  GPIO_InitTypeDef GPIO_initStruct;

  // cs43L22 reset
  GPIO_initStruct.GPIO_Pin = GPIO_Pin_4;;
  GPIO_initStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_initStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_initStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_initStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_initStruct);

  // I2C1
  GPIO_initStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_initStruct.GPIO_OType = GPIO_OType_OD;
  GPIO_initStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_9;
  GPIO_initStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_initStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_initStruct);

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);

  // I2S
  GPIO_initStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_initStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_10 | GPIO_Pin_12;
  GPIO_Init(GPIOC, &GPIO_initStruct);

  GPIO_initStruct.GPIO_Pin = GPIO_Pin_4;
  GPIO_Init(GPIOA, &GPIO_initStruct);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);

  // reset the cs43L22
  GPIO_ResetBits(GPIOD, GPIO_Pin_4);
}

void setupI2S() {
  I2S_InitTypeDef I2S_InitType;
  I2S_InitType.I2S_AudioFreq = I2S_AudioFreq_48k;
  I2S_InitType.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
  I2S_InitType.I2S_Mode = I2S_Mode_MasterTx;
  I2S_InitType.I2S_DataFormat = I2S_DataFormat_16b;
  I2S_InitType.I2S_Standard = I2S_Standard_Phillips;
  I2S_InitType.I2S_CPOL = I2S_CPOL_Low;

  I2S_Init(SPI3, &I2S_InitType);

  I2S_Cmd(SPI3, ENABLE);
}

void setupI2C() {
  I2C_InitTypeDef I2C_InitType;
  I2C_InitType.I2C_ClockSpeed = 100000;
  I2C_InitType.I2C_Mode = I2C_Mode_I2C;
  I2C_InitType.I2C_OwnAddress1 = 99;
  I2C_InitType.I2C_Ack = I2C_Ack_Enable;
  I2C_InitType.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitType.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_Init(I2C1, &I2C_InitType);
  I2C_Cmd(I2C1, ENABLE);
}

void writeI2CData(uint8_t bytesToSend[], uint8_t numOfBytesToSend)
{
  uint8_t currentBytesValue = 0;

  // wait for bus to free
  while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
  // generate start seq
  I2C_GenerateSTART(I2C1, ENABLE);
  while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_SB));
  // send the address
  I2C_Send7bitAddress(I2C1, 0x94, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

  // send the data!
  while (currentBytesValue < numOfBytesToSend)
  {
    I2C_SendData(I2C1, bytesToSend[currentBytesValue]);
    currentBytesValue++;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
  }

  while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF));

  I2C_GenerateSTOP(I2C1, ENABLE);
}

void setupCS32L22()
{
  uint8_t sendBuffer[2];

  GPIO_SetBits(GPIOD, GPIO_Pin_4); // set the reset pin high

  sendBuffer[0] = 0x1A; //set the PCM channel A gain to +6 dB
  sendBuffer[1] = 0x0A;
  writeI2CData(sendBuffer, 2);

  sendBuffer[0] = 0x1B; //set the PCM channel B gain to +6 dB
  sendBuffer[1] = 0x0A;
  writeI2CData(sendBuffer, 2);

  sendBuffer[0] = 0x04; // use only the headphone output
  sendBuffer[1] = 0xAF;
  writeI2CData(sendBuffer, 2);

  // -------------------------
  // required init settings

  sendBuffer[0] = 0x00;
  sendBuffer[1] = 0x99;
  writeI2CData(sendBuffer, 2);

  sendBuffer[0] = 0x47;
  sendBuffer[1] = 0x80;
  writeI2CData(sendBuffer, 2);

  sendBuffer[0] = 0x32;
  sendBuffer[1] = 0xFF;
  writeI2CData(sendBuffer, 2);

  sendBuffer[0] = 0x32;
  sendBuffer[1] = 0x7F;
  writeI2CData(sendBuffer, 2);

  sendBuffer[0] = 0x00;
  sendBuffer[1] = 0x00;
  writeI2CData(sendBuffer, 2);

  //--------------------------

  sendBuffer[0] = 0x02; // this tells the board to not take any
  sendBuffer[1] = 0x9E; // new settings
  writeI2CData(sendBuffer, 2);
}

void setupTimer2()
{
  TIM_TimeBaseInitTypeDef timer;
  TIM_TimeBaseStructInit(&timer);
  timer.TIM_Prescaler = 749;
  timer.TIM_Period = 0xff;
  TIM_TimeBaseInit(TIM2, &timer);
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
  TIM_Cmd(TIM2, ENABLE);
  NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler()
{
  TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
  state ^= 0x01;
  val = 0x00;
}