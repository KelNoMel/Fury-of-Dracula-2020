////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// DraculaView.c: the DraculaView ADT implementation
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-01	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10	v3.0	Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DraculaView.h"
#include "Game.h"
#include "GameView.h"
#include "Map.h"

#include "linkedList.h"
#include "helper.h"

#define ALLCITIES 120
#define WHO_CARES 5

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

////////////////////////////////////////////////////////////////////////
// Additional Interface Helper Functions
static Queue newQueue (void);
static void dropQueue (Queue Q);
static void QueueJoin (Queue Q, PlaceId p);
static PlaceId QueueLeave (Queue Q);
static int QueueIsEmpty(Queue Q);

struct draculaView {
	int pathLen;
	node firstMove[5];
	node lastMove[5];
	GameView gv;
	bool canHide;
	bool canDouble; 
	Map m;
};

////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

DraculaView DvNew(char *pastPlays, Message messages[])
{
	DraculaView new = malloc(sizeof(*new));
	if (new == NULL) { 
		fprintf(stderr, "Couldn't allocate DraculaView!\n");
		exit(EXIT_FAILURE);
	}
	// Initialing data. 
	new->pathLen = strlen(pastPlays); 
	new->gv = GvNew(pastPlays, messages); 
	new->canHide = true;
	new->canDouble = true; 
	new->m = MapNew();

	for (int i = 0; i < 5; i++) {
		new->firstMove[i] = NULL; 
		new->lastMove[i] = NULL; 
	}

	int player = -1; 
	for (int i = 0; pastPlays[i] != '\0'; i++) {
		if (i % 8 != 0) continue; 
		player = letterToPlayer(pastPlays[i]);
		newNode(player, pastPlays, i, new->firstMove, new->lastMove); 
	}
	// Determine if Dracula can HIDE or DOUBLE BACK
	int trail = 0;
	for (node curr = new->lastMove[PLAYER_DRACULA]; curr != NULL; curr = curr->prev) {
		// Break at 5 because of last move is HIDE or DOUBLE BACK it will fall off the trail by next move
		if (trail == 5) break;
		if (curr->id == HIDE) new->canHide = false;
		if (curr->id > 102 && curr->id < 108) new->canDouble = false; 
		trail++;
	}
	return new;
}

void DvFree(DraculaView dv)
{
	for (int i = 0; i < 5; i++) freeList(dv->firstMove[i]);
	GvFree(dv->gv);
	MapFree(dv->m);
	free(dv);
}

////////////////////////////////////////////////////////////////////////
// Game State Information

Round DvGetRound(DraculaView dv)
{	
	return GvGetRound(dv->gv);
}

int DvGetScore(DraculaView dv)
{
	return GvGetScore(dv->gv);
}

int DvGetHealth(DraculaView dv, Player player)
{
	return GvGetHealth(dv->gv, player);
}

PlaceId DvGetPlayerLocation(DraculaView dv, Player player)
{
	return GvGetPlayerLocation(dv->gv, player);
}

PlaceId DvGetVampireLocation(DraculaView dv)
{
	return GvGetVampireLocation(dv->gv);
}

PlaceId *DvGetTrapLocations(DraculaView dv, int *numTraps)
{
	*numTraps = 0;
	return GvGetTrapLocations(dv->gv, numTraps);
}

////////////////////////////////////////////////////////////////////////
// Making a Move

PlaceId *DvGetValidMoves(DraculaView dv, int *numReturnedMoves)
{

	*numReturnedMoves = 0;
	if (dv->pathLen < 32) return NULL; 
	PlaceId dLoc = DvGetPlayerLocation(dv, PLAYER_DRACULA);
	PlaceId *reachable = GvGetReachable(dv->gv, PLAYER_DRACULA, DvGetRound(dv),
						dLoc, numReturnedMoves); 

	// Add the HIDE move if possible
	if (dv->canHide && placeIdToType(dLoc) != SEA) {
		reachable[*numReturnedMoves] = HIDE;
		*numReturnedMoves = *numReturnedMoves + 1;
	}

	// Add all the possible DOUBLE BACK moves 
	int trail = 0;
	for (node curr = dv->lastMove[PLAYER_DRACULA]; curr != NULL; curr = curr->prev) {
		if (!dv->canDouble) break;
		if (trail == 5) break;		// Break at 5, since only 5 types of DOUBLE_BACK

		// Discover the place if it was hidden
		PlaceId id = curr->id; 
		if (id > 101 && id < 108) {
			id = getHiddenLoc(curr, id)->id;
		}

		// Can only DOUBLE BACK if adjacent 
		for (int k = 0; k < *numReturnedMoves; k++) {
			if (id == reachable[k]) {
				reachable[*numReturnedMoves] = DOUBLE_BACK_1 + trail;
				*numReturnedMoves = *numReturnedMoves + 1;
			}
		}
		trail++;
	}

	// Remove all locations on trail 
	trail = 0;
	for (node curr = dv->lastMove[PLAYER_DRACULA]; curr != NULL; curr = curr->prev) {
		if (trail == 5) break;
		int loc = curr->id;
		
		for (int i = 0; i < *numReturnedMoves; i++) {
			if (loc == reachable[i]) {
				collapseArray(reachable, i, *numReturnedMoves);
				*numReturnedMoves = *numReturnedMoves - 1;
			}
		}
		trail++;
	}
	if (*numReturnedMoves == 0) return NULL;
	else return reachable;
}

