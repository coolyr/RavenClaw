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
// DIALOGAGENT.CPP - definition of the CDialogAgent class. This class defines 
//                   the basic capabilities of a DialogAgent: execution, agenda
//                   definition, etc, etc
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
//   [2005-10-22] (antoine): Added methods RequiresFloor and 
//							 IsConversationSynchronous to regulate turn-taking
//                           and asynchronous agent planning/execution
//   [2005-01-21] (jsherwan): Added input line config functionality
//   [2004-12-23] (antoine): modified constructor, agent factory to handle
//							  configurations
//   [2004-05-24] (dbohus):  fixed bug in ExplainMore, WhatCanISay and Timeout
//                            prompt generation
//   [2004-04-24] (dbohus):  added Create method
//   [2004-04-24] (dbohus):  added indices to the last input and binding 
//                            results
//   [2004-04-17] (dbohus):  added support for declaring the grounding models
//                            subsumed by the concept
//   [2004-04-16] (dbohus):  added grounding models on dialog agents
//   [2004-04-01] (dbohus):  fixed potential buffer overrun problem
//   [2003-10-14] (dbohus):  added dynamic ids
//   [2003-07-11] (antoine): added function and macro LanguageModel to allow 
//							 agents to specify a LM for speech recognition
//   [2003-05-09] (dbohus):  fixed bug on default failure criteria
//   [2003-04-15] (dbohus,
//                 antoine): introduced support for *-type expectations
//   [2003-04-09] (dbohus):  created OnCreation and OnInitialization
//   [2003-04-08] (dbohus):  changed so that completion is evaluated on each
//                            call to HasCompleted()
//   [2003-04-03] (dbohus,
//                 antoine): lowercased all grammar expectations declared
//   [2003-02-13] (dbohus):  added dercRestartDialog to return codes for agents
//   [2003-01-27] (dbohus):  split ReOpen into ReOpenTopic and ReOpenConcepts
//   [2003-01-22] (dbohus):  added reset and reopen counters and methods to 
//                            access them   
//   [2002-11-20] (dbohus):  added support for maximum number of attempts on an
//                            agent (after that it will terminate with failure)
//   [2002-11-20] (dbohus):  changed so that an agent can complete with success
//                            or failure
//   [2002-11-20] (dbohus):  added support for customizing binding filters in 
//                            concept expectations / grammar mappings
//   [2002-05-25] (dbohus):  deemed preliminary stable version 0.5
//   [2002-03-13] (dbohus):  started working on this
// 
//-----------------------------------------------------------------------------

#include "DialogAgent.h"
#include "../../../DMCore/Core.h"
#include "../../../DMCore/Agents/Registry.h"
#include "../../../DialogTask/DialogTask.h"

// NULL dialog agent: this object is used designate invalid dialog agent
// references
CDialogAgent NULLDialogAgent("NULL");

//-----------------------------------------------------------------------------
//
// Constructors and destructors
//
//-----------------------------------------------------------------------------
// D: Default constructor
CDialogAgent::CDialogAgent(string sAName, string sAConfiguration, string sAType) :
CAgent(sAName, sAConfiguration, sAType)
{
	// initialize the class members: concepts, subagents, parent, etc
	sDialogAgentName = sAName;
	pdaParent = NULL;
	pdaContextAgent = NULL;
	pGroundingModel = NULL;
	bCompleted = false;
	ctCompletionType = ctFailed;
	bBlocked = false;
	bDynamicAgent = false;
	sDynamicAgentID = "";
	sTriggeredByCommands = "";
	sTriggerCommandsGroundingModelSpec = "";
	iExecuteCounter = 0;
	iResetCounter = 0;
	iReOpenCounter = 0;
	iTurnsInFocusCounter = 0;
	iLastInputIndex = -1;
	iLastExecutionIndex = -1;
	iLastBindingsIndex = -1;
	bInheritedParentInputConfiguration = false;
}

// D: Virtual destructor
CDialogAgent::~CDialogAgent()
{
	// delete all the concepts
	unsigned int i;
	for (i = 0; i < Concepts.size(); i++)
		delete Concepts[i];

	// delete all the subagents
	for (i = 0; i < SubAgents.size(); i++)
		delete SubAgents[i];

	// delete the grounding model
	if (pGroundingModel != NULL)
	{
		delete pGroundingModel;
		pGroundingModel = NULL;
	}

	// set the parent to NULL
	pdaParent = NULL;
}


//-----------------------------------------------------------------------------
// Static function for dynamic agent creation
//-----------------------------------------------------------------------------
CAgent* CDialogAgent::AgentFactory(string sAName, string sAConfiguration)
{
	// this method should never end up being called (since CDialogAgent is an 
	// abstract class) , therefore do a fatal error if this happens
	FatalError("AgentFactory called on CDialogAgent (abstract) class.");
	return NULL;
}

//-----------------------------------------------------------------------------
//
// CAgent overwritten methods
//
//-----------------------------------------------------------------------------

// D: Registers the agent - for a dialog agent, we are registering using the
//    full path in the dialog tree, and we are registering the children
// D：注册代理 - 对于对话代理，我们使用对话树中的【完整路径】进行注册，我们正在注册子代
void CDialogAgent::Register()
{
	//		register this agent
	// <1>	注册当前agent
	AgentsRegistry.RegisterAgent(sName, this);
	//		and all its subagents
	// <2>	递归注册子agent
	for (unsigned int i = 0; i < SubAgents.size(); i++)
	{
		SubAgents[i]->Register();
	}
	//		finally, create the trigger concept
	// <3>	创建trigger Concept ??
	CreateTriggerConcept();
}

// D: Initializes the agent, gets called after creation
void CDialogAgent::Create()
{
	// creates the concepts
	CreateConcepts();		//宏		#define DEFINE_CONCEPTS(CONCEPTS)
	// calls OnCreation
	OnCreation();			//宏		#define ON_CREATION(DO_STUFF)
}

// D: Initializes the agent, gets called after construction
// D：初始化代理，在构建之后调用
void CDialogAgent::Initialize()
{
	// calls OnInitialization
	OnInitialization();	//宏 #define ON_INITIALIZATION(DO_STUFF)
}


// D: Reset triggers an initialize
//复位触发器 - 初始化
void CDialogAgent::Reset()
{
	// clears all the concepts
	// 清空concept
	for (unsigned int i = 0; i < Concepts.size(); i++)
		Concepts[i]->Clear();
	// and calls reset on all the subagents
	// 调用所有子agent的reset
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->Reset();
	// reset the other member variables
	// 重置 其他的成员变量
	bCompleted = false;
	ctCompletionType = ctFailed;
	bBlocked = false;
	iExecuteCounter = 0;
	iReOpenCounter = 0;
	iResetCounter++;
	iTurnsInFocusCounter = 0;
	iLastInputIndex = -1;
	iLastExecutionIndex = -1;
	iLastBindingsIndex = -1;
	// finally, call the OnInitialization
	// 最后，再次调用初始化方法
	OnInitialization();
}

// J: Start copy from Agent.cpp and renamed s2sConfiguration -> s2sInputLineConfiguration
// A: Parses a configuration string into a hash of parameters
// J：从Agent.cpp开始复制并重命名为s2sConfiguration - > s2sInputLineConfiguration
// A：将配置字符串解析为参数哈希
void CDialogAgent::SetInputConfiguration(string sConfiguration)
{

	string sItem, sSlot, sValue;

	// while there are still thing left in the string
	while (!sConfiguration.empty())
	{

		// get the first item
		SplitOnFirst(sConfiguration, ",", sItem, sConfiguration, '%');
		sItem = Trim(sItem);

		// gets the slot and the value
		SplitOnFirst(sItem, "=", sSlot, sValue, '%');
		sSlot = Trim(sSlot);
		sValue = Trim(sValue);

		// fills in the configuration hash
		// 填充配置hash
		SetInputConfigurationParameter(sSlot, sValue);
	}
}

// A: sets an individual parameter
// A：设置单个参数
void CDialogAgent::SetInputConfigurationParameter(string sSlot, string sValue)
{
	s2sInputLineConfiguration.insert(STRING2STRING::value_type(sSlot, sValue));
}

