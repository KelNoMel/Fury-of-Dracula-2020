#include "GameView.h"
#include "Game.h"
#include "Places.h"
#include "Map.h"

// Takes a letter and returns the corresponding PLAYER number
int letterToPlayer(char c);

// Return the smaller integer 
int min(int a, int b); 

// Takes the GameView pastPlays string and finds the Id following index
PlaceId strToId (char *string, int index);	

// Takes a placeId and an array of placeIds, removing any similar Ids
void removeTraps(int removeTrap, int *trapArray, int *numTraps);

// Use if Draculas turn is revealed DOUBLE_BACK or HIDE
// Takes the string position of draculas turn where he doubled/hid and
// returns his current location in pid form
// Checking for Double Back uses pastPlays string and discounts Dublin
// Checking for Hide uses playerLoc
PlaceId assessHideDouble(GameView gv, int i);

// Returns the deaths experienced or current health of an INDIVIDUAL player
// based on setting value
// Setting 1 = hunter deaths
// Setting 2 = player (Hunter or Dracula) health
int playerStats(GameView gv, int setting, Player player);

// Finds adjacent land connections
int findLandAdj (PlaceId *reachable, ConnList curConnection, int j, Player player);

// Finds adjacent sea connections
int findSeaAdj (PlaceId *reachable, ConnList curConnection, int j, Player player);

// Finds adjacent rail connections
int	findRailConnections(GameView gv, ConnList curConnection, PlaceId *reachable, 
						int j, int transitNum);


// Remove the element at index 'index' and move all the following elements
// down by one
void collapseArray(int *array, int index, int size);