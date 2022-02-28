////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// HunterView.c: the HunterView ADT implementation
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-01	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10   v3.0    Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "Game.h"
#include "GameView.h"
#include "HunterView.h"
#include "Map.h"
#include "Places.h"
// add your own #includes here
#define ALLCITIES 120


////////////////////////////////////////////////////////////////////////
// Additional Interface Helper Functions
struct hunterView {
	GameView gv;
};

////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

HunterView HvNew(char *pastPlays, Message messages[])
{
	HunterView new = malloc(sizeof(*new));
	if (new == NULL) {
		fprintf(stderr, "Couldn't allocate HunterView!\n");
		exit(EXIT_FAILURE);
	}

	//Initialising data.
	new->gv = GvNew(pastPlays, messages);

	return new;
}

void HvFree(HunterView hv)
{
	GvFree(hv->gv);
	free(hv);
}

////////////////////////////////////////////////////////////////////////
// Game State Information

Round HvGetRound(HunterView hv)
{
	return GvGetRound(hv->gv);
}

Player HvGetPlayer(HunterView hv)
{
	return GvGetPlayer(hv->gv);
}

int HvGetScore(HunterView hv)
{
	return GvGetScore(hv->gv);
}

int HvGetHealth(HunterView hv, Player player)
{
	return GvGetHealth(hv->gv, player);
}

PlaceId HvGetPlayerLocation(HunterView hv, Player player)
{
	return GvGetPlayerLocation(hv->gv, player);
}

PlaceId HvGetVampireLocation(HunterView hv)
{
	return GvGetVampireLocation(hv->gv);
}

////////////////////////////////////////////////////////////////////////
// Utility Functions

PlaceId HvGetLastKnownDraculaLocation(HunterView hv, Round *round)
{
	
	Round *trackRound = round;
	int found = 0;
	bool canFree = true;
	PlaceId *locArray = GvGetLocationHistory(hv->gv, PLAYER_DRACULA, trackRound, &canFree);
	*trackRound = *trackRound - 1;
	while (found == 0 && *trackRound >= 0) {
		if (placeIsReal(locArray[*trackRound])) {
			*round = *trackRound;
			PlaceId locFound = locArray[*trackRound];
			free(locArray);
			return locFound;
		}
		*trackRound = *trackRound - 1;
	}
	
	return NOWHERE;
}

PlaceId *HvGetShortestPathTo(HunterView hv, Player hunter, PlaceId dest,
                             int *pathLength)
{
	PlaceId currLoc = GvGetPlayerLocation(hv->gv, hunter);		//get the current location of current player
	int visited[ALLCITIES];
	for (int i = 0; i < ALLCITIES; i++) {
		visited[i] = -1;
	}
	visited[currLoc] = currLoc;

	int found = 0;
	int numReturn = 0;
	int length = 0;
	PlaceId v;
	Queue new = newQueue();
	QueueJoin(new,currLoc);
	Round findround[ALLCITIES];                 //find the round of each move
    for (int i = 0; i < ALLCITIES; i++) {
		findround[i] = -1;
	}
	findround[currLoc] = HvGetRound(hv);
	
	// Search breadth-wise for shortest path from destination to source
	while (QueueIsEmpty(new) == 0 && found != 1) {
		v = QueueLeave(new);
		PlaceId *oneMove = GvGetReachableByType(hv->gv, hunter, findround[v], v,1,1,1, &numReturn);
		if (v == dest) {
			found = 1;
			break;
		} else {
			for (int i = 0; i < numReturn; i++) {
				if (visited[oneMove[i]] == -1) {
					visited[oneMove[i]] = v;
					QueueJoin(new,oneMove[i]);
					findround[oneMove[i]] = findround[v]+1;
				}
			}
		}
	}
	dropQueue (new);
	if (found == 0) { // There is no path to the dest
		fprintf(stderr, "Failure in finding a path in getShortestPathTo()\n");
		exit(EXIT_FAILURE);
	}
	
	v = dest; 
	PlaceId m = v;
	while (m != currLoc) {
	    length++;
	    m = visited[m];
    }
	
	PlaceId *Reversepath = malloc(length*sizeof(PlaceId));
	int a = 0;
	while (v != currLoc) {	    
	    Reversepath[a] = v;
		v = visited[v];
		a++;
	}
	PlaceId *path = malloc(length*sizeof(PlaceId));
	int i = length - 1;
	for (int j = 0;j < length  && i >= 0;j++,i--) {
        path[j] = Reversepath[i];       
    }
	*pathLength = length;
	return path;
}

////////////////////////////////////////////////////////////////////////
// Making a Move

PlaceId *HvWhereCanIGo(HunterView hv, int *numReturnedLocs)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*numReturnedLocs = 0;
	Player curr = HvGetPlayer(hv);
	PlaceId currLoc = GvGetPlayerLocation(hv->gv, curr);
	Round round = HvGetRound(hv);
	PlaceId *reachable = GvGetReachable(hv->gv, curr, round, currLoc, numReturnedLocs);	
	return reachable;
}

PlaceId *HvWhereCanIGoByType(HunterView hv, bool road, bool rail,
                             bool boat, int *numReturnedLocs)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*numReturnedLocs = 0;
	Player curr = HvGetPlayer(hv);
	PlaceId currLoc = GvGetPlayerLocation(hv->gv, curr);
	Round round = HvGetRound(hv);
	PlaceId *reachable = GvGetReachableByType(hv->gv, curr, round, currLoc,
												 road, rail, boat, numReturnedLocs);	
	return reachable;
}

PlaceId *HvWhereCanTheyGo(HunterView hv, Player player,
                          int *numReturnedLocs)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*numReturnedLocs = 0;
	PlaceId currLoc = GvGetPlayerLocation(hv->gv, PLAYER_DRACULA);		//get the current location of Dracula
	PlaceId *reachable;
	Round round = HvGetRound(hv);
	
	if (player != 4) {		//the player is not the Dracula
		reachable = GvGetReachable(hv->gv, player, round, currLoc, numReturnedLocs);
	} else {				//the player is the Dracula
		if (currLoc == CITY_UNKNOWN || currLoc == SEA_UNKNOWN) {	//the location of the Dracula is not been revealed
			return NULL;
		} else {
			reachable = GvGetReachable(hv->gv, player, round, currLoc, numReturnedLocs);
		}
	}
	return reachable;
}

PlaceId *HvWhereCanTheyGoByType(HunterView hv, Player player,
                                bool road, bool rail, bool boat,
                                int *numReturnedLocs)
{
	Round round = HvGetRound(hv);
	*numReturnedLocs = 0;
	PlaceId currLoc = GvGetPlayerLocation(hv->gv, player);		//get the current location of Dracula
	
	if (player != 4) {		//the player is not the Dracula
		PlaceId *reachable = GvGetReachableByType(hv->gv, player, round, currLoc, road, rail, boat, numReturnedLocs);
		return reachable;
	} else {
		if (currLoc == CITY_UNKNOWN || currLoc == SEA_UNKNOWN) {	//the location of the Dracula is not been revealed
			return NULL;
		} else {
			PlaceId *reachable = GvGetReachableByType(hv->gv, player, round, currLoc, road, rail, boat, numReturnedLocs);
			return reachable;
		}
	}
}

