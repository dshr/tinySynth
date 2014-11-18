#include "main.h"
#include <math.h>

#define PI 3.14159265f
#define SEMITONE 1.0594630943592953
#define BUFFER_LENGTH 16
#define FREQ_DIV 3
#define SAMPLING_FREQ 48000

#define AMPLITUDE 32000
#define A_FOUR 440.0

float inverse_sampling_freq;
float mtof[128];

int16_t audioBuffer[BUFFER_LENGTH];

float phase;
float osc1_note;

int main(){
  // enable the FPU
  *((volatile unsigned long*)0xE000ED88) = 0xF << 20;

  // do stuff
  int16_t sample;

  setupPLL();
  setupClocks();
  setupGPIO();

  phase = 0.0f;
  osc1_note = 69.0f;
  inverse_sampling_freq = (float) FREQ_DIV / SAMPLING_FREQ;

  GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
  fillInBuffer();
  GPIO_ToggleBits(GPIOD, GPIO_Pin_13);

  setupI2C();
  setupCS32L22();
  setupI2S();

  // fill in the midi note lookup table
  int i;
  mtof[69] = 440.0f;
  for (i = 70; i < 128; i++){
    mtof[i] = (float) SEMITONE * mtof[i-1];
  }
  for (i = 68; i >= 0; i--){
    mtof[i] = (float) mtof[i+1] / SEMITONE;
  }

  i = 0;
  while(1){
    while(!SPI_I2S_GetFlagStatus(SPI3, SPI_FLAG_TXE));
    sample = audioBuffer[i/2];
    SPI_I2S_SendData(SPI3, sample);
    i++;
    if (i == BUFFER_LENGTH*2){
      GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
      i = 0;
      fillInBuffer();
    }
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) != 0){
      osc1_note += 0.00005f;
      if (osc1_note > 127.0f){
        osc1_note = 127.0f;
      }
    }
    else {
      osc1_note = 69.0f;
    }
  }
  return 0;
}

void fillInBuffer() {
  int i;
  GPIO_ToggleBits(GPIOD, GPIO_Pin_14);

  for (i = 0; i < BUFFER_LENGTH; i++)
  {
    // generate sin
    float sample = AMPLITUDE * sawtooth(phase);
    audioBuffer[i] = (int16_t) sample;

    phase += (float) 2 * PI * mtof[(int)osc1_note] * inverse_sampling_freq;
    if (phase > (float) 2 * PI) //wrap around
    {
      phase -= (float) 2 * PI;
      GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
    }
  }
}

float square(float phase){
  if (phase <= PI){
    return 1.0f;
  } else {
    return -1.0f;
  }
}

float sawtooth(float phase){
  return (float) phase/PI - 1;
}
