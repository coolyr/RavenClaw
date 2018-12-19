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
// MAINFORM.CPP - implementation of the CMAInform class. This class implements 
//                the microagent for Inform
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
//   [2005-10-24] (antoine): removed RequiresFloor method (the method inherited
//							 from CDialogAgent is now valid here)
//   [2005-10-19] (antoine): added RequiresFloor method
//   [2004-12-23] (antoine): modified constructor, agent factory to handle
//							  configurations
//   [2004-04-16] (dbohus):  added grounding models on dialog agents
//   [2003-04-25] (dbohus,
//                 antoine): the inform agent doesn't check for completion on 
//                            execute any more. the core takes care of that
//   [2003-04-08] (dbohus):  change completion evaluation on execution
//   [2002-05-25] (dbohus):  deemed preliminary stable version 0.5
//   [2002-03-17] (dbohus):  started working on this
// 
//-----------------------------------------------------------------------------

#include "MAInform.h"
#include "../../../../DMCore/Core.h"

//-----------------------------------------------------------------------------
//
// D: the CMAInform class -- the microagent for Inform
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constructors and Destructors
//-----------------------------------------------------------------------------
// D: constructor
CMAInform::CMAInform(string sAName, string sAConfiguration, string sAType) :
CDialogAgent(sAName, sAConfiguration, sAType)
{
}

// D: destructor - does nothing
CMAInform::~CMAInform()
{
}

//-----------------------------------------------------------------------------
// Static function for dynamic agent creation
//-----------------------------------------------------------------------------
CAgent* CMAInform::AgentFactory(string sAName, string sAConfiguration)
{
	return new CMAInform(sAName, sAConfiguration);
}

//-----------------------------------------------------------------------------
//
// Specialized (overwritten) CDialogAgent methods
//
//-----------------------------------------------------------------------------

// D: the Execute routine: calls upon the Output Manager to send a prompt out
// D��ִ�����̣�����Output Manager������ʾprompt
TDialogExecuteReturnCode CMAInform::Execute()
{

	// call on the output manager to send out the output
	// <1>�����������������output
	vector<COutput*> voTemp = pOutputManager->Output(this, Prompt(), fsFree);

	// <2>���뵱ǰinformAgent����������б�
	voOutputs.insert(voOutputs.end(), voTemp.begin(), voTemp.end());

	// increment the execute counter
	// <3>	����ִ�м�����
	IncrementExecuteCounter();

	//		and return with continue execution
	// <4>	����floor
	return dercTakeFloor;
}

// D: the ReOpenTopic method
// D��ReOpenTopic����
void CMAInform::ReOpenTopic()
{
	// Clears the list of emitted output prompts
	//��������������ʾ�б�
	voOutputs.clear();
	// Do the standard ReOpenTopic
	CDialogAgent::ReOpenTopic();
}

// A: Resets clears the list of outputs for this concept
void CMAInform::Reset()
{
	CDialogAgent::Reset();
	voOutputs.clear();
}

// D: SuccessCriteriaSatisfied: returns true if the inform agent has conveyed all its prompts
// D��SuccessCriteriaSatisfied�����֪ͨ����������������ʾ���򷵻�true
bool CMAInform::SuccessCriteriaSatisfied()
{

	// no output prompts: the agent wasn't executed at all yet
	// û�������ʾ������û��ִ��
	if (voOutputs.empty()) return false;

	// otherwise completes once all the prompts it have been
	// spoken or canceled (due to barge-in)
	//�������������ʾ, �ѱ�˵����ȡ�������ڲ��룩
	bool bCompleted = true;
	for (unsigned int i = 0; i < voOutputs.size(); i++)
	{
		if (voOutputs[i]->GetConveyance() == cNotConveyed)
		{
			bCompleted = false;
			break;
		}
	}
	return bCompleted;
}

// D: The Prompt method
string CMAInform::Prompt()//PROMPT(":non-interruptable inform welcome")
{
#ifdef GALAXY
	// by default, returns the name of the agent
	return FormatString("{inform %s agent=%s}", sDialogAgentName.c_str(), 
		sName.c_str());
#endif
#ifdef OAA
	// returns the name of the agent
	return FormatString("[%s]", sDialogAgentName.c_str());
#endif
}
