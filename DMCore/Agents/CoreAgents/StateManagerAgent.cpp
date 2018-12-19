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
// STATEMANAGERAGENT.CPP - implements the state manager agent class. This agent
//						   keeps a history of states the the DM went through 
//						   throughout the conversation. It can also provide
//						   access to variuos "collapsed" states, which can be
//						   useful for learning
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
//	 [2007-06-02] (antoine): fixed GetLastState and operator[] so that they
//							 return reference to TDialogState instead of 
//							 copies of these objects
//   [2005-10-20] (antoine): added GetStateAsString method
//   [2004-12-23] (antoine): modified constructor, agent factory to handle
//							  configurations
//   [2004-03-28] (dbohus): added functionality for defining the dialog state
//                           names via an external configuration file
//   [2004-03-25] (dbohus): added functionality for broadcasting the dialogue
//                           state to other components
//   [2002-05-25] (dbohus): deemed preliminary stable version 0.5
//   [2002-04-08] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#include "../../../DMInterfaces/DMInterface.h"
#include "StateManagerAgent.h"
#include "../../../DMCore/Agents/Registry.h"
#include "../../../DMCore/Core.h"

//-----------------------------------------------------------------------------
// Constructors and Destructors
//-----------------------------------------------------------------------------

// D: Default constructor
CStateManagerAgent::CStateManagerAgent(string sAName,
	string sAConfiguration,
	string sAType) :
	CAgent(sAName, sAConfiguration, sAType)
{
	// notfhing here	
}

// Virtual destructor - does nothing at this point
CStateManagerAgent::~CStateManagerAgent()
{

}

//-----------------------------------------------------------------------------
// Static function for dynamic agent creation
//-----------------------------------------------------------------------------
CAgent* CStateManagerAgent::AgentFactory(string sAName, string sAConfiguration)
{
	return new CStateManagerAgent(sAName, sAConfiguration);
}


//-----------------------------------------------------------------------------
// CAgent class overwritten methods
//-----------------------------------------------------------------------------

// D: the overwritten Reset method
void CStateManagerAgent::Reset()
{
	vStateHistory.clear();
}

//-----------------------------------------------------------------------------
//
// StateManagerAgent specific methods
//
//-----------------------------------------------------------------------------

// D: Loads the dialog state names from a file
// D：从文件加载对话框状态名称
void CStateManagerAgent::LoadDialogStateNames(string sFileName)
{
	// check if the filename is empty
	if (sFileName == "") return;

	// now, if the filename exists, try opening and reading from it
	// open the file
	FILE *fid;
	fopen_s(&fid, sFileName.c_str(), "r");
	if (!fid)
	{
		Warning(FormatString(
			"Dialog states specification file (%s) could not be open for"\
			" reading.", sFileName.c_str()));
		return;
	}

	// log
	Log(STATEMANAGER_STREAM, "Loading dialog states specification ...");

	// read the file line by line
	char lpszLine[STRING_MAX];
	while (fgets(lpszLine, STRING_MAX, fid) != NULL)
	{
		string sAgentName;
		string sStateName;

		// check for comments
		if ((lpszLine[0] == '#') ||
			((lpszLine[0] == '/') && lpszLine[1] == '/'))
			continue;

		// check for empty lines
		if (lpszLine[0] == '\n')
			continue;

		// check for AgentName = StateName pair
		if (SplitOnFirst((string)lpszLine, "=", sAgentName, sStateName))
		{
			// extract the model name and filename
			sAgentName = Trim(sAgentName);
			sStateName = Trim(sStateName);
			// then add it to the mapping
			s2sDialogStateNames.insert(	STRING2STRING::value_type(sAgentName, sStateName));
		}
	}
	fclose(fid);

	// Log the states loaded
	Log(STATEMANAGER_STREAM, "Dialog states specification loaded successfully.");
}

// D: Sets the expectation state broadcast address
void CStateManagerAgent::SetStateBroadcastAddress(
	string sAStateBroadcastAddress)
{
	sStateBroadcastAddress = sAStateBroadcastAddress;
}

// D: broadcasts the state to other components in the system
// D：将状态广播到系统中的其他组件
void CStateManagerAgent::BroadcastState()
{

	// construct the string state representation
	//构造字符串状态表示
	string sDialogState = GetStateAsString();

	// if we are in a Galaxy configuration, send notification to the hub
//#ifdef GALAXY	
	// log the activity
	/*
		typedef struct {
		string sModuleFunction;		// the name of the module and the function  模块-函数
		STRING2STRING s2sInputs;	// the inputs for this call					输入
		STRING2STRING s2sOutputs;	// the outputs for this call				输出
		bool bBlockingCall;			// indicates if the call is blocking or not	是否Blocking
		} TGIGalaxyCall;
	*/
	Log(STATEMANAGER_STREAM, "Broadcasting dialog manager state to HUB");
	TGIGalaxyCall gcGalaxyCall;
	gcGalaxyCall.sModuleFunction = "main";
	// state broadcast is non-blocking 
	gcGalaxyCall.bBlockingCall = false;
	gcGalaxyCall.s2sInputs.insert(
		STRING2STRING::value_type(":set_dialog_state", "1"));
	gcGalaxyCall.s2sInputs.insert(
		STRING2STRING::value_type(":dialog_state", sDialogState));

	// retrieve the current thread id
	// 检索当前线程id
	DWORD dwThreadId = GetCurrentThreadId();

	// send the message to the Galaxy Interface Thread
	// 发送Message到接口线程
	PostThreadMessage(g_idDMInterfaceThread, WM_GALAXYCALL,
		(WPARAM)&gcGalaxyCall, dwThreadId);	

	// and wait for a reply
	// 等待回复
	MSG Message;
	GetMessage(&Message, NULL, WM_ACTIONFINISHED, WM_ACTIONFINISHED);
//#endif // GALAXY

	// log the activity
	Log(STATEMANAGER_STREAM, "Dialog manager state broadcast completed.");
}

