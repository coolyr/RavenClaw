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
// GROUNDINGMANAGERAGENT.H   - defines the grounding management agent. This 
//                             agent encompasses a grounding model, and uses a 
//                             set of plugable strategies/actions to accomplish 
//                             grounding in dialog
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
//   [2006-01-31] (dbohus): added support for dynamically registering grounding
//                          model types
//   [2004-12-23] (antoine): modified constructor, agent factory to handle
//							  configurations
//   [2003-02-08] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __GROUNDINGMANAGERAGENT_H__
#define __GROUNDINGMANAGERAGENT_H__

#include "../../../Utils/Utils.h"
#include "../../../DMCore/Agents/Agent.h"
#include "../../../DMCore/Agents/CoreAgents/DMCoreAgent.h"
#include "../../../DMCore/Grounding/Grounding.h"

// D: auxiliary define for a map holding the list of external policies
// 外部策略
typedef map<string, CExternalPolicyInterface*> TExternalPolicies;

// D: auxiliary definition of the grounding manager configuration
// 辅助定义接地管理器配置
typedef struct
{
	string sGroundingManagerConfiguration;	//配置参数string
	bool bGroundConcepts;					//是否允许concept Grouding
	bool bGroundTurns;						//是否允许Turn Grouding
	string sConceptGM;						//默认的concept接地模型
	string sTurnGM;							//默认的Turn接地模型
	string sBeliefUpdatingModelName;		//概念更新模型 ["npu", "calista" ]
} TGroundingManagerConfiguration;

// D: type describing a concept grounding request
// D：描述概念接受请求的类型
#define GRS_UNPROCESSED 0		// the unprocessed status for a grounding request	未处理态
#define GRS_PENDING     1		// the pending status for a grounding request		未下最终论断
#define GRS_READY       2       // the ready status for a grounding request			准备态
#define GRS_SCHEDULED   3		// the scheduled status for a grounding request		调度态
#define GRS_EXECUTING   5		// the executing status for a grounding request		执行态
#define GRS_DONE        6   	// the completed status for a grounding request		完成态

typedef struct
{
	CConcept* pConcept;				//概念
	int iGroundingRequestStatus;	//接地请求状态
	int iSuggestedActionIndex;		//建议动作索引
	int iTurnNumber;				//turn id
} TConceptGroundingRequest;

// D: type for the stack of concept grounding requests
// 类型定义 - 接地请求的堆栈
typedef vector <TConceptGroundingRequest, allocator<TConceptGroundingRequest> >
TConceptGroundingRequests;

// D: type for the belief updating model
// 类型的信任更新模型
typedef map <string, vector<float> > STRING2FLOATVECTOR;
typedef map <string, STRING2FLOATVECTOR> STRING2STRING2FLOATVECTOR;

// D: the type definition for a grounding action that was run 
// D：运行的基础操作的类型定义
#define GAT_TURN 0
#define GAT_CONCEPT 1
#define GAT_NONE 2
typedef struct
{
	string sGroundingModelName;         // the name of the grounding model that
	//  took the action
	string sActionName;                 // the name of the grounding action
	int iGroundingActionType;           // the grounding action type 【GAT_TURN，GAT_CONCEPT， GAT_NONE】
	bool bBargeIn;                      // was there a barge-in on the action
} TGroundingActionHistoryItem;
// D: the type defition for a series of actions
typedef vector <TGroundingActionHistoryItem> TGroundingActionHistoryItems;

// D: definition of function type for creating a grounding model
// D：用于 Create Grouding Model 的函数类型的定义
typedef CGroundingModel* (*FCreateGroundingModel)(string);

// D: type definition for the hash containing the grounding models types and factory functions
// 接地模型类型  - hash - 工厂方法 
typedef map <string, FCreateGroundingModel> TGroundingModelsTypeHash;

//-----------------------------------------------------------------------------
//
// D: CGroundingManagerAgent class -
//      implements the grounding model which chooses the right grounding 
//      action based on the current grounding state
// DC 接地管理器代理类 - 实现基于当前接地状态选择正确接地动作的接地模型
//-----------------------------------------------------------------------------

class CGroundingManagerAgent : public CAgent
{

private:
	//---------------------------------------------------------------------
	// private grounding manager agent specific members
	// 接地管理器代理private特定成员
	//---------------------------------------------------------------------

