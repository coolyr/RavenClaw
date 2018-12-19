//=============================================================================
//
//   Copyright (c) 2000-2004, Carnegie Mellon University.  
//   All rights reserved.
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
//
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer. 
//
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in
//      the documentation and/or other materials provided with the
//      distribution.
//
//   This work was supported in part by funding from the Defense Advanced 
//   Research Projects Agency and the National Science Foundation of the 
//   United States of America, and the CMU Sphinx Speech Consortium.
//
//   THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
//   ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
//   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//   PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
//   NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
//   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
//   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================

//-----------------------------------------------------------------------------
// 
// OAAINTERFACE.CPP    - implements the interface between the dialog manager
//                       system and the rest of the OAA architecture. 
//                       Basically this file contains the main "solvables" of
//					     the dialog manager agent, plus a couple of other
//                       functions to provide access to the incoming messages
//						 and allow for calling other OAA agents from the
//                       DM Core. 
//
// ----------------------------------------------------------------------------
// 
// BEFORE MAKING CHANGES TO THIS CODE, please read the appropriate 
// documentation, available in the Documentation folder. 
//
// ANY SIGNIFICANT CHANGES made should be reflected back in the documentation
// file(s)
//
// ANY CHANGES made (even small bug fixed, should be reflected in the history
// below, in reverse chronological order
// 
// HISTORY --------------------------------------------------------------------
//
//   [2002-12-03] (dbohus): added control over bInSession flag
//   [2002-10-22] (dbohus): added verbosity for the DMI
//   [2002-06-06] (dbohus): started working on this by mirroring the 
//							GalaxyInterface files.
// 
//-----------------------------------------------------------------------------

// D: Check that the compiler configuration is correct (these files should 
//    be compiled only when OAA is defined)
#ifndef OAA
#error OAAInterface.cpp should be compiled only in the OAA version
#endif

#include "OAAInterface.h"
#include "../DMCore/DMBridge.h"
#include "../Utils/DebugUtils.h"
#include "../DMCore/Agents/CoreAgents/AllCoreAgents.h"


// D: the timeout period in seconds, access is given through 
//    DMInterface_SetTimeoutPeriod and DMInterface_GetTimeoutPeriod
static int iTimeoutPeriod = 5;

// D: control the dmi interface verbosity level
static int iDMI_Verbosity = 2;

// D: ICL term which holds the current input. The InteractionEventManagerAgent will make a 
// copy of it, as soon as the input is signaled. There's no concurrency
// mechanism used here (the hope is that we won't get a second input in 
// that very short amount of time -- that should always be the case) 
// *** actually it might be a better idea to implement a queue which
//     would solve this concurrency problem - that should be done for both
//     the OAA and the GALAXY DM interfaces
ICLTerm* picltCurrentInput;

// D: String hash which contains various meta-information for the input. 
// The InteractionEventManagerAgent will make a copy of it, as soon as the input is 
// signaled. There's no concurrency mechanism used here (the hope is that we 
// won't get a second input in that very short amount of time -- that 
// should always be the case) 
// *** actually it might be a better idea to implement a queue which
//     would solve this concurrency problem - that should be done for both
//     the OAA and the GALAXY DM interfaces. 
// *** The queue should be formed of structures incorporating both the
//     input and the meta info
STRING2STRING s2sInputMetaInfo;

// *** Missing support for session ids 

//-----------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions for access to DMInterface internal variables (these functions - 
// prefixed with DMI_ - have the same signature for all interfaces)
//-----------------------------------------------------------------------------

// D: set the timeout period
void DMI_SetTimeoutPeriod(int iATimeoutPeriod)
{
	iTimeoutPeriod = iATimeoutPeriod;
}

// D: get the timeout period
int DMI_GetTimeoutPeriod()
{
	return iTimeoutPeriod;
}

// D: this function displays messages related to the oaa interface on 
//    the screen
inline void DMI_DisplayMessage(const char* lpszMessage, int iVerbosity)
{
	if (iDMI_Verbosity >= iVerbosity)
		printf("[OAI@%s] %s\n", GetTimeAsString().c_str(), lpszMessage);
}

// D: disable/enable DMI messages
void DMI_SetVerbosity(int iADMI_Verbosity)
{
	iDMI_Verbosity = iADMI_Verbosity;
}

// D: set/reset the bInSession flag
void DMI_SetInSessionFlag(bool bAInSession)
{
	bInSession = bAInSession;
}

//-----------------------------------------------------------------------------
//
// D: Function for issuing blocking and non-blocking calls towards other 
//    OAA agents. It uses a TOIOAACall structure as input, and sends it
//    for solving to the facilitator
//
//-----------------------------------------------------------------------------
bool CallOAA(TOIOAACall *pOIOAACall)
{
	// merely use oaa_Solve to forward the call to the OAA facilitator
	oaa_Solve(pOIOAACall->picltGoal, pOIOAACall->picltInitialParams,
		&(pOIOAACall->ppicltOutParams), &(pOIOAACall->ppicltSolutions));
	return true;
}

