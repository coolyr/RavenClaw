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
// LFOUTPUT.H - definition of the CLFOutput class. This class is derived 
//				off COutput and encapsulates a logical form (Gemini-type) 
//				output for the dialog manager.
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
//   [2002-06-25] (dbohus): unified Create and CreateWithClones in a single
//							create function which takes a bool parameter
//   [2002-06-17] (dbohus): deemed preliminary stable version 0.5
//	 [2002-06-17] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __LFOUTPUT_H__
#define __LFOUTPUT_H__

// D: Check that the compiler configuration is correct (the logical-form 
//    -type outputs are used only in a OAA configuration so far)
#ifndef OAA
#error LFOutput.h should be compiled only in the OAA version
#endif

#include "Output.h"

//-----------------------------------------------------------------------------
// D: The CLFOutput class.
//-----------------------------------------------------------------------------

class CLFOutput: public COutput {

protected:
	// protected members for holding the frame output information
	//
	string sOutput;				// the output
	string sExpandedOutput;		// the output with concept values replaced
	
public:
	//---------------------------------------------------------------------
	// Constructors and destructors
	//---------------------------------------------------------------------
	// 
	CLFOutput();
	virtual ~CLFOutput();

public:
	//---------------------------------------------------------------------
	// Public abstract methods 
	//---------------------------------------------------------------------
	
	// Overwritten virtual method which clones an output
	//
	virtual COutput* Clone(int iNewOutputId);

	// Overwritten virtual method which creates a certain frame output 
	// from a given string-represented prompt
	//
	virtual bool Create(string sAGeneratorAgentName, string sAOutput, 
					    int iAOutputId);

	// Overwritten virtual method which generates a string representation 
	// for the output that will be sent to the external output component 
	// (in this case a Rosetta-like natural language generator)
	//
	virtual string ToString();
};

#endif // __LFOUTPUT_H__