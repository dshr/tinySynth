#include "note.h"

void initNote(struct Note* note, struct Note* next, struct Note* previous) {
	note->pitch = 0;
	note->phase = 0;
	note->next = next;
	note->previous = previous;
	note->state = 0;
}

struct Note* addNote(int pitch, struct Note* head, int numOfNotes) {
	int i;
	struct Note* tail = head;
	for (i = 0; i < numOfNotes; i++) {
		if (tail->pitch == pitch) break;
		if (i < numOfNotes - 1) tail = tail->next;
	}
	if (tail != head) {
		tail->previous->next = tail->next;
		if (tail->next != NULL) tail->next->previous = tail->previous;
		tail->previous = NULL;
		head->previous = tail;
		tail->next = head;
	}
	struct Note* newHead = tail;
	newHead->pitch = pitch;
	setADSROn(&newHead->envelope, &newHead->state);
	return newHead;
}

struct Note* removeNote(int pitch, struct Note* head, int numOfNotes) {
	// try to find the note with the corresponding pitch in the queue
	int i;
	struct Note* ourNote = NULL;
	struct Note* nextNote = head;
	for (i = 0; i < numOfNotes; i++) {
		if (nextNote->pitch == pitch) ourNote = nextNote;
		if (i < numOfNotes - 1) nextNote = nextNote->next;
	}
	// of there is no such note, do nothing
	if (ourNote == NULL) return head;
	// otherwise turn it off
	setADSROff(&ourNote->envelope, &ourNote->state);
	// move it back in the queue, so all notes that are on are in front of it
	if (ourNote->next->state != 0) {
		nextNote = ourNote->next;
		nextNote->previous = ourNote->previous;
		if (ourNote->previous != NULL) ourNote->previous->next = nextNote;
		else head = nextNote;
		while (nextNote->next != NULL && nextNote->next->state != 0) {
			nextNote = nextNote->next;
		}
		ourNote->previous = nextNote;
		if (nextNote->next != NULL) nextNote->next->previous = ourNote;
		ourNote->next = nextNote->next;
		nextNote->next = ourNote;
	}
	return head;
}