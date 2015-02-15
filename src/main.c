#include "main.h"

#define PI 3.14159265f
#define SEMITONE 1.0594630943592953
#define BUFFER_LENGTH 2
#define SAMPLING_FREQ 48000

#define AMPLITUDE 32000
#define A_FOUR 440.0f

float samplingPeriod;
float mtof[128];
float sineWaveTable[513];

int16_t *currentBuffer;
int16_t *offBuffer;

int16_t buffer1[BUFFER_LENGTH];
int16_t buffer2[BUFFER_LENGTH];

float osc1_phase;
float osc1_note;

float osc2_phase;
float osc2_note;

float osc3_phase;
float osc3_note;

float lfo1_phase;
float lfo1_frequency;

struct ADSR adsr1;
int adsr1_on;

int currentBufferIndex;
int offBufferIndex;

int main(){

	setupPLL();
	setupClocks();
	setupGPIO();

	currentBuffer = &buffer1;
	offBuffer = &buffer2;

	currentBufferIndex = 0;
	offBufferIndex = 0;

	osc1_phase = 0.0f;
	osc1_note = 57.0f;

	osc2_phase = 0.0f;
	osc2_note = 64.0f;

	osc3_phase = 0.0f;
	osc3_note = 68.0f;

	lfo1_phase = 0.0f;
	lfo1_frequency = 0.5f;

	samplingPeriod = (float) 1 / SAMPLING_FREQ;

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
		sineWaveTable[i] = cosf(2 * PI * i/512);
	}

	GPIO_SetBits(GPIOD, GPIO_Pin_12);
	fillInBuffer();
	swapBuffers();
	GPIO_SetBits(GPIOD, GPIO_Pin_13);

	setupI2C();
	setupCS32L22();
	setupIRC();
	setupI2S();

	initADSR(&adsr1, 2000, 50000, 0.0, 300000);
	adsr1_on = 0;

	while(1){
		if (offBufferIndex == 0) {
			fillInBuffer();
		}

		if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) != 0){
			setADSROn(&adsr1, &adsr1_on);
			GPIO_ToggleBits(GPIOD, GPIO_Pin_15);
		}
		else {
			setADSROff(&adsr1, &adsr1_on);
			GPIO_ResetBits(GPIOD, GPIO_Pin_15);
		}
	}
	return 0;
}

void SPI3_IRQHandler()
{
	if (SPI_I2S_GetFlagStatus(SPI3, SPI_FLAG_TXE))
	{
		int16_t sample;
		sample = currentBuffer[currentBufferIndex/2];
		SPI_I2S_SendData(SPI3, sample);
		currentBufferIndex++;
		if (currentBufferIndex == BUFFER_LENGTH*2){
			GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
			swapBuffers();
		}
	}
}

inline void fillInBuffer() {
	GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
	float sample, phaseIncrement, lfo1_value;

	while (offBufferIndex < BUFFER_LENGTH)
	{
		sample = 0;

		// run LFO's
		phaseIncrement = getPhaseIncrementFromFrequency(lfo1_frequency);
		lfo1_value = sine(lfo1_phase, phaseIncrement);
		incrementPhase(&lfo1_phase, phaseIncrement);

		phaseIncrement = getPhaseIncrementFromMIDI(osc1_note);
		sample += square(osc1_phase, phaseIncrement, 0.5f * lfo1_value);
		incrementPhase(&osc1_phase, phaseIncrement);

		phaseIncrement = getPhaseIncrementFromMIDI(osc2_note);
		sample += square(osc2_phase, phaseIncrement, 0.5f * lfo1_value);
		incrementPhase(&osc2_phase, phaseIncrement);

		phaseIncrement = getPhaseIncrementFromMIDI(osc3_note);
		sample += square(osc3_phase, phaseIncrement, 0.5f * lfo1_value);
		incrementPhase(&osc3_phase, phaseIncrement);

		sample *= 0.3 * AMPLITUDE * getADSRLevel(&adsr1);

		runADSR(&adsr1, &adsr1_on);

		offBuffer[offBufferIndex] = (int16_t) sample;
		offBufferIndex++;
	}
}

inline void swapBuffers() {
	int16_t* temp = currentBuffer;
	currentBuffer = offBuffer;
	offBuffer = temp;
	currentBufferIndex = 0;
	offBufferIndex = 0;
}

inline float polyBlep(float phase, float phaseIncrement){
	if (phase < phaseIncrement) {
			phase /= phaseIncrement;
			return phase+phase - phase*phase - 1;
	} else if (phase > 1.0f - phaseIncrement) {
			phase = (phase - 1.0f) / phaseIncrement;
			return phase*phase + phase+phase + 1;
	} else {
		return 0.0f;
	}
}

inline float getPhaseIncrementFromMIDI(float note){
	return getInterpolatedValue(note, mtof) * samplingPeriod;
}

inline float getPhaseIncrementFromFrequency(float frequency){
	return frequency * samplingPeriod;
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

inline float square(float phase, float phaseIncrement, float pulseWidthMod){
	float value = 0;
	if (pulseWidthMod > 0.99f) {
		pulseWidthMod = 0.99f;
	} else if (pulseWidthMod < -0.99f) {
		pulseWidthMod = -0.99f;
	}
	float pulseWidth = 0.5f + 0.5f * pulseWidthMod;
	if (phase <= pulseWidth){
		value = 1.0f;
	} else {
		value = -1.0f;
	}
	value += polyBlep(phase, phaseIncrement);
	incrementPhase(&phase, 1-pulseWidth);
	value -= polyBlep(phase, phaseIncrement);
	return value;
}

inline float sawtooth(float phase, float phaseIncrement){
	return (float) 1 - 2 * phase + polyBlep(phase, phaseIncrement);
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
