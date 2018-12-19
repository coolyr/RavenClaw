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
// MAREQUEST.CPP - implementation of the CMARequest class. This class 
//				   implements the microagent for Request
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
//                 antoine): the request agent doesn't check for completion on 
//                            execute any more. the core takes care of that
//   [2003-04-08] (dbohus):  change completion evaluation on execution
//   [2002-11-20] (dbohus):  changed behavior so that it's an "open request" if
//                            no requested concept is specified
//   [2002-05-25] (dbohus):  deemed preliminary stable version 0.5
//   [2002-03-20] (dbohus):  started working on this
// 
//-----------------------------------------------------------------------------

#include "MARequest.h"
#include "../../../../DMCore/Core.h"

//-----------------------------------------------------------------------------
//
// D: the CMARequest class -- the microagent for Request
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constructors and Destructors
//-----------------------------------------------------------------------------

// D: default constructor
CMARequest::CMARequest(string sAName, string sAConfiguration, string sAType) :
CDialogAgent(sAName, sAConfiguration, sAType)
{
}

// D: virtual destructor - does nothing
CMARequest::~CMARequest()
{
}

//-----------------------------------------------------------------------------
// D: Static function for dynamic agent creation
//-----------------------------------------------------------------------------
CAgent* CMARequest::AgentFactory(string sAName, string sAConfiguration)
{
	return new CMARequest(sAName, sAConfiguration);
}

//-----------------------------------------------------------------------------
//
// Specialized (overwritten) CDialogAgent methods
//
//-----------------------------------------------------------------------------

// D: Execute function: issues the request prompt, then waits for user input
// D��ִ�к�������������prompt��ʾ����Ȼ��ȴ��û�����input��
TDialogExecuteReturnCode CMARequest::Execute()
{

	//		call on the output manager to send out the output
	// <1>	�������������������� 
	//		floor = fsUser �ȴ��û�����
	pOutputManager->Output(this, Prompt(), fsUser);

	//		set the timeout period
	// <2>	���ó�ʱʱ��
	pDMCore->SetTimeoutPeriod(GetTimeoutPeriod());

	//		set the nonunderstanding threshold
	// <3>	����nonunderstanding����ֵ
	pDMCore->SetNonunderstandingThreshold(GetNonunderstandingThreshold());

	//		increment the execute counter
	// <4>	����ִ�м�����
	IncrementExecuteCounter();

	// and return with 
	return dercTakeFloor;//����Floor
}

// A: Resets clears the list of outputs for this concept
void CMARequest::Reset()
{
	CDialogAgent::Reset();
	voOutputs.clear();
}

// D: Declare the expectations: add to the incoming list the expectations 
//    of this particular agent, as specified by the GrammarMapping slot
// D��Declare the expectations��
//	  ��ӵ������б�����ض��������������GrammarMapping slotָ��
int CMARequest::DeclareExpectations(TConceptExpectationList& celExpectationList)
{

	int iExpectationsAdded = 0;
	TConceptExpectationList celLocalExpectationList;
	bool bExpectCondition = ExpectCondition();//�궨�� #define EXPECT_WHEN(Condition)

	//		first get the expectations from the local "expected" concept
	// <1>	���ȴӵ��صġ�Ԥ�ڡ�����õ�����
	string sRequestedConceptName = RequestedConceptName();	//�궨�� ��REQUEST_CONCEPT��xxx����
	string sGrammarMapping = GrammarMapping();				//�궨�� #define GRAMMAR_MAPPING("![Yes]>true, ![No]>false")

	// <2>	�ǿգ�����
	if (!sRequestedConceptName.empty() && !sGrammarMapping.empty())
	{
		parseGrammarMapping(sRequestedConceptName, sGrammarMapping, celLocalExpectationList);
	}
	//		now go through it and add stuff to the 
	// <3>	�����ռ����
	for (unsigned int i = 0; i < celLocalExpectationList.size(); i++)
	{
		//		if the expect condition is not satisfied, disable this 
		//		expectation and set the appropriate reason
		//<4>	����������������㣬����ô�����ֵ�������ʵ���ԭ��
		if (!bExpectCondition)
		{
			celLocalExpectationList[i].bDisabled = true;
			celLocalExpectationList[i].sReasonDisabled = "expect condition false";
		}
		// <5>	��ӵ��б�
		celExpectationList.push_back(celLocalExpectationList[i]);
		iExpectationsAdded++;
	}

	//		now add whatever needs to come from the CDialogAgent side
	//		�����������CDialogAgent�˵��κ�����
	// <6>	���ø��෽�� - ���� TriggeredByCommands()
	iExpectationsAdded += CDialogAgent::DeclareExpectations(celExpectationList);

	//		and finally return the total number of added expectations
	// <7>	��󷵻���ӵ�����������
	return iExpectationsAdded;
}