PlaceId *DvWhereCanIGo(DraculaView dv, int *numReturnedLocs)
{
	*numReturnedLocs = 0;
	PlaceId *reachable = DvGetValidMoves(dv, numReturnedLocs);

	// Remove duplicate location of DOUBLE_BACK_1 and HIDE
	for (int i = 0; i < *numReturnedLocs; i++) {
		if (reachable[i] == DOUBLE_BACK_1 && reachable[i - 1] == HIDE) {
			collapseArray(reachable, i, *numReturnedLocs);
			*numReturnedLocs = *numReturnedLocs - 1;
			continue;
		}
	}

	// Reveal all hidden locations
	for (int i = 0; i < *numReturnedLocs; i++) {
		if (reachable[i] > 101 && reachable[i] < 108) {
			if (reachable[i] == 103) reachable[i] -= 1;
			reachable[i] = getHiddenLoc(dv->lastMove[PLAYER_DRACULA], reachable[i] - 1)->id; 
		}
	}

	if (*numReturnedLocs == 0) return NULL;
	else return reachable;
}

PlaceId *DvWhereCanIGoByType(DraculaView dv, bool road, bool boat,
                             int *numReturnedLocs)
{
	//Untestewd! 
	*numReturnedLocs = 0;
	ConnList curConnection = MapGetConnections(dv->m, 
							DvGetPlayerLocation(dv, PLAYER_DRACULA));
	PlaceId *reachable = DvWhereCanIGo(dv, numReturnedLocs); 
	
	// Check if Dracula can stay in the same place
	bool canStay = false;
	for (int i = 0; i < *numReturnedLocs; i++) {
		if (dv->lastMove[PLAYER_DRACULA]->id == reachable[i]) {
			canStay = true;
			break;
		}
	}

	// Remove all boat connections
	if (!boat) {
		for (ConnList curr = curConnection; curr != NULL; curr = curr->next) {
			for (int i = 0; i < *numReturnedLocs; i++) {
				if (curr->p == reachable[i] && curr->type == BOAT) {
					collapseArray(reachable, i, *numReturnedLocs); 
					*numReturnedLocs = *numReturnedLocs - 1;
				}
			}	
		}
	}

	// Remove all road connections 
	if (!road) {
		for (ConnList curr = curConnection; curr != NULL; curr = curr->next) {
			for (int i = 0; i < *numReturnedLocs; i++) {
				if (curr->p == reachable[i] && curr->type == ROAD) {
					collapseArray(reachable, i, *numReturnedLocs); 
					*numReturnedLocs = *numReturnedLocs - 1;
				}
			}	
		}
	}

	// Add Dracula's current location back into the array if it was removed
	// This is because HIDE/DOUBLE_BACK_1 does not traverse terrain (as mentioned on forums)
	bool removed = true;
	if (canStay) {
		for (int i = 0; i < *numReturnedLocs; i++) {
			if (reachable[i] == dv->lastMove[PLAYER_DRACULA]->id) {
				removed = false;
				break;
			}
		}
	}
	if (canStay && removed) {
		reachable[*numReturnedLocs + 1] = dv->lastMove[PLAYER_DRACULA]->id;
		*numReturnedLocs =  *numReturnedLocs + 1;
	}
	if (*numReturnedLocs == 0) return NULL;
	else return reachable;
}

PlaceId *DvWhereCanTheyGo(DraculaView dv, Player player,
                          int *numReturnedLocs)
{
	*numReturnedLocs = 0;
	// Return NULL if player has not acted 
	PlaceId *reachable = NULL;
	if (dv->lastMove[player] == NULL) return reachable; 
	
	if (player == PLAYER_DRACULA) {
		reachable = DvWhereCanIGo(dv, numReturnedLocs);
	} else {
		reachable = GvGetReachable(dv->gv, player, DvGetRound(dv),
                        GvGetPlayerLocation(dv->gv, player), numReturnedLocs);		
	}
	
	return reachable;
}

PlaceId *DvWhereCanTheyGoByType(DraculaView dv, Player player,
                                bool road, bool rail, bool boat,
                                int *numReturnedLocs)
{
	*numReturnedLocs = 0;
	// Return NULL if player has not acted 
	PlaceId *reachable = NULL; 
	if (dv->lastMove[player] == NULL) return reachable; 

	if (player == PLAYER_DRACULA) {
		reachable = DvWhereCanIGoByType(dv, road, boat, numReturnedLocs);
	} else {
		reachable = GvGetReachableByType(dv->gv, player, DvGetRound(dv),
                        GvGetPlayerLocation(dv->gv, player), road, rail, 
						boat, numReturnedLocs);		
	}
	return reachable;
}