// A: tests if a given parameter exists in the configuration
// A：测试配置中是否存在给定参数
bool CDialogAgent::HasInputConfigurationParameter(string sSlot)
{
	return s2sInputLineConfiguration.find(sSlot) != s2sInputLineConfiguration.end();
}

// A: gets the value for a given parameter
string CDialogAgent::GetInputConfigurationParameterValue(string sSlot)
{

	STRING2STRING::iterator i = s2sInputLineConfiguration.find(sSlot);

	if (i == s2sInputLineConfiguration.end())
	{
		return "";
	}
	else
	{
		return (string)((*i).second);
	}

	return "";
}
// J: End copy from Agent.cpp and renamed s2sConfiguration -> s2sInputLineConfiguration

// J: Default Input Config Init string (will be overridden) by most dialog agents
// 默认输入配置大多数对话代理的初始字符串（将被覆盖）
string CDialogAgent::InputLineConfigurationInitString()//#define INPUT_LINE_CONFIGURATION(CONFIG_LINE)
{
	return "";
}

// J: Returns the name of the derived input line config. Only calculates derived config
// once per session
// 返回派生输入行config的名称。 每个会话只计算一次派生配置
STRING2STRING CDialogAgent::GetInputLineConfiguration()
{
	if (!bInheritedParentInputConfiguration)
	{
		bInheritedParentInputConfiguration = true;
		// Sets hash based on init string
		// 通过初始string Config解析配置 
		SetInputConfiguration(InputLineConfigurationInitString());
		if (pdaParent)
			//如果父类不为空，继承父类的配置
			InheritParentInputConfiguration();
	}
	return s2sInputLineConfiguration;
}

// J: Selectively inherits parent's input line configuration
// J：选择性地继承父级的输入行配置
void CDialogAgent::InheritParentInputConfiguration()
{
	STRING2STRING s2sParentConfig = pdaParent->GetInputLineConfiguration();
	if (s2sParentConfig.size() > 0)
	{
		STRING2STRING::iterator iPtr;
		for (iPtr = s2sParentConfig.begin();
			iPtr != s2sParentConfig.end();
			iPtr++)
		{

			// only if the agent hasn't had this slot filled in yet
			// will it take its parent's value
			if (!HasInputConfigurationParameter(iPtr->first))
				SetInputConfigurationParameter(iPtr->first, iPtr->second);
		}
	}
}

//-----------------------------------------------------------------------------
//
// Fundamental Dialog Agent methods. Most of these are to be 
// overwritten by derived agent classes
//
//-----------------------------------------------------------------------------

// D: create the concepts for this agent: does nothing, is to be overwritten 
//    by derived classes
void CDialogAgent::CreateConcepts()
{
}

// D: returns true if the dialog agent is executable - by default, returns true
// 默认可执行【Expect不可执行】
bool CDialogAgent::IsExecutable()
{
	return true;
}

// D: the Execute: for this class, it merely returns continue execution
// D：Execute：对于这个类，它只返回继续执行
TDialogExecuteReturnCode CDialogAgent::Execute()
{
	// increment the execute counter
	// 递增执行计数器
	IncrementExecuteCounter();

	//返回 继续执行状态
	return dercContinueExecution;
}

// D: the DeclareExpectations: for this class, it calls DeclareExpectations for
//    all the subagents; also, if the agent is triggered by a command, 
//    it adds that expectation
// D：DeclareExpectations：对于这个类，它为所有子代理调用DeclareExpectations; 
//	  此外，如果代理由命令触发，它会添加触发器字符串解析出的期望
int CDialogAgent::DeclareExpectations(TConceptExpectationList&	rcelExpectationList)
{
	int iExpectationsAdded = 0;
	bool bExpectCondition = ExpectCondition(); //宏定义 #define EXPECT_WHEN(Condition)

	//		if there's a trigger, construct the expectation list for that trigger
	// <1>	如果有触发器，则构造该触发器的期望列表
	/*
	TRIGGERED_BY_COMMANDS("[QueryRoomDetails],[QueryProjector.projector]", "expl") 
	TRIGGERED_BY_COMMANDS("@(..)[QueryRoomDetails],@(..)[QueryRoomSize]", "expl")
	*/
	if (TriggeredByCommands() != "")
	{
		TConceptExpectationList celTriggerExpectationList;

		//	construct the expectation list for the triggering commands
		//	为触发器命令组织期望列表 => [(i.e. AgentName/ConceptName)]
		//	函数：	CConcept& CDialogAgent::C(const char *lpszConceptPath, ...)		=>		函数返回对sConceptPath中相对概念路径指向的概念的引用
		//			string CConcept::GetAgentQualifiedName()						=>		返回概念的名称，用所有者的名称限定 (i.e. AgentName/ConceptName)
		// <2>	解析 GRAMMAR_MAPPING("![Yes]>true, ![No]>false")
		//			TRIGGERED_BY_COMMANDS("[QueryRoomDetails],[QueryProjector.projector]", "expl")

		//####################################################################################################
		parseGrammarMapping( C("_%s_trigger", sDialogAgentName.c_str()).GetAgentQualifiedName(), 
			TriggeredByCommands(),	
			celTriggerExpectationList
			);
		//####################################################################################################

		//		go through it and add stuff to the current agent expectation list
		// <3>	遍历它, 添加到当前代理期望列表
		for (unsigned int i = 0; i < celTriggerExpectationList.size(); i++)
		{
			/*
				因为默认DialogAgent的实现是：解析TRIGGERED_BY_COMMANDS("[QueryRoomDetails],[QueryProjector.projector]", "expl")
				所以？ 一般实现方式为bmExplicitValue， 映射值的方式[True|False]
				要看【子类】的实现方式！！！
			*/
			//		set the expectation to bind the trigger to true
			// <4>	设置将触发器触发， 绑定到的期望值为true
			celTriggerExpectationList[i].bmBindMethod = bmExplicitValue;
			celTriggerExpectationList[i].sExplicitValue = "true";

			//		if the expect condition is not satisfied, disable this 
			//		trigger expectation and set the appropriate reason
			// <5>	如果期望条件不满足，请禁用此触发器期望值并设置适当的原因
			if (!bExpectCondition)//宏定义 #define EXPECT_WHEN(Condition)
			{
				celTriggerExpectationList[i].bDisabled = true;
				celTriggerExpectationList[i].sReasonDisabled =
					"expect condition false";
			}
			// <6> 添加到期望列表
			rcelExpectationList.push_back(celTriggerExpectationList[i]);
			iExpectationsAdded++;
		}//	for (unsigned int i = 0; i < celTriggerExpectationList.size(); i++)
	}

	//			finally go through the subagents and gather their expectations
	//	<7>		最后遍历子代理并收集他们的期望
	for (unsigned int i = 0; i < SubAgents.size(); i++)
	{
		iExpectationsAdded += SubAgents[i]->DeclareExpectations(rcelExpectationList);
	}
	// <8>		返回添加的expection的数目
	return iExpectationsAdded;//返回添加的expection的数目
}

// D: Declares the concepts that the agent subsumes
void CDialogAgent::DeclareConcepts(
	TConceptPointersVector& rcpvConcepts,
	TConceptPointersSet& rcpsExclude)
{

	// first copy the agent's concepts
	for (unsigned int i = 0; i < Concepts.size(); i++)
		Concepts[i]->DeclareConcepts(rcpvConcepts, rcpsExclude);

	// and the do the same for all the subagents
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->DeclareConcepts(rcpvConcepts, rcpsExclude);
}

