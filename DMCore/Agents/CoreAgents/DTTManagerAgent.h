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
// DTTMANAGERAGENT.H   - defines the dialog task tree manager agent class. This
//						 class handles construction and various operations
//						 on the dialog task tree
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
//   [2004-12-23] (antoine): modified constructor, agent factory, etc to handle
//							  configurations
//   [2002-10-22] (dbohus): added support for destroying and for recreating
//                          the dialog task tree
//   [2002-10-07] (dbohus): added support for specifying which of the library
//                          (discourse) agents to be used in the tree
//   [2002-05-25] (dbohus): deemed preliminary stable version 0.5
//   [2002-01-10] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __DTTMANAGERAGENT_H__
#define __DTTMANAGERAGENT_H__

#include "../../../Utils/Utils.h"
#include "../../../DMCore/Agents/Agent.h"
#include "../../../DMCore/Agents/DialogAgents/DialogAgent.h"

//-----------------------------------------------------------------------------
// CDTTManagerAgent Class - 
//   This class is an agent which handles the construction and various other
//   operations on the dialog task tree
// CDTTManagerAgent�� - �����Ǵ���Ի�������[DTT]�Ĺ���͸������������Ĵ���
//-----------------------------------------------------------------------------

// D: enumeration type describing the various mounting methods
// ö������ : �������ְ�װ����
typedef enum
{
	mmAsLastChild,			//���һ������  0
	mmAsFirstChild,			//��һ������    1
	mmAsLeftSibling,		//���ֵ�       2
	mmAsRightSibling,		//���ֵ�       3
} TMountingMethod;

// D: an strings describing the mounting methods (used for logging purposes)
static string MountingMethodAsString[] = { "last child",
"first child",
"left sibling",
"right sibling" };

// D: structure holding information about a discourse agent that is to be 
//    used in the tree (so that they can be added dynamically when the tree is constructed)
// �ṹ�壬��������ʹ�õĻ���agent����Ϣ��ʹ�õ�������ʱ���ǿ��Ա���̬����ӣ�
typedef struct
{
	string sDAName;					//����
	string sDAType;					//����
	FRegisterAgent fRegisterAgent;	//����ע��agent�ĺ���ԭ��
	string sDAConfiguration;		//���ò���
} TDiscourseAgentInfo;

// D: finally, the actual DTTManagerAgent class
//CDTTManagerAgent�� - �����Ǵ���Ի�������[DTT]�Ĺ���͸������������Ĵ���
class CDTTManagerAgent : public CAgent
{

private:
	// private members
	//
	CDialogAgent* pdaDialogTaskRoot;		// the dialog task root		���ڵ�Root

	// a vector containing the information about the discourse agents to be used	// ʹ�õ�agent����Ϣ�б�
	vector<TDiscourseAgentInfo, allocator<TDiscourseAgentInfo> > vdaiDAInfo;

public:

	//---------------------------------------------------------------------
	// Constructor and destructor
	//---------------------------------------------------------------------
	//
	// Default constructor
	CDTTManagerAgent(string sAName, string sAConfiguration = "", string sAType = "CAgent:CDTTManagerAgent");

	// Virtual destructor 
	virtual ~CDTTManagerAgent();

	// static function for dynamic agent creation
	static CAgent* AgentFactory(string sAName, string sAConfiguration);

	//---------------------------------------------------------------------
	// CAgent Class overwritten methods 
	//---------------------------------------------------------------------
	// ���� - ����
	virtual void Reset();	// overwritten Reset

public:

	//---------------------------------------------------------------------
	// DTTManagerAgent class specific public methods
	// ���е�public����
	//---------------------------------------------------------------------

	// Function that specifies that a certain discourse agent will be used
	// ʹ���ض���agent
	void Use(string sDAType, string sDAName, FRegisterAgent fRegisterAgent, string sConfiguration);

	// Functions that create the dialog task tree
	// �����Ի��������ĺ���
	void CreateDialogTree();			// creates the dialog tree			����DTT
	void DestroyDialogTree();           // destroys the dialog tree			����DTT
	void ReCreateDialogTree();          // recreates the dialog tree		���´���DTT
	void CreateDialogTaskTree();		// creates the task part			��������DTT	�� => #define DECLARE_DIALOG_TASK_ROOT(RootAgentName, RootAgentType, GroundingModelSpec)

	// Function that registers all the types of agents (agencies) existent
	// in the dialog task tree
	// ע��Ի��������д��ڵ���������Agent��agencies���ĺ���
	void CreateDialogTaskAgentome();	// registers all the dialog task �� =>	#define DECLARE_AGENTS(DECLARATIONS)
	//   (developer specified) agents	������ר�ã�����

	// Returns the root of the dialog task tree
	// ���ظ��ڵ�
	CDialogAgent* GetDialogTaskTreeRoot();

	// Mount a subtree somewhere in the dialog task tree
	// ��������
	void MountAgent(CDialogAgent* pdaWhere, CDialogAgent* pdaWho, TMountingMethod mmHow, string sDynamicAgentID = "");
	CDialogAgent* MountAgent(CDialogAgent* pWhere, string sAgentType, string sAgentName, string sAConfiguration, TMountingMethod mmHow, string sDynamicAgentID = "");

	// Unmount an agent from a tree
	// ж����agent
	void UnMountAgent(CDialogAgent* pdaWho);

	// Mount an array of agents
	// ����һ��agent
	void MountAgentsFromArrayConcept(CDialogAgent *pdaParent, string sArrayConceptName, string sAgentsType, string sAgentsName, string sAgentsConceptName = "", string sAgentsDynamicID = "#");

	//------------------------------------------------------------------------
	// Methods dealing with "relative" relationships between the agents in 
	// the dialog task tree
	// ����Ի��������еĴ���֮��ġ���ԡ���ϵ�ķ���
	//------------------------------------------------------------------------
	// 
	string GetParentName(string sAgentPath);
	bool IsParentOf(string sParentAgentPath, string sAgentPath);
	bool IsChildOf(string sChildAgentPath, string sAgentPath);
	bool IsAncestorOf(string sAncestorAgentPath, string sAgentPath);
	bool IsAncestorOrEqualOf(string sAncestorAgentPath, string sAgentPath);
	bool IsDescendantOf(string sDescendantAgentPath, string sAgentPath);
	bool IsSiblingOf(string sAgent1Path, string sAgent2Path);
};

//-----------------------------------------------------------------------------
//
// D: Stubs for direct access to common DTTManager agent methods
//
//-----------------------------------------------------------------------------

// D: Mount a subtree somewhere in the dialog task tree (overloaded)
CDialogAgent* MountAgent(CDialogAgent* pWhere, string sAgentType,
	string sAgentName, string sAConfiguration,
	TMountingMethod mmHow);

// D: Mount an array of agents corresponding to an array concept 
void MountAgentsFromArrayConcept(CDialogAgent *pdaParent,
	string sArrayConceptName,
	string sAgentsType,
	string sAgentsName,
	string sAgentsConceptName = "",
	string sAgentsDynamicID = "#");


//-----------------------------------------------------------------------------
// 
// Macros for easy definition of the dialog task tree. See a sample 
// DialogTask.cpp file for an example of how these macros are used.
//
//-----------------------------------------------------------------------------

// D: macro for defining the dialog task root
// ������ڵ�	 �� => DECLARE_DIALOG_TASK_ROOT(Map, CMap, "")
#define DECLARE_DIALOG_TASK_ROOT(RootAgentName, RootAgentType, \
	GroundingModelSpec)\
	void CDTTManagerAgent::CreateDialogTaskTree()
{
		\
			Log(DTTMANAGER_STREAM, "Creating Dialog Task Tree ..."); \
			pdaDialogTaskRoot = (CDialogAgent *)\
			AgentsRegistry.CreateAgent(#RootAgentType, \
#RootAgentName); \
			pdaDialogTaskRoot->SetParent(NULL); \
			pdaDialogTaskRoot->CreateGroundingModel(GroundingModelSpec); \
			pdaDialogTaskRoot->Initialize(); \
			pdaDialogTaskRoot->Register(); \
	}\

	// D: macro for declaring the agents (defining and registering all the 
	//    agent types in a task
	// D��������������ĺ꣨�����ע�������е����д�������TYPE)
#define DECLARE_AGENTS(DECLARATIONS)\
	void CDTTManagerAgent::CreateDialogTaskAgentome()
{
		\
			Log(DTTMANAGER_STREAM, "Registering  dialog task agent types ..."); \
			DECLARATIONS\
	}\

	// D: macro for registering a particular agent type - to be used within a 
	//    DECLARE_AGENTS declaration
	// D������ע���ض��������͵ĺ� - Ҫ��DECLARE_AGENTS������ʹ��
#define DECLARE_AGENT(AgentClass)\
	AgentsRegistry.RegisterAgentType(#AgentClass, \
	AgentClass::AgentFactory); \

#endif // __DTTMANAGERAGENT_H__