////////////////////////////////////////////////////////////////////////
// Your own interface functions

// Takes two locations and a pointer to an int. 
// Returns Dracula's shortest path and the lenth of the path. 
PlaceId DvBFS(DraculaView dv, PlaceId origin, PlaceId dest, int *path, int player) {
	assert (dv->m != NULL);
	bool found = false; 
	int visiting = -1; 
	int len = 0;
	int visited[ALLCITIES]; 
	int numReturn = 0;
	for (int i = 0; i < ALLCITIES; i++) visited[i] = -1; 
	
	static bool firstrec = true; 

	visited[origin] = origin; 
	Queue Q = newQueue(); 
	QueueJoin(Q, origin);

	//printf("DRACULA'S TRAIL IS %s\n", placeIdToName(dv->lastMove[PLAYER_DRACULA]->id));

	// BFS
	while (!found && QueueIsEmpty(Q) == 0) {
		visiting = QueueLeave(Q);
		PlaceId *oneMove = GvGetReachableByType(dv->gv, player, WHO_CARES, visiting,1,0,1, &numReturn);

		// Remove all the locations that are on Dracula's trail
		// For now i dont want Dracula to DOUBLE_BACK. That is saved for teleporting 
		if (player == PLAYER_DRACULA) {
			node curr = dv->lastMove[PLAYER_DRACULA]; 
			for (int i = 0; i < 5; i++) {
				if (curr == NULL) break;
				for (int k = 0; k < numReturn; k++) {
					if (oneMove[k] == curr->id) {
						collapseArray(oneMove, k, numReturn); 
						numReturn--; 
						break; 
					}
				}
				curr = curr->prev;
			}
			// Do not enter into a city where a hunter is present
			if (firstrec) {
				for (int i = 0; i < numReturn; i++) {
					if (hasHunter(dv, oneMove[i])) {
						collapseArray(oneMove, i, numReturn);
						firstrec = false;
						break;
					}
				}
				firstrec = false; 
			}
			// Try not to kill dracula at sea 
		}

		if (visiting == dest) {
			found = true;
			break;
		} else {
			for (int i = 0; i < numReturn; i++) {
				if (visited[oneMove[i]] == -1) {
					visited[oneMove[i]] = visiting;
					QueueJoin(Q,oneMove[i]);
				}
			}
		}
		free(oneMove); 
	}
	dropQueue(Q);

	// Return 0 if path not found
	// Impliment a falesafe when no path can be found (for example, dracula is surrounded by his trail
	// or by hunters)
	if (!found) {
		printf("SOMETHING IS WRONG\n");
		return -1;		// Signal failure 
	}

	// Traceback path
	int i = 0; 
	// PlaceId *path = malloc(ALLCITIES * sizeof(PlaceId));
    for (int v = dest; v != origin; v = visited[v]) {
		path[i] = v; 
		len++; 
		i++;
	}
    path[len] = origin; 

	// Reverse array to correct flight order
	int end = len; 
	int temp; 
	for (int start = 0; start < end; start++) {
		temp = path[end];
		path[end] = path[start];
		path[start] = temp; 
		end--; 	
	}
	// free q
	return len + 1;
}

bool DvcanDB(DraculaView dv) {
	return dv->canDouble;
}

bool DvcanHI(DraculaView dv) {
	return dv->canHide;
}

PlaceId locToMove(DraculaView dv, PlaceId id) {
	int numMoves = -1;
	int *moves = DvGetValidMoves(dv, &numMoves);  
	for (int i = 0; i < numMoves; i++) {
		if (id == moves[i]) {
			id = moves[i]; 
			return id; 
		}

		if (moves[i] > 101 && moves[i] < 108) {
			int hidden = getHiddenLoc(dv->lastMove[PLAYER_DRACULA], moves[i] - 1)->id;
			if (id == hidden) {
				id = moves[i]; 
				return id; 
			}
		}
	}
	printf("SOMETRHING IS TERRIBLY WRONG IN LOCTOMOVE\n");
	//exit(EXIT_FAILURE);
	return id; 
}

bool hasHunter(DraculaView dv, PlaceId id) {
	for (int i = 0; i < 4; i++) {
		if (id == DvGetPlayerLocation(dv, i)) {
			return true; 
		}
	}
	return false; 
}
//////////////////////////////////////////////////////////////////////////////
///////////////////////////// QUEUE STUFF ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static Queue newQueue (void) {
	Queue new = malloc (sizeof(Queue));
	new->head = NULL;
	new->tail = NULL;
	return new;
}

static void dropQueue (Queue Q) {
	assert (Q != NULL);
	for (QueueNode *curr = Q->head, *next;curr != NULL; curr = next) {
		next = curr->next;
		free(curr);
	}
	free (Q);
}

static void QueueJoin (Queue Q, PlaceId p) {
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

static PlaceId QueueLeave (Queue Q) {

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

static int QueueIsEmpty(Queue Q) {
	return (Q->head == NULL);

}