// D: create a grounding model for this agent
// D：为此agent程序创建接地模型
void CDialogAgent::CreateGroundingModel(string sGroundingModelSpec)
{
	if (!(pGroundingManager->GetConfiguration().bGroundTurns) ||
		(sGroundingModelSpec == "none") ||
		(sGroundingModelSpec == ""))
	{
		pGroundingModel = NULL;// 脚本大部分都是“”  ？？
	}
	else
	{
		// extract the model type and policy
		string sModelType, sModelPolicy;
		if (!SplitOnFirst(sGroundingModelSpec, ".", sModelType, sModelPolicy))
		{
			// if there is no model type, set it to the default 
			// grounding manager configuration
			// 如果没有模型类型，请将其设置为默认接受管理器配置
			sModelType = pGroundingManager->GetConfiguration().sTurnGM;//设置为默认的接地模型
			sModelPolicy = sGroundingModelSpec;
		}
		//###########################################################################
		// sModelType = 'GMConcept' , 'GMReqeustAgent', 'GMRequestAgent_LR', 'GMRequestAgent_Experment', ...猜的？
		// sModelType = 'concept_default', 'request_default', 'request_lr', 'request_experiment', ...
		// create the grounding model
		// 创建接地模型
		// sModelType		[GMConcept] | GMRequestAgent | GMRequesAgent_LR｜　．．．　
		// sModelPolicy		‘expl’ ，‘expl_impl’， ‘request_default’， ‘request_lr’...
		/*
		sModelType = GMConcept
		sModelPolicy = ‘expl’, ‘expl_impl’, ..
		*/
		pGroundingModel = pGroundingManager->CreateGroundingModel(sModelType, sModelPolicy);
		// intialize it
		// 加载接地L oadPolicy
		pGroundingModel->Initialize();
		//###########################################################################

		// set the request agent
		((CGMRequestAgent *)pGroundingModel)->SetRequestAgent(this);
	}
}

// D: obtain the grounding model for an agent
// D：获取代理的接地模型
CGroundingModel* CDialogAgent::GetGroundingModel()
{
	return pGroundingModel;
}

// D: Declares the grounding models that the agent subsumes
void CDialogAgent::DeclareGroundingModels(
	TGroundingModelPointersVector& rgmpvModels,
	TGroundingModelPointersSet& rgmpsExclude)
{

	// first put in the grounding model associated with this agent
	if (pGroundingModel &&
		rgmpsExclude.find(pGroundingModel) == rgmpsExclude.end())
	{
		rgmpvModels.push_back(pGroundingModel);
		rgmpsExclude.insert(pGroundingModel);
	}

	// the get the grounding models from the agents' concepts
	for (unsigned int i = 0; i < Concepts.size(); i++)
		Concepts[i]->DeclareGroundingModels(rgmpvModels, rgmpsExclude);

	// and the do the same for all the subagents
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->DeclareGroundingModels(rgmpvModels, rgmpsExclude);
}

// D: Returns a boolean indicating if the expectations declared are active 
//    or not. For this class, it always returns true.
//D： 返回一个布尔值，表示所声明的期望是否有效。 对于这个类，它总是返回true。
bool CDialogAgent::ExpectCondition()	//宏定义 #define EXPECT_WHEN(Condition)
{
	return true;
}


// D: the DeclareBindingPolicy function: for the default agent, it always
//    returns MIXED_INITIATIVE
// 返回绑定策略
string CDialogAgent::DeclareBindingPolicy()
{
	return MIXED_INITIATIVE;
}

// D: the DeclareFocusClaims: for this class, it checks it's own ClaimsFocus
//    condition and it's command trigger condition if one exists, 
//    then it calls DeclareFocusClaims for all the subagents
// D：DeclareFocusClaims：对于这个类，
//		<1>	它检查它自己的ClaimsFocus条件
//		<2>	和它的command trigger condition条件（如果存在），
//	然后它为所有子代理调用DeclareFocusClaims
int CDialogAgent::DeclareFocusClaims(TFocusClaimsList& fclFocusClaims)
{
	int iClaimsAdded = 0;

	//		check its own claim focus condition and command trigger condition if one exists
	// <1>	检查自己的claim focus condition 条件和command trigger condition条件（如果存在）
	bool bDeclareFocusClaim = ClaimsFocus();	//宏 ： #define TRIGGERED_BY(Condition)

	if (!TriggeredByCommands().empty())			//宏 :  #define TRIGGERED_BY_COMMANDS(Commands, GroundingModelSpec)
	{
		//############################### Trigger 条件 ##########################################################
		// <2>	如果concept被 [update或Grouded] =>  触发Focus Shift
		bDeclareFocusClaim = bDeclareFocusClaim ||
			C(FormatString("_%s_trigger", sDialogAgentName.c_str())).IsUpdatedAndGrounded(); //当前Concept被更新？
		//############################### Trigger 条件 ##########################################################
	}

	//		declare the focus claim, in case we have one
	// <3>	有焦点声明
	if (bDeclareFocusClaim)
	{
		/*
			typedef struct
			{
				string sAgentName;				// the name of the agent that claims focus		// 声明claim的agent的名称
				bool bClaimDuringGrounding;     // indicates whether or not the focus is		// 是否在Grouding期间声明Claim的
				//  claimed during grounding
			} TFocusClaim;
		*/
		TFocusClaim fcClaim;
		fcClaim.sAgentName = sName;
		fcClaim.bClaimDuringGrounding = ClaimsFocusDuringGrounding();//宏 #define CAN_TRIGGER_DURING_GROUNDING
		// <4>	添加到声明列表
		fclFocusClaims.push_back(fcClaim);
		iClaimsAdded++;
		// and also clear the triggering concept, if there is one
		// 并且清除触发概念，如果有一个
		if (!TriggeredByCommands().empty())
			C(FormatString("_%s_trigger", sDialogAgentName.c_str())).Clear(); //为什么要清空？
	}

	// then call it for the subagents, so that they can also claim focus if needed 
	// <5>	然后为子代理调用它，以便他们也可以在需要时声明焦点
	for (unsigned int i = 0; i < SubAgents.size(); i++)
	{
		iClaimsAdded += SubAgents[i]->DeclareFocusClaims(fclFocusClaims);
	}

	// finally return the number of claims added
	// <6>	最后返回添加的声明数
	return iClaimsAdded;
}

// D: the Precondition: for this class, it does nothing (always returns
//    true)
// 检查 precondition
bool CDialogAgent::PreconditionsSatisfied()
{
	return true;
}

// D: the focus claim condition: for this class, it does always returns  false
// D：焦点声明条件：对于这个类，它总是返回false
bool CDialogAgent::ClaimsFocus() //宏 ： #define TRIGGERED_BY(Condition)
{
	return false;
}

// D: indicates if the agent claims the focus while grounding is in progress
//    by default, this is false
// D：表示代理声明焦点，而接地在默认情况下正在进行，这是假的
bool CDialogAgent::ClaimsFocusDuringGrounding()//宏 #define CAN_TRIGGER_DURING_GROUNDING
{
	return false;
}

// D: the string describing the grammar concepts which trigger this 
//    agent: the default agent implicitly is not triggered by 
//    commands
// D：描述触发此代理的语法概念的字符串：默认代理隐式不由命令触发
string CDialogAgent::TriggeredByCommands()//宏  #define TRIGGERED_BY_COMMANDS(Commands, GroundingModelSpec)
{
	return "";
}

// D: this method creates a triggering concept, in case one is needed (if the
//    agent is to be triggered by a command
// D：此方法创建一个触发概念，如果需要一个触发概念（如果代理由命令触发）
void CDialogAgent::CreateTriggerConcept()
{
	//		if the agent is to be triggered by a user command, 
	// <1>	如果代理由用户命令触发
	if (!TriggeredByCommands().empty())
	{
		//		add a trigger concept
		// <2>	添加一个trigger concept
		Concepts.push_back(new CBoolConcept(FormatString("_%s_trigger", sDialogAgentName.c_str()), csUser));
		//		set the grounding model
		// <3>	设置Grouding model
		Concepts.back()->CreateGroundingModel(sTriggerCommandsGroundingModelSpec);
		//		and set it's owner dialog agent
		// <4>	设置所有者agent
		Concepts.back()->SetOwnerDialogAgent(this);
	}
}

// D: the SuccessCriteriaSatisfied method: by default an agent completes 
//    successfully when all the subagents have completed
// D：SuccessCriteriaSatisfied方法：默认情况下，代理在所有子代理完成后成功完成
bool CDialogAgent::SuccessCriteriaSatisfied()	//重载：子函数实现 #define SUCCEEDS_WHEN(Condition)
{
	// check that all subagents have completed
	for (unsigned int i = 0; i < SubAgents.size(); i++)
	if (!SubAgents[i]->HasCompleted())
		return false;

	return true;
}

