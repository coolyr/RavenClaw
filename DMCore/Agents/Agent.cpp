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
// AGENT.CPP - implementation of the CAgent base class
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
//   [2004-12-23] (antoine): added configuration methods, modified constructor 
//							 and factory method to handle configurations
//   [2004-04-24] (dbohus): added create method
//   [2002-05-25] (dbohus): deemed preliminary stable version 0.5
//   [2001-12-30] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#include "Agent.h"
#include "../../DMCore/Log.h"

// D: reference to the external AgentsRegistry object
extern CRegistry AgentsRegistry;

//-----------------------------------------------------------------------------
// Constructors and Destructor
//-----------------------------------------------------------------------------

// D: constructor for the CAgent class
CAgent::CAgent(string sAName, string sAConfiguration, string sAType)
{
	sName = sAName;
	sType = sAType;
	//将配置字符串string解析为参数Hash
	// sAConfiguration = "key=value,key=value,.."
	SetConfiguration(sAConfiguration);
}

// D: destructor
//析构 CAgent
CAgent::~CAgent()
{
	// unregister the agent, but only if it's not the NULL agent
	// 从注册表删除agent, 只有当nane != NULL 的时候
	if (sName != "NULL")
		UnRegister();
}

//-----------------------------------------------------------------------------
// Static function for dynamic agent creation 静态函数创建动态agent
//-----------------------------------------------------------------------------
CAgent* CAgent::AgentFactory(string sAName, string sAConfiguration)
{
	// this method should never end up being called (since CAgent is an 
	// abstract class) , therefore do a fatal error if this happens
	//#这个方法应该永远不会被调用（因为CAgent是一个抽象类），因此，如果发生这种情况，做一个致命错误
	FatalError("AgentFactory called on CAgent (abstract) class.");
	return NULL;
}

//-----------------------------------------------------------------------------
// Create, Initialize, Reset methods
//-----------------------------------------------------------------------------

// D: Create method: does nothing
//虚函数，子类实现
void CAgent::Create()
{
}

// D: Initialize method: does nothing
//虚函数，子类实现
void CAgent::Initialize()//宏 #define ON_INITIALIZATION(DO_STUFF)
{
}

// D: Reset method: does nothing
//虚函数，子类实现
void CAgent::Reset()
{
}

// A: Parses a configuration string into a hash of parameters
//将配置字符串解析为参数Hash
void CAgent::SetConfiguration(string sConfiguration)
{
	// append to the current list of parameters
	//# 定义类型 typedef map <string, string> STRING2STRING;
	STRING2STRING lval = StringToS2SHash(sConfiguration);
	//把解析出来的参数Hash,填充到成员变量s2sConfiguration中 
	AppendToS2S(s2sConfiguration, lval);
}

// D: appends to the configuration from a hash
void CAgent::SetConfiguration(STRING2STRING s2sAConfiguration)
{
	// append to the current configuration
	AppendToS2S(s2sConfiguration, s2sAConfiguration);
}

// A: sets an individual parameter
void CAgent::SetParameter(string sParam, string sValue)
{
	s2sConfiguration.insert(STRING2STRING::value_type(sParam, sValue));
}

// A: tests if a given parameter exists in the configuration
bool CAgent::HasParameter(string sParam)
{
	return s2sConfiguration.find(sParam) != s2sConfiguration.end();
}

// A: gets the value for a given parameter
// 通过参数名获取参数值
string CAgent::GetParameterValue(string sSlot)
{

	STRING2STRING::iterator i = s2sConfiguration.find(sSlot);

	if (i == s2sConfiguration.end())
	{
		return "";
	}
	else
	{
		return (string)((*i).second);
	}

	return "";
}

//-----------------------------------------------------------------------------
// Methods for access to private and protected members
//-----------------------------------------------------------------------------

// D: return the agent name
string CAgent::GetName()
{
	return sName;
}

// D: return the agent type
string CAgent::GetType()
{
	return sType;
}

// D: registers the agent
//虚函数的默认实现 - 注册agent
void CAgent::Register()
{
	//this 指向当前agent实例的指针
	AgentsRegistry.RegisterAgent(sName, this);
}

// D: unregisters the agent
//虚函数默认实现 - 从注册表删除agent
void CAgent::UnRegister()
{
	AgentsRegistry.UnRegisterAgent(sName);
}