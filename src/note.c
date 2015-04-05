#include "note.h"

void initNote(struct Note* note, struct Note* next, struct Note* previous) {
	note->pitch = 0;
	note->phase = 0;
	note->next = next;
	note->previous = previous;
	note->state = 0;
}

struct Note* addNote(int pitch, struct Note* head) {
	struct Note* tail = head;
	while(tail->next != NULL) {
		tail = tail->next;
	}
	setADSROff(&tail->envelope, &tail->state);

	if (tail != head) {
		tail->previous->next = NULL;
		tail->previous = NULL;
		head->previous = tail;
		tail->next = head;
	}
	struct Note* newHead = tail;
	newHead->pitch = pitch;
	setADSROn(&newHead->envelope, &newHead->state);
	return newHead;
}

struct Note* removeNote(int pitch, struct Note* head) {
	// find the note with the corresponding pitch in the queue
	struct Note* ourNote = head;
	while(ourNote->pitch != pitch) {
		ourNote = ourNote->next;
	}
	// turn it off
	setADSROff(&ourNote->envelope, &ourNote->state);
	// move back in the queue, so all notes that are on are in front of it
	if (ourNote->next->state != 0) {
		struct Note* nextNote = ourNote->next;
		nextNote->previous = ourNote->previous;
		ourNote->previous->next = nextNote;
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