// D: the FailureCriteriaSatisfied method: by default an agent completes 
//    with a failure when the number of attempts at execution exceeds the 
//    number of maximum attempts, and the success criteria has not been
//    met yet
// D：FailureCriteriaSatisfied方法：默认情况下，当执行尝试次数超过最大尝试次数，
//    并且尚未满足成功条件时，代理会完成失败
bool CDialogAgent::FailureCriteriaSatisfied()//#define FAILS_WHEN(Condition)
{
	bool bFailed = (iExecuteCounter >= GetMaxExecuteCounter()) &&
		!SuccessCriteriaSatisfied();

	if (bFailed)
		Log(DIALOGTASK_STREAM, "Agent reached max attempts (%d >= %d), failing", iExecuteCounter, GetMaxExecuteCounter());

	return bFailed;
}

// D: the GetMaxExecuteCounter function specifies how many times an agent it
//    to be executed before it terminates with a failure (by default a 
//    very large int)
int CDialogAgent::GetMaxExecuteCounter()
{
	return 10000;
}

// D: the OnCreation method: by default, nothing happens upon the creation
//    of the agent
void CDialogAgent::OnCreation()
{
}

// D: the OnDestruction method: by default, nothing happens upon the destruction
//    of the agent
void CDialogAgent::OnDestruction()
{
	// go through all the subagents and call the method
	// 便利所有的子agent，调用销毁函数
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->OnDestruction();
}

// D: the OnInitialization method: by default, nothing happens upon initialization
// D：初始化方法：默认情况下，初始化时不会发生任何事情
void CDialogAgent::OnInitialization()	//宏 #define ON_INITIALIZATION(DO_STUFF)
{
}


// D: the OnCompletion method: by default, nothing happens upon completion
// D：OnCompletion方法：默认情况下，完成后什么都不发生
void CDialogAgent::OnCompletion()//宏 #define ON_COMPLETION(DO_STUFF)
{
}

// D: the ReOpen method: by default, ReOpen calls upon ReOpenConcepts
//    and ReOpenTopic
void CDialogAgent::ReOpen()
{
	ReOpenConcepts();
	ReOpenTopic();
}

// D: the ReOpenConcepts method: by default, ReOpenConcepts calls ReOpen on
//    all the concepts held by that agent (and its children)
void CDialogAgent::ReOpenConcepts()
{
	// call ReOpen on all the concepts
	for (unsigned int i = 0; i < Concepts.size(); i++)
		Concepts[i]->ReOpen();
	// call ReOpenConcepts on all the subagents
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->ReOpenConcepts();
}

// D: the ReOpenTopic method
// D：ReOpenTopic方法
void CDialogAgent::ReOpenTopic()
{
	// set completion to false
	//设置完成标志为false
	bCompleted = false;
	ctCompletionType = ctFailed;

	// unblock the agent
	//非阻塞
	bBlocked = false;

	// reset the counters
	// 重置计数器

	iExecuteCounter = 0;
	iReOpenCounter++;
	iTurnsInFocusCounter = 0;

	// call ReOpenTopic on all the subagents
	// 递归调用子孩子
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->ReOpenTopic();
	// call the OnInitialization method
	OnInitialization();
}

// D: the IsAMainTopic: implicitly the agent is not a main topic
bool CDialogAgent::IsAMainTopic()
{
	return false;
}

// D: the prompt: by default nothing
string CDialogAgent::Prompt()
{
	return "";
}

// D: the timeout prompt: by default returns the same thing as the prompt
//    but adds version=timeout
string CDialogAgent::TimeoutPrompt()
{
	string sPrompt = Trim(Prompt());
	// check that the prompt is not empty
	if (sPrompt == "")
		return "";

	string sTimeoutPrompt;

	// check if we are dealing with a composed prompt
	if (sPrompt[0] == '{')
	{
		// then insert timeout in each of them
		do
		{
			// find the closing bracket
			int iClosingPos = FindClosingQuoteChar(sPrompt, 1, '{', '}');
			// add to the timeout prompt
			sTimeoutPrompt += sPrompt.substr(0, iClosingPos - 1);
			sTimeoutPrompt += " version=timeout}";
			// check if we're done
			if (iClosingPos >= (int)sPrompt.length()) break;
			sPrompt = sPrompt.substr(iClosingPos,
				sPrompt.length() - iClosingPos);
		} while (sPrompt != "");

	}
	else
	{
		// o/w the prompt is simple, so just add version=timeout at 
		// the end
		sTimeoutPrompt = sPrompt + " version=timeout";
	}

	return sTimeoutPrompt;
}

// D: the explain more prompt: by default adds a version=explain_more to 
//    the default prompt
string CDialogAgent::ExplainMorePrompt()
{

	string sPrompt = Trim(Prompt());
	// check that the prompt is not empty
	if (sPrompt == "")
		return "";

	string sExplainMorePrompt;

	// check if we are dealing with a composed prompt
	if (sPrompt[0] == '{')
	{
		// then insert explain more in each of them
		do
		{
			// find the closing bracket
			int iClosingPos = FindClosingQuoteChar(sPrompt, 1, '{', '}');
			// add to the explain-more prompt
			sExplainMorePrompt += sPrompt.substr(0, iClosingPos - 1);
			sExplainMorePrompt += " version=explain_more}";
			// check if we're done
			if (iClosingPos >= (int)sPrompt.length()) break;
			sPrompt = sPrompt.substr(iClosingPos,
				sPrompt.length() - iClosingPos);
		} while (sPrompt != "");
	}
	else
	{
		// o/w the prompt is simple, so just add version=explain_more at 
		// the end
		sExplainMorePrompt = sPrompt + " version=explain_more";
	}

	return sExplainMorePrompt;
}

// D: this function creates a versioned prompt
// D：此函数创建一个版本化提示
string CDialogAgent::CreateVersionedPrompt(string sVersion)
{
	// sVersion = what_can_i_say
	string sPrompt = Trim(Prompt());
	// check that the prompt is not empty
	if (sPrompt == "")
		return "";

	// the versioned prompt
	string sVersionedPrompt;

	// check if we are dealing with a composed prompt
	if (sPrompt[0] == '{')
	{
		// then insert the version in each of them
		do
		{
			// find the closing bracket
			int iClosingPos = FindClosingQuoteChar(sPrompt, 1, '{', '}');
			// add to the timeout prompt
			sVersionedPrompt += sPrompt.substr(0, iClosingPos - 1);
			sVersionedPrompt += " version=" + sVersion + "}";
			// check if we're done
			if (iClosingPos >= (int)sPrompt.length()) 
				break;
			sPrompt = sPrompt.substr(iClosingPos, sPrompt.length() - iClosingPos);
		} while (sPrompt != "");

	}
	else
	{
		// o/w the prompt is simple, so just add version=timeout at 
		// the end
		sVersionedPrompt = sPrompt + " version=" + sVersion;
	}

	return sVersionedPrompt;
}

// D: the establish_context prompt: by default calls upon the parent agent, 
//    if there is one
// D：establish_context提示：默认情况下调用父代理，如果有一个
string CDialogAgent::EstablishContextPrompt()
{
	if (pdaContextAgent && (pdaContextAgent != this))
		return pdaContextAgent->EstablishContextPrompt();
	else if (pdaParent)
		return pdaParent->EstablishContextPrompt();
	else
		return "";
}

// D: the "what can i say" prompt: by default returns the default prompt
//    but adds version=what_can_i_say
string CDialogAgent::WhatCanISayPrompt()
{
	string sPrompt = Trim(Prompt());
	// check that the prompt is not empty
	if (sPrompt == "")
		return "";

	string sWhatCanISayPrompt;

	// check if we are dealing with a composed prompt
	if (sPrompt[0] == '{')
	{
		// then insert what can i say in each of them
		do
		{
			// find the closing bracket
			int iClosingPos = FindClosingQuoteChar(sPrompt, 1, '{', '}');
			// add to the what_can_i_say prompt
			sWhatCanISayPrompt += sPrompt.substr(0, iClosingPos - 1);
			sWhatCanISayPrompt += " version=what_can_i_say}";
			// check if we're done
			if (iClosingPos >= (int)sPrompt.length()) break;
			sPrompt = sPrompt.substr(iClosingPos,
				sPrompt.length() - iClosingPos);
		} while (sPrompt != "");
	}
	else
	{
		// o/w the prompt is simple, so just add version=what_can_i_say at 
		// the end
		sWhatCanISayPrompt = sPrompt + " version=what_can_i_say";
	}

	return sWhatCanISayPrompt;
}

