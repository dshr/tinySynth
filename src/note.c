#include "note.h"

inline void initNote(struct Note* note, int numOfNotes) {
	note->pitch = 0;
	note->state = 0;
	note->position = numOfNotes;
}

inline int addNote(int pitch, struct Note notes[], int numOfNotes) {
	int i;
	int newHead = 0;
	for (i = 0; i < numOfNotes; i++) {
		if (notes[i].position == numOfNotes || notes[i].pitch == pitch) {
			notes[i].pitch = pitch;
			notes[i].position = 0;
			newHead = i;
			break;
		}
	}
	for (i = 0; i < numOfNotes; i++) {
		if (notes[i].position < numOfNotes) notes[i].position++;
	}
	return newHead;
}

inline int removeNote(int pitch, struct Note notes[], int numOfNotes) {
	int i, j;
	struct Note* ourNote = NULL;
	for (i = 0; i < numOfNotes; i++) {
		if (notes[i].pitch == pitch) {
			ourNote = &notes[i];
			int lastActiveNotePosition = ourNote->position;
			for (j = 0; j < numOfNotes; j++) {
				if (notes[j].position > ourNote->position && notes[j].state == 1) {
					if (lastActiveNotePosition < notes[j].position)
						lastActiveNotePosition = notes[j].position;
					notes[j].position--;
				}
			}
			ourNote->position = lastActiveNotePosition;
			ourNote->state = 0;
		}
	}
	int newHead = 0;
	for (i = 0; i < NOTES; i++) {
		if (notes[i].position == 1) {
			newHead = i;
			break;
		}
	}
	return newHead;
}
