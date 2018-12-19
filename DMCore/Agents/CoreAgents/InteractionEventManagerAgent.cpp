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
// INTERACTIONEVENTMANAGERAGENT.CPP - handles interaction events such as
//								prompt delivery notification, new user inputs,
//								barge-in...
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
//   [2005-11-07] (antoine): added LastEventIsComplete to handle partial and
//                           complete events
//   [2005-06-21] (antoine): started this
//
//-----------------------------------------------------------------------------

#include "../../../DMInterfaces/DMInterface.h"
#include "../../../DMCore/Core.h"

#include "InteractionEventManagerAgent.h"

#include "../../../DMCore/Events/GalaxyInteractionEvent.h"

//---------------------------------------------------------------------
// Constructor and destructor
//---------------------------------------------------------------------
//
// A: Default constructor
//Ĭ�Ϲ�����
CInteractionEventManagerAgent::CInteractionEventManagerAgent(string sAName, string sAConfiguration, string sAType) : CAgent(sAName, sAConfiguration, sAType)
{

	// create a "NewInput" event to handle communication across the threads
	hNewInteractionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(hNewInteractionEvent != NULL);
}

// A: destructor
//Ĭ����������
CInteractionEventManagerAgent::~CInteractionEventManagerAgent()
{
	CloseHandle(hNewInteractionEvent);
}

// A: static function for dynamic agent creation
//��̬������̬����agent
CAgent* CInteractionEventManagerAgent::AgentFactory(string sAName, string sAConfiguration)
{
	return new CInteractionEventManagerAgent(sAName, sAConfiguration);
}

//---------------------------------------------------------------------
// CAgent Class overwritten methods 
//---------------------------------------------------------------------
//��Ҫ����
void CInteractionEventManagerAgent::Reset()
{
}

//��Ҫ����
void CInteractionEventManagerAgent::Initialize()
{
	pieLastEvent = NULL;
	pieLastInput = NULL;
}

//---------------------------------------------------------------------
// InteractionEventManagerAgent class specific public methods
//---------------------------------------------------------------------

// A: Indicates if the queue has at least one event
bool CInteractionEventManagerAgent::HasEvent()
{
	return !qpieEventQueue.empty();
}

// A: Dequeues one event from the queue
// A���Ӷ�����ȡ��һ���¼�
CInteractionEvent *CInteractionEventManagerAgent::GetNextEvent()
{
	if (!qpieEventQueue.empty())
	{

		//		pops the event from the event queue
		// <1>	�Ӷ���ɾ����һ��event
		CInteractionEvent *pieNext = qpieEventQueue.front();
		qpieEventQueue.pop();

		//		updates pointer to last event processed
		// <2>	����ָ���������¼���ָ��
		pieLastEvent = pieNext;

		//		also if the event was an input
		// <3>	����¼������� => End  => �����¼�
		if (pieNext->GetType() == IET_USER_UTT_END)
		{
			pieLastInput = pieNext;
		}

		//		pushes the event in the history
		// <4>	��ӵ���ʷ�¼�����
		vpieEventHistory.push_back(pieNext);

		return pieNext;
	}
	else
	{
		return NULL;
	}
}

// A: Returns a pointer to the last event processed
CInteractionEvent *CInteractionEventManagerAgent::GetLastEvent()
{
	return pieLastEvent;
}

// A: Returns a pointer to the last user input processed
// A������ָ��������һ���û������ָ��
CInteractionEvent *CInteractionEventManagerAgent::GetLastInput()
{
	return pieLastInput;
}

// A: Check if the last event matches a certain grammar expectation
// A��������һ���¼��Ƿ�ƥ��ĳ��slotԤ��
bool CInteractionEventManagerAgent::LastEventMatches(string sGrammarExpectation)
{
	// delegate it to the InteractionEvent class
	// ����ί�ɸ�InteractionEvent��
	return pieLastEvent->Matches(sGrammarExpectation);
}

// A: Check if the last user input matches a certain grammar expectation
bool CInteractionEventManagerAgent::LastInputMatches(string sGrammarExpectation)
{
	// delegate it to the InteractionEvent class
	return pieLastInput->Matches(sGrammarExpectation);
}

