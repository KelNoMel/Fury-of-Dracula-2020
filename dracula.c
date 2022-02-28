////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// dracula.c: your "Fury of Dracula" Dracula AI
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-01	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10	v3.0	Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <math.h>


#include "dracula.h"
#include "DraculaView.h"
#include "Game.h"
#include "linkedList.h"
#include "helper.h"

#define ALLCITIES 120
#define SEA_WEIGHT 0.008
#define HUNTER_WEIGHT 0.5

// Do not allow DOUBLE_BACK or HIDE moves
void randomValidNoSpecial(int *moves, int numMoves, int *play);
// Any valid move
void randomValid(int *moves, int numMoves, int *play);
// Move average distance away from hunters
int averageDist(DraculaView dv, int numLocs, int *locs, int *hunterLocs);

void decideDraculaMove(DraculaView dv)
{
	// Disgusting hardcode, will need to impliment a searching algo later
	char play[3]; 
	play[2] = '\0';
	int round = DvGetRound(dv); 
	//int dLoc = DvGetPlayerLocation(dv, PLAYER_DRACULA);
	int numMoves = -1; 
	int *moves = DvGetValidMoves(dv, &numMoves); 
	int numLocs = -1;
	int *locs = DvWhereCanIGo(dv, &numLocs); 
	int hunterLocs[4]; 
	for (int i = 0; i < 4; i++) hunterLocs[i] = DvGetPlayerLocation(dv, i); 

	// Testing shortest path
	PlaceId *path = malloc(ALLCITIES * sizeof(PlaceId));

	int BFSdist = DvBFS(dv, HAMBURG, NUREMBURG, path, PLAYER_DRACULA); 
	printf("DISTANCE WAS %d\n", BFSdist); 
	if (BFSdist == -1) {
		randomValidNoSpecial(moves, numMoves, path); 
	}

	// Failsafe if there is not appropriate shortest path 
	for (int i = 0; i < BFSdist; i++) {
		printf("Move %d to %s\n", i, placeIdToName(path[i]));
		registerBestPlay(play, "HELP"); 
	}
	free(path);

	// First time init 
	if (round == 0) {
		play[0] = 'C'; play[1] = 'D'; play[3] = '\0';
		registerBestPlay(play, "Go Away");
		return;
	}

	// Still use this, until they catch on
	switch(round) {
		case 0: 
			play[0] = 'C'; play[1] = 'D';
			break;
		case 1:
			play[0] = 'G'; play[1] = 'A';
			break;
		case 2:
			play[0] = 'K'; play[1] = 'L';
			break;
		case 3:
			play[0] = 'D'; play[1] = '3';
			break;
		case 4:
			play[0] = 'H'; play[1] = 'I';
			break;
		// Round 5 dracula should TP
		case 6:
			play[0] = 'C'; play[1] = 'D';
			break;
		// Run away to sea 
		case 7:
			play[0] = 'G'; play[1] = 'A';
			break;
		case 8:
			play[0] = 'C'; play[1] = 'N';
			break;
		case 9:
			play[0] = 'B'; play[1] = 'S';
			break;
		case 10:
			play[0] = 'I'; play[1] = 'O';
			break;
		case 11:
			play[0] = 'T'; play[1] = 'S';
			break;
		case 12:
			play[0] = 'M'; play[1] = 'S';
			break;
		case 13:
			play[0] = 'C'; play[1] = 'G';
			break;
		// Setup another teleport 
		case 14:
			play[0] = 'H'; play[1] = 'I';
			break;
		case 15:
			play[0] = 'D'; play[1] = '1';
			break;
		// Round 16 dracula should TP
	}
	// Teleport
	if (numMoves == 0 && round != 0) { 
		play[0] = 'T'; play[1] = 'P'; 
	} 

	if (round > 16) {
		int final = averageDist(dv, numLocs, locs, hunterLocs); 
		const char *place = placeIdToAbbrev(final);
		play[0] = place[0]; play[1] = place[1];
	}

	registerBestPlay(play, "Go Away");
}

void randomValidNoSpecial(int *moves, int numMoves, int *play) {
	int index = rand() % (numMoves - 1);
	while (moves[index] > 101 && moves[index] < 108) index = rand() % (numMoves - 1);
	const char *place = placeIdToAbbrev(moves[index]);
	play[0] = place[0]; play[1] = place[1];
}

void randomValid(int *moves, int numMoves, int *play) {
	const char *place = placeIdToAbbrev(moves[rand() % (numMoves - 1)]);
	play[0] = place[0]; play[1] = place[1];
}

////////////////////////////////////////////////////////////////////////
//// The problem at the moment is that if Dracula is surrounded, he ////
//// would prefer to hide rather than immediately move out to sea	////
////////////////////////////////////////////////////////////////////////
int averageDist(DraculaView dv, int numLocs, int *locs, int *hunterLocs) {
	// Weight each location based on its proximity to hunters
	// Discourage moving to sea and to locations occupied by hunters. 
	double locWeight[numLocs]; 
	for (int i = 0; i < numLocs; i++) {
		for (int k = 0; k < 4; k++) {
			int *buffer = malloc(sizeof(PlaceId) * ALLCITIES); 
			locWeight[i] += DvBFS(dv, hunterLocs[k], locs[i], buffer, k);
			free(buffer); 
		}
		// Converge the weight between 1 and -1
		printf("%s weighted at %lf\n", placeIdToName(locs[i]), locWeight[i]); 
		locWeight[i] = 2 * (atan(locWeight[i]) / M_PI);
		if (placeIdToType(locs[i]) == SEA) locWeight[i] -= SEA_WEIGHT;
		if (hasHunter(dv, locs[i]) && placeIdToType(locs[i]) == LAND) locWeight[i] -= HUNTER_WEIGHT;
		printf("%s weighted at %lf\n", placeIdToName(locs[i]), locWeight[i]); 
	}

	// Choose the location furtherest from hunters (highest number)
	int weightMove = -1;
	double highestWeigt = -1;
	for (int i = 0; i < numLocs; i++) {
		if (highestWeigt < locWeight[i]) {
			highestWeigt = locWeight[i]; 
			weightMove = locs[i];
		}
	}
	printf("Location chosen was %d, %s\n", weightMove, placeIdToName(weightMove)); 

	// Determine the move to get to weightMove (might be HIDE or DOUBLE_BACK)
	weightMove = locToMove(dv, weightMove);
	
	// Prefer to HIDE than to DOUBLE_BACK
	// I think this may be redundant, but im keeping it in anyway just in case. 
	if (weightMove == DOUBLE_BACK_1 && DvcanHI(dv)) weightMove = HIDE; 

	return weightMove;
}
// BFS each hunter and determine which move moves away from them. Weight locations with distance from port, etc. 

// Determine which places are best to set up teleports

// Determine if it is safe to cycle through a round of HP stacking

// Manage where to put vampires??