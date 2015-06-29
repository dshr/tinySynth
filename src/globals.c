#include "globals.h"

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