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
// GALAXYINTERFACE.CPP - implements the interface between the dialog manager
//                       system and the rest of the Galaxy architecture. 
//                       Basically these are the main galaxy server functions
//                       for the dialog manager, plus a couple of other
//                       functions to provide access to the incoming frames
//						 and allow for calling other Galaxy servers from the
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
// ANY CHANGES made (even small bug fixes, should be reflected in the history
// below, in reverse chronological order
// 
// HISTORY --------------------------------------------------------------------
//
//   [2005-02-08] (antoine,dbohus): added DMI_SendEndSession function
//   [2004-05-07] (dbohus): making blocking calls through GalIO_DispatchViaHub
//   [2004-04-01] (dbohus): fixed potential buffer overrun problem
//   [2003-10-10] (dbohus): fixed bug: the pLastGalSS_Environment was not set
//                           in begin_session and end_session
//   [2003-04-23] (dbohus): added support for session names, and identifying
//                           notification frames based on that 
//   [2002-12-03] (dbohus): added control over bInSession flag
//   [2002-11-21] (dbohus): fixed bug in galaxy calls so that only the slots 
//                           that exist in the return frame are retrieved
//   [2002-10-22] (dbohus): added support for DMI verbosity
//	 [2002-06-06] (dbohus): moved some of the variable declarations in the 
//							 CPP file since they are really only internal
//   [2002-05-25] (dbohus): deemed preliminary stable version 0.5
//   [2002-01-04] (dbohus): started working on this
//                           adapted most functions from the previous 
//                           Communicator dialog manager
// 
//-----------------------------------------------------------------------------

// D: Check that the compiler configuration is correct (these files should 
//    be compiled only when GALAXY is defined)
#ifndef GALAXY
#error GalaxyInterface.cpp should be compiled only in the galaxy version
#endif

extern "C"
{
#include <galaxy/galaxy_all.h>
#define SERVER_FUNCTIONS_INCLUDE "GalaxyInterfaceFunctions.h"
#define USE_SERVER_DATA
#include "galaxy/server_functions.h"
}

#include "GalaxyInterface.h"
#include "../DMCore/DMBridge.h"
#include "../Utils/DebugUtils.h"
#include "../DMCore/Agents/CoreAgents/AllCoreAgents.h"

#include "../DMCore/Log.h"

#include <set>

extern COutputManagerAgent *pOutputManager;

//-----------------------------------------------------------------------------
// Galaxy Interface internal variables
//-----------------------------------------------------------------------------

// D: specifies the input arguments for the DM Server. For now, none taken,
static char *oas[] = {
	"-config file", "the configuration file for Ravenclaw", "default.cfg",
	NULL
};

// D: pointer to a GalIO_CommStruct (used to communicate with the hub)
static void *pHubCommStruct;

// D: pointer to the last Galaxy environment (used to communicate with the hub)
static GalSS_Environment *pLastGalSS_Environment;

// D: variable which maintains the latest incoming frame
static Gal_Frame gfIncomingFrame;

// D: this variable keeps count on whether we are in a dialog session or not
static bool bInSession = false;

// D: Frame which holds the current parse. The InteractionEventManagerAgent will make a 
// copy of it, as soon as the input is signaled. There's no concurrency
// mechanism used here (the hope is that we won't get a second frame in 
// that very short amount of time -- that should always be the case) 
// *** actually it might be a better idea to implement a queue which
//     would solve this concurrency problem - that should be done for both
//     the OAA and the GALAXY DM interfaces
Gal_Frame gfCurrentParseFrame;

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

// A: Frame holding an incoming interaction event, until it's pushed on to
//    the InteractionEventManagerAgent's event queue
Gal_Frame gfLastEvent;

// D: the timeout period in seconds (default 8), access is given through 
//    DMInterface_SetTimeoutPeriod and DMInterface_GetTimeoutPeriod
static int iTimeoutPeriod = 8;

