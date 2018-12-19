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
// STATEMANAGERAGENT.H   - defines the state manager agent class. This agent
//						   keeps a history of states the the DM went through 
//						   throughout the conversation. It can also provide
//						   access to variuos "collapsed" states, which can be
//						   useful for learning
// STATEMANAGERAGENT.H - 定义状态管理器代理类。 该代理保持DM在整个会话中经历的状态的历史。 
//	它还可以提供对变量“折叠”状态的访问，这对于学习是有用的
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


#pragma once
#ifndef __STATEMANAGERAGENT_H__
#define __STATEMANAGERAGENT_H__

#include "../../../Utils/Utils.h"
#include "../../../DMCore/Agents/Agent.h"
#include "../../../DMCore/Agents/DialogAgents/DialogAgent.h"
#include "../../../DMCore/Agents/CoreAgents/DMCoreAgent.h"

//-----------------------------------------------------------------------------
// CStateManagerAgent Class - 
//   This class is an agent which keeps track of state information in the 
//   dialog manager
//CStateManagerAgent类 - 这个类是跟踪对话管理器中的状态信息的代理
//-----------------------------------------------------------------------------
// D: structure describing the state of the dialog manager at some point
// 描述对话管理器在某一点的状态
typedef struct
{
	TFloorStatus fsFloorStatus;			// who has the floor?						//谁拥有floor [未知、系统、用户、自由]
	string sFocusedAgentName;			// the name of the focused agent			//焦点agent名称
	TExecutionStack esExecutionStack;	// the execution stack						//执行栈
	TExpectationAgenda eaAgenda;		// the expectation agenda					//期望agenda
	TSystemAction saSystemAction;		// the current system action				//当前系统action
	string sInputLineConfiguration;		// string representation of the input		//当前输入的config
	// line config at this state (lm, etc)
	int iTurnNumber;					// the current turn number					//当前turn数
	int iEHIndex;						// the execution history index matching		//执行历史index
	// this di`alog state
	string sStateName;					// the name of the current dialog state		//状态名
} TDialogState;

// D: the CStateManager class definition
//这个类是跟踪对话管理器中的状态信息的代理
class CStateManagerAgent : public CAgent
{

public:
	// dialog state name definitions
	// 对话state名称 [agent名 -> state名]
	STRING2STRING s2sDialogStateNames;

	// private vector containing a history of the states that the DM went through
	// private向量, 包含DM经历的状态的历史
	vector<TDialogState, allocator<TDialogState>> vStateHistory;

	// variable containing the state broadcast address
	// 变量包含状态广播地址
	string sStateBroadcastAddress;

public:

	//---------------------------------------------------------------------
	// Constructor and destructor
	//---------------------------------------------------------------------
	//
	// Default constructor
	CStateManagerAgent(string sAName, string sAConfiguration = "", string sAType = "CAgent:CStateManagerAgent");

	// Virtual destructor
	virtual ~CStateManagerAgent();

	// static function for dynamic agent creation
	static CAgent* AgentFactory(string sAName, string sAConfiguration);

	//---------------------------------------------------------------------
	// CAgent Class overwritten methods 
	//---------------------------------------------------------------------
	//
	virtual void Reset();	// overwritten Reset

public:

	//---------------------------------------------------------------------
	// StateManagerAgent class specific public methods
	// 特定的public 方法
	//---------------------------------------------------------------------

	// Load the dialog state name mapping from a file
	// 从文件加载对话框状态名称映射
	void LoadDialogStateNames(string sFileName);

	// Set the expectation agenda broadcast address
	// 设置期望agenda广播地址
	void SetStateBroadcastAddress(string sAStateBroadcastAddress);

	// Broadcast the state to the other components in the system
	// 将状态广播到系统中的其他组件
	void BroadcastState();

	// Updates the state information
	// 更新状态信息
	void UpdateState();

	// Returns a string representing the state
	// 返回表示状态的字符串
	string GetStateAsString(TDialogState dsState);
	string GetStateAsString();

	// Access to the length of the history
	// 获取历史的长度
	int GetStateHistoryLength();

	// Access the last state
	// 获取最后的状态
	TDialogState &GetLastState();

	// Indexing operator to access states 
	// 重载操作符[] ,获取状态
	TDialogState &operator[](unsigned int i);

};

#endif // __STATEMANAGERAGENT_H__