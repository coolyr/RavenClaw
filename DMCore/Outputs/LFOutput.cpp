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
// LFOUTPUT.CPP - implementation of the CLFOutput class. This class is derived 
//				  off COutput and encapsulates a logical form (Gemini-type)
//				  output for the dialog manager.
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
//   [2004-02-24] (dbohus):  changed outputs so that we no longer clone 
//                            concepts but use them directly
//   [2002-07-03] (dbohus): BUGFIX: fixed the concept parsing in CreateOutput 
//   [2002-06-25] (dbohus): unified Create and CreateWithClones in a single
//							create function which takes a bool parameter
//   [2002-06-17] (dbohus): deemed preliminary stable version 0.5
//	 [2002-06-17] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#include "LFOutput.h"

// D: Check that the compiler configuration is correct (the logical-form 
//    -type outputs are used only in a OAA configuration so far)
#ifndef OAA
#error LFOutput.cpp should be compiled only in the OAA version
#endif

#include "../../DMCore/Log.h"
#include "../../DMCore/Agents/CoreAgents/OutputManagerAgent.h"

// D: a pointer to the actual Output Manager agent
extern COutputManagerAgent* pOutputManager;

//-----------------------------------------------------------------------------
// D: Constructors and Destructor
//-----------------------------------------------------------------------------
// D: Constructor - does nothing so far
CLFOutput::CLFOutput()
{
}

// D: Virtual destructor - does nothing so far
CLFOutput::~CLFOutput()
{
}

//-----------------------------------------------------------------------------
// D: COutput-Specific public methods
//-----------------------------------------------------------------------------

// D: virtual function for cloning an output 
COutput* CLFOutput::Clone(int iNewOutputId)
{
	// create the new output
	CLFOutput* plfoClone = new CLFOutput;
	// copy the Output and ExtendedOutput fields
	plfoClone->sOutput = sOutput;
	plfoClone->sExpandedOutput = sExpandedOutput;
	// copy the rest of the stuff using the auxiliary COutput::close() method
	clone(plfoClone, iNewOutputId);
	// finally return the cloned output
	return plfoClone;
}

// D: creates the internal representation starting from a string description
//    of the prompt
bool CLFOutput::Create(string sAGeneratorAgentName, string sAOutput,
	int iAOutputId)
{

	sOutput = sAOutput;
	iOutputId = iAOutputId;
	sGeneratorAgentName = sAGeneratorAgentName;

	sExpandedOutput = "";
	sAct = "";
	sObject = "";

	// work on another copy sToParse
	string sToParse = sAOutput;
	string sTemp;

	// normalize the syntax 
	sToParse = Trim(sToParse, " \n\t}{");

	// check that the prompt is not empty
	if (sToParse == "") return false;

	// go through the string and look for concepts prefixed with the "<" sign
	while (SplitOnFirst(sToParse, "<", sTemp, sToParse))
	{
		// add the string so far
		sExpandedOutput += sTemp;
		// trim the remaining
		sToParse = TrimLeft(sToParse);
		// and go through it until the concept name is identified
		unsigned int i = 0;
		while (isalnum(sToParse[i]) || (sToParse[i] == '.') ||
			(sToParse[i] == '/')) i++;
		// obtain the concept name
		string sConceptName = sToParse.substr(0, i);
		sToParse = sToParse.substr(i, sToParse.length() - i);

		CConcept * pConcept = &pGeneratorAgent->C(sConceptName);
		if (pConcept != &NULLConcept)
		{
			// if the concept could be identified
			vcpConcepts.push_back(pConcept);
			pConcept->SetWaitingConveyance();
			vbNotifyConcept.push_back(true);
			sExpandedOutput += "'" + Trim(pConcept->GroundedHypToString()) + "'";
		}
		else
			FatalError(FormatString("Could not find concept %s relative to "\
			"agent %s for LF-output (dump below).\n%s",
			sConceptName.c_str(), sGeneratorAgentName.c_str(),
			sAOutput.c_str()));
	}

	// get the rest of the parse
	sExpandedOutput += sTemp;

	// finally, check if the starting functor is the name of an output 
	// device
	string sFunctor;
	SplitOnFirst(sExpandedOutput, "(", sFunctor, sTemp);
	if (pOutputManager->GetOutputDevice(sFunctor) == NULL)
	{
		// if not, then use the default output device
		sOutputDevice = pOutputManager->GetDefaultOutputDeviceName();
		sExpandedOutput = pOutputManager->GetDefaultOutputDevice()->sServerCall
			+ "(" + sExpandedOutput + ")";
	}
	else
	{
		// o/w set the output device to the functor, and replace the
		// device name with the device servercall in the expanded output
		sOutputDevice = sFunctor;
		sExpandedOutput =
			pOutputManager->GetOutputDevice(sFunctor)->sServerCall +
			"(" + sTemp;
	}

	return true;
}

// D: returns the string representation  to be sent to a Gemini-type output 
//    external agent. 
string CLFOutput::ToString()
{
	// return the expanded output
	return sExpandedOutput;
}
