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
// GROUNDINGMODEL.H   - definition of the CGroundingModel base class for
//                      grounding models
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
//   [2004-02-24] (dbohus): addeded support for full state and collapsed state
//   [2004-02-09] (dbohus): changed model data to "policy"
//   [2003-02-12] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __GROUNDINGMODEL_H__
#define __GROUNDINGMODEL_H__

#include "../GroundingUtils.h"

// D: define a constant for the value of an unavailable action (hopefully 
//    value is strange enough so that no actual value will ever end up being
//    this)
// D：为不可用操作的值定义一个常量（希望值很奇怪，所以没有实际的值会永远是这样）
#define UNAVAILABLE_ACTION INVALID_EVENT

//-----------------------------------------------------------------------------
// D: Auxiliary datastructures capturing the model data
//-----------------------------------------------------------------------------

// D: First, a datastructure describing the state/action/value information
typedef struct
{
	string sStateName;  // the name of the state				状态名
	map<int, float> i2fActionsValues;						//动作 -> 值
	// hash which holds the action/value information
	// the actions are the hash keys, the values
	// are the values
} TStateActionsValues;

// D: Then define the policy as an array of state/action/values
// D：然后将策略定义为 【状态/操作/值】 的数组
typedef vector<TStateActionsValues> TPolicy;

//-----------------------------------------------------------------------------
// CExternalPolicyInterface Class - 
//   This class implements an interface to an external policy (a policy that 
//     is actually implemented by an external module, and accessed over a 
//     socket)
//外部策略接口类 - 此类实现对外部策略（实际上由外部模块实现，并通过套接字访问的策略）的接口，
//-----------------------------------------------------------------------------
class CExternalPolicyInterface
{

private:
	//---------------------------------------------------------------------
	// Private members
	//---------------------------------------------------------------------

	unsigned long int sSocket;         // the socket connection

public:
	//---------------------------------------------------------------------
	// Constructors and destructors
	//---------------------------------------------------------------------

	CExternalPolicyInterface(string sAHost);
	~CExternalPolicyInterface();

	//---------------------------------------------------------------------
	// Compute the suggested action index
	//---------------------------------------------------------------------

	int ComputeSuggestedActionIndex(CState& rState);
};

//-----------------------------------------------------------------------------
// CGroundingModel Class - 
//   This is the base of the grounding model classes
// CGroundingModel类 - 这是接地模型类的基础
//-----------------------------------------------------------------------------

class CGroundingModel
{

protected:
	//---------------------------------------------------------------------
	// protected class members
	//---------------------------------------------------------------------
	//###############################################################################################################################
	string sName;       // the model name			模型名	CGMRequestAgent(string sAModelPolicy = "", string sAName = "UNKNOWN");
	string sModelPolicy;// the model policy name	策略名  ‘expl’, ‘expl_impl’, [‘request_default’ ‘request_lr’...]

	TPolicy pPolicy;    // the policy for the model: for each state, a state-actions-values structure
	//###############################################################################################################################
	bool bExternalPolicy;							//表示外部模块用于实现决策策略
	// indicates that an external module is used for
	//  implementing the decision policy

	CExternalPolicyInterface* pepiExternalPolicy;

	string sExternalPolicyHost;						//指定外部策略的套接字连接的主机（hostname：port）
	// specifies the host for the socket connection 
	//  for the external policy (hostname:port)

	vector<int> viActionMappings;					//将此模型的本地action（编号（0..n））映射到全局action索引（由接地管理器保存）
	// maps the local actions for this model (numbered
	// (0..n) to the global action number (as held
	// by the grounding manager)

	string sExplorationMode;						//指示要由模型执行的探索的类型
	// indicates the type of exploration to be performed
	// by the model

	float fExplorationParameter;					//参数，用于控制模型的探测量（用于ε-贪婪探测的ε和用于boltzmann探测的温度）
	// parameter that controls the amount of exploration
	// the model does (epsilon for epsilon-greedy 
	// exploration and temperature for boltzmann 
	// exploration)