	//#########################################################################
	/*
	concept Grouding:
					expl_impl = expl_impl.pol 文件内容
					expl = expl.pol	文件内容
	Turn Grouding:
					request_default = request_default.pol 文件内容
					request_lr = request_lr.pol 文件内容
	*/
	// hash holding the grounding models policies 
	// (key = model_name, value= model policy string)
	// 定义policy Hash
	STRING2STRING s2sPolicies;
	//#########################################################################

	// hash holding various constant parameters for feature computation
	// hash用于特征计算的各种常数参数
	STRING2FLOAT s2fConstantParameters;

	// hash holding the belief updating models
	// Hash信念更新模型
	STRING2STRING2FLOATVECTOR s2s2vfBeliefUpdatingModels;

	// hash holding information about the various concepts
	// hash持有关于各种 concept 的信息
	STRING2STRING2FLOATVECTOR s2s2vfConceptValuesInfo;

	// hash holding information about the concept type
	// hash持有关于各种 concept 的 type
	STRING2STRING s2sConceptTypeInfo;

	// hash holding the precomputed belief updating features
	// 哈希保存预先计算的信念更新特征
	STRING2FLOAT s2fBeliefUpdatingFeatures;

	//############################CGroundingAction#########################################
	//      GROUNDING_ACTION(NO_ACTION, NO_ACTION, Configuration)
	//		#define NO_ACTION CGANoAction	=>   真实名字是类名

	// array holding the grounding actions available 
	// 保持可用的接地操作的Vector
	vector <CGroundingAction *> vpgaActions;

	// parallel array holding the names of the grounding actions
	// 并行数组，保存接地动作的名称 [CGANoAction,     CGAAskRepeat,	EXPL_CONF,    IMPL_CONF, ...]
	TStringVector vsActionNames;
	//###########################CGroundingAction##########################################


	// hash with pointers to the externally implemented policies
	// 哈希与指向外部实现的策略的指针
	TExternalPolicies mapExternalPolicies;

	// the grounding manager configuration
	// 接地管理器配置 - 结构体
	TGroundingManagerConfiguration gmcConfig;

	//############################GroundingRequests##################################
	// flag which indicates if we need to ground turns
	// 标志，指示我们是否需要turns Grouding 
	bool bTurnGroundingRequest;

	// vector implementing the concept grounding stack
	// 矢量实现concept接地堆栈
	TConceptGroundingRequests vcgrConceptGroundingRequests;

	// flag indicating if the stack is locked
	// 加锁标志 - vcgrConceptGroundingRequests
	bool bLockedGroundingRequests;
	//##########################GroundingRequests####################################

	// the history of grounding actions
	// 接地操作的历史
	vector<TGroundingActionHistoryItems> vgahiGroundingActionsHistory;

	/*
	concept_default			= >		 CGMConcept::GroundingModelFactory

	request_default			= >		 CGMRequestAgent::GroundingModelFactory
	request_lr				= >		 CGMRequestAgent_LR::GroundingModelFactory
	request_handcrafted		= >		 CGMRequestAgent_HandCrafted::GroundingModelFactory
	request_numnonu			= >		 CGMRequestAgent_NumNonu::GroundingModelFactory
	request_experiment		= >		 CGMRequestAgent_Experiment::GroundingModelFactory
	*/
	// the hash containing the grounding model factories
	// hash包含接地模型工厂
	TGroundingModelsTypeHash gmthGroundingModelTypeRegistry;

public:

	//---------------------------------------------------------------------
	// Constructor and destructor
	//---------------------------------------------------------------------
	//
	// Default constructor
	CGroundingManagerAgent(string sAName, string sAConfiguration = "", string sAType = "CAgent:CGroundingManagerAgent");

	// Virtual destructor
	virtual ~CGroundingManagerAgent();

	// Static function for dynamic agent creation
	static CAgent* AgentFactory(string sAName, string sAConfiguration);

public:

	//---------------------------------------------------------------------
	//
	// GroundingManagerAgent class specific public methods
	// 特有的spublic方法
	//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	// Access to the configuration information
	// 访问配置信息的方法
	//---------------------------------------------------------------------

	// Set the configuration string
	// 设置配置信息
	void SetConfiguration(string sAGroundingManagerConfiguration);

	// Returns the configuration structure
	// 返回配置信息 - 结构体
	TGroundingManagerConfiguration GetConfiguration();

	//---------------------------------------------------------------------
	// Operations with the model data
	// 操作模型数据的方法
	//---------------------------------------------------------------------

