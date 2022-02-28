////////////////////////////////////////////////////////////////////////
// Runs a player's game turn ...
//
// Can  produce  either a hunter player or a Dracula player depending on
// the setting of the I_AM_DRACULA #define
//
// This  is  a  dummy  version of the real player.c used when you submit
// your AIs. It is provided so that you can test whether  your  code  is
// likely to compile ...
//
// Note that this is used to drive both hunter and Dracula AIs. It first
// creates an appropriate view, and then invokes the relevant decideMove
// function,  which  should  use the registerBestPlay() function to send
// the move back.
//
// The real player.c applies a timeout, and will halt your  AI  after  a
// fixed  amount of time if it doesn 't finish first. The last move that
// your AI registers (using the registerBestPlay() function) will be the
// one used by the game engine. This version of player.c won't stop your
// decideMove function if it goes into an infinite loop. Sort  that  out
// before you submit.
//
// Based on the program by David Collien, written in 2012
//
// 2017-12-04	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v1.2	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10	v1.3	Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Game.h"
#ifdef I_AM_DRACULA
# include "dracula.h"
# include "DraculaView.h"
#else
# include "hunter.h"
# include "HunterView.h"
#endif

// Moves given by registerBestPlay are this long (including terminator)
#define MOVE_SIZE 3

// The minimum static globals I can get away with
static char latestPlay[MOVE_SIZE] = "";
static char latestMessage[MESSAGE_SIZE] = "";

// A pseudo-generic interface, which defines
// - a type `View',
// - functions `ViewNew', `decideMove', `ViewFree',
// - a trail `xtrail', and a message buffer `xmsgs'.
#ifdef I_AM_DRACULA

typedef DraculaView View;

# define ViewNew DvNew
# define decideMove decideDraculaMove
# define ViewFree DvFree

# define xPastPlays "GCD.... SSZ.... HFR.... MSR.... DCD.V.. GGA.... SGA.... HMU.... MAL.... DGAT... GGATD.. SGAD... HZA.... MMS.... DKLT... GCDV... SGA.... HMU.... MAO.... DD3T... GGA.... SCDTD.. HVE.... MMS.... DHIT... GCDTD.. SGA.... HBD.... MMR.... DTPT... GBE.... SCDTD.. HKLT... MGO.... DTPT... GGA.... SBE.... HCDTD.. MVE.... DGAT... GGATD.. SBC.... HGAD... MBD.... DCNT... GGA.... SSZ.... HSZ.... MVI.... DBS.... GGA.... SVI.... HBD.... MMU.... DIO.... GGA.... SMU.... HVI.... MMI.... DTS.... GGA.... SZU.... HZA.... MNP.... DMS.... GGA.... SFR.... HMU.... MTS...."
# define xMsgs { "", "", "", "" }

#else

typedef HunterView View;

# define ViewNew HvNew
# define decideMove decideHunterMove
# define ViewFree HvFree

