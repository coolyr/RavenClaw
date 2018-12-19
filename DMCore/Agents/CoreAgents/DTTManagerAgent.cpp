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
// DTTMANAGERAGENT.CPP - implements the dialog task tree manager agent class. 
//						 This class handles construction and various operations
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

#include "DTTManagerAgent.h"
#include "../../../DMCore/Agents/Registry.h"
#include "../../../DMCore/Agents/DialogAgents/AllDialogAgents.h"
#include "../../../DMCore/Core.h"

//-----------------------------------------------------------------------------
// Constructors and Destructors
//-----------------------------------------------------------------------------

// D: constructor
CDTTManagerAgent::CDTTManagerAgent(string sAName, string sAConfiguration, string sAType) :
CAgent(sAName, sAConfiguration, sAType)
{
	// nothing here
}

// D: destructor - destroys all the agents that were left in the dialog task tree
// 析构函数 - 销毁对话任务树中剩余的所有代理
CDTTManagerAgent::~CDTTManagerAgent()
{
	if (pdaDialogTaskRoot)
	{
		Log(DTTMANAGER_STREAM, "Deallocating the dialog task tree.");
		// call the OnDestruction method
		// 删除所有子agent
		pdaDialogTaskRoot->OnDestruction();
		// then delete
		// 删除根节点
		delete pdaDialogTaskRoot;
	}
}

//-----------------------------------------------------------------------------
// Static function for dynamic agent creation
// 静态方法 - 动态创建agent
//-----------------------------------------------------------------------------
CAgent* CDTTManagerAgent::AgentFactory(string sAName, string sAConfiguration)
{
	return new CDTTManagerAgent(sAName, sAConfiguration);
}

//-----------------------------------------------------------------------------
// CAgent class overwritten methods
//-----------------------------------------------------------------------------

// D: the overwritten Reset method - calls reset on the tree of agents
// 重置 - 覆盖
void CDTTManagerAgent::Reset()
{
	if (pdaDialogTaskRoot) //当根节点不为空，调用根节点覆盖的reset方法
		pdaDialogTaskRoot->Reset();
}

//-----------------------------------------------------------------------------
//
// CDTTManager class specific methods
//
//-----------------------------------------------------------------------------

// D: Function that specifies that a certain discourse agent will be used
// D：指定将使用某个话语代理的功能
// fRegisterAgent  ==> void CRegistry::RegisterAgentType(string sAgentTypeName, FCreateAgent fctCreateAgent)
void CDTTManagerAgent::Use(string sDAType, string sDAName, FRegisterAgent fRegisterAgent, string sDAConfiguration)
{
	/*
		// 结构体，保存树中使用的话语agent的信息（使得当构建树时它们可以被动态地添加）
		typedef struct
		{
			string sDAName;					//名称
			string sDAType;					//类型
			FRegisterAgent fRegisterAgent;	//定义注册agent的函数原型
			string sDAConfiguration;		//配置参数
		} TDiscourseAgentInfo;
	*/
	// fill in a structure with discourse agent information
	// 填充agent信息结构体
	TDiscourseAgentInfo dai;
	dai.sDAName = sDAName;
	dai.sDAType = sDAType;
	dai.fRegisterAgent = fRegisterAgent;
	dai.sDAConfiguration = sDAConfiguration;

	// add it to the array holding discourse agent info
	// 加到 使用列表里
	vdaiDAInfo.push_back(dai);
}

// D: This function creates the dialog tree. It first creates the dialog task
//    agentome and tree, and then it creates the discourse agentome and 
//    mounts the discourse agents
// D：此函数创建对话树。 它首先创建对话任务代理和树，然后创建话语代理和挂载话语代理
void CDTTManagerAgent::CreateDialogTree()
{
	Log(DTTMANAGER_STREAM, "Starting Dialog Tree Creation Phase ...");

	//		register all the agents for the dialog task
	// <1>	注册所有的agent [DECLARATIONS]
	CreateDialogTaskAgentome();//注册Type	宏 =>	#define DECLARE_AGENTS(DECLARATIONS)

	//		create the actual task tree
	// <2> - 创建实际的Task Tree [DECLARE_DIALOG_TASK_ROOT]
	//	   - AgentsRegistry.CreateAgent(RootAgentType, RootAgentName); 里面调用root节点的create()，此函数会递归创建concept和subAgent
	CreateDialogTaskTree();

	//		register all the generic, task-independent discourse agents
	// <3>	注册  所有 <基本的 任务独立的> 回话Agent[Type类型]		[vdaiDAInfo里的信息什么时候填充的?  => 宏定义LIBRARY_AGENT填充的]
	for (unsigned int i = 0; i < vdaiDAInfo.size(); i++)
		(vdaiDAInfo[i].fRegisterAgent)();

	//		mount all the discourse agents that were specified to be used
	// <4>	挂载  所有的特定的[基本的，任务独立的]会话Agent
	for (unsigned int i = 0; i < vdaiDAInfo.size(); i++)	//宏  => #define CORE_CONFIGURATION(Configuration)里定义的
		MountAgent(
		pdaDialogTaskRoot,
		vdaiDAInfo[i].sDAType,
		vdaiDAInfo[i].sDAName,
		vdaiDAInfo[i].sDAConfiguration,
		mmAsLastChild); //都是最后一个 【当前agent的所有孩子存储在一个vector里, 顺序存储】
						//把所有的特定[任务独立]agent都挂载到root节点的[子列表subAgents]里

	Log(DTTMANAGER_STREAM, "Dialog Tree Creation Phase completed successfully.");
}