	// Load the policies
	// 加载策略
	virtual void LoadPoliciesFromString(string sPolicies);
	virtual void LoadPoliciesFromFile(string sFileName);

	// Return the policy for a certain grounding model
	// 返某个接地模型的策略
	virtual string GetPolicy(string sModelName);

	// Create an external policy interface
	// 创建外部策略借口
	CExternalPolicyInterface* CreateExternalPolicyInterface(string sAHost);

	// Release all external policy interaces
	// 释放所有外部策略
	void ReleaseExternalPolicyInterfaces();

	//---------------------------------------------------------------------
	// Methods for the belief updating model
	// 信念更新模型的方法
	//---------------------------------------------------------------------

	// Load the belief updating model
	// 加载信念更新模型
	virtual void LoadBeliefUpdatingModel(string sAFileName);

	// Set the belief updating model
	// 设置信念更新模型
	virtual void SetBeliefUpdatingModelName(string sABeliefUpdatingModelName);

	// Returns the name of the belief updating model
	// 返回信念更新模型名
	virtual string GetBeliefUpdatingModelName();

	// Returns an actual belief updating model, corresponding to a certain system action
	// 返回对应于某个系统动作的实际信任更新模型
	virtual STRING2FLOATVECTOR& GetBeliefUpdatingModelForAction(string sSystemAction);

	// Returns the value of a constant parameter
	// 返回常量参数的值
	virtual float GetConstantParameter(string sParam);

	// Precompute belief updating features
	// 预计算信念更新特征
	virtual void PrecomputeBeliefUpdatingFeatures(CConcept* pIConcept, CConcept* pNewConcept, string sSystemAction);

	// Returns the value of a feature for the grounding process
	// 返回接地过程的特征的值
	virtual float GetGroundingFeature(string sFeatureName);

	// Returns the string value of a feature for the grounding process
	// 返回接地过程的特征的字符串值
	virtual string GetGroundingFeatureAsString(string sFeatureName);

	// Clears the belief updating features
	// 清除信念更新特征
	virtual void ClearBeliefUpdatingFeatures();

	//---------------------------------------------------------------------
	// Methods for access to concept priors, confusability and concept
	// type information
	// 访问概念先验，混淆性和概念类型信息的方法
	//---------------------------------------------------------------------

	// Return the prior for a given concept hypothesis
	// 返回给定概念假设的先验
	virtual float GetPriorForConceptHyp(string sConcept, string sHyp);

	// Return the confusability score for a concept hypothesis
	// 返回概念假设的混淆性分数
	virtual float GetConfusabilityForConceptHyp(string sConcept, string sHyp);

	// Return the concept type
	// 返回概念類型
	virtual string GetConceptTypeInfoForConcept(string sConcept);

	//---------------------------------------------------------------------
	// Methods for identifying grounding actions
	// 识别接地动作的方法
	//---------------------------------------------------------------------

	// Add a grounding action to be used by the grounding manager
	// 添加接地操作以供接地管理器使用
	void UseGroundingAction(string sActionName, CGroundingAction* pgaAGroundingAction);

	// Convert grounding action names to indices and the other way around
	// 接地名称 和 索引互换
	int GroundingActionNameToIndex(string sGroundingActionName);
	string GroundingActionIndexToName(unsigned int iGroundingActionIndex);

	// Access a grounding action via the indexing operator
	// 通过索引操作符访问接地操作
	CGroundingAction* operator[] (string sGroundingActionName);
	CGroundingAction* operator[] (unsigned int iGroundingActionIndex);

	//---------------------------------------------------------------------
	// Methods for requesting grounding needs
	// 请求接地的方法
	//---------------------------------------------------------------------

	// signal the need for grounding the turn
	// 表明当前turn需要grouding
	void RequestTurnGrounding(bool bATurnGroundingRequest = true);

	// signal the need for grounding a certain concept
	// 表明需要Grouding当前的concept
	void RequestConceptGrounding(CConcept* pConcept);

	// force the grounding manager to schedule grounding for a concept; returns the action that got scheduled
	// 强制接地管理器为一个概念安排接地; 返回已调度的操作
	string ScheduleConceptGrounding(CConcept* pConcept);

	//---------------------------------------------------------------------
	// Methods for manipulating the queue of concept grounding requests
	// 操作 概念接地请求队列 的方法
	//---------------------------------------------------------------------

	// Lock the concept grounding requests queue
	// 锁住概念请求队列
	void LockConceptGroundingRequestsQueue();