	//#########################################################################################
	CState stFullState;										//给定poit处模型的完整状态
	// the full state of the model at a given point

	CBeliefDistribution bdBeliefState;						//在给poit处模型的聚合置信状态
	// the aggregated belief state of the model at 
	// a given point

	CBeliefDistribution bdActionValues;
	// the values for various actions at a given point		//在给定点（状态）的各种动作的值[动作的概率分布]，
	// (state)

	int iSuggestedActionIndex;								//模型对当前状态建议的动作的索引（这是由接地管理器存储的绝对动作索引)
	// the index of the action suggested by the 
	// model from the current state (this is the
	// absolute action index, as stored by the
	// grounding manager
	//#########################################################################################


public:
	//---------------------------------------------------------------------
	// Constructors and destructors
	//---------------------------------------------------------------------

	CGroundingModel(string sAModelPolicy = "",string sAModelName = "UNKNOWN");
	CGroundingModel(CGroundingModel& rAGroundingModel);
	virtual ~CGroundingModel();

	//---------------------------------------------------------------------
	// Static factory method
	//---------------------------------------------------------------------

	static CGroundingModel* GroundingModelFactory(string sModelPolicy);

	//---------------------------------------------------------------------
	// Methods for access to members
	//---------------------------------------------------------------------

	// Access to policy name
	// 获取策略名
	virtual string GetType();
	virtual string GetModelPolicy();
	virtual string GetName();
	virtual void SetName(string sAName);

	//---------------------------------------------------------------------
	// Fundamental grounding model methods. These are to be overwritten by 
	// by derived grounding model classes
	// 基本接地模型方法。 这些将被导出的接地模型类覆盖
	//---------------------------------------------------------------------

	// Virtual function for initializing the model. By default, it loads
	// the model data
	//于初始化模型的虚函数。 默认情况下，它加载模型数据
	virtual void Initialize();

	// Abstract virtual method for cloning the model
	virtual CGroundingModel* Clone() = 0;

	// Virtual method for loading the model policy (through the 
	// grounding manager agent)
	// 用于加载模型策略的虚拟方法（通过接地管理器代理）
	virtual bool LoadPolicy();

	// Virtual method that computes the aggregated belief state 
	// for this model (done by first computing the full state, and then
	// collapsing it to the aggregated belief state)
	// 计算此模型的聚合置信状态的虚方法
	//（通过首先计算完整状态，然后将其折叠到聚合置信状态来完成）
	virtual void ComputeState();

	// Virtual method that computes the expected values of the 
	// various actions and returns a corresponding probability distribution 
	// over the actions
	//虚拟方法，其计算各种动作的期望值，并且在动作上返回对应的概率分布
	virtual void ComputeActionValuesDistribution();

	// Virtual method that only computes the action the model suggests we 
	// should take, but does not run the action yet
	virtual int ComputeSuggestedActionIndex();

	// Virtual method that runs an action of the grounding model
	virtual void Run();

	// Virtual method that runs a particular action 
	virtual void RunAction(int iActionIndex);

	// Virtual method for logging the current state of the model and 
	// the suggested action
	virtual void LogStateAction();

protected:
	//---------------------------------------------------------------------
	// Auxiliary protected methods
	//---------------------------------------------------------------------

	// Abstract virtual method for computing the full state of the model
	virtual void computeFullState() = 0;

	// Abstract virtual method for collapsing the full state into the 
	// aggregated belief state
	virtual void computeBeliefState() = 0;

	// Converting the belief state and the action values to a string 
	// representation
	//
	string beliefStateToString();
	string actionValuesToString();
};

// D: type definition for a vector of grounding model pointers
typedef vector <CGroundingModel *> TGroundingModelPointersVector;

// D: type definition for a set of grounding model pointer
typedef set < CGroundingModel *, less < CGroundingModel * >,
	allocator < CGroundingModel * > > TGroundingModelPointersSet;

#endif // __GROUNDINGMODEL_H__