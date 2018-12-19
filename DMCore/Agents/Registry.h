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
// REGISTRY.H - definition of the CRegistry class. This class registers 
//              the existing agents and agenttypes by name, and provides
//              various related functionality: CallAgent, (UnRegister), 
//              ancestor/descendant, etc.
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
//   [2002-05-25] (dbohus): deemed preliminary stable version 0.5
//   [2001-12-30] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include "../../Utils/Utils.h"
#include "Agent.h"

//-----------------------------------------------------------------------------
//
// CRegistry Class - holds a mapping from agent names to agents, and one from
//                   agent type names to static create functions for agents 
//                   of those types . Provides basic access to these mappings,
//					 and also some other agent related functions
//CRegistry类 - 保存代理名称到代理的映射，一个从代理类型名称到静态创建这些类型的代理的函数。 
//				提供对这些映射以及一些其他代理相关功能的基本访问
//-----------------------------------------------------------------------------

class CAgent;		// forward class declaration

// D: definition of hash type to hold mapping from agent names to agents
typedef map <string, CAgent*, less<string>, allocator<CAgent*> > TAgentsHash;

// D: definition of function type for creating an agent
//	构造agent的函数指针
typedef CAgent* (*FCreateAgent)(string, string);

// D: definition of function type for registering an agent
// 定义注册agent的函数[this指针]
typedef void(*FRegisterAgent)(void);

/*
	//D: registers the agent
	//虚函数的默认实现 - 注册agent
	void CAgent::Register()
	{
		//this 指向当前agent实例的指针
		AgentsRegistry.RegisterAgent(sName, this);
	}
*/


// D: definition of a hash type to hold mappings from agent type names to 
//   create functions (see Agent.h) for those agents
typedef map <string, FCreateAgent, less<string>, allocator<FCreateAgent>> TAgentsTypeHash;

class CRegistry
{

private:
	//	hash holding agent name -> agent mapping
	//	#Hash保持 agent名称 -> agent[this指针]
	TAgentsHash AgentsHash;

	//	hash holding agent type name -> agent creation function mapping
	//	#Hash 保存 Agent类型名 -> 构造函数的映射
	//	typedef CAgent* (*FCreateAgent)(string, string);
	//		返回：CAgent*				指针
	//		参数：string, string		两个string参数
	TAgentsTypeHash AgentsTypeHash;

public:
	// Constructors and Destructor
	//
	CRegistry();

	// Initializes the registry, empties everything
	//清除注册表
	void Clear();

	//------------------------------------------------------------------------
	// Registry specific functions for agent names
	// 操作Agent name的函数
	//------------------------------------------------------------------------

	// Register and unregister an agent
	// 注册和清除agent
	void RegisterAgent(string sAgentName, CAgent* pAgent);
	void UnRegisterAgent(string sAgentName);
	bool IsRegisteredAgent(string sAgentName);

	// Obtain a pointer to the agent
	// 重载运算符[]，获取agent指针
	CAgent* operator[](string sAgentName);

	//------------------------------------------------------------------------
	// Registry specific functions for agent types
	// 操作Agent type的函数
	//------------------------------------------------------------------------

	// Register and unregister an agent type
	// 注册和清除Agent Type
	void RegisterAgentType(string sAgentTypeName, FCreateAgent fctCreateAgent);
	void UnRegisterAgentType(string sAgentTypeName);
	bool IsRegisteredAgentType(string sAgentType);

	// Create a new agent of that type
	// 创建制定类型的Agent [通过AgentType索引出create函数]
	CAgent* CreateAgent(string sAgentTypeName, string sAgentName, string sAgentConfiguration = "");
};

//-----------------------------------------------------------------------------
// The AgentRegistry object and access to Call 
//-----------------------------------------------------------------------------
extern CRegistry AgentsRegistry;

#endif // __REGISTRY_H__
