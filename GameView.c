////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// GameView.c: GameView ADT implementation
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
#include <string.h>

#include "Game.h"
#include "GameView.h"
#include "Map.h"
#include "Places.h"

#include "linkedList.h"
#include "helper.h"

struct gameView {
	char *pastPlays;	
	int pathLen;
	char *playerAbrev;
	node firstMove[5];	// history moves of 4 hunters and Dracula(this node always points to the first one)
	node lastMove[5];	// Each player's earliest move 
	Map m;
};

////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

GameView GvNew(char *pastPlays, Message messages[])
{
	GameView new = malloc(sizeof(*new));
	if (new == NULL) {
		fprintf(stderr, "Couldn't allocate GameView!\n");
		exit(EXIT_FAILURE);
	}
	// Initialing data.
	new->m = MapNew();
	new->pastPlays = strdup(pastPlays);
	new->pathLen = strlen(pastPlays); 
	new->playerAbrev = strdup("GSHMD\0");

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
	return new;
}

void GvFree(GameView gv)
{
	for (int i = 0; i < 5; i++) freeList(gv->firstMove[i]);
	MapFree(gv->m);
	free(gv->playerAbrev);
	free(gv->pastPlays); 
	free(gv);
}

////////////////////////////////////////////////////////////////////////
// Game State Information


Round GvGetRound(GameView gv)
{
	int arraysize = strlen(gv->pastPlays) + 1;
	return arraysize / 40;
}

Player GvGetPlayer(GameView gv)
{
	int arraysize = strlen(gv->pastPlays);
	if (arraysize < 7) return PLAYER_LORD_GODALMING;
	else return (letterToPlayer(gv->pastPlays[arraysize - 7]) + 1) % 5; 
	
	fprintf(stderr, "Failure in determining player turn in getPlayer()\n");
	exit(EXIT_FAILURE);
}

int GvGetScore(GameView gv)
{
	int round = (strlen(gv->pastPlays) + 1) / 40;
	
	// Find number of matured vampires
	int maturedVamps = 0; 
	for (int i = 0; gv->pastPlays[i] != '\0'; i++) {
		if (gv->pastPlays[i] == 'D' && gv->pastPlays[i + 5] == 'V') maturedVamps++; 
	}

	// Find number of hunter deaths
	int hunterDeaths = 0;
	for (int hunter = 0; hunter < 4; hunter++) {
		Player player = letterToPlayer(gv->playerAbrev[hunter]);
		hunterDeaths = hunterDeaths + playerStats(gv, 1, player);
	}

	return GAME_START_SCORE - round - (maturedVamps * 13) - (hunterDeaths * 6);
}

int GvGetHealth(GameView gv, Player player)
{
	return playerStats(gv, 2, player);
}

PlaceId GvGetPlayerLocation(GameView gv, Player player)
{
	if (gv->lastMove[player] == NULL) return NOWHERE;

	// For hunters
	if (player != PLAYER_DRACULA) {
		// Check if the hunter died last turn, return St Joseph... if so
		if (GvGetHealth(gv, player) == 0) return ST_JOSEPH_AND_ST_MARY;
		return gv->lastMove[player]->id;
	
	// For dracula
	} else {
		PlaceId playerLoc = gv->lastMove[player]->id;
		// Find actual location if turn was double back or hide
		if (playerLoc > 101 && playerLoc < 108) {
			playerLoc = getHiddenLoc(gv->lastMove[PLAYER_DRACULA], playerLoc)->id; 
		}
		if (placeIsReal(playerLoc)) return playerLoc;
		else if (playerLoc == CITY_UNKNOWN) return CITY_UNKNOWN;
		else if (playerLoc == SEA_UNKNOWN) return SEA_UNKNOWN;
		else if (playerLoc == TELEPORT) return CASTLE_DRACULA;
	}
fprintf(stderr, "Failure in determining player location in GvGetPlayerLocation()\n");
exit(EXIT_FAILURE);
}

