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
// GALAXYINTERFACE.H   - defines an interface to a Galaxy "Communicator-like"
//						 outside world (i.e. a Galaxy architecture. 
//                       The CPP file also contains the Galaxy server functions
//                       of the dialog manager.
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

#pragma once
#ifndef __GALAXYINTERFACE_H__
#define __GALAXYINTERFACE_H__

// D: Check that the compiler configuration is correct (these files should 
//    be compiled only when GALAXY is defined)
#ifndef GALAXY
#error GalaxyInterface.h should be compiled only in the galaxy version
#endif

#include "../Utils/Utils.h"

// D: Frame which holds the current parse. The InteractionEventManagerAgent will make a 
// copy of it, as soon as the input is signaled. There's no concurrency
// mechanism used here (the hope is that we won't get a second frame in 
// that very short amount of time -- that should always be the case) 
// *** actually it might be a better idea to implement a queue which
//     would solve this concurrency problem - that should be done for both
//     the OAA and the GALAXY DM interfaces
extern Gal_Frame gfCurrentParseFrame;

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
extern STRING2STRING s2sInputMetaInfo;

// A: Frame holding an incoming interaction event, until it's pushed on to
//    the InteractionEventManagerAgent's event queue
extern Gal_Frame gfLastEvent;


// D: a handle to the standard output (for color printing in the console)
extern HANDLE hStdOutput;

// D: string holding the session id
extern string sSessionID;

//-----------------------------------------------------------------------------
// Functions for access to DMInterface internal variables (these functions - 
// prefixed with DMI_ - have the same signature for all interfaces)
//-----------------------------------------------------------------------------

// D: access to the timeout period internal variable
void DMI_SetTimeoutPeriod(int iATimeoutPeriod);
int  DMI_GetTimeoutPeriod();

// D: print messages on the screen
void DMI_DisplayMessage(const char* lpszMessage, int iVerbosity=0);

// D: disable/enable the dmi interface messages
void DMI_SetVerbosity(int iADMI_Verbosity);

// D: set/reset the bInSession flag
void DMI_SetInSessionFlag(bool bAInSession);

// D: obtain the session id
int DMI_GetIntSessionID();
string DMI_GetSessionID();

//-----------------------------------------------------------------------------
// Functions for calling upon the services of another galaxy server
// (this is Galaxy-specific)
//用于调用另一个星系服务器的服务的函数（这是特定于Galaxy的）
//-----------------------------------------------------------------------------
typedef struct {
	string sModuleFunction;		// the name of the module and the function  模块-函数
	STRING2STRING s2sInputs;	// the inputs for this call					输入
	STRING2STRING s2sOutputs;	// the outputs for this call				输出
	bool bBlockingCall;			// indicates if the call is blocking or not	是否Blocking
} TGIGalaxyCall;

typedef struct {
	string sModuleFunction;		// the name of the module and the function
	string sActionType;
	STRING2STRING s2sProperties;	// the inputs for this call
} TGIGalaxyActionCall;

// D: Function for issuing blocking and non-blocking calls towards other 
//    Galaxy servers. It uses a TGIGalaxyCall structure as input, and based on
//    it, it assembles an appropriate frame to be sent out to the server.
bool CallGalaxyModuleFunction(TGIGalaxyCall *pGalaxyCall);

// A: sends an action request to the Galaxy Hub
bool SendActionThroughHub(TGIGalaxyActionCall *pGalaxyCall);

// A&D: Sends a :close_session message to the Galaxy hub
void DMI_SendEndSession();

#endif // __GALAXYINTERFACE_H__