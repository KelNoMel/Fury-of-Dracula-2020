// An ADT for the Queue used in assignment

#include "GameView.h"
#include "Game.h"
#include "Places.h"

typedef struct QueueNode {
	PlaceId id;			//include other places
	struct QueueNode* next;
	struct QueueNode* pre;
	int count;			//a counter for the monving number
} QueueNode;

typedef struct Queue {
	QueueNode *head;
	QueueNode *tail;
} *Queue;

// Initializes the Queue
Queue newQueue (void);

// Frees the Queue from memory
void dropQueue (Queue Q);

// Adds a Queue Node containing a pid to the back of the queue
void QueueJoin (Queue Q, PlaceId p);

// Removes a Queue Node from the front of the queue and returns its' pid
PlaceId QueueLeave (Queue Q);

// Checks if the Queue is empty
int QueueIsEmpty(Queue Q);