PlaceId GvGetVampireLocation(GameView gv)
{
	int arraysize = strlen(gv->pastPlays) + 1;
	int i = 37; //First vampire created
	
	if (arraysize < 32) return NOWHERE;
	
	
	// Find most recent vampire created
	while (i + 520 < arraysize) {
		assert(gv->pastPlays[i] == 'V');
		i = i + 520;
	}
	i = i - 5; // Return to start of turn

	// Check if vampire is already matured, return NOWHERE if so
	if ((arraysize - i) > 240) return NOWHERE;
	
	// Within range, analyse vampire location
	PlaceId vampLoc = strToId(gv->pastPlays, i + 1);

	// Check if hunters killed vampire, if so, return NOWHERE (NOT TRIGGERING!!!!)
	for (int j = 0; j < 29; j++) {
		// End of pastPlays string, vampire still alive, end the loop
		if (i > arraysize) j = 29;
		
		
		// Hunters turn, check if they have encountered the vampire and killed it
		if ((j % 5) != 0) {
			PlaceId huntLoc = strToId(gv->pastPlays, i + 1);
			if (huntLoc == vampLoc) return NOWHERE; // Vampire is killed, return NOWHERE
		}
		i = i + 8;
	}
	
	// Vampire is still alive
	if (placeIsReal(vampLoc)) return vampLoc;
	else return CITY_UNKNOWN;
}	

PlaceId *GvGetTrapLocations(GameView gv, int *numTraps)
{
	*numTraps = 0;
	int *trapArray = NULL;

	// Determine starting point (used to consider decayed traps)
	node startnode = NULL;
	if (gv->lastMove[PLAYER_DRACULA] == NULL || gv->lastMove[PLAYER_DRACULA]->moveno <= 6) {
		startnode = gv->firstMove[PLAYER_DRACULA];
	} else {
		startnode = gv->lastMove[PLAYER_DRACULA]->prev->prev->prev->prev->prev;
	}

	// Find all the traps Dracula has set 
	for (node curr = startnode; curr != NULL; curr = curr->next) {
		if (curr->turnStr[3] == 'T') {
			int loc = curr->id;
			if (loc > 101 && loc < 108) {
				loc = getHiddenLoc(gv->lastMove[PLAYER_DRACULA], loc)->id;
			}
			trapArray = (int *) realloc(trapArray, sizeof(int) * (*numTraps + 1));
			trapArray[*numTraps] = loc; 
			*numTraps = *numTraps + 1;
		}
	}
	// Go through each hunter
	for (int i = 0; i < 4; i++) {
		//Go through each hunter's turns
		for (node curr = gv->firstMove[i]; curr != NULL; curr = curr->next) {
			// Go through all the hunter's encounters
			for (int j = 3; j < 7; j++) {
				if (curr->turnStr[j] == 'T') {
					// Check for hunter interactions with trap
					removeTraps(curr->id ,trapArray, numTraps);
					trapArray = realloc(trapArray, sizeof(int) * (*numTraps + 1));
				}
			}
		}
	}

	return trapArray;
}


////////////////////////////////////////////////////////////////////////
// Game History

PlaceId *GvGetMoveHistory(GameView gv, Player player,
                          int *numReturnedMoves, bool *canFree)
{
	node last = gv->firstMove[player];
	while (last->next != NULL) last = last->next;
	*numReturnedMoves = last->moveno;
	*canFree = true;
	PlaceId *His = (PlaceId *) malloc((*numReturnedMoves) * sizeof(PlaceId));
	int i = 0;
	for (node curr = gv->firstMove[player];curr != NULL; curr = curr->next) {
		His[i] = curr->id;
		i++;
	}
	return His;
}

PlaceId *GvGetLastMoves(GameView gv, Player player, int numMoves,
                        int *numReturnedMoves, bool *canFree)
{
	node last = gv->lastMove[player]; 
	if (last->moveno <= numMoves) *numReturnedMoves = last->moveno;
	else *numReturnedMoves = numMoves;
	if (*numReturnedMoves == 0) return NULL; 

	// Get starting node
	for (int i = 0; i < *numReturnedMoves - 1; i++) last = last->prev;

	*canFree = true;
	PlaceId *His = (PlaceId *) malloc((*numReturnedMoves) * sizeof(PlaceId));
	int i = 0;
	for (node curr = last;curr != NULL; curr = curr->next){
		His[i] = curr->id;
		i++;
	}
	return His;
}