//-----------------------------------------------------------------------------
// D: Functions for managing installation/removal of time triggers
//-----------------------------------------------------------------------------

static string sLastTimeTriggerInstalled = "";	// keeps the last time trigger 
// that was installed

// D: function for removing the last installed time trigger
void RemoveLastTimeTrigger()
{
	// check if there still is a last time trigger installed
	if (sLastTimeTriggerInstalled != "")
	{
		char *lpszTimeTriggerSpec = (char *)(sLastTimeTriggerInstalled.c_str());
		oaa_RemoveTrigger(
			"time",
			icl_NewTermFromString(lpszTimeTriggerSpec),
			icl_NewTermFromString("oaa_Solve(service_inactivity_timeout(foo), [])"),
			icl_NewTermFromString("[address(name(alarm))]"),
			NULL);
		sLastTimeTriggerInstalled = "";
	}
}

// D: function for installing a time trigger. It will also remove the last
//    time trigger that was installed
void InstallTimeTrigger(string sTimeTriggerSpec)
{

	// remove the previous trigger if there was one
	RemoveLastTimeTrigger();

	// install the new one
	char *lpszTimeTriggerSpec = (char *)(sTimeTriggerSpec.c_str());
	oaa_AddTrigger(
		"time",
		icl_NewTermFromString(lpszTimeTriggerSpec),
		icl_NewTermFromString("oaa_Solve(service_inactivity_timeout(foo), [])"),
		icl_NewTermFromString("[address(name(alarm))]"),
		NULL);

	// mark it as the last installed one
	sLastTimeTriggerInstalled = sTimeTriggerSpec;
}

//-----------------------------------------------------------------------------
// 
// D: OAA Solvables for the RavenClaw Dialog Manager 
//
//-----------------------------------------------------------------------------
// D: Solvable for new arriving inputs
int resolved_lf(ICLTerm* icltGoal, ICLTerm* icltParams,
	ICLTerm* icltSolutions)
{

	DMI_DisplayMessage("resolved_lf called; starting DM processing.", 1);

	// remove the last time trigger
	RemoveLastTimeTrigger();

	// copy the new input
	picltCurrentInput = icl_CopyTerm(icltGoal);

	// clear the meta information (no meta information to be set in this case)
	s2sInputMetaInfo.clear();

	// Call Dialog Management Module
	DoDialogFlow();

	// *** fakely call the start_inactivity_timeout. This will need to 
	// dissappear from here and be called by the TTS agent upon synthesis 
	// completion
	DMI_DisplayMessage("FAKE Calling start_inactivity.", 1);
	ICLTerm *X;
	oaa_Solve(icl_NewTermFromString("start_inactivity_timeout(foo)"),
		icl_NewTermFromString("[]"),
		&X, &X);

	// and get back finally 
	DMI_DisplayMessage("DM processing finished.", 1);

	return 0;
}

// D: Solvable for signaling the start of an inactivity timeout
int start_inactivity_timeout(ICLTerm* icltGoal, ICLTerm* icltParams,
	ICLTerm* icltSolutions)
{

	DMI_DisplayMessage("start_inactivity_timeout called; installing time "\
		"trigger.", 1);

	InstallTimeTrigger(FormatString("time_phrase('in %d seconds')",
		iTimeoutPeriod));

	DMI_DisplayMessage("Time trigger successfully installed.", 1);

	return 0;
}

// D: Solvable for signaling that the timeout has elapsed
int service_inactivity_timeout(ICLTerm* icltGoal, ICLTerm* icltParams,
	ICLTerm* icltSolutions)
{
	DMI_DisplayMessage("service_inactivity_timeout called; starting DM "\
		"processing.", 1);

	// the new input is empty in this case
	picltCurrentInput = icl_NewTermFromString("resolved_lf(timeout)");

	// clear the last installed trigger, since it was just triggered
	sLastTimeTriggerInstalled = "";

	// clear the meta information (no meta information to be set in this case)
	s2sInputMetaInfo.clear();
	s2sInputMetaInfo.insert(STRING2STRING::value_type(TIMEOUT_ELAPSED, ""));

	// Call Dialog Management Module
	DoDialogFlow();

	// *** fakely call the start_inactivity_timeout. This will need to 
	// dissappear from here and be called by the TTS agent upon synthesis 
	// completion
	DMI_DisplayMessage("FAKE Calling start_inactivity.", 1);
	ICLTerm *X;
	oaa_Solve(icl_NewTermFromString("start_inactivity_timeout(foo)"),
		icl_NewTermFromString("[]"),
		&X, &X);

	// and get back finally 
	DMI_DisplayMessage("DM processing finished.", 1);
	return 0;
}

// D: Solvable for signaling the cancellation of an inactivity timeout
int cancel_inactivity_timeout(ICLTerm* icltGoal, ICLTerm* icltParams,
	ICLTerm* icltSolutions)
{

	// *** this will need to be called externally as soon as there's 
	// a significant input

	DMI_DisplayMessage("cancel_inactivity_timeout called; removing time "\
		"trigger.", 1);

	RemoveLastTimeTrigger();

	DMI_DisplayMessage("Time trigger successfully removed.", 1);

	return 0;
}

