#include "setup.h"

void setupClocks() {
  // enable GPIO ports A, B, C and D
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | //I2S WS signal
   RCC_AHB1Periph_GPIOB | // I2C_SDA & I2C_SCL
   RCC_AHB1Periph_GPIOC | // I2S_MCK, I2S_SCK, I2S_SD
   RCC_AHB1Periph_GPIOD, ENABLE); // reset pin on the DAC

  // enable the serial peripherals
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1 | RCC_APB1Periph_SPI3, ENABLE);

  // enable the ADC
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
}

void setupPLL() {
  RCC_I2SCLKConfig(RCC_I2S2CLKSource_PLLI2S);
  RCC_PLLI2SCmd(ENABLE);
  while(!RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY));
}

void setupGPIO() {
  GPIO_InitTypeDef GPIO_initStruct;

  // cs43L22 reset & the LEDs
  GPIO_initStruct.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_12 | GPIO_Pin_13 |
  GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_initStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_initStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_initStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_initStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_initStruct);

  // I2C1
  GPIO_initStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_9;
  GPIO_initStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_initStruct.GPIO_OType = GPIO_OType_OD;
  GPIO_initStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_initStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_initStruct);

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);

  // I2S
  GPIO_initStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_10 | GPIO_Pin_12;
  GPIO_initStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_Init(GPIOC, &GPIO_initStruct);

  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);

  GPIO_initStruct.GPIO_Pin = GPIO_Pin_4;
  GPIO_Init(GPIOA, &GPIO_initStruct);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_SPI3);

  // user button
  GPIO_initStruct.GPIO_Pin = GPIO_Pin_0;
  GPIO_initStruct.GPIO_Mode = GPIO_Mode_IN;
  GPIO_initStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_initStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_initStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_initStruct);

  // ADC
  GPIO_initStruct.GPIO_Pin = GPIO_Pin_1;
  GPIO_initStruct.GPIO_Mode = GPIO_Mode_AN;
  GPIO_initStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_initStruct);

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

  setupPLL();
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

void setupADC() {
  ADC_CommonInitTypeDef ADC_CommonInitType;
  ADC_CommonStructInit(&ADC_CommonInitType);
  ADC_CommonInitType.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitType.ADC_Prescaler = ADC_Prescaler_Div2;
  ADC_CommonInitType.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitType.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitType);

  ADC_InitTypeDef ADC_InitType;
  ADC_StructInit(&ADC_InitType);
  ADC_InitType.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitType.ADC_ScanConvMode = DISABLE;
  ADC_InitType.ADC_ContinuousConvMode = DISABLE;
  ADC_InitType.ADC_ExternalTrigConvEdge = 0;
  ADC_InitType.ADC_ExternalTrigConv = 0;
  ADC_InitType.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitType.ADC_NbrOfConversion = 1;
  ADC_Init(ADC1, &ADC_InitType);
  ADC_Cmd(ADC1, ENABLE);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 1, ADC_SampleTime_144Cycles);
}

void writeI2CData(uint8_t bytesToSend[], uint8_t numOfBytesToSend){
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

void setupCS32L22(){
  int delaycount;
  uint8_t sendBuffer[2];

  GPIO_SetBits(GPIOD, GPIO_Pin_4); // set the reset pin high

  delaycount = 1000000; // wait for the DAC to enter reset mode
  while (delaycount > 0)
  {
    delaycount--;
  }

  sendBuffer[0] = 0x04; // use only the headphone output
  sendBuffer[1] = 0xAF;
  writeI2CData(sendBuffer, 2);

  sendBuffer[0] = 0x05; // clock is 256*48k, don't divide MCLK by 2
  sendBuffer[1] = 0x20;
  writeI2CData(sendBuffer, 2);

  sendBuffer[0] = 0x06; // master mode, i2s mode, 16bit word length
  sendBuffer[1] = 0x07;
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

void setupIRC() {
  SPI_I2S_ITConfig(SPI3, SPI_I2S_IT_TXE, ENABLE);
  NVIC_InitTypeDef NVIC_InitType;
  NVIC_InitType.NVIC_IRQChannel = SPI3_IRQn;
  NVIC_InitType.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitType.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitType.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitType);
}
