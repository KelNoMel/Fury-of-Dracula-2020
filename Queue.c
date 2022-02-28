#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Game.h"
#include "Places.h"
#include "Map.h"
#include "Queue.h"

// Initializes the Queue
Queue newQueue (void) {
	Queue new = malloc (sizeof(Queue));
	new->head = NULL;
	new->tail = NULL;
	return new;
}

// Frees the Queue from memory
void dropQueue (Queue Q) {
	assert (Q != NULL);
	for (QueueNode *curr = Q->head, *next;curr != NULL; curr = next) {
		next = curr->next;
		free(curr);
	}
	free (Q);
}

// Adds a Queue Node containing a pid to the back of the queue
void QueueJoin (Queue Q, PlaceId p) {
	assert(Q != NULL);
	QueueNode *new = malloc (sizeof(QueueNode));
	assert (new != NULL);
	new->id = p;
	new->next = NULL;
	
	if (Q->head == NULL)
		Q->head = new;
	if (Q->tail != NULL)
		Q->tail->next = new;
	Q->tail = new;
}

// Removes a Queue Node from the front of the queue and returns its' pid
PlaceId QueueLeave (Queue Q) {
	if (Q->head == NULL) {
		Q->tail = NULL;
		return NOWHERE;
	}
	assert (Q != NULL);
	assert (Q->head != NULL);
	QueueNode *oldHead = Q->head;
	int takeId = oldHead->id;
	Q->head = Q->head->next;
	free(oldHead);
	return takeId;
}

// Checks if the Queue is empty
int QueueIsEmpty(Queue Q) {
	return (Q->head == NULL);
}
