#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Game.h"
#include "Places.h"
#include "helper.h"
#include "Map.h"
#include "GameView.h"
#include "linkedList.h"

struct gameView {
	char *pastPlays; 
	int pathLen;
	char *playerAbrev;
	node firstMove[5];	
	node lastMove[5];	
	Map m;
};

int letterToPlayer(char c) {
	switch(c) {
		case 'G':
			return PLAYER_LORD_GODALMING; 
		case 'S': 
			return PLAYER_DR_SEWARD;
		case 'H':
			return PLAYER_VAN_HELSING;
		case 'M':
			return PLAYER_MINA_HARKER;
		case 'D':
			return PLAYER_DRACULA;
	}
	fprintf(stderr, "A non-player char was passed to letterToPlayer(), |%c|\n", c);
	exit(EXIT_FAILURE);
}

PlaceId strToId(char *string, int index) {
	char location[3];
	location[0] = string[index];
	location[1] = string[index + 1];
	location[2] = '\0';
	
	return placeAbbrevToId(location);
}

void removeTraps(int removeTrap, int *trapArray, int *numTraps) {
	// Check for hunter interactions with trap
	for (int count = 0; count < *numTraps; count++) {
		if (removeTrap == trapArray[count]) {
			for (int count2 = count; count2 < *numTraps - 1; count2++) {
				trapArray[count2] = trapArray[count2 + 1];
			}
			*numTraps = *numTraps - 1;
			count--;
		}
	}
}

PlaceId assessHideDouble(GameView gv, int i) {
	PlaceId playerLoc = strToId(gv->pastPlays, i + 1);
	while ((playerLoc == HIDE) || (gv->pastPlays[i + 1] == 'D' && gv->pastPlays[i + 2] != 'U')) {
		if (gv->pastPlays[i + 1] == 'D') {
			// Minus 48 to turn string character into its' ASCII equivalent
			int turnsBack = (gv->pastPlays[i + 2] - 48);

			i = i - (40 * turnsBack);
		} else if (playerLoc == HIDE) {
			i = i - 40;
		}
		playerLoc = strToId(gv->pastPlays, i + 1);
	}
	return playerLoc;
}

int playerStats(GameView gv, int setting, Player player) {
	char playerChar = gv->playerAbrev[player]; 
	int hunterHP = GAME_START_HUNTER_LIFE_POINTS;
	int draculaHP = GAME_START_BLOOD_POINTS;
	int deathNum = 0;
	PlaceId presentLoc = NOWHERE;
	PlaceId pastLoc = NOWHERE;
	
	for (int i = 0; gv->pastPlays[i] != '\0'; i++) {
	    if (i % 8 != 0) continue;

		pastLoc = strToId(gv->pastPlays, i - 39);
		presentLoc = strToId(gv->pastPlays, i + 1);
        
		if (gv->pastPlays[i] == playerChar || playerChar == 'D') {
			if (playerChar == gv->pastPlays[i]) {
				presentLoc = strToId(gv->pastPlays, i + 1);
				presentLoc = assessHideDouble(gv, i);
			}

			// If hunterHP is 0 at start of turn, restore it to 9
			if (hunterHP == 0) {
				hunterHP = GAME_START_HUNTER_LIFE_POINTS;
			}

			// Checking for encounters (Traps, battles etc.)
			for (int j = i + 3; j < i + 7; j++) {
				if (gv->pastPlays[j] == 'T') {
					hunterHP -= 2;
				} else if (gv->pastPlays[j] == 'D') {
					hunterHP -= 4;
					draculaHP -= 10;
				}
				
				// On hunter death, end encounter early and make sure hunterHP is 0
				if (hunterHP < 1) {
					hunterHP = 0;
					deathNum++;
					j = i + 7;
				}
			}

			// End turn health changes (Healing/SeaHPLoss)
			if (gv->pastPlays[i] == playerChar && hunterHP != 0 && pastLoc == presentLoc
																	 && playerChar != 'D') {
				hunterHP += 3;
				if (hunterHP > 9) {
					hunterHP = 9;
				}
			} else if (gv->pastPlays[i] == 'D') {
				if (presentLoc == CASTLE_DRACULA) {
					draculaHP += 10;
				} else if (placeIdToType(presentLoc) == SEA) {
					draculaHP -= 2;
				}
			}

			if (playerChar == gv->pastPlays[i]) {
				presentLoc = pastLoc;
			}
		}
	}
	
	if (setting == 1) {
		return deathNum;
	} else if (setting == 2 && playerChar != 'D') {
		return hunterHP;
	} else if (setting == 2 && playerChar == 'D') {
		return draculaHP;
	}

	fprintf(stderr, "playerStats function fail\n");
	exit(EXIT_FAILURE);
}

int findLandAdj (PlaceId *reachable, ConnList curConnection,
 				int j, Player player) {		
	while (curConnection != NULL) {
		if (curConnection->type == ROAD) {
			reachable[j] = curConnection->p;
			j++;
			
			// Dracula cannot specifically go to ST JOSEPH/ST MARY
			if (player == PLAYER_DRACULA && curConnection->p == ST_JOSEPH_AND_ST_MARY) {
				j--; // Next connection will overwrite ST_JOSEPH
			}
		}
		curConnection = curConnection->next;
	}

	return j;
}

int findSeaAdj (PlaceId *reachable, ConnList curConnection,
 				int j, Player player) {		
	while (curConnection != NULL) {
		if (curConnection->type == BOAT) {
			reachable[j] = curConnection->p;
			j++;
		}
		curConnection = curConnection->next;
	}

	return j;
}

int	findRailConnections(GameView gv, ConnList curConnection, PlaceId *reachable,
						int j, int transitNum) {
	if (transitNum == 0) {
		return j;
	}
	
	while (curConnection != NULL) {
		if (curConnection->type == RAIL) {				
			// Check for duplicate stations (All stations must be unique)
			int findDup = 0;
			for (int i = 0; i < j; i++) {
				if (reachable[i] == curConnection->p) {
					findDup++;
					break;
				}
			}
			// No Duplicate, add location
			if (findDup == 0) {
				reachable[j] = curConnection->p;
				j++;
			}
			// If multiple train trips are possible, call recursively
			if (transitNum > 1) {
				ConnList transitLoc = MapGetConnections(gv->m, curConnection->p);
				j = findRailConnections(gv, transitLoc, reachable, j, transitNum - 1);
			}		
		}
		curConnection = curConnection->next;
	}
	return j;
}

int min(int a, int b) {
	return a < b ? a : b; 
}

void collapseArray(int *array, int index, int size) {
	for (int i = index; i < size - 1; i++) {
		array[i] = array[i + 1];
	}
}