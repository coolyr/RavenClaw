//=============================================================================
//
//   Copyright (c) 2000-2005, Carnegie Mellon University.  
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
// GALAXYINTERACTIONEVENT.H - encapsulates an interaction event such as a notification
//							of prompt delivery, a new user input, barge-in...
// GALAXYINTERACTIONEVENT.H - 封装交互事件，如notification，prompt, 新用户输入，插入...
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
//   [2005-06-22] (antoine): started this
//
//-----------------------------------------------------------------------------

#pragma once
#ifndef __GALAXYINTERACTIONEVENT_H__
#define __GALAXYINTERACTIONEVENT_H__

//#ifdef GALAXY

#include "InteractionEvent.h"

//-----------------------------------------------------------------------------
// CInteractionEvent Class - 
//   This class is the base class for events related to interaction with the 
//   user and the outside world.
//-----------------------------------------------------------------------------
class CGalaxyInteractionEvent : public CInteractionEvent
{

public:
	//static string sConfidenceFeatureName;
	// string specifying the name of the confidence
	// slot specifier in the helios results

private:
	//---------------------------------------------------------------------
	// Private members
	//---------------------------------------------------------------------
	//

	//	the Galaxy frame containing the event
	//	包含事件的Galaxy Frame
	void  *gfEvent;

public:

	//---------------------------------------------------------------------
	// Constructor and destructor
	//---------------------------------------------------------------------
	//
	// Default constructor
	CGalaxyInteractionEvent();

	// Constructs an event from a Galaxy frame
	CGalaxyInteractionEvent(void *gfAEvent);

	// destructor
	~CGalaxyInteractionEvent();

public:

	//---------------------------------------------------------------------
	// CGalaxyInteractionEvent-specific methods
	//---------------------------------------------------------------------
	//

	// Returns the Galaxy frame for the event
	void *GetEventFrame();
};

//#endif // GALAXY

#endif // __INTERACTIONEVENT_H__
