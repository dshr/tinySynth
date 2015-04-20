#include "filter.h"

#define OVERSAMPLING 2

float tanhLookUpTable[6001];

void fillInTanhLookUpTable() {
	int i;
	for (i = 0; i < 6001; i++)
		tanhLookUpTable[i] = tanhf((float) (i * 0.001f) - 3.0f);
}

inline void initFilter(struct Filter* filter, float f, float r, float d) {
	int i;
	filter->y_a = 0.0f;
	filter->y_b = 0.0f;
	filter->y_c = 0.0f;
	filter->y_d = 0.0f;
	filter->y_d_1 = 0.0f;
	filter->g = 0.0f;

	setFrequency(filter, f);
	setResonance(filter, r);

	filter->oversamplingCoefficients[0] = 4.01230529e-03f;
	filter->oversamplingCoefficients[1] = -2.19517848e-18f;
	filter->oversamplingCoefficients[2] = -2.33215245e-02f;
	filter->oversamplingCoefficients[3] = -3.34765041e-02f;
	filter->oversamplingCoefficients[4] = 7.16025226e-02f;
	filter->oversamplingCoefficients[5] = 2.82377526e-01f;
	filter->oversamplingCoefficients[6] = 3.97611349e-01f;
	filter->oversamplingCoefficients[7] = 2.82377526e-01f;
	filter->oversamplingCoefficients[8] = 7.16025226e-02f;
	filter->oversamplingCoefficients[9] = -3.34765041e-02f;
	filter->oversamplingCoefficients[10] = -2.33215245e-02f;
	filter->oversamplingCoefficients[11] = -2.19517848e-18f;
	filter->oversamplingCoefficients[12] = 4.01230529e-03f;

	for (i = 0; i < 13; i++) {
		filter->oversamplingFilter[i] = 0.0f;
	}
}

inline float polytan(float x) {
	return x + (0.33333333333f * x * x * x)
					 + (0.13333333333f * x * x * x * x * x)
					 + (0.05396825397f * x * x * x * x * x * x * x);
}

inline float polyexp(float x) {
	return 1 + x
					 + (0.5f * x * x)
					 + (0.1666666667f * x * x * x)
					 + (0.04166666667f * x * x * x * x);
}

inline float tanhLookUp(float x) {
	if (x >= 3.0f) return 0.99f;
	if (x <= -3.0f) return -0.99f;
	return getInterpolatedValue((x + 3.0f) * 1000.0f, tanhLookUpTable);
}

void setFrequency(struct Filter* filter, float f) {
	if (f > 12000.0f) f = 12000.0f;
	if (f < 0.0f) f = 0.0f;
	filter->frequency = f;
	filter->g = 1 - polyexp(-2 * polytan(2 * PI * filter->frequency/(OVERSAMPLING * SAMPLING_FREQ)));
}

void setResonance(struct Filter* filter, float r) {
	if (r > 4.0f) r = 4.0f;
	if (r < 0.0f) r = 0.0f;
	filter->resonance = r;
}

void filterSample(struct Filter* f, float* sample) {
	int i, j;
	float s = *sample;

	for (i = 0; i < OVERSAMPLING; i++) {
		if (i > 0) s = 0.0f;
		f->y_a = f->y_a + f->g *
			(tanhLookUp(s - f->resonance *
				((f->y_d_1 + f->y_d)*0.5f) - tanhLookUp(f->y_a)));
		f->y_b = f->y_b + f->g * (tanhLookUp(f->y_a) - tanhLookUp(f->y_b));
		f->y_c = f->y_c + f->g * (tanhLookUp(f->y_b) - tanhLookUp(f->y_c));

		f->y_d_1 = f->y_d;

		f->y_d = f->y_d + f->g * (tanhLookUp(f->y_c) - tanhLookUp(f->y_d));

		f->oversamplingFilter[0] = f->y_d;
		s = 0.0f;
		for (j = 0; j < 13; j++) {
			s += f->oversamplingCoefficients[j] * f->oversamplingFilter[j];
		}
		for (j = 1; j < 13; j++) {
			f->oversamplingFilter[j] = f->oversamplingFilter[j-1];
		}
	}
	*sample = s;
}
