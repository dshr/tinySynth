#include "main.h"
#include <math.h>

#define PI 3.14159265f
#define SEMITONE 1.0594630943592953
#define BUFFER_LENGTH 4
#define SAMPLING_FREQ 48000

#define AMPLITUDE 31000
#define A_FOUR 440.0

float inverseSamplingFrequency;
float mtof[128];
float sineWaveTable[513];

int16_t audioBuffer[BUFFER_LENGTH];

float osc1_phase;
float osc1_note;

float osc2_phase;
float osc2_note;

int main(){
  // enable the FPU
  *((volatile unsigned long*)0xE000ED88) = 0xF << 20;

  // do stuff
  int16_t sample;

  setupPLL();
  setupClocks();
  setupGPIO();

  osc1_phase = 0.0f;
  osc1_note = 69.0f;

  osc1_phase = 0.0f;
  osc1_note = 76.0f;

  inverseSamplingFrequency = (float) 1 / SAMPLING_FREQ;

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

  // fill in the sine wavetable
  for (i = 0; i < 513; i++){
    sineWaveTable[i] = sinf(2 * PI * i/512);
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
      if (osc1_note > 120.0f){
        osc1_note = 120.0f;
      }
      osc2_note += 0.00005f;
      if (osc2_note > 127.0f){
        osc2_note = 127.0f;
      }
      GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
    }
    else {
      osc1_note = 69.0f;
      osc2_note = 76.0f;
      GPIO_ResetBits(GPIOD, GPIO_Pin_15);
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
    float sample = 0.5 * AMPLITUDE * (sine(osc1_phase) + sine(osc2_phase));
    audioBuffer[i] = (int16_t) sample;

    incrementPhase(&osc1_phase, osc1_note);
    incrementPhase(&osc2_phase, osc2_note);
  }
}

void incrementPhase(float* phase, float note){
  *phase += (float) 2 * PI *
      getInterpolatedValue(note, mtof) * inverseSamplingFrequency;
  if (*phase > (float) 2 * PI) //wrap around
  {
    *phase -= (float) 2 * PI;
  }
}

float sine(float phase){
  float wavetablePhase = 512 * phase/(2*PI); // the sineWaveTable goes from 0 to
  // 511, so when phase == 2pi, we want it to be that and not 513.
  return getInterpolatedValue(wavetablePhase, sineWaveTable);
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

float getInterpolatedValue(float value, float* array){
  int roundedValue = (int)value;
  float decimalPart = value - roundedValue;
  if (decimalPart > 0.0f){
    float diff = array[roundedValue + 1] - array[roundedValue];
    return array[roundedValue] + (decimalPart * diff);
  } else {
    return array[roundedValue];
  }
}
