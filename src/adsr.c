#include "adsr.h"

void initADSR(struct ADSR* adsr, unsigned a, unsigned d, float s, unsigned r){
	adsr->attackTime = a;
	adsr->decayTime = d;
	adsr->sustainLevel = s;
	adsr->releaseTime = r;
	adsr->index = 0;
	adsr->level = 0.00000000001f;
	adsr->offLevel = 0;
}

void setAttack(struct ADSR* adsr, unsigned a){
	adsr->attackTime = a;
}
void setDecay(struct ADSR* adsr, unsigned d){
	adsr->decayTime = d;
}
void setSustain(struct ADSR* adsr, float s){
	if (s > 1) s = 1;
	adsr->sustainLevel = s;
}
void setRelease(struct ADSR* adsr, unsigned r){
	adsr->releaseTime = r;
}

void setADSROn(struct ADSR* adsr, int* on){
	if (!*on) {
		adsr->index = 0;
		adsr->level = 0.00000000001f;
		adsr->offLevel = 0;
		*on = 1;
	}
}

void setADSROff(struct ADSR* adsr){
	adsr->index = 0;
	adsr->offLevel = adsr->level;
}

void runADSR(struct ADSR* adsr, int* on){
	adsr->index++;
	if (*on) {
		if (adsr->index <= adsr->attackTime){
			adsr->level = (float) adsr->index / (float) adsr->attackTime;
		}
		if ((adsr->index > adsr->attackTime) && (adsr->attackTime + adsr->decayTime)){
			adsr->level = 1 - ((1 - adsr->sustainLevel)
				*(adsr->index - adsr->attackTime))/(float)adsr->decayTime;
		}
		if (adsr->index >= (adsr->attackTime + adsr->decayTime)) {
			adsr->index = adsr->attackTime + adsr->decayTime + 1;
			adsr->level = adsr->sustainLevel;
		}
	} else {
		adsr->level = adsr->offLevel - ((float) adsr->index / (float)adsr->releaseTime);
		if (adsr->level <= 0.01) adsr->level =  0.00000000001;
	}
}

float getADSRLevel(struct ADSR* adsr){
	return adsr->level;
}