// D: Virtual function which specifies if this is a task agent or not
//    (by default all agents are task agents)
// 是否是任务agent
bool CDialogAgent::IsDTSAgent()
{
	return true;
}

// A: Virtual function which specifies if the execution of this agent
//    has to be synchronized with the actual flow of the conversation
//    or if it can be anticipated (i.e. whether this execution yields
//    side effects for the conversation)
bool CDialogAgent::IsConversationSynchronous()
{
	return false;
}

// A: Virtual function which specifies if this agent requires the 
//    floor for its execution (by default, agents do not)
bool CDialogAgent::RequiresFloor()
{
	return false;
}

// Virtual function used to cancel the effects of an agent which 
// was executed within the DM but not realized (i.e. there was an
// interruption of the normal flow of the dialogue due to a user
// barge-in or another external event)
// By default: decrement execution counter and set to incomplete
void CDialogAgent::Undo()
{
	iExecuteCounter--;
	ResetCompleted();
}

//-----------------------------------------------------------------------------
// 
// Other methods, mainly for providing access to public and protected members
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Relative access to Concepts
//-----------------------------------------------------------------------------
// D: the function returns a reference to the concept pointed by the 
//    relative concept path in sConceptPath
// D：函数返回对sConceptPath中相对概念路径指向的concept的引用
CConcept& CDialogAgent::C(string sConceptPath)
{

	string sAgentPath, sConceptName;
	// check if we have an agent path in the concept name
	//检查我们在概念名称中是否有代理路径
	if (SplitOnLast(sConceptPath, "/", sAgentPath, sConceptName))
	{
		// if yes, identify the agent and call the local concept retrieval for
		// that agent
		//如果是，标识代理并调用该代理的本地概念检索
		return A(sAgentPath).LocalC(sConceptName);
	}
	else
	{
		// if not, go and try to find the concept locally
		return LocalC(sConceptPath);
	}
}

// D: A printf-like version of the C() function
// 类似printf的版本的C（）函数
CConcept& CDialogAgent::C(const char *lpszConceptPath, ...)
{
	static char buffer[STRING_MAX];

	// get the arguments
	va_list pArgs;
	va_start(pArgs, lpszConceptPath);

	// print the path into the buffer
	_vsnprintf(buffer, STRING_MAX, lpszConceptPath, pArgs);

	// and finally call the standard C() function to deal with it
	return C((string)buffer);
}

// D: the function returns a pointer to a local concept indicated by 
//    sConceptName. If the concept is not found locally, we try to locate
//    it in the parent
// D：函数返回指向由sConceptName指示的本地概念的指针。 
// 如果在本地未找到概念，我们尝试在父级中找到它
CConcept& CDialogAgent::LocalC(string sConceptName)
{

	// if the agent has a defined context agent,
	// look for the concept there
	CDialogAgent *pdaNextContext = NULL;
	if (pdaContextAgent)
	{
		pdaNextContext = pdaContextAgent;
	}
	else
	{
		pdaNextContext = pdaParent;
	}

	// convert the eventual # signs with agent dynamic id
	sConceptName = ReplaceSubString(sConceptName, "#", GetDynamicAgentID());

	// Optimization code: if no concepts, then try the parent directly 
	// (if a parent exists)
	if (Concepts.size() == 0)
	{
		if (pdaNextContext) return pdaNextContext->LocalC(sConceptName);
		else
		{
			// if there's no parent, we're failing
			FatalError("Concept " + sConceptName +
				" could not be located (accessed).");
			return NULLConcept;
		}
	}

	// split the concept into base and rest in case we deal with a complex
	// concept (i.e. arrays or structures i.e. hotel.name)
	string sBaseConceptName, sRest;
	SplitOnFirst(sConceptName, ".", sBaseConceptName, sRest);

	// A: Checks if we want a merged history version of the concept
	bool bMergeConcept = false;
	if (sBaseConceptName.at(0) == '@')
	{
		bMergeConcept = true;
		sBaseConceptName = sBaseConceptName.substr(1, sBaseConceptName.length());
	}

	// now go through the list of concepts and see if you can find it
	for (unsigned int i = 0; i < Concepts.size(); i++)
	{
		if (Concepts[i]->GetName() == sBaseConceptName)
		{
			// if we have found it
			if (bMergeConcept)
			{
				CConcept *pcMerged = Concepts[i]->operator[](sRest).CreateMergedHistoryConcept();
				return *pcMerged;
			}
			else
			{
				return Concepts[i]->operator[](sRest);
			}
		}
	}

	// if it was not in the concepts owned by this agency, try in the parent/context
	// check if there is any parent/context
	if (!pdaNextContext)
	{
		// if there's no parent, we're failing
		FatalError("Concept " + sConceptName + " could not be identified "\
			"in the dialog task hierarchy.");
		return NULLConcept;
	}
	else
	{
		// if there's a parent/context, check in there
		return pdaNextContext->LocalC(sConceptName);
	}
}

//-----------------------------------------------------------------------------
// Relative access to Agents
//-----------------------------------------------------------------------------
// D: the function returns a pointer to the agent pointed by the relative
//    agent path in sDialogAgentPath
// D：函数返回一个指向sDialogAgentPath中路径指向的agent的指针
CDialogAgent& CDialogAgent::A(string sDialogAgentPath)
{

	// split the relative agent path into the first component (until /)
	// and the rest
	string sFirstComponent, sRest;

	if (SplitOnFirst(sDialogAgentPath, "/", sFirstComponent, sRest))
	{

		// if split is successful, recurse based on the first component 
		if (sFirstComponent == "")
		{

			// the path starts at the root, so it's an absolute path
			// to an agent. We can (optimally) find the agent using the 
			// registry
			CDialogAgent* pdaAgent = (CDialogAgent*)
				AgentsRegistry[sDialogAgentPath];
			if (pdaAgent)
			{
				// if the agent was found
				return *pdaAgent;
			}
			else
			{
				// if the agent was not found, fail
				FatalError("Agent " + sDialogAgentPath + " does not exist"\
					" in the dialog task hierarchy.");
				return NULLDialogAgent;
			}

		}
		else if (sFirstComponent == "..")
		{

			// then start at the parent
			if (!pdaParent)
			{
				// if there's no parent, we're failing
				FatalError("Agent " + sDialogAgentPath + " could not be "\
					"identified in the dialog task hierarchy "\
					"relative to " + GetName() + ".");
				return NULLDialogAgent;
			}
			else
			{
				// otherwise recurse on the parent
				return pdaParent->A(sRest);
			}

		}
		else if (sFirstComponent == ".")
		{

			// then it's in this agent
			return A(sRest);

		}
		else
		{

			// then it must be one of the descendants. Locate quickly using
			// the registry
			CDialogAgent* pdaAgent = (CDialogAgent*)
				AgentsRegistry[GetName() + "/" + sDialogAgentPath];
			if (pdaAgent)
			{
				// if the agent was found
				return *pdaAgent;
			}
			else
			{
				// if the agent was not found, fail
				FatalError("Agent " + sDialogAgentPath + " could not be "\
					"identified in the dialog task hierarchy "\
					"relative to " + GetName() + ".");
				return NULLDialogAgent;
			}
		}

	}
	else
	{
		// we have no path

		// it is possible that the agent referent is just "" at this point, 
		// which means we need to return the current agent
		if ((sDialogAgentPath == "") || (sDialogAgentPath == "."))
			return *this;

		// or it is possible that it's just a reference to the parent ..
		if (sDialogAgentPath == "..")
		{
			if (!pdaParent)
			{
				// if there's no parent, we're failing
				FatalError("Agent .. could not be "\
					"identified in the dialog task hierarchy "\
					"relative to " + GetName() + ".");
				return NULLDialogAgent;
			}
			else
			{
				// otherwise recurse on the parent
				return *pdaParent;
			}
		}

		// if not, try and find the agent locally (it has to 
		// be one of the subagents). Locate quickly using the registry.
		CDialogAgent* pdaAgent = (CDialogAgent*)
			AgentsRegistry[GetName() + "/" +
			sDialogAgentPath];
		if (pdaAgent)
		{
			// if the agent was found
			return *pdaAgent;
		}
		else
		{
			// if the agent was not found, fail
			FatalError("Agent " + sDialogAgentPath + " could not be "\
				"identified in the dialog task hierarchy "\
				"relative to " + GetName() + ".");
			return NULLDialogAgent;
		}
	}
}

