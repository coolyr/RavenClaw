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
// GMREQUESTAGENT_EXPERIMENT.H - definition of the CGMRequestAgent grounding model 
//                      class turn-level grouding in request agents
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
//   [2006-01-31] (dbohus): added support for dynamically registering grounding
//                          model types
//   [2004-12-29] (antoine): started working on this based on GMRequestAgent
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __GMREQUESTAGENT_EXPERIMENT_H__
#define __GMREQUESTAGENT_EXPERIMENT_H__

#include "GMRequestAgent.h"

//-----------------------------------------------------------------------------
// Description of the model: 
//  The model has 5 states: failed, understanding and first, second and
//  subsequent non-understandings. 
//  The only action from the failed state should be FAIL_REQUEST
//  The only action from the understanding state should be NO_ACTION
//  The actual non-understanding actions can be taken from the nonunderstanding
//   states
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Model defines
//-----------------------------------------------------------------------------

#define SI_VERY_FIRST_NONUNDERSTANDING_CTL 2
#define SS_VERY_FIRST_NONUNDERSTANDING_CTL "VERY_FIRST_NONUNDERSTANDING_CTL"

#define SI_NONUNDERSTANDING_1_CTL 3
#define SS_NONUNDERSTANDING_1_CTL "NONUNDERSTANDING_1_CTL"

#define SI_NONUNDERSTANDING_2_CTL 4
#define SS_NONUNDERSTANDING_2_CTL "NONUNDERSTANDING_2_CTL"

#define SI_NONUNDERSTANDING_MORE_CTL 5
#define SS_NONUNDERSTANDING_MORE_CTL "NONUNDERSTANDING_MORE_CTL"

#define SI_VERY_FIRST_NONUNDERSTANDING_EXP 6
#define SS_VERY_FIRST_NONUNDERSTANDING_EXP "VERY_FIRST_NONUNDERSTANDING_EXP"

#define SI_NONUNDERSTANDING_1_EXP 7
#define SS_NONUNDERSTANDING_1_EXP "NONUNDERSTANDING_1_EXP"

#define SI_NONUNDERSTANDING_2_EXP 8
#define SS_NONUNDERSTANDING_2_EXP "NONUNDERSTANDING_2_EXP"

#define SI_NONUNDERSTANDING_MORE_EXP 9
#define SS_NONUNDERSTANDING_MORE_EXP "NONUNDERSTANDING_MORE_EXP"

//-----------------------------------------------------------------------------
// CGMRequestAgent Class - This is grounding model class for request agents
//-----------------------------------------------------------------------------

// A: forward declaration of the CMARequest class (since it's used internally
//    in the CGMConcept)
class CMARequest;

class CGMRequestAgent_Experiment : public CGMRequestAgent
{

public:

	//---------------------------------------------------------------------
	// Constructors, Destructors
	//---------------------------------------------------------------------

	CGMRequestAgent_Experiment(string sAModelPolicy = "",
		string sAName = "UNKNOWN");
	CGMRequestAgent_Experiment(CGMRequestAgent_Experiment&
		rAGMRequestAgent_Experiment);
	virtual ~CGMRequestAgent_Experiment();

	//---------------------------------------------------------------------
	// Static factory method
	//---------------------------------------------------------------------

	static CGroundingModel* GroundingModelFactory(string sModelPolicy);

	//---------------------------------------------------------------------
	// Member access methods
	//---------------------------------------------------------------------

	// Access to type
	virtual string GetType();

	//---------------------------------------------------------------------
	// Overwritten virtual methods which implement the specific 
	// request-agent grounding model functionality
	//---------------------------------------------------------------------

	// Overwritten method for cloning the model
	virtual CGroundingModel* Clone();

	// Overwritten method for loading the model policy
	virtual bool LoadPolicy();

protected:
	//---------------------------------------------------------------------
	// Auxiliary, overwritten protected methods
	//---------------------------------------------------------------------

	// Overwritten method that computes the full state for this model
	virtual void computeFullState();

	// Overwritten method that computes the belief state for this model
	virtual void computeBeliefState();
};


#endif // __GMREQUESTAGENT_H__