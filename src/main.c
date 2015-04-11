#include "main.h"

#define NOTES 6

float samplingPeriod;
float mtof[128];
float sineWaveTable[513];

float *currentBuffer;
float *offBuffer;

float buffer1[BUFFER_LENGTH];
float buffer2[BUFFER_LENGTH];

float lfo1_phase;
float lfo1_frequency;

struct Note notes[NOTES];
float inverseNumberOfNotes = (float) 1 / NOTES;
int monoMode = 0;

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

	for (i = 0; i < NOTES; i++) {
		initNote(&notes[i], NOTES);
		initADSR(&notes[i].ampEnvelope, 1000, 50000, 0.8, 60000);
		initADSR(&notes[i].filterEnvelope, 1000, 50000, 0.8, 60000);
		initFilter(&notes[i].filter, 2000.0f, 3.0f, 4.0f);
	}

	level = 0.1;

	fillInBuffer();
	swapBuffers();

	setupI2C();
	setupCS32L22();
	setupIRC();
	setupI2S();
	setupUSART();

	GPIO_SetBits(GPIOD, GPIO_Pin_12);
	i = 0;
	while(1){
		if (offBufferIndex == 0) {
			fillInBuffer();
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
			float* temp = currentBuffer;
			currentBuffer = offBuffer;
			offBuffer = temp;
			currentBufferIndex = 0;
			offBufferIndex = 0;
		}
	}
}

void USART2_IRQHandler()
{
	if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE))
	{
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
		if (messageCounter > 2) {
			if (midiMessage[0] > 175) {
				int i;
				switch (midiMessage[1]){
					case 10:
						if (midiMessage[2] == 127)
							monoMode = 1;
						else
							monoMode = 0;
						break;
					case 67:
						for (i = 0; i < NOTES; i++)
							setAttack(&notes[i].ampEnvelope, midiMessage[2]*5000);
						break;
					case 68:
						for (i = 0; i < NOTES; i++)
							setDecay(&notes[i].ampEnvelope, midiMessage[2]*5000);
						break;
					case 69:
						for (i = 0; i < NOTES; i++)
							setSustain(&notes[i].ampEnvelope, (float)midiMessage[2] / 127.0f);
						break;
					case 70:
						for (i = 0; i < NOTES; i++)
							setRelease(&notes[i].ampEnvelope, midiMessage[2]*5000);
						break;
					case 72:
						for (i = 0; i < NOTES; i++)
							setDrive(&notes[i].filter,((float)midiMessage[2] / 127.0f) * 20.0f);
						break;
					case 73:
						for (i = 0; i < NOTES; i++)
							setFrequency(&notes[i].filter,((float)midiMessage[2] / 127.0f) * 12000.f);
						break;
					case 74:
						for (i = 0; i < NOTES; i++)
							setResonance(&notes[i].filter,((float)midiMessage[2] / 127.0f) * 4.0f);
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
				addNote(midiMessage[1], notes, NOTES);
				GPIO_SetBits(GPIOD, GPIO_Pin_15);
			} else if (midiMessage[0] < 144) {
				removeNote(midiMessage[1], notes, NOTES);
				GPIO_ResetBits(GPIOD, GPIO_Pin_15);
			}
			GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
			messageCounter = 0;
		}
	}
}

inline void fillInBuffer() {
	int i;
	GPIO_ToggleBits(GPIOD, GPIO_Pin_14);
	float sample, phaseIncrement, lfo1_value;

	struct Note* theNote = NULL;
	if (monoMode) {
		for (i = 0; i < NOTES; i++) {
			if (notes[i].position == 1) {
				theNote = &notes[i];
				break;
			}
		}
	}

	while (offBufferIndex < BUFFER_LENGTH)
	{
		sample = 0;
		// run LFO's
		phaseIncrement = getPhaseIncrementFromFrequency(lfo1_frequency);
		lfo1_value = sine(lfo1_phase, phaseIncrement);
		incrementPhase(&lfo1_phase, phaseIncrement);
		if (monoMode && theNote != NULL) {
			phaseIncrement = getPhaseIncrementFromMIDI(theNote->pitch +
				(vibratoAmount * lfo1_value) + 0.41f);
			sample += square(theNote->phase, phaseIncrement,
				pulseWidthModAmount * lfo1_value) * getADSRLevel(&theNote->ampEnvelope);
			incrementPhase(&theNote->phase, phaseIncrement);
			filterSample(&theNote->filter, &sample);
			runADSR(&theNote->ampEnvelope, &theNote->state);
		} else {
			for (i = 0; i < NOTES; i++) {
				phaseIncrement = getPhaseIncrementFromMIDI(notes[i].pitch +
					(vibratoAmount * lfo1_value) + 0.41f);
				float noteSample = square(notes[i].phase, phaseIncrement,
					pulseWidthModAmount * lfo1_value) * getADSRLevel(&notes[i].ampEnvelope);
				incrementPhase(&notes[i].phase, phaseIncrement);
				filterSample(&notes[i].filter, &noteSample);
				runADSR(&notes[i].ampEnvelope, &notes[i].state);
				sample += noteSample;
			}
		}
		sample *= inverseNumberOfNotes;
		sample *= level;
		sample *= AMPLITUDE;

		offBuffer[offBufferIndex] = sample;
		offBufferIndex++;
	}
}

inline void swapBuffers() {
	float* temp = currentBuffer;
	currentBuffer = offBuffer;
	offBuffer = temp;
	currentBufferIndex = 0;
	offBufferIndex = 0;
}

inline float polyBlep(float phase, float phaseIncrement) {
	float inversePhaseIncrement = 1.0f / phaseIncrement;
	if (phase < phaseIncrement) {
			phase *= inversePhaseIncrement;
			return phase+phase - phase*phase - 1;
	} else if (phase > 1.0f - phaseIncrement) {
			phase = (phase - 1.0f) * inversePhaseIncrement;
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
