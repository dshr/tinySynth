#include "adsr.h"

#ifndef _NOTE
#define _NOTE

struct Note {
	float pitch;
	float phase;
	struct Note* next;
	struct Note* previous;
	struct ADSR envelope;
	int state;
};

void initNote(struct Note* note, struct Note* next, struct Note* previous);
struct Note* addNote(int pitch, struct Note* head);
struct Note* removeNote(int pitch, struct Note* head);
#endif
