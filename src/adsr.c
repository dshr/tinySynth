#include "adsr.h"

void initADSR(struct ADSR* adsr, unsigned a, unsigned d, float s, unsigned r){
	setSustain(adsr, s);
	setAttack(adsr, a);
	setDecay(adsr, d);
	setRelease(adsr, r);
	adsr->index = 0;
	adsr->level = 0.00000000001f;
}

void setAttack(struct ADSR* adsr, unsigned a){
	if (a == 0) a = 1;
	adsr->attackTime = a;
}
void setDecay(struct ADSR* adsr, unsigned d){
	if (d == 0) d = 1;
	adsr->decayTime = d;
	adsr->decayMultiplier = 1.0f
	 + (logf(adsr->sustainLevel) - logf(1.0f)) / ((float)adsr->decayTime + 1);
}
void setSustain(struct ADSR* adsr, float s){
	if (s > 1.0f) s = 1.0f;
	if (s < 0.00000000001f) s = 0.00000000001f;
	adsr->sustainLevel = s;
	adsr->decayMultiplier = 1.0f
	 + (logf(adsr->sustainLevel) - logf(1.0f)) / ((float)adsr->decayTime + 1);
}
void setRelease(struct ADSR* adsr, unsigned r){
	if (r == 0) r = 1;
	adsr->releaseTime = r;
}

void setADSROn(struct ADSR* adsr, int* on){
	if (!*on) {
		adsr->index = 0;
		adsr->level = 0.00000000001f;
		*on = 1;
	}
}

void setADSROff(struct ADSR* adsr){
	adsr->releaseMultiplier = 1.0f
	 + (logf(0.00000000001f) - logf(adsr->level)) / ((float)adsr->releaseTime + 1);
}

void runADSR(struct ADSR* adsr, int* on){
	adsr->index++;
	if (*on) {
		if (adsr->index <= adsr->attackTime){
			adsr->level = (float) adsr->index / (float) adsr->attackTime;
		}
		if (adsr->index > adsr->attackTime){
			adsr->level *= adsr->decayMultiplier;
		}
		if (adsr->index >= (adsr->attackTime + adsr->decayTime)) {
			adsr->level = adsr->sustainLevel;
		}
	} else {
		adsr->level *= adsr->releaseMultiplier;
		if (adsr->level <= 0.01) adsr->level =  0.00000000001;
	}
}

float getADSRLevel(struct ADSR* adsr){
	return adsr->level;
}