// A: Check if the last event is a complete or a partial event
// A��������һ���¼�������¼����ǲ����¼�
bool CInteractionEventManagerAgent::LastEventIsComplete()
{
	return pieLastEvent->IsComplete();
}

// A: Returns the confidence score for the last event
// A���������һ��event�����Ŷȵ÷�
float CInteractionEventManagerAgent::GetLastEventConfidence()
{
	// delegate it to the Input class
	return pieLastEvent->GetConfidence();
}

// D: Returns the string value of a grammar concept
// D�������﷨������ַ���ֵ
string CInteractionEventManagerAgent::GetValueForExpectation(string sGrammarExpectation)
{
	// delegate it to the Input class
	//����ί�и�Input��
	return pieLastEvent->GetValueForExpectation(sGrammarExpectation);
}

// A: Waits for an interaction event to arrive from the Interaction Manager
// A���ȴ������¼��ӽ�������������
void CInteractionEventManagerAgent::WaitForEvent()
{
	// <1>	��ǰδ����event����Ϊ��
	if (qpieEventQueue.empty())//��ǰδ����event����Ϊ��
	{
		//		retrieve the current thread id
		// <2>	������ǰ�߳�id
		DWORD dwThreadId = GetCurrentThreadId();

		//		send a message to the galaxy interface to wait for input
		// <3>	������Ϣ��galaxy����ȴ�����input
		PostThreadMessage(g_idDMInterfaceThread, WM_WAITINTERACTIONEVENT, 0, dwThreadId);

		// log that we started waiting for an input
		// ��־�����ǿ�ʼ�ȴ�����
		Log(INPUTMANAGER_STREAM, "Waiting for interaction event ...");

		//		and then wait for the utterance to appear
		// <4>	Ȼ��ȴ����ֵĻ���utterance
		WaitForSingleObject(hNewInteractionEvent, INFINITE);

		//		process the new event
		// <5>	�������¼�
		CInteractionEvent *pieEvent = NULL;

		//#####################################��ȡ �¼�################################################
		//#ifdef GALAXY
		// identify the type of event
		// �¼�����
		/*
			#define IET_DIALOG_STATE_CHANGE	"dialog_state_change"
			#define IET_USER_UTT_START	"user_utterance_start"
			#define IET_USER_UTT_END	"user_utterance_end"
			#define IET_PARTIAL_USER_UTT "partial_user_utterance"
			#define IET_SYSTEM_UTT_START	"system_utterance_start"
			#define IET_SYSTEM_UTT_END	"system_utterance_end"
			#define IET_SYSTEM_UTT_CANCELED	"system_utterance_canceled"
			#define IET_FLOOR_OWNER_CHANGES "floor_owner_changes"
			#define IET_SESSION "session"
			#define IET_GUI "gui"
		*/
		string sType = (string)Gal_GetString((Gal_Frame)gfLastEvent, ":event_type");
		Gal_Frame gfEventFrame = Gal_CopyFrame((Gal_Frame)gfLastEvent);

		//		create the appropriate event object
		// <6>	�����¼������¼�������gfEventFrame�洢���ڹ��캯���н�������Ҫ���¼�����pieEvent
		pieEvent = new CGalaxyInteractionEvent(gfEventFrame);

		if (sType == IET_USER_UTT_END)
		{
			Log(INPUTMANAGER_STREAM, "New user input [User:%s]", pieEvent->GetStringProperty("[uttid]").c_str());
		}
		//#endif
		//#####################################��ȡ �¼�################################################

		//		push the event at the end of the event queue
		// <7>	����δ�����¼�����
		qpieEventQueue.push(pieEvent);

		// log it
		Log(INPUTMANAGER_STREAM, "New interaction event (%s) arrived "
			"(dumped below)\n%s",
			sType.c_str(), pieEvent->ToString().c_str());

	}//��ǰδ����event����Ϊ��
}

// A: Used by the Galaxy Bridge to signal that a new event has arrived
void CInteractionEventManagerAgent::SignalInteractionEventArrived()
{
	// signal that the input has appeared
	SetEvent(hNewInteractionEvent);
}

