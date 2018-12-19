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
// DIALOGAGENCY.H   - definition of the CDialogAgency class. This class defines 
//                    the basic capabilities of a DialogAgency, and provides
//  		          useful macros for deriving classes which implement
//					  particular agencies.
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
//   [2004-12-23] (antoine): modified constructor, agent factory to handle
//							  configurations
//   [2004-04-21] (jsherwan): added logging to fatal errors in NextAgentToExecute()
//   [2004-04-12] (dbohus): changed order of creating concepts and subagents
//                           so that concepts exist by the time the subagents
//                           are created
//   [2004-04-16] (dbohus): added grounding models on dialog agents
//   [2003-10-16] (dbohus): fixed bug in execution planning (execution policy
//                           was called twice)
//   [2003-10-15] (dbohus): added macro for execution policy
//   [2003-04-25] (dbohus): agencies don't check for completion on execute 
//                           any more. the core takes care of that
//   [2003-04-09] (dbohus): created OnCreation and OnInitialization
//   [2003-04-08] (dbohus): change completion evaluation on execution
//   [2002-05-25] (dbohus): deemed preliminary stable version 0.5
//   [2002-03-28] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __DIALOGAGENCY_H__
#define __DIALOGAGENCY_H__

#include "../DialogAgent.h"
#include <string>

//-----------------------------------------------------------------------------
// CDialogAgency class: this class implements a dialog agency
//-----------------------------------------------------------------------------

// D: defines for the execution policies

// the left-to-right-open execution policy: the agency attempts to execute the
// subagents in a left-to-right order. Preconditions and non-blocked conditions
// are checked for each agent. This is the DEFAULT policy
//从左向右打开执行策略：代理尝试以从左到右的顺序执行子代理。 检查每个代理的前提条件和非阻塞条件。 这是DEFAULT策略
#define LEFT_TO_RIGHT_OPEN "left_to_right_open"

// the left-to-right-forced execution policy: the agency executes the agents
// in strict order from left to right; it blocks all the agents that are not
// currently under execution, and it doesn't check for any preconditions
//从左到右强制执行策略：代理按照从左到右的严格顺序执行代理; 它阻止所有当前未处于执行中的代理，并且不检查任何先决条件
#define LEFT_TO_RIGHT_ENFORCED "left_to_right_enforced"


class CDialogAgency : public CDialogAgent
{

public:

	//---------------------------------------------------------------------
	// Constructors and destructors
	//---------------------------------------------------------------------

	// default constructor
	CDialogAgency(string sAName,
		string sAConfiguration = "",
		string sAType = "CAgent:CDialogAgent:CDialogAgency");

	// virtual destructor
	virtual ~CDialogAgency();

	// static function for dynamic agent creation
	static CAgent* AgentFactory(string sAName, string sAConfiguration);


public:

	//---------------------------------------------------------------------
	// CAgent specific methods
	// CAgent特有方法
	//---------------------------------------------------------------------

	// Overwritten create
	//
	virtual void Create();

	// Overwritten Initialize
	//
	virtual void Initialize();

public:

	//---------------------------------------------------------------------
	// Fundamental Dialog Agent methods
	// 基本对话代理方法
	//---------------------------------------------------------------------

	// Creates the subagents, gets called by Initialize
	// 创建子代理，由Initialize调用
	virtual void CreateSubAgents();

	// Function for execution. 
	// 执行
	virtual TDialogExecuteReturnCode Execute();

	// This function returns the next agent planned for execution, 
	// according to the current execution policy used
	// 此函数返回计划执行的下一个代理，根据当前使用的执行策略
	virtual CDialogAgent* NextAgentToExecute();//#define EXECUTION_POLICY(CONTENTS)

	// The execution policy used
	//执行策略 “left_to_right_open”
	virtual string GetExecutionPolicy();

};

//-----------------------------------------------------------------------------
// 
// Macros for easily defining dialog agencies (classes derived from 
// CDialogAgency), which handle multiple subagents
// 
//-----------------------------------------------------------------------------

// D: macro for agency definition
#define DEFINE_AGENCY(AgencyClass, OTHER_CONTENTS)\
class AgencyClass : public CDialogAgency
{
	\
public:\
	AgencyClass(string sAName, \
	string sAType = "CAgent:CDialogAgent:CDialogAgency:"#AgencyClass) : \
	CDialogAgency(sAName, sAType)
	{
		\
	}\
	virtual ~AgencyClass() { ; }\
	static CAgent* AgentFactory(string sAName, string sAConfiguration)
		{
		\
		return new AgencyClass(sAName, sAConfiguration); \
	}\
	OTHER_CONTENTS\
}; \


// D: macro for defining an execution policy
// 典型的执行策略包括具有if和else语句的代码块，以确定对话任务的行为的变化。
#define EXECUTION_POLICY(CONTENTS) \
	public:\
	virtual CDialogAgent* NextAgentToExecute()
{
		\
		CONTENTS; \
	}\

	// D: macro for subagents definition section within an agency
	// D：代理中子代理定义部分的宏
#define DEFINE_SUBAGENTS(SUBAGENTS)\
	public:\
	virtual void CreateSubAgents()
		{
		\
		CDialogAgent* pNewAgent; \
		SUBAGENTS\
	}\

	// D: macros for defining a subagent within the subagent definition section
	// D：用于在子代理定义部分中定义子代理的宏
#define SUBAGENT(SubAgentName, SubAgentType, GroundingModelSpec)\
	pNewAgent = (CDialogAgent *)\
	AgentsRegistry.CreateAgent(#SubAgentType, \
#SubAgentName); \
	pNewAgent->SetParent(this); \
	pNewAgent->CreateGroundingModel(GroundingModelSpec); \
	SubAgents.push_back(pNewAgent); \
	pNewAgent->Initialize(); \

	// D: macro for concepts definition section within an agency
#define DEFINE_CONCEPTS(CONCEPTS)\
	public:\
	virtual void CreateConcepts()
		{
		\
		CONCEPTS\
	}\

	// D: macro for defining a concept within the concept definition section
#define USER_CONCEPT(ConceptName, ConceptType, GroundingModelSpec)\
	Concepts.push_back(new ConceptType(#ConceptName, csUser)); \
	Concepts.back()->CreateGroundingModel(GroundingModelSpec); \
	Concepts.back()->SetOwnerDialogAgent(this); \

#define SYSTEM_CONCEPT(ConceptName, ConceptType)\
	Concepts.push_back(new ConceptType(#ConceptName, csSystem)); \
	Concepts.back()->CreateGroundingModel("none"); \
	Concepts.back()->SetOwnerDialogAgent(this); \

#endif // __DIALOGAGENCY_H__

