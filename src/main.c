#include "main.h"
#include <math.h>

#define PI 3.14159265f
#define SEMITONE 1.0594630943592953
#define BUFFER_LENGTH 2
#define SAMPLING_FREQ 48000

#define AMPLITUDE 32000
#define A_FOUR 440.0f

float inverseSamplingFrequency;
float mtof[128];
float sineWaveTable[513];

int16_t audioBuffer[BUFFER_LENGTH];

float osc1_phase;
float osc1_note;

float osc2_phase;
float osc2_note;

int main(){
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

  // fill in the midi note lookup table
  int i;
  mtof[69] = A_FOUR;
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

  GPIO_SetBits(GPIOD, GPIO_Pin_12);
  fillInBuffer();
  GPIO_SetBits(GPIOD, GPIO_Pin_13);

  setupI2C();
  setupCS32L22();
  setupI2S();

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
      osc1_note -= 0.00005f;
      if (osc1_note < 69.0f){
        osc1_note = 69.0f;
      }
      osc2_note -= 0.00005f;
      if (osc2_note < 76.0f){
        osc2_note = 76.0f;
      }
      GPIO_ResetBits(GPIOD, GPIO_Pin_15);
    }
  }
  return 0;
}

inline void fillInBuffer() {
  int i;
  GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
  float sample, phaseIncrement;

  for (i = 0; i < BUFFER_LENGTH; i++)
  {
    sample = 0;

    phaseIncrement = getPhaseIncrement(osc1_note);
    sample += sawtooth(osc1_phase, phaseIncrement);
    incrementPhase(&osc1_phase, phaseIncrement);

    phaseIncrement = getPhaseIncrement(osc2_note);
    sample += sawtooth(osc2_phase, phaseIncrement);
    incrementPhase(&osc2_phase, phaseIncrement);

    sample *= 0.5f * AMPLITUDE;

    audioBuffer[i] = (int16_t) sample;
  }
}

inline float polyBlep(float phase, float phaseIncrement){
  if (phase < phaseIncrement) {
      phase /= phaseIncrement;
      return phase+phase - phase*phase - 1;
  } else if (phase > 1.0 - phaseIncrement) {
      phase = (phase - 1.0) / phaseIncrement;
      return phase*phase + phase+phase + 1;
  } else {
    return 0.0;
  }
}

inline float getPhaseIncrement(float note){
  return getInterpolatedValue(note, mtof) * inverseSamplingFrequency;
}

inline void incrementPhase(float* phase, float increment){
  *phase += increment;
  if (*phase > 1.0f) //wrap around
  {
    *phase -= (float) 1.0f;
  }
}

inline float sine(float phase, float phaseIncrement){
  float wavetablePhase = 512 * phase; // the sineWaveTable goes from 0 to
  // 511, so when phase == 1, we want it to be that and not 513.
  return getInterpolatedValue(wavetablePhase, sineWaveTable);
}

inline float square(float phase, float phaseIncrement){
  float value = 0;
  if (phase <= 0.5f){
    value = 1.0f;
  } else {
    value = -1.0f;
  }
  value += polyBlep(phase, phaseIncrement);
  incrementPhase(&phase, 0.5f);
  value -= polyBlep(phase, phaseIncrement);
  return value;
}

inline float sawtooth(float phase, float phaseIncrement){
  return (float) 2 * phase - 1 - polyBlep(phase, phaseIncrement);
}

inline float getInterpolatedValue(float value, float* array){
  int roundedValue = (int)value;
  float decimalPart = value - roundedValue;
  if (decimalPart > 0.0f){
    float diff = array[roundedValue + 1] - array[roundedValue];
    return array[roundedValue] + (decimalPart * diff);
  } else {
    return array[roundedValue];
  }
}
