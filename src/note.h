#include "adsr.h"
#include "filter.h"

#ifndef _NOTE
#define _NOTE

struct Note {
	float pitch;
	int state;
	int position;
};

void initNote(struct Note* note, int numOfNotes);
int addNote(int pitch, struct Note notes[], int numOfNotes);
int removeNote(int pitch, struct Note notes[], int numOfNotes);
#endif