	// Unlock the concept grounding requests queue
	// 解锁
	void UnlockConceptGroundingRequestsQueue();

	// Set the grounding request state for a concept
	// 设置概念接地请求的状态
	void SetConceptGroundingRequestStatus(CConcept* pConcept,int iAGroundingRequestStatus);

	// Get the grounding request state for a concept
	// 获取状态
	int GetConceptGroundingRequestStatus(CConcept* pConcept);

	// Signal that a concept grounding request has completed
	// 概念接地请求完成
	void ConceptGroundingRequestCompleted(CConcept* pConcept);

	// Remove a grounding request
	// 移除请求
	void RemoveConceptGroundingRequest(CConcept* pConcept);

	// Purge the grounding requests queue
	// 清除接地请求队列
	void PurgeConceptGroundingRequestsQueue();

	//---------------------------------------------------------------------
	// Methods for setting and accessing grounding actions history 
	// information
	// 设置和访问接地动作历史信息的方法
	//---------------------------------------------------------------------

	// adds a grounding action history item to the current history
	// 将接地动作历史项目添加到当前历史
	void GAHAddHistoryItem(string sGroundingModelName,string sActionName, int iGroundingActionType);

	// sets the barge-in flag on the last grounding history item
	// 在最后一个接地历史项目上设置插入标志
	void GAHSetBargeInFlag(int iTurnNum, bool bBargeInFlag);

	// obtains the turn grounding action performed in a certain turn
	// 获得在一定turn中执行的turn接地动作
	string GAHGetTurnGroundingAction(int iTurnNumber);

	// counts how many times an action was taken in the last N turns
	// 计算在最后N圈内进行一次动作的次数
	int GAHCountTakenInLastNTurns(bool bAlsoHeard, string sActionName, int iNumTurns = -1);

	// counts how many times an action was taken from a particular grounding model    
	// 计算从特定接地模型采取操作的次数
	int GAHCountTakenByGroundingModelInLastNTurns(bool bAlsoHeard, string sActionName, string sGroundingModelName, int iNumTurns = -1);

	//---------------------------------------------------------------------
	// Methods for registering and creating various grounding model types
	// 注册和创建各种接地模型类型的方法
	//---------------------------------------------------------------------

	// register a grounding model type
	// 注册接地模型类型
	void RegisterGroundingModelType(string sName, FCreateGroundingModel fctCreateGroundingModel);

	// create a grounding model 
	// 创建一个接地模型
	CGroundingModel* CreateGroundingModel(string sModelType,string sModelPolicy);

	//---------------------------------------------------------------------
	// Methods for accessing the state of the grounding management layer
	// 获取接地管理器的状态
	//---------------------------------------------------------------------

	// Indicates if there's a pending request
	// 表示是否有待处理的请求
	bool HasPendingRequests();

	// Indicates if there's a pending turn grounding request
	// 指示是否有待处理turn接地请求
	bool HasPendingTurnGroundingRequest();

	// Indicates if there's a pending concept grounding request
	// 指示是否有待处理概念接地请求
	bool HasPendingConceptGroundingRequests();

	// Indicates if there are unprocessed concept grounding requests
	// 指示是否存在未处理的概念接地请求
	bool HasUnprocessedConceptGroundingRequests();

	// Indicates if there are scheduled concept grounding requests
	// 指示是否有调度的概念接地请求
	bool HasScheduledConceptGroundingRequests();

	// Indicates if there are executing concept grounding requests
	// 指示是否有正在执行概念接地请求
	bool HasExecutingConceptGroundingRequests();

	// Check if a certain concept is undergoing grounding
	// 检查某个概念是否正在接地
	bool GroundingInProgressOnConcept(CConcept* pConcept);

	// Returns the scheduled action for a concept
	// 返回概念的计划操作
	string GetScheduledGroundingActionOnConcept(CConcept* pConcept);

	//---------------------------------------------------------------------
	// Method for running the grounding process
	// 运行接地过程
	//---------------------------------------------------------------------

	virtual void Run();


private:

	//---------------------------------------------------------------------
	// Auxiliary private methods
	// private 方法
	//---------------------------------------------------------------------

	// Return the index of a concept grounding request, or -1 if not found
	// 返回概念接地请求的索引，如果未找到，则返回-1
	int getConceptGroundingRequestIndex(CConcept* pConcept);

	// Load a policy from its description file
	// 从描述文件加载策略
	string loadPolicy(string sFileName);
};

#endif // __GROUNDINGMANAGERAGENT_H__
