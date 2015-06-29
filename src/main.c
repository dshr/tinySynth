#include "main.h"

//
// The PolyBLEP generation is adapter from the addendum of Phaseshaping
// oscillator algorithms for musical sound synthesis by
// Kleimola J., Lazzarini V., Timoney J., and Välimäki, V.
//
// http://research.spa.aalto.fi/publications/papers/smc2010-phaseshaping/
// accessed on 13.12.2014
//

float samplingPeriod;
float mtof[128];
float sineWaveTable[513];

float *currentBuffer;
float *offBuffer;

float buffer1[BUFFER_LENGTH];
float buffer2[BUFFER_LENGTH];

float lfo1_phase;
float lfo1_frequency;
float lfo1_pulseWidthModAmount;
float lfo1_vibratoAmount;

float lfo2_phase;
float lfo2_frequency;
float lfo2_vibratoAmount;
float lfo2_filterModAmount;

struct Note notes[NOTES];
int headPos = 0;
float tracking = 1.0f;

float osc_phase[3];
float osc_volume[3];
int osc_coarse[3];
float osc_fine[3];
int osc_waveform[3];

float pitch = 69.0f;
int portamentoCounter = 0;
int portamento = 3000;
float portamentoModifier;
float pitchBend = 0.0f;
struct ADSR ampEnvelope;
struct Filter filter;
struct ADSR filterEnvelope;
float frequency;
float filterEnvelopeDepth = 2000.0f;

float level;

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

	fillInTanhLookUpTable();

	for (i = 0; i < NOTES; i++) {
		initNote(&notes[i], NOTES);
	}

	for (i = 0; i < 3; i++) {
		osc_phase[i] = 0.0f;
		osc_volume[i] = 1.0f;
		osc_coarse[i] = 0.0f;
		osc_fine[i] = 0.0f;
		osc_waveform[i] = 127;
	}

	initFilter(&filter, 2000.0f, 0.0f, 4.0f);
	initADSR(&ampEnvelope, 0.0f, 0.0f, 1.0f, 1000.0f);
	initADSR(&filterEnvelope, 10000.0f, 0.0f, 1.0f, 1000.0f);

	level = 0.1f;

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
				 (message > 175 && message < 192)	||
				 (message > 223 && message < 240)))  {
			midiMessage[messageCounter] = message;
			messageCounter++;
		} else if (messageCounter > 0) {
			midiMessage[messageCounter] = message;
			messageCounter++;
		}
		if (messageCounter > 2) {
			if (midiMessage[0] > 223) {
				pitchBend = (midiMessage[2] * 128 + midiMessage[1] - 8192) * 0.00006103515f * 24.0f;
			} else if (midiMessage[0] > 175) {
				switch (midiMessage[1]){
					case 47:
						osc_volume[0] = (float)midiMessage[2] / 127.0f;
						break;
					case 48:
						osc_coarse[0] = (((float)midiMessage[2] / 63.0f) - 1.0f) * 24;
						break;
					case 49:
						osc_fine[0] = ((float)midiMessage[2] / 63.0f) - 1.0f;
						break;
					case 50:
						osc_waveform[0] = midiMessage[2];
						break;
					case 51:
						osc_volume[1] = (float)midiMessage[2] / 127.0f;
						break;
					case 52:
						osc_coarse[1] = (((float)midiMessage[2] / 63.0f) - 1.0f) * 24;
						break;
					case 53:
						osc_fine[1] = ((float)midiMessage[2] / 63.0f) - 1.0f;
						break;
					case 54:
						osc_waveform[1] = midiMessage[2];
						break;
					case 55:
						osc_volume[2] = (float)midiMessage[2] / 127.0f;
						break;
					case 56:
						osc_coarse[2] = (((float)midiMessage[2] / 63.0f) - 1.0f) * 24;
						break;
					case 57:
						osc_fine[2] = ((float)midiMessage[2] / 63.0f) - 1.0f;
						break;
					case 58:
						osc_waveform[2] = midiMessage[2];
						break;
					case 59:
						portamento = ((float)midiMessage[2] / 127.0f) * 10000;
						break;
					case 60:
						lfo2_frequency = (float)midiMessage[2] / 12.0f;
						break;
					case 61:
						lfo2_vibratoAmount = (float)midiMessage[2] / 127.0f;
						break;
					case 62:
						lfo2_filterModAmount = ((float)midiMessage[2] / 127.0f) * 12000.0f;
						break;
					case 63:
						setAttack(&filterEnvelope, midiMessage[2]*5000);
						break;
					case 64:
						setDecay(&filterEnvelope, midiMessage[2]*5000);
						break;
					case 65:
						setSustain(&filterEnvelope, (float)midiMessage[2] / 127.0f);
						break;
					case 66:
						setRelease(&filterEnvelope, midiMessage[2]*50000);
						break;
					case 67:
						setAttack(&ampEnvelope, midiMessage[2]*5000);
						break;
					case 68:
						setDecay(&ampEnvelope, midiMessage[2]*5000);
						break;
					case 69:
						setSustain(&ampEnvelope, (float)midiMessage[2] / 127.0f);
						break;
					case 70:
						setRelease(&ampEnvelope, midiMessage[2]*50000);
						break;
					case 71:
						filterEnvelopeDepth = (((float)midiMessage[2] - 64.0f) / 64.0f) * 12000.f;
						break;
					case 72:
						tracking = (float)midiMessage[2] / 127.0f;
						break;
					case 73:
						frequency = ((float)midiMessage[2] / 127.0f) * 12000.f;
						break;
					case 74:
						setResonance(&filter,((float)midiMessage[2] / 127.0f) * 4.0f);
						break;
					case 75:
						lfo1_pulseWidthModAmount = (float)midiMessage[2] / 127.0f;
						break;
					case 76:
						lfo1_vibratoAmount = (float)midiMessage[2] / 127.0f;
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
				headPos = addNote(midiMessage[1], notes, NOTES);
				setADSROn(&ampEnvelope, &notes[headPos].state);
				notes[headPos].state = 0;
				setADSROn(&filterEnvelope, &notes[headPos].state);
				GPIO_SetBits(GPIOD, GPIO_Pin_15);
				portamentoModifier = (notes[headPos].pitch - pitch) / (float) portamento;
				portamentoCounter = 0;
			} else if (midiMessage[0] < 144) {
				headPos = removeNote(midiMessage[1], notes, NOTES);
				if (!notes[headPos].state) {
					setADSROff(&ampEnvelope);
					setADSROff(&filterEnvelope);
				} else {
					portamentoModifier = (notes[headPos].pitch - pitch) / (float) portamento;
					portamentoCounter = 0;
				}
				GPIO_ResetBits(GPIOD, GPIO_Pin_15);

			}
			GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
			messageCounter = 0;
		}
	}
}

