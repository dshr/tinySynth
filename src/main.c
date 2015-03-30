#include "main.h"

float samplingPeriod;
float mtof[128];
float sineWaveTable[513];

float *currentBuffer;
float *offBuffer;

float buffer1[BUFFER_LENGTH];
float buffer2[BUFFER_LENGTH];

float osc1_phase;
float osc1_note;

float osc2_phase;
float osc2_note;

float osc3_phase;
float osc3_note;

float lfo1_phase;
float lfo1_frequency;

struct ADSR adsr1;
struct Filter filter;
int adsr1_on;
float level;
float pulseWidthModAmount;
float vibratoAmount;

int currentBufferIndex;
int offBufferIndex;

int messageCounter;
int midiMessage[3];

int main(){

	setupPLL();
	setupClocks();
	setupGPIO();

	currentBuffer = buffer1;
	offBuffer = buffer2;

	currentBufferIndex = 0;
	offBufferIndex = 0;

	messageCounter = 0;

	osc1_phase = 0.0f;
	osc2_phase = 0.0f;

	lfo1_phase = 0.0f;
	lfo1_frequency = 0.5f;

	samplingPeriod = (float) 1 / SAMPLING_FREQ;

	// fill in the midi note lookup table
	int i;
	mtof[69] = A_FOUR;
	for (i = 70; i < 128; i++){
		mtof[i] = SEMITONE * mtof[i-1];
	}
	for (i = 68; i >= 0; i--){
		mtof[i] = mtof[i+1] / SEMITONE;
	}

	// fill in the sine wavetable
	for (i = 0; i < 513; i++){
		sineWaveTable[i] = cosf(2 * PI * i/512);
	}

	GPIO_SetBits(GPIOD, GPIO_Pin_12);
	fillInBuffer();
	swapBuffers();

	setupI2C();
	setupCS32L22();
	setupIRC();
	setupI2S();
	setupUSART();

	initADSR(&adsr1, 1000, 50000, 0.8, 60000);
	initFilter(&filter, 2000.0f, 3.0f, 4.0f);
	adsr1_on = 0;
	setADSROff(&adsr1, &adsr1_on);
	level = 0.1;

	while(1){
		if (offBufferIndex == 0) {
			fillInBuffer();
		}
		if (messageCounter > 2) {
			if (midiMessage[0] > 175) {
				switch (midiMessage[1]){
					case 67:
						setAttack(&adsr1, midiMessage[2]*10000);
						break;
					case 68:
						setDecay(&adsr1, midiMessage[2]*10000);
						break;
					case 69:
						setSustain(&adsr1, (float)midiMessage[2] / 127.0f);
						break;
					case 70:
						setRelease(&adsr1, midiMessage[2]*10000);
						break;
					case 72:
						setDrive(&filter,((float)midiMessage[2] / 127.0f) * 20.0f);
						break;
					case 73:
						setFrequency(&filter,((float)midiMessage[2] / 127.0f) * 12000.f);
						break;
					case 74:
						setResonance(&filter,((float)midiMessage[2] / 127.0f) * 4.0f);
						break;
					case 75:
						pulseWidthModAmount = (float)midiMessage[2] / 127.0f;
						break;
					case 76:
						vibratoAmount = (float)midiMessage[2] / 127.0f;
						break;
					case 77:
						lfo1_frequency = (float)midiMessage[2] / 12.0f;
						break;
					case 78:
						level = (float)midiMessage[2] / 127.0f;
						break;
					default: break;
				}
			} else if (midiMessage[0] > 143) {
				osc1_note = midiMessage[1];
				setADSROn(&adsr1, &adsr1_on);
				GPIO_SetBits(GPIOD, GPIO_Pin_15);
			} else if (midiMessage[0] < 144 && osc1_note == midiMessage[1]) {
				setADSROff(&adsr1, &adsr1_on);
				GPIO_ResetBits(GPIOD, GPIO_Pin_15);
			}
			GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
		}
		GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0); // this is weird, stuff breaks without this :(
	}
	return 0;
}

void SPI3_IRQHandler()
{
	if (SPI_I2S_GetFlagStatus(SPI3, SPI_FLAG_TXE))
	{
		int16_t sample;
		sample = (int16_t) currentBuffer[currentBufferIndex/2];
		SPI_I2S_SendData(SPI3, sample);
		currentBufferIndex++;
		if (currentBufferIndex == BUFFER_LENGTH*2){
			GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
			swapBuffers();
		}
	}
}

void USART2_IRQHandler()
{
	if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE))
	{
		if (messageCounter > 2)	messageCounter = 0;
		int message = USART_ReceiveData(USART2);
		if (messageCounter == 0 &&
				((message > 127 && message < 160) ||
				 (message > 175 && message < 192))) {
			midiMessage[messageCounter] = message;
			messageCounter++;
		} else if (messageCounter > 0) {
			midiMessage[messageCounter] = message;
			messageCounter++;
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

		phaseIncrement = getPhaseIncrementFromMIDI(osc1_note + (vibratoAmount * lfo1_value) + 0.41f);
		sample += square(osc1_phase, phaseIncrement, pulseWidthModAmount * lfo1_value);
		incrementPhase(&osc1_phase, phaseIncrement);

		phaseIncrement = getPhaseIncrementFromMIDI(osc1_note  + (vibratoAmount * lfo1_value) + 0.41f);
		sample += sawtooth(osc2_phase, phaseIncrement);
		incrementPhase(&osc2_phase, phaseIncrement);

		sample *= level * getADSRLevel(&adsr1);

		runADSR(&adsr1, &adsr1_on);

		offBuffer[offBufferIndex] = sample;
		offBufferIndex++;
	}
	filterSamples(&filter, offBuffer);
	int i;
	for (i = 0; i < BUFFER_LENGTH; i++) {
		offBuffer[i] *= AMPLITUDE;
	}
}

inline void swapBuffers() {
	float* temp = currentBuffer;
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