//-----------------------------------------------------------------------------
// D: The OAA Callback / Dispatcher function 
//-----------------------------------------------------------------------------
// D: This function gets called whenever a new solvable is directed towards
//    the RavenClaw dialog manager. 
int OAACallBack(ICLTerm* icltGoal, ICLTerm* icltParams,
	ICLTerm* icltSolutions)
{

	// display a message to indicate that the new input has arrived
	DMI_DisplayMessage(
		FormatString("New OAA request (dumped below). Started DM processing.\n"\
		"Goal: %s \nParameters: %s\nSolutions: %s.",
		icl_NewStringFromTerm(icltGoal),
		icl_NewStringFromTerm(icltParams),
		icl_NewStringFromTerm(icltSolutions)).c_str(), 2);

	string sGoal = icl_Functor(icltGoal);

	// dispatch the call to the appropriate routine
	if (sGoal == "resolved_lf")
		return resolved_lf(icltGoal, icltParams, icltSolutions);
	else if (sGoal == "start_inactivity_timeout")
		return start_inactivity_timeout(icltGoal, icltParams, icltSolutions);
	else if (sGoal == "service_inactivity_timeout")
		return service_inactivity_timeout(icltGoal, icltParams, icltSolutions);
	else if (sGoal == "cancel_inactivity_timeout")
		return cancel_inactivity_timeout(icltGoal, icltParams, icltSolutions);

	return 0;
}


//-----------------------------------------------------------------------------
// D: Function that declares the OAA solvables for the RavenClaw dialog manager
//-----------------------------------------------------------------------------
#pragma warning (disable:4100)	// disable unused params warning
bool SetupOAAConnection(int argc, char *argv[])
{

	// Print splash information
	printf("RAVENCLAW Dialog Manager [OAA Configuration]. ");
	printf("Compiled on %s, at %s. \n\n",
		__DATE__, __TIME__);

	// If _RUN_DEBUGGING_TESTS is defined in DebugUtils, then the 
	// RunDebuggingTests function will be called
#ifdef _RUN_DEBUGGING_TESTS
	DMI_DisplayMessage("Running debugging tests ...", 0);
	RunDebuggingTests();
	DMI_DisplayMessage("Debugging tests completed.", 0);
#endif

	DMI_DisplayMessage("Connecting to facilitator.", 0);

	// construct an ICL term describing the solvables
	ICLTerm* mySolvablesAsTerm = icl_NewList(NULL);
	icl_AddToList(mySolvablesAsTerm,
		icl_NewTermFromString("resolved_lf(_LF)"),
		TRUE);
	icl_AddToList(mySolvablesAsTerm,
		icl_NewTermFromString("start_inactivity_timeout(_LF)"),
		TRUE);
	icl_AddToList(mySolvablesAsTerm,
		icl_NewTermFromString("service_inactivity_timeout(_LF)"),
		TRUE);
	icl_AddToList(mySolvablesAsTerm,
		icl_NewTermFromString("cancel_inactivity_timeout(_LF)"),
		TRUE);

	// connect to facilitator
	if (com_Connect("parent", ICL_VAR) < 0)
	{
		DMI_DisplayMessage("Could not establish connection to facilitator.", 0);
		exit(1);
	}

	// register the solvables
	if (!oaa_Register("parent", "dm", mySolvablesAsTerm))
	{
		DMI_DisplayMessage("Could not register DM solvables with facilitator.", 0);
		exit(1);
	}

	// register the OAA callback
	if (!oaa_RegisterCallback("app_do_event", OAACallBack))
	{
		printf("Could not OAA callback.");
		exit(1);
	}

	// free the ICL term 
	icl_Free(mySolvablesAsTerm);

	// display connection
	DMI_DisplayMessage("RavenClaw connection to facilitator completed "\
		"successfully.", 0);

	// Initialize the RavenClaw framework
	InitializeSystem();

	// Initialize a dialog session. So far under OAA, there's no begin and 
	// end session signal, so each run of the dialog manager will represent
	// a session
	TRavenClawConfigParams rcpSessionParams(rcpRavenClawInitParams);
	rcpSessionParams.Set(RCP_LOG_DIR. "");
	InitializeDialogSession(rcpSessionParams);

	// Call Dialog Management Module
	DoDialogFlow();

	// do the OAA main loop
	oaa_MainLoop(TRUE);

	return true;
}

//-----------------------------------------------------------------------------
// D: The MAIN function
//-----------------------------------------------------------------------------
// D: this is the main function of the RavenClaw dialog manager when operating
//    as an OAA agent. 
int main(int argc, char* argv[])
{
	// OAA initialization
	oaa_Init(argc, argv);

	// setup the OAA connection
	SetupOAAConnection(argc, argv);

	// terminate
	return 0;
}
#pragma warning (default:4100)