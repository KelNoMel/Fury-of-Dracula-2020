////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// hunter.c: your "Fury of Dracula" hunter AI.
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-01	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10	v3.0	Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////

#include "Game.h"
#include "hunter.h"
#include "HunterView.h"
#include "Map.h"

#include <stdio.h>

// Helper functions
PlaceId *SearchArea(PlaceId *susLocs, PlaceId dracLoc, HunterView hv);
int connListNum (ConnList list);
PlaceId *checkOff(PlaceId *susLocs, PlaceId checkAgainst, int *numSusLocs);
void placeIdRegister (PlaceId moveId);
Player CanGetThePlaceFirst(HunterView hv,PlaceId dest);

struct hunterView {
	GameView gv;
};

struct gameView {
	char *pastPlays;	
	int pathLen;
	char *playerAbrev;
	node firstMove[5];	// history moves of 4 hunters and Dracula(this node always points to the first one)
	node lastMove[5];	// Each player's earliest move 
	Map m;
};

void decideHunterMove(HunterView hv)
{
	//TO IMPLEMENT: FOLLOW DRACULA INTO THE OCEAN
	int possNum = 0; // Number of possible moves
	PlaceId moveId = NOWHERE; // PlaceId that is returned
	
	// Getting initial information
	Player player = HvGetPlayer(hv); // Find player
	int round = HvGetRound(hv);
	
	// Round 0, distribute as best as possible
	if (round == 0) {
		if (player == PLAYER_LORD_GODALMING) {
			moveId = LONDON;
		} else if (player == PLAYER_DR_SEWARD) {
			moveId = ALICANTE;
		} else if (player == PLAYER_VAN_HELSING) {
			moveId = LEIPZIG;
		} else if (player == PLAYER_MINA_HARKER) {
			moveId = BELGRADE;
		} else {
			moveId = MARSEILLES;
		}
		
		placeIdRegister (moveId);
		return;
	}
	
	// Get rest of information
	int health = HvGetHealth(hv, player); // Get Health
	//printf("%d Health\n", health);
	PlaceId currLoc = GvGetPlayerLocation(hv->gv, player); // Get current location;
	int dracRound = 0;
	PlaceId dracLoc = HvGetLastKnownDraculaLocation(hv, &dracRound);
	//PlaceId vampLoc = HvGetVampireLocation(hv);
	//Player bestselection = -1;
	//if (vampLoc != NOWHERE && vampLoc != CITY_UNKNOWN) {
	//    bestselection = CanGetThePlaceFirst(hv,vampLoc);
    //}
	//const char *place = placeIdToName(currLoc);
	//printf("%s currloc\n", place);
	//const char *dest = placeIdToName(dracLoc);
	//printf("%s dracloc\n", dest);

	// Move around a bit before you can research Dracula
	if (round < 6 && dracLoc == NOWHERE) {
		PlaceId *possMov = HvWhereCanIGo(hv, &possNum);
		int rNum = rand() % possNum;
		moveId = possMov[rNum];
		placeIdRegister (moveId);
		return;
	}

	// If Dracula is advanced (Likely using seas/TP to evade), make a player camp CD
	if (round > 75 && player == PLAYER_VAN_HELSING) {
		if (currLoc == CASTLE_DRACULA) {
			placeIdRegister (currLoc);
		} else {
			int pathLength = 0;
			PlaceId *travelPath;
			travelPath = HvGetShortestPathTo(hv, player, dracLoc, &pathLength);
			moveId = travelPath[0];
			placeIdRegister (moveId);
		}
		return;
	}

	// Create locations to investigate
	int numSusLocs = 0;
	PlaceId *susLocs;
	if (placeIsReal(dracLoc)) {
		susLocs = SearchArea(&numSusLocs, dracLoc, hv);
	}
	
	// Pick a random location when searching (placeholder in case function times out)
	PlaceId *possMov = HvWhereCanIGo(hv, &possNum);
	int rNum = rand() % possNum;
	moveId = possMov[rNum];
	placeIdRegister (moveId);

	// Commit analysis of situation and specialised actions
	// Specialised actions: Heal/Research, Investigate, Attack. Should there be more?

	// Low Health, heal up/trail gone cold, research
	if (health <= 4 || dracLoc == NOWHERE || (round - dracRound) > 6) {
		moveId = HvGetPlayerLocation(hv, player);

	//Vampire's location is revealed, if the player is the one can firstly reach the place, then let him do it
	/*} else if (vampLoc != NOWHERE && vampLoc != CITY_UNKNOWN && player == bestselection) {   
        int pathLength = 0;
	    PlaceId *travelPath = HvGetShortestPathTo(hv, player, vampLoc, &pathLength);
        moveId = travelPath[0];
	*/
	// Dracula's past location is found (investigate/attack)
	} else if (dracLoc != NOWHERE && (round - dracRound) <= 6) {
		//printf("REEE\n");
		int pathLength = 0;
		PlaceId *travelPath;
		if (currLoc != dracLoc) {
			travelPath = HvGetShortestPathTo(hv, player, dracLoc, &pathLength);
			moveId = travelPath[0];
		}
		
		// If close, and current dracula location is unknown, investigate area
		if (pathLength <= 2 && (round - dracRound) > 1) {
			PlaceId *possMov = HvWhereCanIGo(hv, &possNum);
			int foundSusLoc = 0;
			
			// Find the intersection between reachable and suspicious nodes
			for (int i = 0; i < possNum; i++) {
				for (int j = 0; j < numSusLocs; j++) {
					if (possMov[i] == susLocs[j]) {
						moveId = possMov[i];
						foundSusLoc++;
						break;
					}
				}
			}
			if (foundSusLoc == 0 && currLoc != dracLoc) {
				moveId = travelPath[0];
			}
			
		// If close, and current dracula location is known, attack
		// Or, if still far away, continue travelling
		} else if (pathLength > 2 || (round - dracRound) == 1) {
			if (currLoc == dracLoc) {
				moveId = currLoc;
			} else {
				moveId = travelPath[0];
			}
			
		}
	}
	//for(int i = 0; i < numSusLocs; i++) {
	//	const char *place = placeIdToAbbrev(susLocs[i]);
	//	printf("%s\n", place);
	//}
	//printf("%d\n", round);
	// Got valid placeId, convert into abbrev
	placeIdRegister (moveId);
}

