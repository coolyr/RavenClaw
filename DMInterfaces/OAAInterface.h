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
// OAAINTERFACE.H      - defines an interface to an Open Agent Architecture
//						 outside world (like the RIACS PSA system). The CPP 
//						 files contains the interfacing functions specific 
//						 to this system.
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
//   [2002-12-03] (dbohus): added control over bInSession flag
//   [2002-10-22] (dbohus): added verbosity for the DMI
//   [2002-06-06] (dbohus): started working on this by mirroring the 
//							GalaxyInterface files.
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __OAAINTERFACE_H__
#define __OAAINTERFACE_H__


// D: Check that the compiler configuration is correct (these files should 
//    be compiled only when OAA is defined)
#ifndef OAA
#error OAAInterface.h should be compiled only in the OAA version
#endif

#include "../Utils/Utils.h"

// D: Include OAA-specific headers
extern "C"
{
#include <libicl.h>			
#include <liboaa.h>			
#include <libcom_tcp.h>		
}


// D: ICL term which holds the current input. The InteractionEventManagerAgent will make a 
// copy of it, as soon as the input is signaled. There's no concurrency
// mechanism used here (the hope is that we won't get a second input in 
// that very short amount of time -- that should always be the case) 
// *** actually it might be a better idea to implement a queue which
//     would solve this concurrency problem - that should be done for both
//     the OAA and the GALAXY DM interfaces
extern ICLTerm* picltCurrentInput;

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



//-----------------------------------------------------------------------------
// Functions for access to DMInterface internal variables (these functions - 
// prefixed with DMI_ - have the same signature for all interfaces)
//-----------------------------------------------------------------------------

// D: access to the timeout period internal variable
void DMI_SetTimeoutPeriod(int iATimeoutPeriod);
int  DMI_GetTimeoutPeriod();

// D: print messages on the screen
void DMI_DisplayMessage(const char* lpszMessage, int iVerbosity);

// D: disable/enable the dmi interface messages
void DMI_SetVerbosity(int iADMI_Verbosity);

// D: set/reset the bInSession flag
void DMI_SetInSessionFlag(bool bAInSession);

//-----------------------------------------------------------------------------
// Functions for calling upon the services of another OAA agents (this is 
// OAA-specific)
//-----------------------------------------------------------------------------

// D: structure defining the contents of an external OAA call 
typedef struct
{
	ICLTerm *picltGoal;
	ICLTerm *picltInitialParams;
	ICLTerm *ppicltOutParams;
	ICLTerm *ppicltSolutions;
} TOIOAACall;

// D: Function for issuing blocking and non-blocking calls towards other 
//    OAA agents. It uses a TOIOAACall structure as input, and sends it
//    for solving to the facilitator
bool CallOAA(TOIOAACall *pOAACall);


#endif // __OAAINTERFACE_H__