PlaceId *GvGetLocationHistory(GameView gv, Player player,
                              int *numReturnedLocs, bool *canFree)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	node last = gv->lastMove[player];

	*numReturnedLocs = last->moveno;
	*canFree = true;
	PlaceId *His = (PlaceId *) malloc((*numReturnedLocs) * sizeof(PlaceId));

	int i = 0;
	for (node curr = gv->firstMove[player];curr != NULL; curr = curr->next) {
		His[i] = curr->id; 
		// If the place was a HIDE or DOULBE BACK
		if (curr->id > 101 && curr->id < 108) {
			His[i] = getHiddenLoc(curr, curr->id)->id;
		}
		i++; 
	}

	return His;
}

PlaceId *GvGetLastLocations(GameView gv, Player player, int numLocs,
                            int *numReturnedLocs, bool *canFree)
{
	node last = gv->lastMove[player]; 
	// Determine what *numReturnLocs should be
	if (last->moveno <= numLocs) *numReturnedLocs = last->moveno;
	else *numReturnedLocs = numLocs;
	
	if (*numReturnedLocs == 0) return NULL;

	*canFree = true;
	PlaceId *His = (PlaceId *) malloc((*numReturnedLocs) * sizeof(PlaceId));

	// Get starting node
	for (int i = 0; i < *numReturnedLocs - 1; i++) last = last->prev;
	
	int i = 0;
	for (node curr = last;curr != NULL; curr = curr->next) {
		His[i] = curr->id; 
		// If the place was a HIDE or DOULBE BACK
		if (curr->id > 101 && curr->id < 108) {
			His[i] = getHiddenLoc(curr, curr->id)->id;
		}
		i++; 
	}	
	return His;
}

////////////////////////////////////////////////////////////////////////
// Making a Move

PlaceId *GvGetReachable(GameView gv, Player player, Round round,
                        PlaceId from, int *numReturnedLocs)
{
	PlaceId *reachable = NULL;
	int j = 1;		// 1 for hunter, -1 when dracula is detected as player

	ConnList curConnection = MapGetConnections(gv->m, from);
	if (player != 4) {	//if the palyer is a hunter
		int numraildis = (round + player) % 4;		//how many stops can the player take(it may be wrong!!!!!!!!!!!!!!!!!!!!!!!!!!
		int nummoves = MapNumConnections(gv->m, ROAD) + \
						MapNumConnections(gv->m, BOAT) + \
						numraildis * MapNumConnections(gv->m, RAIL);
		reachable = (PlaceId *) malloc(nummoves * sizeof(PlaceId));
		reachable[0] = from;
		
		j = findLandAdj (reachable, curConnection, j, player);
		j = findSeaAdj (reachable, curConnection, j, player);
		j = findRailConnections(gv, curConnection, reachable, j, numraildis);
		
	} else {	//the player is Dracula
		int nummoves = MapNumConnections(gv->m, ROAD) + MapNumConnections(gv->m, BOAT);
		reachable = (PlaceId *) malloc((nummoves + 6) * sizeof(PlaceId));
		reachable[0] = from;
		j = findLandAdj (reachable, curConnection, j, player);
		j = findSeaAdj (reachable, curConnection, j, player);
	}
	*numReturnedLocs = j;
	return reachable;
}

PlaceId *GvGetReachableByType(GameView gv, Player player, Round round,
                              PlaceId from, bool road, bool rail,
                              bool boat, int *numReturnedLocs)
{
	int j = 1;			//leave 0 for the given place
	ConnList curConnection = MapGetConnections(gv->m, from);
	PlaceId *reachable = NULL;
	int numraildis = (round + player) % 4;			//how many stops can the player take(it may be wrong!!!!!!!!!!!!!!!!!!!!!!!!!!
	int nummoves = MapNumConnections(gv->m,ROAD) + MapNumConnections(gv->m,BOAT) + numraildis * MapNumConnections(gv->m,RAIL);
	reachable = (PlaceId *)malloc(nummoves*sizeof(PlaceId));
	reachable[0] = from;

	//road
	if (road == 1) {
		j = findLandAdj (reachable, curConnection, j, player);
	}
	//rail
	if (rail == 1 && player != PLAYER_DRACULA) {
		j = findRailConnections(gv, curConnection, reachable, j, numraildis);
	}
	//boat
	if (boat == 1) {
		j = findSeaAdj (reachable, curConnection, j, player);
	}

	*numReturnedLocs = j;
	return reachable;

}
////////////////////////////////////////////////////////////////////////
// Your own interface functions