// D: Updates the state information
// 更新状态信息
void CStateManagerAgent::UpdateState()
{
	// log the activity
	Log(STATEMANAGER_STREAM, "Updating dialog state ...");

	//		first, if necessary, assemble the agenda of concept expectations
	// <1>	首先，如果agenda被修改过，组合概念的期望议程  => ContinueWith ： 在有agent压栈的时候会被更新
	if (pDMCore->bAgendaModifiedFlag)
	{
		//################################################
		// 重新组装agenda
		pDMCore->assembleExpectationAgenda();
		//################################################

	}

	//		construct the dialog state
	// <2>	构造对话状态
	TDialogState dsDialogState;
	dsDialogState.fsFloorStatus = pDMCore->fsFloorStatus;
	dsDialogState.sFocusedAgentName = pDMCore->GetAgentInFocus()->GetName();
	dsDialogState.esExecutionStack = pDMCore->esExecutionStack;
	dsDialogState.eaAgenda = pDMCore->eaAgenda;
	dsDialogState.saSystemAction = pDMCore->saSystemAction;
	dsDialogState.iTurnNumber = pDMCore->iTurnNumber;
	dsDialogState.iEHIndex = pDMCore->esExecutionStack.front().iEHIndex;

	//		compute the dialog state 
	// <3>	###计算对话状态 => State Name###
	dsDialogState.sStateName = "";
	if (s2sDialogStateNames.empty())
	{
		dsDialogState.sStateName = dsDialogState.sFocusedAgentName; //状态名 -> 焦点agent名
	}
	else
	{
		// go through the mapping and find something that matches the focus
		STRING2STRING::iterator iPtr;
		for (iPtr = s2sDialogStateNames.begin(); iPtr != s2sDialogStateNames.end(); iPtr++)
		{
			if (dsDialogState.sFocusedAgentName.find(iPtr->first) != -1)//如果焦点agent名包含s2sDialogStateNames中的状态名，赋已有的
			{
				dsDialogState.sStateName = iPtr->second;
				break;
			}
		}
		// if we couldn't find anything in the mapping, then set it to 
		// _unknown_
		if (dsDialogState.sStateName == "")//s2sDialogStateNames非空，但是没找到包含的state名
			dsDialogState.sStateName = "_unknown_";
	}

	//		adds the input line configuration as part of the state
	// <4>	添加输入配置[input line confiuration]作为状态的一部分
	STRING2STRING s2sInputLineConfiguration = pDMCore->GetAgentInFocus()->GetInputLineConfiguration();

	dsDialogState.sInputLineConfiguration =	S2SHashToString(s2sInputLineConfiguration);

	//		and push the state in history
	// <5>	把当前state加入历史state
	vStateHistory.push_back(dsDialogState);

	// <6>	获取最新更新的当前状态 并 log
	TDialogState ds = GetLastState();

	// log the finish
	Log(STATEMANAGER_STREAM, "Dialog state update completed: %s at %d "
		"(iEHIndex=%d):\n%s",
		dsDialogState.sFocusedAgentName.c_str(), vStateHistory.size() - 1,
		dsDialogState.iEHIndex, GetStateAsString().c_str());
}

// A: Returns a string representing the state
// A：返回表示状态的字符串
string CStateManagerAgent::GetStateAsString(TDialogState dsState)
{
	string sDialogState =
		FormatString("turn_number = %d\nnotify_prompts = %s\ndialog_state = "
		"%s\nnonu_threshold = %.4f\nstack = {\n%s\n}\nagenda = {\n%s\n}\n"
		"input_line_config = {\n%s\n}",
		dsState.iTurnNumber,
		pOutputManager->GetPromptsWaitingForNotification().c_str(),
		dsState.sStateName.c_str(),
		pDMCore->GetNonunderstandingThreshold(),
		Trim(pDMCore->executionStackToString(
		dsState.esExecutionStack)).c_str(),
		Trim(pDMCore->expectationAgendaToBroadcastString(
		dsState.eaAgenda)).c_str(),
		dsState.sInputLineConfiguration.c_str()
		);

	return sDialogState;
}

// 
string CStateManagerAgent::GetStateAsString()
{
	return GetStateAsString(vStateHistory.back());
}

// D: Access to the length of the history
// D：访问历史的长度
int CStateManagerAgent::GetStateHistoryLength()
{
	return vStateHistory.size();
}

// D: Access the last state
TDialogState &CStateManagerAgent::GetLastState()
{
	return vStateHistory.back();
}

// D: Indexing operator to access states 
TDialogState &CStateManagerAgent::operator[](unsigned int i)
{
	return vStateHistory[i];
}