// D: A printf-like version of the A() function
CDialogAgent& CDialogAgent::A(const char *lpszDialogAgentPath, ...)
{
	static char buffer[STRING_MAX];

	// get the arguments
	va_list pArgs;
	va_start(pArgs, lpszDialogAgentPath);

	// print the path into the buffer
	_vsnprintf(buffer, STRING_MAX, lpszDialogAgentPath, pArgs);

	// and finally call the standard A() function to deal with it
	return A((string)buffer);
}

//-----------------------------------------------------------------------------
// Adding and Deleting subagents
//-----------------------------------------------------------------------------

// D: add a subagent, in the location indicated by pdaWhere and mmMethod. for
//    the mmLastChild and mmFirstChild methods, pdaWhere will be NULL
// D：在由pdaWhere和 mmMethod指示的位置中添加子agent。 对于mmLastChild和mmFirstChild方法，pdaWhere将为NULL
void CDialogAgent::AddSubAgent(CDialogAgent* pdaWho, CDialogAgent* pdaWhere, TAddSubAgentMethod asamMethod)
{
	TAgentsVector::iterator iPtr = SubAgents.begin();
	//		insert it in the right place
	// <1>	插入到合适的位置
	switch (asamMethod)
	{
	case asamAsFirstChild:		//第一个孩子
		SubAgents.insert(SubAgents.begin(), pdaWho);
		break;
	case asamAsLastChild:		//最后一个孩子
		SubAgents.insert(SubAgents.end(), pdaWho);
		break;
	case asamAsRightSibling:	//右兄弟
		while ((iPtr != SubAgents.end()) && ((*iPtr) != pdaWhere))
			iPtr++;
		iPtr++;
		SubAgents.insert(iPtr, pdaWho);
		break;
	case asamAsLeftSibling:		//左兄弟
		while ((iPtr != SubAgents.end()) && ((*iPtr) != pdaWhere))
			iPtr++;
		SubAgents.insert(iPtr, pdaWho);
		break;
	}
	//		set the parent
	// <2>	设置parent
	pdaWho->SetParent(this);
	//		set it to dynamic
	// <3>	设置动态flag
	pdaWho->SetDynamicAgent();
	// and register it
	// <4>	注册
	pdaWho->Register();
}

// D: deletes a subagent
// 删除子节点
void CDialogAgent::DeleteSubAgent(CDialogAgent* pdaWho)
{
	TAgentsVector::iterator iPtr;
	for (iPtr = SubAgents.begin(); iPtr != SubAgents.end(); iPtr++)
	if ((*iPtr) == pdaWho)
	{
		// eliminate it
		// 删除指针
		SubAgents.erase(iPtr);
		// call the on destruction method
		// 调用销毁函数
		pdaWho->OnDestruction();
		// finally, destroy it (this will also unregister it)            
		// 删除指针 [这也会 从注册表删除]
		delete pdaWho;
		return;
	}
}

// D: deletes all the dynamic subagents
void CDialogAgent::DeleteDynamicSubAgents()
{
	TAgentsVector::iterator iPtr;
	bool bFound;
	do
	{
		bFound = false;
		for (iPtr = SubAgents.begin(); iPtr != SubAgents.end(); iPtr++)
		if ((*iPtr)->IsDynamicAgent())
		{
			CDialogAgent* pdaWho = (*iPtr);
			// eliminate it
			SubAgents.erase(iPtr);
			// call the OnDestruction method
			pdaWho->OnDestruction();
			// finally, destroy it (this will also unregister it)
			delete pdaWho;
			bFound = true;
			break;
		}
	} while (bFound);

	// now recursively call it on the remaining subagents
	for (iPtr = SubAgents.begin(); iPtr != SubAgents.end(); iPtr++)
		(*iPtr)->DeleteDynamicSubAgents();
}

//-----------------------------------------------------------------------------
// Operations related to setting and getting the parent for a dialog 
// agent
//-----------------------------------------------------------------------------
// D: set the parent
// 重置父节点 + 更新名称
void CDialogAgent::SetParent(CDialogAgent* pdaAParent)
{
	// set the new parent
	pdaParent = pdaAParent;
	// and update the name of the agent
	UpdateName();
}

// D: return the parent
CDialogAgent* CDialogAgent::GetParent()
{
	return pdaParent;
}

// D: updates the name of the agent, by looking up the parent and concatenating
//    names as /name/name/name. Also calls UpdateName for the children, since 
//    their names need to be updated in this case, too
//D：通过查找父级, 连接名称为 [/name/name/name] 来更新代理的名称。 
//   也调用孩子的UpdateName，因为在这种情况下，他们的名字也需要更新
void CDialogAgent::UpdateName()
{
	// analyze if we have or not a parent, and update the name
	// 重置name = 【从根节点到当前节点的路径】
	if (pdaParent)
	{
		sName = pdaParent->GetName() + "/" + sDialogAgentName;
	}
	else
	{
		sName = "/" + sDialogAgentName;
	}

	// and now update the children, too
	// 相同方法更新它所有的孩子节点
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->UpdateName();
}

//-----------------------------------------------------------------------------
// Operations related to setting and getting the context agent for a dialog 
// agent
//-----------------------------------------------------------------------------
// D: set the context agent
void CDialogAgent::SetContextAgent(CDialogAgent* pdaAContextAgent)
{
	// set the new context agent
	pdaContextAgent = pdaAContextAgent;
}

// D: return the context agent
// D：返回上下文代理
CDialogAgent* CDialogAgent::GetContextAgent()
{
	// if there is a context agent
	if (pdaContextAgent)
		// then return that
		return pdaContextAgent;
	else
		// o/w return self
		return this;
}

//-----------------------------------------------------------------------------
// Obtaining the main topic
//-----------------------------------------------------------------------------
// D: returns the main topic for an agent
CDialogAgent* CDialogAgent::GetMainTopic()
{
	// if this is a main topic, return it
	if (IsAMainTopic()) return this;
	else if (!pdaParent)
	{
		Log(DMCORE_STREAM, "%s has no parent -> MainTopic=NULL", sName.c_str());
		// if it's not a main topic and it doesn't have a parent, return NULL
		return NULL;
	}
	else
	{
		// return the main topic of its parent
		return pdaParent->GetMainTopic();
	}
}

//-----------------------------------------------------------------------------
// Access to completed, reset, reopen information
//-----------------------------------------------------------------------------

// D: return the agent completion status
// 返回完成状态
bool CDialogAgent::HasCompleted()//宏定义： #define COMPLETED(Agent) (A(#Agent).HasCompleted())
{
	// if the agent has the completed flag set, return true
	// 如果有结束flag -> 【成功 | 失败】
	if (bCompleted) return true;

	// o/w check HasSucceeded and HasFailed
	// 函数检查成功还是失败 - 结束类型
	return HasSucceeded() || HasFailed();
}

// D: set the agent completion status
void CDialogAgent::SetCompleted(TCompletionType ctACompletionType)
{
	bCompleted = true;
	ctCompletionType = ctACompletionType;
}

// D: resets the agent completion status
void CDialogAgent::ResetCompleted()
{
	bCompleted = false;
	ctCompletionType = ctFailed;
}

// D: indicates if the agent has completed with a failure
// 检测agent是否结束为failed
bool CDialogAgent::HasFailed()
{

	// if the agent is already marked as failed, return true
	// 已经完成，并且结束类型为Failed
	if (bCompleted && (ctCompletionType == ctFailed))
		return true;

	// o/w check if the failure condition was recently matched
	// 测试失败条件是否匹配
	return FailureCriteriaSatisfied();//重载-子函数实现： #define FAILS_WHEN(Condition)
}