inline void fillInBuffer() {
	GPIO_ResetBits(GPIOD, GPIO_Pin_14);
	float sample, phaseIncrement, lfo1_value, lfo2_value;
	int i;

	while (offBufferIndex < BUFFER_LENGTH)
	{
		sample = 0.0f;
		// run LFO's
		phaseIncrement = getPhaseIncrementFromFrequency(lfo1_frequency);
		lfo1_value = sine(lfo1_phase, phaseIncrement);
		incrementPhase(&lfo1_phase, phaseIncrement);
		phaseIncrement = getPhaseIncrementFromFrequency(lfo2_frequency);
		lfo2_value = sine(lfo2_phase, phaseIncrement);
		incrementPhase(&lfo2_phase, phaseIncrement);

		if (portamentoCounter >= portamento) {
			portamentoModifier = 0.0f;
			pitch = notes[headPos].pitch;
		}

		pitch += portamentoModifier;
		portamentoCounter++;

		for (i = 0; i < 3; i++)
		{
			phaseIncrement = getPhaseIncrementFromMIDI(pitch + pitchBend +
				(lfo1_vibratoAmount * lfo1_value) + (lfo2_vibratoAmount * lfo2_value)
				+ osc_coarse[i] + osc_fine[i]);
			if (osc_waveform[i] > 84) {
				sample += osc_volume[i] * square(osc_phase[i], phaseIncrement, lfo1_pulseWidthModAmount * lfo1_value);
			} else if (osc_waveform[i] > 42) {
				sample += osc_volume[i] * sawtooth(osc_phase[i], phaseIncrement);
			} else {
				sample += osc_volume[i] * sine(osc_phase[i], phaseIncrement);
			}
			incrementPhase(&osc_phase[i], phaseIncrement);
		}

		setFrequency(&filter,
								 frequency
								 + (filterEnvelopeDepth * getADSRLevel(&filterEnvelope))
								 + (lfo2_filterModAmount * lfo2_value)
								 + (tracking * getInterpolatedValue(pitch + pitchBend, mtof)));

		sample *= 0.33f;
		filterSample(&filter, &sample);

		sample *= level * getADSRLevel(&ampEnvelope);
		sample *= AMPLITUDE;

		runADSR(&ampEnvelope, &notes[headPos].state);
		runADSR(&filterEnvelope, &notes[headPos].state);

		offBuffer[offBufferIndex] = sample;
		offBufferIndex++;
	}
	GPIO_SetBits(GPIOD, GPIO_Pin_14);
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
	if (note > 127.0f) {
		int difference = note - 127;
		int octaves = (difference / 12) + 1;
		return getInterpolatedValue(note - (12.0f * octaves), mtof)
																* 2 * octaves * samplingPeriod;
	}
	if (note < 0.0f) {
		int octaves = ((note / 12) + 1) * -1;
		return getInterpolatedValue(note + (12.0f * octaves), mtof)
																* 0.5f * octaves * samplingPeriod;
	}
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