// D: Preconditions: by default, preconditions for a request agent are that 
//    the requested concept is not available
bool CMARequest::PreconditionsSatisfied()
{
	if (RequestedConceptName() != "")
		return !RequestedConcept()->IsAvailableAndGrounded();
	else
		return true;
}

// D: SuccessCriteriaSatisfied: Request agents are completed when the concept 
//	  has become grounded
// �������ýӵ�ʱ������������
bool CMARequest::SuccessCriteriaSatisfied()
{
	if (RequestedConceptName() != "")//�����concept���ӵط���True
		return RequestedConcept()->IsUpdatedAndGrounded();
	else
	{
		// if it's an open request, then it completes when it's been tried and 
		// some concept got bound in the previous input pass
		//�������һ���򿪵�������ô���������Ժ�һЩ������ǰ������봫�ݰ����
		return ((iTurnsInFocusCounter > 0) && !pDMCore->LastTurnNonUnderstanding());//����ִ�й���������һ���û����������ĵ�
	}
}

// A: FailureCriteriaSatisfied: Request agents fail when the maximum number
//    of attempts has been made
// A��FailureCriteriaSatisfied����������ڳ�������Դ���ʱʧ��
bool CMARequest::FailureCriteriaSatisfied()//#define FAILS_WHEN(Condition)
{
	bool bFailed = (iTurnsInFocusCounter >= GetMaxExecuteCounter()) &&
		!SuccessCriteriaSatisfied();

	if (bFailed)
		Log(DIALOGTASK_STREAM, "Agent reached max attempts (%d >= %d), failing",
		iTurnsInFocusCounter, GetMaxExecuteCounter());

	return bFailed;
}

// D: Returns the request prompt as a string
// D�����ַ�����ʽ����������ʾ
string CMARequest::Prompt()
{
#ifdef GALAXY
	// by default, request the name of the requested concept
	if(RequestedConceptName() != "") {
		// get the first requested concept
		string sConceptName, sFoo;
		SplitOnFirst(RequestedConceptName(), ";", sConceptName, sFoo);
		return FormatString("{request %s agent=%s}", sConceptName.c_str(), 
			sName.c_str());
	} else
		return FormatString("{request generic agent=%s}", sName.c_str());
#endif
#ifdef OAA
	// by default return the name of the requested concept
	if(RequestedConceptName() != "") {
		// get the first requested concept
		string sConceptName, sFoo;
		SplitOnFirst(RequestedConceptName(), ";", sConceptName, sFoo);
		return FormatString("[%s]", sConceptName.c_str());
	} else 
		return "[generic]";
#endif
}


//-----------------------------------------------------------------------------
//
// Request Microagent specific methods
//
//-----------------------------------------------------------------------------

// D: Returns a string describing the concept mapping (nothing for this class)
//    Is to be overwritten by derived classes
// D������һ����������ӳ����ַ����������û���κζ������������า��
//#define GRAMMAR_MAPPING(GrammarMappingAsString)
string CMARequest::GrammarMapping()
{
	return "";
}

// D: Returns the name of the requested concept (nothing for this class); is to 
//    be overwritten by derived classes
// D����������ĸ�������ƣ����������û���κζ�����; ���������า��
//		�궨�� ��REQUEST_CONCEPT��xxx����
string CMARequest::RequestedConceptName()
{
	return "";
}

// D: Returns a pointer to the requested concept (uses the concept name)
// D������ָ������������ָ�루ʹ�ø������ƣ�
CConcept* CMARequest::RequestedConcept()
{
	if (RequestedConceptName() != "")
	{
		// get the first requested concept
		string sConceptName, sFoo;
		SplitOnFirst(RequestedConceptName(), ";", sConceptName, sFoo);
		return &C(sConceptName);
	}
	else return &NULLConcept;
}

// D: Retuns the timeout duration : by default it returns whatever the 
//    default value is in the DMCore
int CMARequest::GetTimeoutPeriod()
{
	return pDMCore->GetDefaultTimeoutPeriod();
}

// D: Retuns the nonunderstanding threshold: by default it returns whatever the 
//    default value is in the DMCore
float CMARequest::GetNonunderstandingThreshold()
{
	return pDMCore->GetDefaultNonunderstandingThreshold();
}