// D: indicates if the agent has completed successfully
// D：表示代理是否已成功完成
bool CDialogAgent::HasSucceeded() //宏定义： #define SUCCEEDED(Agent) (A(#Agent).HasSucceeded())
{

	// if the agent is already marked as succeeded, return true
	// 如果代理已经标记为已成功，则返回true
	if (bCompleted && (ctCompletionType == ctSuccess))
		return true;

	// o/w check if the success criterion was recently matched
	// o/w检查成功标准是否最近匹配SuccessCriteriaSatisfied
	return SuccessCriteriaSatisfied();//重载：宏定义： #define SUCCEEDS_WHEN(Condition)
}

//-----------------------------------------------------------------------------
// Access to Blocked/Unblocked information
//-----------------------------------------------------------------------------
// D: return true if one of the agent's ancestors is blocked
// 如果其中一个代理的祖先被阻塞，则返回true
bool CDialogAgent::IsAgentPathBlocked()
{
	// go recursively through the parents to find out if there's anything blocked
	// 递归地通过父母来确定是否有任何阻塞
	if (pdaParent)
		return pdaParent->IsAgentPathBlocked() || IsBlocked();
	else
		return IsBlocked();
}

// D: return true if the agent is blocked
// D：如果代理被阻止，则返回true
bool CDialogAgent::IsBlocked()
{
	return bBlocked;
}

// D: block the agent
void CDialogAgent::Block()
{
	// set blocked to true
	bBlocked = true;
	// and call recursively for all the subagents
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->Block();
}

// D: unblock the agent
void CDialogAgent::UnBlock()
{
	// set blocked to false
	bBlocked = false;
	// and call recursively for all the subagents
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->UnBlock();
}

//-----------------------------------------------------------------------------
// Access to dynamic agent ID information
//-----------------------------------------------------------------------------
// D: sets the dynamic agent flag
// 设置动态agent flag
void CDialogAgent::SetDynamicAgent()
{
	bDynamicAgent = true;
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->SetDynamicAgent();
}

// D: obtains the dynamic agent flag
bool CDialogAgent::IsDynamicAgent()
{
	return bDynamicAgent;
}

// D: setting the dynamic agent ID
// 设置动态agent Id = [groudingAction动态创建的agent]
void CDialogAgent::SetDynamicAgentID(string sADynamicAgentID)
{
	// set the agent to dynamic
	bDynamicAgent = true;
	// set the dynamic agent ID
	sDynamicAgentID = sADynamicAgentID;
	// and set it for its subagents, too
	for (unsigned int i = 0; i < SubAgents.size(); i++)
		SubAgents[i]->SetDynamicAgentID(sDynamicAgentID);
}

// D: returning the dynamic agent ID
string CDialogAgent::GetDynamicAgentID()
{
	return sDynamicAgentID;
}

//-----------------------------------------------------------------------------
// Execute counter methods, Turns in focus count methods
//-----------------------------------------------------------------------------
// D: increment the execute count
// 增加执行次数计数
void CDialogAgent::IncrementExecuteCounter()
{
	iExecuteCounter++;//增加执行计数器
}

// D: obtain the value of the execute count
int CDialogAgent::GetExecuteCounter()
{
	return iExecuteCounter;
}

//-----------------------------------------------------------------------------
// Access to reset and reopen information
//-----------------------------------------------------------------------------
// D: check if an agent was reset
bool CDialogAgent::WasReset()
{
	return iResetCounter > 0;
}

// D: check if an agent was reopened
bool CDialogAgent::WasReOpened()
{
	return iReOpenCounter > 0;
}

//-----------------------------------------------------------------------------
// Access to turns in focus counter
//-----------------------------------------------------------------------------
// D: increment the turns in focus counter
// D：增加focus计数器的圈数
void CDialogAgent::IncrementTurnsInFocusCounter()
{
	iTurnsInFocusCounter++;
}

// D: obtain the value of the turns in focus counter
int CDialogAgent::GetTurnsInFocusCounter()
{
	return iTurnsInFocusCounter;
}

//-----------------------------------------------------------------------------
// Access to the last input index 
//-----------------------------------------------------------------------------
// D: set the last input index 
void CDialogAgent::SetLastInputIndex(int iInputIndex)
{
	iLastInputIndex = iInputIndex;
}

// D: obtain a pointer to the last input index
int CDialogAgent::GetLastInputIndex()
{
	return iLastInputIndex;
}

//-----------------------------------------------------------------------------
// Access to the last execution index 
//-----------------------------------------------------------------------------
// A: set the last execution index 
// 设置最后执行索引
void CDialogAgent::SetLastExecutionIndex(int iExecutionIndex)
{
	iLastExecutionIndex = iExecutionIndex;
}

// D: obtain a pointer to the last execution index
int CDialogAgent::GetLastExecutionIndex()
{
	return iLastExecutionIndex;
}

//-----------------------------------------------------------------------------
// Access to the last bindings index
//-----------------------------------------------------------------------------
// D: set the last bindings index 
// D：设置最后的绑定索引
void CDialogAgent::SetLastBindingsIndex(int iBindingsIndex)
{
	iLastBindingsIndex = iBindingsIndex;//TBindingHistory bhBindingHistory;     // the binding history	//绑定历史
}

// D: get the last bindings index 
int CDialogAgent::GetLastBindingsIndex()
{
	return iLastBindingsIndex;
}

//-----------------------------------------------------------------------------
// 
// Protected methods for parsing various declarative constructs
//
//-----------------------------------------------------------------------------