// When a new dracLoc is revealed, create a list of potential next moves to investigate
// Note: Does not include SEA nodes, since you can't find out dracula there
// The array will contain the location plus all its' road connections
// Its' implementation will check the first element to see if dracLoc is updated
// Then it will investigate the rest of the locations
// When it investigates a location, it removes it from the message string passed on
PlaceId *SearchArea(int *j, PlaceId dracLoc, HunterView hv) {
	*j = 0;
	PlaceId *susLocs = (PlaceId *)malloc((*j) * sizeof(PlaceId));
	ConnList curConnection = MapGetConnections(hv->gv->m, dracLoc);
	susLocs[0] = dracLoc;
	int stop = 0;

	while (stop == 0) {
		if (curConnection->type == ROAD) {
			susLocs = (PlaceId *) realloc(susLocs, sizeof(PlaceId) * (*j + 1));
			susLocs[*j] = curConnection->p;
			*j = *j + 1;

			// Dracula can't go to ST JOSEPH...
			if (curConnection->p == ST_JOSEPH_AND_ST_MARY) {
				*j = *j - 1;
			}
		}
		if (curConnection->next != NULL) {
			curConnection = curConnection->next;
		} else {
			stop++;
		}
	}

	// Remove areas already investigated
	for (int player = 0; player < 4; player++) {
		int compareLoc = 0;
		bool canFree = true;
		PlaceId *investigated = GvGetLastLocations(hv->gv, player, 3, &compareLoc, &canFree);
		

		for (int z = 0; z < 3; z++) {
			susLocs = checkOff(susLocs, investigated[z], j);
		}
	}
	
	return susLocs;
}

// Turns a valid placeId and registers it
// NOTE: Does not have a message system in place
void placeIdRegister (PlaceId moveId) {
	char moveAbbrev[3];
	moveAbbrev[2] = '\0';
	const char *place = placeIdToAbbrev(moveId);
	moveAbbrev[0] = place[0];
	moveAbbrev[1] = place[1];
	
	registerBestPlay(moveAbbrev, "Let's go!");
}

PlaceId *checkOff(PlaceId *susLocs, PlaceId checkAgainst, int *numSusLocs) {
	for (int i = 0; i < *numSusLocs; i++) {
		if (susLocs[i] == checkAgainst) {
			for (int j = i; j < *numSusLocs - 1; j++) {
				susLocs[j] = susLocs[j + 1];
			}
			*numSusLocs = *numSusLocs - 1;
		}
	}
	return susLocs;
}

int connListNum (ConnList list) {
	int numConnections;
	ConnList dup = list;
	while (list->next != NULL) {
		numConnections++;
		list = list->next;
	}
	list = dup;
	return numConnections;
}

Player CanGetThePlaceFirst(HunterView hv,PlaceId dest) {

    int pathLength = -1;
    int waylen[4];

    PlaceId *shortestpath = HvGetShortestPathTo(hv, 0, dest, &pathLength);
    waylen[0] = pathLength;
    shortestpath = HvGetShortestPathTo(hv, 1, dest, &pathLength);
    waylen[1] = pathLength;
    shortestpath = HvGetShortestPathTo(hv, 2, dest, &pathLength);
    waylen[2] = pathLength;
    shortestpath = HvGetShortestPathTo(hv, 3, dest, &pathLength);
    waylen[3] = pathLength;
    free(shortestpath);

    Player bestSelection = 0;
    int shortest = waylen[0];
    for (int i = 1;i < 4;i++) {
        if (waylen[i] < shortest) {
            bestSelection = i;
        }
    }
    return bestSelection;
}