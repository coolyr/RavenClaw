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
//默认构造器
CInteractionEventManagerAgent::CInteractionEventManagerAgent(string sAName, string sAConfiguration, string sAType) : CAgent(sAName, sAConfiguration, sAType)
{

	// create a "NewInput" event to handle communication across the threads
	hNewInteractionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(hNewInteractionEvent != NULL);
}

// A: destructor
//默认析构函数
CInteractionEventManagerAgent::~CInteractionEventManagerAgent()
{
	CloseHandle(hNewInteractionEvent);
}

// A: static function for dynamic agent creation
//静态方法动态闯将agent
CAgent* CInteractionEventManagerAgent::AgentFactory(string sAName, string sAConfiguration)
{
	return new CInteractionEventManagerAgent(sAName, sAConfiguration);
}

//---------------------------------------------------------------------
// CAgent Class overwritten methods 
//---------------------------------------------------------------------
//需要覆盖
void CInteractionEventManagerAgent::Reset()
{
}

//需要覆盖
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
// A：从队列中取出一个事件
CInteractionEvent *CInteractionEventManagerAgent::GetNextEvent()
{
	if (!qpieEventQueue.empty())
	{

		//		pops the event from the event queue
		// <1>	从队列删除第一个event
		CInteractionEvent *pieNext = qpieEventQueue.front();
		qpieEventQueue.pop();

		//		updates pointer to last event processed
		// <2>	更新指向最后处理的事件的指针
		pieLastEvent = pieNext;

		//		also if the event was an input
		// <3>	如果事件是输入 => End  => 输入事件
		if (pieNext->GetType() == IET_USER_UTT_END)
		{
			pieLastInput = pieNext;
		}

		//		pushes the event in the history
		// <4>	添加到历史事件队列
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
// A：返回指向处理的最后一个用户输入的指针
CInteractionEvent *CInteractionEventManagerAgent::GetLastInput()
{
	return pieLastInput;
}

// A: Check if the last event matches a certain grammar expectation
// A：检查最后一个事件是否匹配某个slot预期
bool CInteractionEventManagerAgent::LastEventMatches(string sGrammarExpectation)
{
	// delegate it to the InteractionEvent class
	// 将其委派给InteractionEvent类
	return pieLastEvent->Matches(sGrammarExpectation);
}

// A: Check if the last user input matches a certain grammar expectation
bool CInteractionEventManagerAgent::LastInputMatches(string sGrammarExpectation)
{
	// delegate it to the InteractionEvent class
	return pieLastInput->Matches(sGrammarExpectation);
}

// A: Check if the last event is a complete or a partial event
// A：检查最后一个事件是完成事件还是部分事件
bool CInteractionEventManagerAgent::LastEventIsComplete()
{
	return pieLastEvent->IsComplete();
}

// A: Returns the confidence score for the last event
// A：返回最后一个event的置信度得分
float CInteractionEventManagerAgent::GetLastEventConfidence()
{
	// delegate it to the Input class
	return pieLastEvent->GetConfidence();
}

// D: Returns the string value of a grammar concept
// D：返回语法概念的字符串值
string CInteractionEventManagerAgent::GetValueForExpectation(string sGrammarExpectation)
{
	// delegate it to the Input class
	//将它委托给Input类
	return pieLastEvent->GetValueForExpectation(sGrammarExpectation);
}

// A: Waits for an interaction event to arrive from the Interaction Manager
// A：等待交互事件从交互管理器到达
void CInteractionEventManagerAgent::WaitForEvent()
{
	// <1>	当前未处理event队列为空
	if (qpieEventQueue.empty())//当前未处理event队列为空
	{
		//		retrieve the current thread id
		// <2>	检索当前线程id
		DWORD dwThreadId = GetCurrentThreadId();

		//		send a message to the galaxy interface to wait for input
		// <3>	发送消息到galaxy界面等待输入input
		PostThreadMessage(g_idDMInterfaceThread, WM_WAITINTERACTIONEVENT, 0, dwThreadId);

		// log that we started waiting for an input
		// 日志，我们开始等待输入
		Log(INPUTMANAGER_STREAM, "Waiting for interaction event ...");

		//		and then wait for the utterance to appear
		// <4>	然后等待出现的话语utterance
		WaitForSingleObject(hNewInteractionEvent, INFINITE);

		//		process the new event
		// <5>	处理新事件
		CInteractionEvent *pieEvent = NULL;

		//#####################################获取 事件################################################
		//#ifdef GALAXY
		// identify the type of event
		// 事件类型
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
		// <6>	创建事件对象，事件属性在gfEventFrame存储，在构造函数中解析成需要的事件对象pieEvent
		pieEvent = new CGalaxyInteractionEvent(gfEventFrame);

		if (sType == IET_USER_UTT_END)
		{
			Log(INPUTMANAGER_STREAM, "New user input [User:%s]", pieEvent->GetStringProperty("[uttid]").c_str());
		}
		//#endif
		//#####################################获取 事件################################################

		//		push the event at the end of the event queue
		// <7>	加入未处理事件队列
		qpieEventQueue.push(pieEvent);

		// log it
		Log(INPUTMANAGER_STREAM, "New interaction event (%s) arrived "
			"(dumped below)\n%s",
			sType.c_str(), pieEvent->ToString().c_str());

	}//当前未处理event队列为空
}

// A: Used by the Galaxy Bridge to signal that a new event has arrived
void CInteractionEventManagerAgent::SignalInteractionEventArrived()
{
	// signal that the input has appeared
	SetEvent(hNewInteractionEvent);
}