# define xPastPlays "GLO.... SAL.... HLI.... MBE.... DCD.V.. GEC.... SSR.... HVI.... MKL.... DGAT... GNS.... SMR.... HBD.... MGATD.. DBCT... GHA.... SMI.... HSZ.... MGA.... DSOT... GBR.... SVE.... HCN.... MBCT... DSAT... GPR.... SVI.... HBE.... MBC.... DIO.... GVI.... SBC.... HKL.... MSOT... DTS..V. GBC.... SSO.... HSZ.... MSAT... DD1.... GSO.... SVA.... HBE.... MSA.... DMS.... GSA.... SSA.... HSA.... MIO.... DAO.... GVA.... SVA.... HIO.... MSA.... DLST... GVA.... SVA.... HIO.... MSA.... DSNT... GVA.... SIO.... HTS.... MIO.... DSRT... GIO.... STS.... HRO.... MTS.... DBO.V.. GIO.... STS.... HRO.... MTS.... DNAT... GIO.... SMS.... HTS.... MMS.... DPAT... GIO.... SMS.... HTS.... MMS.... DBUT.M. GIO.... SBA.... HMS.... MBA.... DCOT.M. GIO.... SBA.... HMS.... MBA.... DLIT.M. GIO.... SSR.... HMR.... MTO.... DBRT.V. GIO.... SSR.... HMR.... MTO.... DPRT.M. GIO.... SPAT... HBUT... MMR.... DVIT... GTS.... SCOT... HAM.... MPA.... DBDT... GMS.... SFR.... HCO.... MST.... DKLT... GMS.... SFR.... HCO.... MST.... DCDT.M. GTS.... SBRT... HST.... MNU.... DGAT... GIO.... SBDT... HNU.... MPRT... DCN.V.. GBS.... SBD.... HPR.... MBD.... DBS..M. GCNV... SSZ.... HBD.... MGAT... DIO.... GBC.... SCN.... HCN.... MCN.... DTS..M. GCN.... SVR.... HBC.... MBC.... DD1..M. GBE.... SCN.... HCN.... MCN.... DMS.... GBC.... SGA.... HVR.... MBE.... DBAT... GBC.... SGA.... HVR.... MBE.... DSRT... GBC.... SCN.... HBS.... MSO.... DSNT... GBC.... SCN.... HBS.... MSO.... DMAT... GSO.... SBS.... HIO.... MVA.... DLST... GSO.... SBS.... HIO.... MVA.... DCAT... GSO.... SIO.... HTS.... MIO.... DGRT.M. GSO.... SIO.... HTS.... MIO.... DAL.VM. GSO.... STS.... HMS.... MTS.... DSRT.M. GSO.... STS.... HMS.... MTS.... DSNT.M. GSO.... SMS.... HAO.... MMS.... DMAT.M. GSO.... SMS.... HAO.... MMS.... DHIT.M. GSO.... SALV... HMS.... MAL.... DCAT.M. GVA.... SGR.... HAL.... MMATT.. DAO.... GIO.... SCAT... HMA.... MLS.... DGWT.M. GTS.... SGR.... HCA.... MCA.... DDUT.M. GMS.... SCA.... HGR.... MGR.... DD1T... GAO.... SGR.... HCA.... MMA.... DHIT... GLS.... SCA.... HGR.... MCA.... DIR.... GLS.... SCA.... HGR.... MCA.... DSWT... GLS.... SAO.... HCA.... MAO.... DLO.VM. GLS.... SAO.... HCA.... MAO.... DMNT.M. GAO.... SGW.... HAO.... MIR.... DEDT.M. GIR.... SDUT... HIR.... MDU.... DHIT... GIR.... SDU.... HIR.... MDU.... DD1T... GIR.... SIR.... HLV.... MIR.... DNS..M. GIR.... SIR.... HLV.... MIR.... DHAT.V. GIR.... SLV.... HMNT... MSW.... DLIT... GIR.... SLV.... HMN.... MSW.... DNUT.M. GLV.... SEDTT.. HED.... MLO.... DMUT... GED.... SMN.... HMN.... MMN.... DZAT... GED.... SMN.... HMN.... MMN.... DSZT... GED.... SED.... HED.... MED.... DBET.M. GED.... SED.... HED.... MED.... DBC.VM. GED.... SNS.... HNS.... MNS.... DGAT.M. GED.... SNS.... HNS.... MNS.... DCDT.M. GNS.... SHA.... HHA.... MHA.... DKLT.M. GHA.... SBR.... HVI.... MBR.... DSZT.M. GBR.... SBD.... HBD.... MPR.... DBET.M. GBD.... SKLT... HSZT... MVI.... DSJT.V. GZA.... SBET... HSO.... MBC.... DZAT.M. GZATD.. SSZ.... HSJT... MSO.... DMUT.M. GMUTD.. SZA.... HZA.... MSJ.... DZUT... GJM.... SVI.... HMU.... MZA.... DMRT... GZA.... SVE.... HMI.... MMU.... DTOT... GMU.... SMU.... HST.... MNU.... DCFT... GZUT... SZU.... HGE.... MST.... DBO.V.. GMRT... SMR.... HCFT... MPA.... DBB.... GTOT... SCF.... HCF.... MBOV... DSNT... GTO.... SNA.... HBO.... MSR.... DLST... GBO.... SBO.... HTO.... MBO.... DAO.... GSNT... SSR.... HSR.... MMA.... DMS.... GSN.... SSN.... HLST... MCA.... DTS.... GLS.... SLS.... HLS.... MLS.... DIO.... GSN.... SSN.... HCA.... MCA.... DBS.... GMA.... SLS.... HLS.... MLS.... DCNT... GMA.... SLS.... HLS.... MLS.... DGAT... GMA.... SAL.... HBA.... MAO.... DCDT... GSR.... SMS.... HMS.... MNS.... DHIT... GMR.... STS.... HTS.... MHA.... DKL.V.. GMI.... SIO.... HIO.... MVI.... DBCT... GVE.... SSA.... HBS.... MBD.... DD1T.M. GBD.... SBE.... HCN.... MKLV... DSOT.M. GGA.... SBCTT.. HBC.... MBC.... DVAT.M. GBC.... SBC.... HSOT... MSO.... DATT.M. GSO.... SSO.... HBE.... MSA.... DHIT... GSJ.... SVAT... HSO.... MVA.... DIO.... GVA.... SATTT.. HVA.... MAT.... DTS.... GAT.... SAT.... HAT.... MVA.... DGOT... GVA.... SAT.... HVA.... MAT.... DVET... GAT.... SVA.... HAT.... MVA.... DMUT... GVA.... SAT.... HVA.... MAT.... DZAT... GVA.... SAT.... HVA.... MAT.... DSZ.V.. GVA.... SIO.... HIO.... MIO.... DKLT... GVA.... SIO.... HIO.... MIO.... DCDT.M. GSO.... SBS.... HCD.... MSA.... DGAT.M. GBC.... SCN...."
# define xMsgs { "", "", "" }

#endif

int main(void)
{
	char *pastPlays = xPastPlays;
	Message msgs[] = xMsgs;

	View state = ViewNew(pastPlays, msgs);
	decideMove(state);
	ViewFree(state);

	printf("Move: %s, Message: %s\n", latestPlay, latestMessage);
	return EXIT_SUCCESS;
}

// Saves characters from play (and appends a terminator)
// and saves characters from message (and appends a terminator)
void registerBestPlay(char *play, Message message)
{
	strncpy(latestPlay, play, MOVE_SIZE - 1);
	latestPlay[MOVE_SIZE - 1] = '\0';

	strncpy(latestMessage, message, MESSAGE_SIZE - 1);
	latestMessage[MESSAGE_SIZE - 1] = '\0';
}