// D: This function destroys the dialog tree. Basically it calls delete on the 
//    root agent
// D：此函数销毁对话树。 基本上，它调用根代理上的删除
void CDTTManagerAgent::DestroyDialogTree()
{
	Log(DTTMANAGER_STREAM, "Starting Dialog Tree Destruction Phase ...");
	if (pdaDialogTaskRoot != NULL)
	{
		// call the OnDestruction method
		// 调用根节点 销毁函数
		pdaDialogTaskRoot->OnDestruction();
		// then delete
		// 删除根节点
		delete pdaDialogTaskRoot;
	}
	Log(DTTMANAGER_STREAM, "Dialog Tree Destruction Phase completed successfully.");
}

// D: This function recreates the dialog tree
// 重建 对话树 DTT
void CDTTManagerAgent::ReCreateDialogTree()
{
	Log(DTTMANAGER_STREAM, "Starting Dialog Tree ReCreation Phase ...");

	// create the actual task tree
	CreateDialogTaskTree();

	// mount all the discourse agents that were specified to be used
	for (unsigned int i = 0; i < vdaiDAInfo.size(); i++)
		MountAgent(pdaDialogTaskRoot, vdaiDAInfo[i].sDAType,
		vdaiDAInfo[i].sDAName, vdaiDAInfo[i].sDAConfiguration,
		mmAsLastChild);

	Log(DTTMANAGER_STREAM, "Dialog Tree ReCreation Phase completed successfully.");
}

// D: returns the root of the dialog task tree
// D：返回对话任务树的根
CDialogAgent* CDTTManagerAgent::GetDialogTaskTreeRoot()
{
	return pdaDialogTaskRoot;
}

// D: Mount a subtree somewhere in the dialog task tree
// 在对话框任务树中的某个位置安装子树
void CDTTManagerAgent::MountAgent(CDialogAgent* pdaWhere, CDialogAgent* pdaWho, TMountingMethod mmHow, string sDynamicAgentID)
{

	// log the mounting operation
	Log(DTTMANAGER_STREAM, "Mounting %s as %s of %s .", pdaWho->GetName().c_str(), MountingMethodAsString[mmHow].c_str(), pdaWhere->GetName().c_str());

	//	perform the mounting: use the AddSubAgent routine and then register
	//  the added agent; analyze depending on which mounting method is used
	//	执行挂载：使用AddSubAgent添加子代理例程，然后注册添加的代理; 根据使用的安装方法进行分析
	switch (mmHow)
	{
	case mmAsLastChild:
		if (!pdaWhere)
			FatalError("Cannot mount " + pdaWho->GetName() + " to NULL.");
		pdaWhere->AddSubAgent(pdaWho, NULL, asamAsLastChild);
		break;
	case mmAsFirstChild:
		if (!pdaWhere)
			FatalError("Cannot mount " + pdaWho->GetName() + " to NULL.");
		pdaWhere->AddSubAgent(pdaWho, NULL, asamAsFirstChild);
		break;
	case mmAsLeftSibling:
		if (!pdaWhere || !pdaWhere->GetParent())
			FatalError("Cannot mount " + pdaWho->GetName() + " to NULL.");
		pdaWhere->GetParent()->AddSubAgent(pdaWho, pdaWhere, asamAsLeftSibling);
		break;
	case mmAsRightSibling:
		if (!pdaWhere || !pdaWhere->GetParent())
			FatalError("Cannot mount " + pdaWho->GetName() + " to NULL.");
		pdaWhere->GetParent()->AddSubAgent(pdaWho, pdaWhere, asamAsRightSibling);
		break;
	}

	// set the dynamic agent ID
	// 设置动态agent Id
	pdaWho->SetDynamicAgentID(sDynamicAgentID);
}

// D: Mount a subtree somewhere in the dialog task tree
// 挂在子树
CDialogAgent* CDTTManagerAgent::MountAgent(
	CDialogAgent* pWhere,
	//############agent#############
	string sAgentType,
	string sAgentName,
	string sAgentConfiguration,
	//############agent#############
	TMountingMethod mmHow,
	string sDynamicAgentID
	)
{
	//		create the agent
	// <1>	创建出agent
	CDialogAgent* pNewAgent = (CDialogAgent *)AgentsRegistry.CreateAgent(sAgentType, sAgentName, sAgentConfiguration);
	//		place it in the tree
	// <2>	挂载到合适的位置
	MountAgent(pWhere, pNewAgent, mmHow, sDynamicAgentID);
	//		and the call its Initialize method
	// <3>	调用初始化函数
	pNewAgent->Initialize();
	// <4>	finally, return it
	return pNewAgent;
}