// D: Parse a grammar mapping into a list of expectations
// D：将语法映射解析到期望列表中
// 将触发条件字符串解析成一个个TConceptExpectation对象，然后添加到引用列表里rcelExpectationList
/*

	TRIGGERED_BY_COMMANDS("[QueryRoomDetails],[QueryProjector.projector]", "expl")
	TRIGGERED_BY_COMMANDS("@(..)[QueryRoomDetails],@(..)[QueryRoomSize]", "expl")

	parseGrammarMapping( C("_%s_trigger", sDialogAgentName.c_str()).GetAgentQualifiedName(),
						TriggeredByCommands(),
						celTriggerExpectationList
	sConceptName		=	"_AgentName/ConceptName_triger"     <===	C("_%s_trigger", sDialogAgentName.c_str()).GetAgentQualifiedName(),
	sGrammarMapping		=	"@[Repeat]" 或者 "@[Quit]"。。		<===	TriggeredByCommands()
	rcelExpectationList =	TConceptExpectationList
	);


	string sRequestedConceptName = RequestedConceptName();			//宏定义 “REQUEST_CONCEPT（xxx）”
	string sGrammarMapping = GrammarMapping();						//宏定义 #define GRAMMAR_MAPPING("![Yes]>true, ![No]>false")
	if (!sRequestedConceptName.empty() && !sGrammarMapping.empty())
	{
		parseGrammarMapping(sRequestedConceptName, sGrammarMapping,	celLocalExpectationList);
	}
*/	
void CDialogAgent::parseGrammarMapping(string sConceptNames, 
	string sGrammarMapping,
	TConceptExpectationList& rcelExpectationList)
{

	// empty the list
	rcelExpectationList.clear();

	// parse it, construct the appropriate expectation structures and add them
	// to the list 
	// 解析它，构造适当的期望结构并将它们添加到列表中
	string sItem;	// take one item at a time

	// while there are still thing left in the string
	// 字符串中还有对象
	while (!sGrammarMapping.empty())//while (!sGrammarMapping.empty())
	{
		/*
		
						TRIGGERED_BY_COMMANDS("[QueryRoomDetails],[QueryProjector.projector]", "expl")
						TRIGGERED_BY_COMMANDS("@(..)[QueryRoomDetails],@[QueryRoomSize]", "expl")

						 GRAMMAR_MAPPING(
						 "![QueryWhere]>1,"
						 "![QueryHowFar]>2,"
						 "![QueryCategory]>3,"
						 "![QueryDirections]>4" )

						 GRAMMAR_MAPPING("@(../../RequestWaitForCommands)[QueryHowFar]>true")

						 GRAMMAR_MAPPING("![Yes]>true,"	 "![No]>false" )

						 GRAMMAR_MAPPING("[QueryHowFar.Origin]")

						 GRAMMAR_MAPPING("![District]," "![Location]" )

						 value1|confidence1; value2|confidence >: filter_function

		*/

		TConceptExpectation ceExpectation; // the concept expectation definition

		//		get the first item
		// <1>	解析出一个item [concept] 
		SplitOnFirst(sGrammarMapping, ",", sItem, sGrammarMapping);
		sItem = Trim(sItem);

		// process the item and construct the expectation
		// 解析item，构造expection
		string sLeftSide, sRightSide;

		//		decide what is the binding method
		// <2>	决定是什么绑定方法 "![Yes]>:true"
		/*
			bmSlotValue			=>	    绑定slot的值				GRAMMAR_MAPPING("[QueryCategory.Origin]")
			bmExplicitValue		=>		绑定期望中的显式值		GRAMMAR_MAPPING("![Yes]>true," "![No]>false" )
			bmBindingFilter		=>		应用自定义过滤器			[value1|confidence1; value2|confidence] >: filter_function
		*/
		if (SplitOnFirst(sItem, ">", sLeftSide, sRightSide))
		{
			sRightSide = Trim(sRightSide);
			if (sRightSide[0] == ':')
				ceExpectation.bmBindMethod = bmBindingFilter;
			else
				ceExpectation.bmBindMethod = bmExplicitValue;
		}
		else
		{
			ceExpectation.bmBindMethod = bmSlotValue;
		}

		sLeftSide = Trim(sLeftSide);//slot名

		//		analyze if the expectation is open at this point or not
		// <3>	分析期望是否在这一点open
		/*
		(i.e. do we have 
			 [slot] or 
			![slot] or 
			@[slot] or 
			@(agent,agent)[slot])
		*/
		if (sLeftSide[0] == '[')
		{
			//		if a simple concept mapping, then we declare it only if it's under the main topic (disable it otherwise)
			// <4>	如果一个简单的概念映射，那么我们只声明它在相同主题下可用（其它情况禁用它）
			ceExpectation.bDisabled = !pDTTManager->IsAncestorOrEqualOf( pDMCore->GetCurrentMainTopicAgent()->GetName(), sName);
			if (ceExpectation.bDisabled)
			{
				ceExpectation.sReasonDisabled = "[] not under topic";//不在相同主题下
			}
			ceExpectation.sGrammarExpectation = sLeftSide;	//slot name => [QueryHowFar.Origin]
			ceExpectation.sExpectationType = "";			// ??
		}
		else if (sLeftSide[0] == '!')
		{
			//		if a ![] concept mapping, declare it only if we are under focus
			// <5>	当处于焦点时可用
			ceExpectation.bDisabled = !pDMCore->AgentIsInFocus(this);
			if (ceExpectation.bDisabled)
			{
				ceExpectation.sReasonDisabled = "![] not under focus";
			}
			ceExpectation.sGrammarExpectation =	sLeftSide.substr(1, sLeftSide.length() - 1); //slot name => [Yes]
			ceExpectation.sExpectationType = "!";	// '!'
		}
		else if ((sLeftSide[0] == '@') || (sLeftSide[0] == '*'))//else if ((sLeftSide[0] == '@') || (sLeftSide[0] == '*'))
		{
			if (sLeftSide[1] == '[') // a @[] or *[]
			{
				// <6>	if a @[] or *[] concept mapping, then always declare it
				ceExpectation.bDisabled = false;	//不禁用 = 可用
				ceExpectation.sGrammarExpectation = sLeftSide.substr(1, sLeftSide.length() - 1);// slot name => [QueryRoomSize]
			}
			else if (sLeftSide[1] == '(') //  @(agent,agent)[] or *(agent,agent)[]
			{
				// <7>	if a @(agent,agent)[] or *(agent,agent)[] concept mapping, 
				//		then declare it only if the focus is under one of those agents
				//		start by constructing the list of agents
				string sAgents;
				SplitOnFirst(sLeftSide, ")", sAgents, ceExpectation.sGrammarExpectation);
				sAgents = sAgents.substr(2, sAgents.length() - 2);
				TStringVector vsAgents = PartitionString(sAgents, ";");

				//		figure out the focused task agent
				// <8>	找出聚焦的任务代理
				CDialogAgent* pdaDTSAgentInFocus = pDMCore->GetDTSAgentInFocus();
				if (!pdaDTSAgentInFocus)
					FatalError("Could not find a DTS agent in focus.");
				string sFocusedAgentName = pdaDTSAgentInFocus->GetName();

				//		go through the agents in the list and figure out if they contain the focus
				// <9>	通过列表中的代理，确定它们是否包含焦点
				ceExpectation.bDisabled = true;//不可用
				for (unsigned int i = 0; i < vsAgents.size(); i++)
				{
					// <10>	如果(vsAgents[i])是sFocusedAgentName的祖先或者它们相等，则返回true   => 可用
					if (pDTTManager->IsAncestorOrEqualOf(A(vsAgents[i]).GetName(), sFocusedAgentName))
					{
						ceExpectation.bDisabled = false;//可用
						break;
					}
				}
				if (ceExpectation.bDisabled)
				{
					ceExpectation.sReasonDisabled =
						FormatString("%c(%s) not containing focus", sLeftSide[0], sAgents.c_str());
				}
			}
			//		finally, set the expectation type
			// <11>	finally，设置期望类型
			ceExpectation.sExpectationType = FormatString("%c", sLeftSide[0]);	// @ 或者 *
		}//else if ((sLeftSide[0] == '@') || (sLeftSide[0] == '*'))

		//		close the expectation if the agent path is blocked
		// <12>	如果代理路径被阻塞，则关闭期望值
		if (IsAgentPathBlocked())
		{
			ceExpectation.bDisabled = true;
			ceExpectation.sReasonDisabled = "agent path blocked";
		}

		//		if we bind an explicitly specified concept value
		// <13>	如果我们绑定一个明确指定的概念值
		if (ceExpectation.bmBindMethod == bmExplicitValue)//GRAMMAR_MAPPING("![Yes]>true,"	 "![No]>false" )
		{
			// set the sExplicitValue member
			ceExpectation.sExplicitValue = sRightSide;// sExplicitValue = true|false
		}
		else if (ceExpectation.bmBindMethod == bmBindingFilter)
		{
			//		set the sBindingFilterName member
			// <14>	设置sBindingFilterName成员   => [value1|confidence1; value2|confidence] >: filter_function
			ceExpectation.sBindingFilterName = sRightSide.substr(1, sRightSide.length() - 1);
		}

		//			fill in the rest of the members of the expectation structure
		// <15>		填充期望结构的其余成员
		ceExpectation.pDialogAgent = this;
		//sConceptName = "_AgentName/ConceptName_triger"
		ceExpectation.vsOtherConceptNames = PartitionString(sConceptNames, ";,");
		//			now go through this and replace the names with the agent qualified versions
		// <16>		现在遍历，并用代理限定的版本替换名称
		for (unsigned int i = 0; i < ceExpectation.vsOtherConceptNames.size(); i++)
			ceExpectation.vsOtherConceptNames[i] =	C(Trim(ceExpectation.vsOtherConceptNames[i])).GetAgentQualifiedName();

		//			now assing the name of the main expected concept
		// <17>		现在分配主要的预期概念的名称 : sConceptNames是从当前agent获取到的，所以都是需要的
		ceExpectation.sConceptName = ceExpectation.vsOtherConceptNames[0];
		//			and delete the first one from the others list
		// <18>		并从其他列表中删除第一个
		ceExpectation.vsOtherConceptNames.erase( ceExpectation.vsOtherConceptNames.begin() );

		//			finally lowecase the grammar expectation
		// <19>		小写语法期望
		ceExpectation.sGrammarExpectation =	ToLowerCase(ceExpectation.sGrammarExpectation);// 带着【】？

		// <20>		add the expectation to the list
		rcelExpectationList.push_back(ceExpectation);
	}//while (!sGrammarMapping.empty())
}