// D: control of dmi interface verbosity
static int iDMI_Verbosity = 2;

// D: a handle to the standard output (for color printing in the console)
HANDLE hStdOutput;

// D: int and string holding the session id
int iSessionID;
string sSessionID;

#define GLI_COLOR FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN

//-----------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions for access to DMInterface internal variables (these functions - 
// prefixed with DMI_ - have the same signature for all interfaces)
//-----------------------------------------------------------------------------

// D: set the timeout period
// 设置系统超时阶段
void DMI_SetTimeoutPeriod(int iATimeoutPeriod)
{
	iTimeoutPeriod = iATimeoutPeriod;
}

// D: get the timeout period
int DMI_GetTimeoutPeriod()
{
	return iTimeoutPeriod;
}

// D: display a message related to the galaxy interface on the screen
inline void DMI_DisplayMessage(const char* lpszMessage, int iVerbosity)
{
	if (iDMI_Verbosity >= iVerbosity)
	{
		char lpszBuffer[STRING_MAX];
		DWORD cWritten;
		_snprintf_s(lpszBuffer, STRING_MAX, "[GLI@%s] %s\n",
			GetTimeAsString().c_str(), lpszMessage);
		SetConsoleTextAttribute(hStdOutput, GLI_COLOR);
		WriteFile(hStdOutput, lpszBuffer, lstrlen(lpszBuffer), &cWritten, NULL);
	}
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

// A: obtain the session id
int DMI_GetIntSessionID()
{
	return iSessionID;
}

// D: obtain the session id
string DMI_GetSessionID()
{
	return sSessionID;
}

//-----------------------------------------------------------------------------
//
// D: Function for issuing blocking and non-blocking calls towards other 
//    Galaxy servers. It uses a TGIGalaxyCall structure as input, and based on
//    it, it assembles an appropriate frame to be sent out to the server.
//
//-----------------------------------------------------------------------------
bool CallGalaxyModuleFunction(TGIGalaxyCall *pGalaxyCall)
{

	Gal_Frame gfFrameToHub, gfFrameFromHub;
	GalIO_MsgType gmtMessageType;

	// create a frame to send to the HUB
	gfFrameToHub = Gal_MakeFrame((char *)pGalaxyCall->sModuleFunction.c_str(),
		GAL_CLAUSE);

	// set the inputs for the call: go through the s2sInputs hash, and set
	// the slot-values in the Galaxy frame
	STRING2STRING::iterator iPtr;
	for (iPtr = pGalaxyCall->s2sInputs.begin();
		iPtr != pGalaxyCall->s2sInputs.end();
		iPtr++)
	{
		// set the slot in the frame to the corresponding value
		Gal_SetProp(gfFrameToHub, (char *)(iPtr->first.c_str()),
			Gal_StringObject((char *)(iPtr->second.c_str())));
	}

	// check if the call is blocking or not
	if (pGalaxyCall->bBlockingCall)
	{
		// if we have a blocking call
		int iDummy;
		char *lpszFrame = Gal_PPFrameToString(gfFrameToHub, NULL, &iDummy);
		string sMessage = FormatString("Calling another galaxy server "\
			"(blocking/GalIO_CommDispatchFrame) : %s (dump below).\n%s\n",
			pGalaxyCall->sModuleFunction.c_str(), lpszFrame);
		DMI_DisplayMessage(sMessage.c_str(), 2);
		free(lpszFrame);

		// make the blocking call
		gfFrameFromHub = GalIO_DispatchViaHub(
			(GalIO_CommStruct *)pHubCommStruct, gfFrameToHub, &gmtMessageType);

		// retrieve the results of the call in the s2sOutputs hash
		STRING2STRING::iterator iPtr;
		for (iPtr = pGalaxyCall->s2sOutputs.begin();
			iPtr != pGalaxyCall->s2sOutputs.end();
			iPtr++)
		{
			// set the slot in the frame to the corresponding value, but only if
			// it exists
			string sSlot = iPtr->first;
			if (Gal_GetObject(gfFrameFromHub, (char *)(sSlot.c_str())))
			{
				iPtr->second = Gal_GetString(gfFrameFromHub,
					(char *)(iPtr->first.c_str()));
			}
		}

	}
	else
	{
		// if we have a non-blocking call
		int iDummy;
		char *lpszFrame = Gal_PPFrameToString(gfFrameToHub, NULL, &iDummy);
		string sMessage = FormatString("Calling another galaxy server "\
			"(non-blocking/GalIO_CommWriteFrame) : %s (dump below).\n%s\n",
			pGalaxyCall->sModuleFunction.c_str(), lpszFrame);
		DMI_DisplayMessage(sMessage.c_str(), 2);
		free(lpszFrame);

		// make a non-blocking call
		GalIO_CommWriteFrame((GalIO_CommStruct *)pHubCommStruct,
			gfFrameToHub, 0);

		// no results to retrieve, since we're returning immediately 
		// (non-blocking)
	}

	Gal_FreeFrame(gfFrameToHub);

	return true;
}

//-----------------------------------------------------------------------------
//
// A: sends an action request to the Galaxy Hub
//
//-----------------------------------------------------------------------------
bool SendActionThroughHub(TGIGalaxyActionCall *pGalaxyCall)
{

	// create a frame to send to the HUB
	Gal_Frame gfFrameToHub = Gal_MakeFrame((char *)pGalaxyCall->sModuleFunction.c_str(),
		GAL_CLAUSE);


	// sets the action type
	Gal_SetProp(gfFrameToHub, ":action_level", Gal_StringObject("high"));
	Gal_SetProp(gfFrameToHub, ":action_type", Gal_StringObject(
		pGalaxyCall->sActionType.c_str()));

	// creates the properties frame
	Gal_Frame gfProperties = Gal_MakeFrame("properties", GAL_CLAUSE);

	// set the inputs for the call: go through the s2sInputs hash, and set
	// the slot-values in the Galaxy frame
	STRING2STRING::iterator iPtr;
	for (iPtr = pGalaxyCall->s2sProperties.begin();
		iPtr != pGalaxyCall->s2sProperties.end();
		iPtr++)
	{
		// set the slot in the frame to the corresponding value
		Gal_SetProp(gfProperties, (char *)(iPtr->first.c_str()),
			Gal_StringObject((char *)(iPtr->second.c_str())));
	}

	Gal_SetProp(gfFrameToHub, ":properties", Gal_FrameObject(gfProperties));

	// perform a non-blocking call
	int iDummy;
	char *lpszFrame = Gal_PPFrameToString(gfFrameToHub, NULL, &iDummy);
	string sMessage = FormatString("Calling another galaxy server "\
		"(non-blocking/GalIO_CommWriteFrame) : %s (dump below).\n%s\n",
		pGalaxyCall->sModuleFunction.c_str(), lpszFrame);
	DMI_DisplayMessage(sMessage.c_str(), 2);
	free(lpszFrame);

	// make a non-blocking call
	GalIO_CommWriteFrame((GalIO_CommStruct *)pHubCommStruct,
		gfFrameToHub, 0);

	// no results to retrieve, since we're returning immediately 
	// (non-blocking)

	return true;
}

//-----------------------------------------------------------------------------
//
//   Galaxy Server functions
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// D: this function is called upon start-up of the Dialog Manager as a
//    Galaxy server
//-----------------------------------------------------------------------------
void *_GalSS_init_server(Gal_Server *server, int argc, char **argv)
{

	// check the arguments
	if (!GalUtil_OACheckUsage(argc, argv, oas, NULL))
	{
		GalUtil_OAPrintUsage(argc, argv, oas);
		exit(1);
	}

	set<CConcept*> scTest;
	if (scTest.find(&NULLConcept) == scTest.end())
	{

	}

	// Kill the galaxy verbosity, we want to see only our own diagnostic
	// messages
	GalUtil_SetVerbose(0);

	// Obtain the handle to the standard output
	hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	// Initialize the high resolution timer
	InitializeHighResolutionTimer();

	// Print splash information
	DMI_DisplayMessage("RAVENCLAW Dialog Manager [Galaxy Configuration]. ", 1);
	DMI_DisplayMessage(FormatString("Compiled on %s, at %s. \n\n",
		__DATE__, __TIME__).c_str());
	DMI_DisplayMessage("_GalSS_init_server called.", 1);

	// If _RUN_DEBUGGING_TESTS is defined in DebugUtils, then the 
	// RunDebuggingTests function will be called
#ifdef _RUN_DEBUGGING_TESTS
	DMI_DisplayMessage("Running debugging tests ...", 0);
	RunDebuggingTests();
	DMI_DisplayMessage("Debugging tests completed.", 0);
#endif

	// Extract the configuration filename parameter
	char *lpszConfigurationFileName;
	if (!GalUtil_OAExtract(argc, argv, oas, "-config", GAL_OA_STRING,
		&lpszConfigurationFileName))
		InitializeSystem("");
	else
		InitializeSystem((string)lpszConfigurationFileName);

	// TK: also look at a possible server name and port specification
	{
		string name = rcpRavenClawInitParams.Get(RCP_SERVER_NAME);
		if (!name.empty())
		{
			cerr << "found server name: " << name << endl;
			lpszDMServerName = new char[name.size() + 1];
			strcpy(lpszDMServerName, name.c_str());
		}
	}
	{
		unsigned short port = (unsigned short)atoi(
			rcpRavenClawInitParams.Get(RCP_SERVER_PORT).c_str());
		if (port) iDMServerPort = port;
	}
	GalSS_InitializeServerDefaults(server, lpszDMServerName, iDMServerPort);
	cerr << "server: " << lpszDMServerName << " port: " << iDMServerPort << endl;

	bInSession = false;
	return (void *)NULL;
}

//-----------------------------------------------------------------------------
// D: this function is called by the Galaxy architecture upon the start
//    of each new session
//-----------------------------------------------------------------------------
Gal_Frame begin_session(Gal_Frame frame, void *server_data)
{

	if (bInSession)
	{
		end_session(frame, server_data);
	}


	int iDummy;
	// set inSession to true
	bInSession = true;

	// update hub communication structure and incoming frame	
	pLastGalSS_Environment = (GalSS_Environment *)server_data;
	pHubCommStruct = GalSS_EnvComm(pLastGalSS_Environment);
	gfIncomingFrame = frame;

	DMI_DisplayMessage("begin_session called.", 0);

	char *lpszFrame = Gal_PPFrameToString(frame, NULL, &iDummy);
	DMI_DisplayMessage(FormatString("Frame received (dump below).\n%s",
		lpszFrame).c_str(), 2);
	free(lpszFrame);

	// construct the session initialization parameters starting from 
	// the RavenClaw initialization parameters
	TRavenClawConfigParams rcpSessionParams(rcpRavenClawInitParams);

	// look for the session folder in the hub frame, add it to the core
	// initialization paramters
	if (Gal_GetObject(frame, ":hub_logdir"))
	{
		rcpSessionParams.Set(RCP_LOG_DIR,
			(string)Gal_GetString(frame, ":hub_logdir"));
	}
	else
	{
		DMI_DisplayMessage(":hub_logdir not found in begin_session frame.\
						   						   						   						   			Logging will be done under .\\ .", 2);
		rcpSessionParams.Set(RCP_LOG_DIR, "");
	}

	// same for the log filename prefix 
	if (Gal_GetObject(frame, ":hub_log_prefix"))
	{
		rcpSessionParams.Set(RCP_LOG_PREFIX,
			FormatString("%s-", Gal_GetString(frame, ":hub_log_prefix")));
	}
	else
	{
		DMI_DisplayMessage(":hub_log_prefix not found in begin_session frame.\
						   						   						   						   			Using default filename.", 2);
		rcpSessionParams.Set(RCP_LOG_PREFIX, "");
	}

	if (Gal_GetObject(frame, ":session_start_timestamp"))
	{
		char *session_start_timestamp = _strdup(Gal_GetString(frame, ":session_start_timestamp"));
		DMI_DisplayMessage(FormatString("Init timestamp: %I64i", _atoi64(session_start_timestamp)).c_str(), 0);
		SetSessionStartTimestamp(_atoi64(session_start_timestamp));
	}
	else
		fprintf(stderr, "Can't find :session_start_timestamp\n");

	if (Gal_GetObject(frame, ":sess_id"))
	{
		iSessionID = Gal_GetInt(frame, ":sess_id");
		DMI_DisplayMessage(FormatString("Session id: %d", iSessionID).c_str(), 0);
	}
	else
	{
		fprintf(stderr, "Can't find :session_start_timestamp\n");
		iSessionID = 0;
	}
	sSessionID = FormatString("%s-%d", lpszDMServerName, iSessionID);

	// initialize the dialog session
	InitializeDialogSession(rcpSessionParams);

	// Call the Dialog Management Module
	DoDialogFlow();

	return frame;
}

//-----------------------------------------------------------------------------
// D: this function is called by the Galaxy architecture upon the 
//    end of each session
//-----------------------------------------------------------------------------
Gal_Frame end_session(Gal_Frame frame, void *server_data)
{

	int iDummy;

	// if the session has been terminated in the meantime, return 
	if (!bInSession) return frame;

	// update hub communication structure and incoming frame
	pLastGalSS_Environment = (GalSS_Environment *)server_data;
	pHubCommStruct = GalSS_EnvComm(pLastGalSS_Environment);
	gfLastEvent = frame;

	Gal_SetProp(gfLastEvent, ":event_type", Gal_StringObject(IET_SESSION));
	Gal_SetProp(gfLastEvent, ":event_complete", Gal_IntObject(1));
	// creates the properties frame
	Gal_Frame gfProperties = Gal_MakeFrame("properties", GAL_CLAUSE);
	Gal_SetProp(gfProperties, TERMINATE_SESSION, Gal_StringObject("true"));
	Gal_SetProp(gfLastEvent, ":properties", Gal_FrameObject(gfProperties));


	DMI_DisplayMessage("end_session called; sending terminate to Core", 1);

	char *lpszFrame = Gal_PPFrameToString(frame, NULL, &iDummy);
	DMI_DisplayMessage(FormatString("Frame received (dump below).\n%s",
		lpszFrame).c_str(), 2);
	free(lpszFrame);


	// Call the Dialog Management Module
	DoDialogFlow();

	bInSession = false;
	return frame;
}


//-----------------------------------------------------------------------------
// D: this function is called by the Galaxy architecture each time the 
//    server connects to the Hub.
//-----------------------------------------------------------------------------
Gal_Frame reinitialize(Gal_Frame frame, void *server_data)
{
	// update hub communication structures and incoming frame
	pLastGalSS_Environment = (GalSS_Environment *)server_data;
	pHubCommStruct = GalSS_EnvComm(pLastGalSS_Environment);
	gfIncomingFrame = frame;

	DMI_DisplayMessage("reinitialize called. Hub connection completed.", 1);
	return frame;
}

//-----------------------------------------------------------------------------
// D: this function is called by the Galaxy architecture when a
//    timeout period elapses
//-----------------------------------------------------------------------------
#pragma warning (disable : 4100)
void service_timeout(void *args)
{

	// if the session has been terminated in the meantime, simply return
	if (!bInSession) return;

	// log this call
	DMI_DisplayMessage("service_timeout called.", 1);

	// add meta information on the input to specify that we're dealing with 
	// a timeout
	s2sInputMetaInfo.clear();
	s2sInputMetaInfo.insert(
		STRING2STRING::value_type(TIMEOUT_ELAPSED, "true"));

	// Call the Dialog Management Module
	DoDialogFlow();

	DMI_DisplayMessage("DM processing finished.", 1);
}

//-----------------------------------------------------------------------------
// A: this is the function that gets called each time an interaction
//    event happens
//-----------------------------------------------------------------------------
Gal_Frame handle_event(Gal_Frame frame, void *server_data)
{

	int iDummy;

	// if the session has been terminated in the meantime, simply return
	if (!bInSession) return frame;

	// update hub communication structures and incoming frame
	pLastGalSS_Environment = (GalSS_Environment *)server_data;
	//pHubCommStruct = GalSS_EnvComm(pLastGalSS_Environment);
	gfLastEvent = frame;

	char *lpszFrame = Gal_PPFrameToString(frame, NULL, &iDummy);
	DMI_DisplayMessage("handle_event called.", 1);
	DMI_DisplayMessage(FormatString("Frame received (dump below).\n%s",
		lpszFrame).c_str(), 2);
	free(lpszFrame);

	DoDialogFlow();

	DMI_DisplayMessage("DM processing finished.", 1);

	// and finally return
	return frame;
};

//-----------------------------------------------------------------------------
// D: set the timeout 
//-----------------------------------------------------------------------------
Gal_Frame start_inactivity_timeout(Gal_Frame frame, void *server_data)
{

	// if the session has been terminated in the meantime, simply return
	if (!bInSession) return frame;

	// update hub communication structures and incoming frame	
	pLastGalSS_Environment = (GalSS_Environment *)server_data;
	pHubCommStruct = GalSS_EnvComm(pLastGalSS_Environment);
	gfIncomingFrame = frame;

	DMI_DisplayMessage(
		FormatString("start_inactivity_timeout called; installing time "\
		"trigger (%d secs)", iTimeoutPeriod).c_str(), 1);

	// add a timed task, so that a timeout will be called after iTimeoutPeriod
	// seconds	
	Gal_AddTimedTask(service_timeout, NULL, 1000 * iTimeoutPeriod);

	DMI_DisplayMessage("Time trigger successfully installed.", 1);

	return frame;
}

//-----------------------------------------------------------------------------
// D: cancel the timeout
//-----------------------------------------------------------------------------
Gal_Frame cancel_inactivity_timeout(Gal_Frame frame, void *server_data)
{

	// if the session has been terminated in the meantime, simply return
	if (!bInSession) return frame;

	char *szReason;

	// update hub communication structures and incoming frame	
	pLastGalSS_Environment = (GalSS_Environment *)server_data;
	pHubCommStruct = GalSS_EnvComm(pLastGalSS_Environment);
	gfIncomingFrame = frame;

	DMI_DisplayMessage("cancel_inactivity_timeout called; removing the "\
		"trigger", 1);

	if (Gal_GetObject(frame, ":why"))
	{
		// The value of why is either "startutt", "beginout" or "hangup"
		// To indicate why the timeout was cancelled. So far this is not
		// used, but it might be later on
		szReason = Gal_GetString(frame, ":why");
	}
	Gal_RemoveTimedTask(service_timeout, NULL);

	DMI_DisplayMessage("Time trigger successfully removed.", 1);

	return frame;
}

// A&D: Sends a :close_session message to the Galaxy hub
void DMI_SendEndSession()
{
	Gal_Frame gfFrameToHub;
	gfFrameToHub = Gal_MakeFrame("main", GAL_CLAUSE);

	Gal_SetProp(gfFrameToHub, ":close_session", Gal_StringObject(""));

	GalIO_CommWriteFrame((GalIO_CommStruct *)pHubCommStruct, gfFrameToHub, GAL_FALSE);

	return;
}