// D: Unmount an agent from a tree
// 从树中卸载一个agent
void CDTTManagerAgent::UnMountAgent(CDialogAgent* pdaWho)
{
	// check if the agent really exists
	if (pdaWho)
	{
		if (!pdaWho->GetParent())
		{
			// if it has no parent, then that is a fatal error
			FatalError("Cannot unmount a root dialog agent.");
		}
		// 调用父节点，卸载当前节点
		pdaWho->GetParent()->DeleteSubAgent(pdaWho);
	}
	else
	{
		Error("Cannot unmount NULL agent.");
	}
}

// D: Mounts an array of agents corresponding to the elements of an array
//    concept
// 通过概念数组 挂载一组agent
void CDTTManagerAgent::MountAgentsFromArrayConcept(
	CDialogAgent *pdaParent,	// the parent where the agents will be mounted
	string sArrayConceptName,	// the name of the array concept
	string sAgentsType,			// the type of the agents to be mounted
	string sAgentsName,		    // the name for the agents to be mounted
	//  this should normally include a # somewhere
	//  which will get replaced by the index
	string sAgentsConceptName,	// the name of the concept for each agent
	string sAgentsDynamicID)    // the dynamic id to use for each agent
	//  this can be any string containing #, 
	//  and # will be replaced by the index
{

	// go through the array and for each element create and mount an agent
	for (int i = 0; i < pdaParent->C(sArrayConceptName).GetSize(); i++)
	{
		CDialogAgent* pNewAgent =
			pDTTManager->MountAgent(pdaParent, sAgentsType,
			ReplaceSubString(sAgentsName, "#", FormatString("%d", i)),
			"", mmAsLastChild);
		// if the agent has a concept associated with it, give it the part
		//  of the array 
		if (sAgentsConceptName != "")
		{
			pNewAgent->C(sAgentsConceptName) =
				pdaParent->C("%s.%d", sArrayConceptName.c_str(), i);
		}
		// set the agent index in the array        
		pNewAgent->SetDynamicAgentID(
			ReplaceSubString(sAgentsDynamicID, "#", FormatString("%d", i)));
	}
}


//-----------------------------------------------------------------------------
// Functions dealing with "relative" relationships between the agents in the 
//  dialog task tree. 
//-----------------------------------------------------------------------------

// D: returns the name of the parent of a given agent
string CDTTManagerAgent::GetParentName(string sAgentPath)
{
	string sParent, sAgent;
	SplitOnLast(sAgentPath, "/", sParent, sAgent);
	return sParent;
}

// D: returns true if sParentAgent is the parent of sAgent
bool CDTTManagerAgent::IsParentOf(string sParentAgentPath, string sAgentPath)
{
	return (GetParentName(sAgentPath) == sParentAgentPath);
}

// D: returns true if sChildAgent is a child of aAgent
bool CDTTManagerAgent::IsChildOf(string sChildAgentPath, string sAgentPath)
{
	return (GetParentName(sChildAgentPath) == sAgentPath);
}

// D: returns true if sAncestorAgent is an ancestor of sAgent
bool CDTTManagerAgent::IsAncestorOf(string sAncestorAgentPath, string sAgentPath)
{
	return (sAgentPath.substr(0, sAncestorAgentPath.length()) ==
		sAncestorAgentPath) &&
		(sAncestorAgentPath != sAgentPath);;
}

// D: returns true if sAncestorAgent is an ancestor of sAgent or if they are
//    equal
// D：如果sAncestorAgent是sAgent的祖先或者它们相等，则返回true
bool CDTTManagerAgent::IsAncestorOrEqualOf(string sAncestorAgentPath, string sAgentPath)
{
	return (sAgentPath.substr(0, sAncestorAgentPath.length()) == sAncestorAgentPath);
}

// D: returns true if sDescendantAgent is a descendant of sAgent
bool CDTTManagerAgent::IsDescendantOf(string sDescendantAgentPath,
	string sAgentPath)
{
	return IsAncestorOf(sAgentPath, sDescendantAgentPath);
}

// D: returns true if the 2 agents are siblings
bool CDTTManagerAgent::IsSiblingOf(string sAgent1Path, string sAgent2Path)
{
	return GetParentName(sAgent1Path) == GetParentName(sAgent2Path);
}


//-----------------------------------------------------------------------------
//
// D: Stubs for direct access to common DTTManager agent methods
//
//-----------------------------------------------------------------------------

// D: Mount a subtree somewhere in the dialog task tree (overloaded)
CDialogAgent* MountAgent(CDialogAgent* pWhere, string sAgentType,
	string sAgentName, string sAgentConfiguration,
	TMountingMethod mmHow)
{
	return pDTTManager->MountAgent(pWhere, sAgentType, sAgentName,
		sAgentConfiguration, mmHow);
}

// D: Mount an array of agents corresponding to an array concept 
void MountAgentsFromArrayConcept(CDialogAgent *pdaParent,
	string sArrayConceptName,
	string sAgentsType,
	string sAgentsName,
	string sAgentsConceptName)
{
	pDTTManager->MountAgentsFromArrayConcept(pdaParent, sArrayConceptName,
		sAgentsType, sAgentsName, sAgentsConceptName);
}