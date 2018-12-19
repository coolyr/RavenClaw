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
// DMCORE.CPP - implements the dialog core thread
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
//   [2003-05-13] (dbohus): changed so that configuration parameters are in a 
//                           hash, which gets also logged
//   [2003-02-14] (dbohus): added pGroundingManager agent
//   [2002-05-25] (dbohus): deemed preliminary stable version 0.5
//   [2002-04-13] (dbohus): moved all functionality out into the DMCoreAgent
//   [2002-01-26] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

//#include <SVN_Data.h>
#include "../DMCore/Core.h"
#include "../DialogTask/DialogTask.h"

//自己添加的信息
#define OLYMPUS_SVN_BRANCH "http://trac.speech.cs.cmu.edu/svn/olympus/tags/2.6.0"
#define OLYMPUS_SVN_REVISION "4583"
//-----------------------------------------------------------------------------
// Definitions for the dialog core agents
//-----------------------------------------------------------------------------
CDMCoreAgent			*pDMCore;
COutputManagerAgent		*pOutputManager;
CInteractionEventManagerAgent	*pInteractionEventManager;
CTrafficManagerAgent	*pTrafficManager;
CStateManagerAgent		*pStateManager;
CDTTManagerAgent		*pDTTManager;
CGroundingManagerAgent  *pGroundingManager;

//-----------------------------------------------------------------------------
// Dialog core initialization
//-----------------------------------------------------------------------------
void InitializeDialogCore(TRavenClawConfigParams rcpParams)
{

	// Log the core initialization parameters
	//#log记录初始化参数
	Log(CORETHREAD_STREAM, "Initializing Core ...");
	Log(CORETHREAD_STREAM, "Core initialization parameters dumped below:\n%s", rcpParams.ToString().c_str());

	// Initialize the random number gererator
	//#初始化随机数生成器
	_timeb theTime = GetTime(); // ??
	srand((unsigned)(theTime.millitm + theTime.time * 1000));

	//		Create a new Dialog Management Core Agent, and register it
	//<1>	创建一个新的Dialog Management Core Agent，并注册它
	AgentsRegistry.Clear();
	AgentsRegistry.RegisterAgentType("CDMCoreAgent", CDMCoreAgent::AgentFactory);		//注册 <agentType, create Function>
	pDMCore = (CDMCoreAgent*)AgentsRegistry.CreateAgent("CDMCoreAgent", "DMCoreAgent"); //创建agent
	if (!pDMCore)
		FatalError("Could not create DMCore agent.");
	pDMCore->Initialize(); //初始化 agent
	pDMCore->Register();	   //注册agent[this指针]

	// set the default core configuration 
	//设置默认配置参数
	pDMCore->SetDefaultTimeoutPeriod(atoi(rcpParams.Get(RCP_DEFAULT_TIMEOUT).c_str()));//默认超时
	pDMCore->SetDefaultNonunderstandingThreshold((float)atof(rcpParams.Get(RCP_DEFAULT_NONUNDERSTANDING_THRESHOLD).c_str()));//默认不理解阈值

	//#######################################################################
	// create all the other dialog core agents
	// 创建其他的辅助 对话核心agent
	Log(CORETHREAD_STREAM, "Creating auxiliary core dialog core agents ...");


	//		create the interaction event manager
	//<2>	交互事件管理器
	AgentsRegistry.RegisterAgentType("CInteractionEventManagerAgent", CInteractionEventManagerAgent::AgentFactory);
	pInteractionEventManager = (CInteractionEventManagerAgent *)AgentsRegistry.CreateAgent("CInteractionEventManagerAgent", "InteractionEventManagerAgent");
	if (!pInteractionEventManager)
		FatalError("Could not create InteractionEventManager agent.");
	pInteractionEventManager->Initialize(); //初始化agent
	pInteractionEventManager->Register();	//注册agent[this指针] 


	//		create the output manager
	//<3>	输出管理器
	AgentsRegistry.RegisterAgentType("COutputManagerAgent", COutputManagerAgent::AgentFactory);
	pOutputManager = (COutputManagerAgent *)AgentsRegistry.CreateAgent("COutputManagerAgent", "OutputManagerAgent");
	if (!pOutputManager)
		FatalError("Could not create OutputManager agent.");
	pOutputManager->Initialize();	//初始化agent
	pOutputManager->Register();		//注册agent[this指针]


	//		create the galaxy stub
	//<4>	创建 galaxy 信息枢纽
	AgentsRegistry.RegisterAgentType("CTrafficManagerAgent", CTrafficManagerAgent::AgentFactory);
	pTrafficManager = (CTrafficManagerAgent *)AgentsRegistry.CreateAgent("CTrafficManagerAgent", "TrafficManagerAgent");
	if (!pTrafficManager)
		FatalError("Could not create TrafficManager agent.");
	pTrafficManager->Initialize();			//初始化agent
	pTrafficManager->Register();			//注册agent[this指针]


	//		create the state manager
	//<5>	创建状态管理器
	AgentsRegistry.RegisterAgentType("CStateManagerAgent", CStateManagerAgent::AgentFactory);
	pStateManager = (CStateManagerAgent *)AgentsRegistry.CreateAgent("CStateManagerAgent", "StateManagerAgent");
	if (!pStateManager)
		FatalError("Could not create StateManager agent.");
	pStateManager->Initialize();			//初始化agent
	pStateManager->Register();				//注册agent[this指针]
	// set the state broadcast address
	// 设置状态广播地址
	// 设置状态名 【agentName -> stateName】
	pStateManager->LoadDialogStateNames(rcpParams.Get(RCP_DIALOG_STATES_FILE));


	//		create the dialog task tree manager
	//<6>	创建对话框任务树[ DTT ]管理器
	AgentsRegistry.RegisterAgentType("CDTTManagerAgent", CDTTManagerAgent::AgentFactory);
	pDTTManager = (CDTTManagerAgent *)AgentsRegistry.CreateAgent("CDTTManagerAgent", "DTTManagerAgent");
	if (!pDTTManager)
		FatalError("Could not create DTTManager agent.");
	pDTTManager->Initialize();			//初始化agent
	pDTTManager->Register();			//注册agent[this指针]


	//		create the grounding manager
	//<7>	创建接地管理器
	AgentsRegistry.RegisterAgentType("CGroundingManagerAgent", CGroundingManagerAgent::AgentFactory);
	pGroundingManager = (CGroundingManagerAgent *)AgentsRegistry.CreateAgent("CGroundingManagerAgent", "GroundingManagerAgent");
	if (!pGroundingManager)
		FatalError("Could not create GroundingManager agent.");
	pGroundingManager->Initialize();		// 初始化agent
	pGroundingManager->Register();		// 注册agent[this指针]

	//#####################################################################################
	// set the configuration
	// 设置配置参数
	pGroundingManager->SetConfiguration(rcpParams.Get(RCP_GROUNDING_MANAGER_CONFIGURATION));
	// 从接地file中加载polices => 【只是从中读出需要的内容，以string的形式，存储到policy的hash表中.】
	//【当真实创建Agent的时候，运行Create函数的时候再在policy Hash表中查找到配置的Grouding模型，进行policy字符串的解析，创建相应的对象】
	// and load the models specifications from the grounding policies file
	// 从接地策略文件加载模型策略 <policy Hash表>
	// hash holding the grounding models policies 
	// (key = model_name, value= model policy string)
	// 定义policy Hash
	// STRING2STRING s2sPolicies;
	/*
		expl_impl = expl_impl.pol 文件内容
		expl = expl.pol	文件内容
		request_default = request_default.pol 文件内容
		request_lr = request_lr.pol 文件内容
	*/
	if (rcpParams.Get(RCP_GROUNDING_POLICIES) != "")
		pGroundingManager->LoadPoliciesFromString(rcpParams.Get(RCP_GROUNDING_POLICIES));
	else
		pGroundingManager->LoadPoliciesFromFile(rcpParams.Get(RCP_GROUNDING_POLICIES_FILE));
	//#####################################################################################

	Log(CORETHREAD_STREAM, "Auxiliary core dialog management agents created successfully.");

	Log(CORETHREAD_STREAM, "Core initialization completed successfully.");
}

//-----------------------------------------------------------------------------
// Dialog core termination
//-----------------------------------------------------------------------------
void TerminateDialogCore()
{
	Log(CORETHREAD_STREAM, "Terminating Core ...");

	//删除所有核心的Agent
	// destroy the core dialog management agent
	delete pDMCore;
	pDMCore = NULL;
	// and all the other core agents
	delete pDTTManager;
	pDTTManager = NULL;
	delete pTrafficManager;
	pTrafficManager = NULL;
	delete pOutputManager;
	pOutputManager = NULL;
	delete pInteractionEventManager;
	pInteractionEventManager = NULL;
	delete pStateManager;
	pStateManager = NULL;
	delete pGroundingManager;
	pGroundingManager = NULL;

	// and finally clear up the registry
	// 清空注册表
	AgentsRegistry.Clear();

	// and log that the core terminated successfully
	Log(CORETHREAD_STREAM, "Core terminated successfully.");
}

//-----------------------------------------------------------------------------
// THE DIALOG CORE THREAD FUNCTION
// 核心线程函数
//-----------------------------------------------------------------------------
//LPVOID是一个没有类型的指针，也就是说你可以将任意类型的指针赋值给LPVOID类型的变量（一般作为参数传递），然后在使用的时候在转换回来。
DWORD WINAPI DialogCoreThread(LPVOID pParams)
{
	// <1>	初始化配置参数
	TRavenClawConfigParams rcpParams = *(TRavenClawConfigParams *)pParams;
	// <2>	日志初始化
	InitializeLogging(
		rcpParams.Get(RCP_LOG_DIR),
		rcpParams.Get(RCP_LOG_PREFIX),
		DEFAULT_LOG_FILENAME,
		rcpParams.Get(RCP_LOGGED_STREAMS),
		rcpParams.Get(RCP_DISPLAYED_STREAMS),
		rcpParams.Get(RCP_EXIT_ON_FATAL_ERROR)
		);

	// Print out Olympus version stuff
	// 记录Olympus版本号
	Log(CORETHREAD_STREAM, "Olympus Branch: %s\nOlympus Revision: %s", OLYMPUS_SVN_BRANCH, OLYMPUS_SVN_REVISION);

	// Initialize the core: create all the core agents
	// <3>	初始化core：创建所有核心Agent
	InitializeDialogCore(rcpParams);

	// Call the dialog task initialize function 
	// <4>	调用对话任务初始化函数
	//		每当新会话开始时调用的函数,开发者定制一些特殊服务 [宏定义添加]
	DialogTaskOnBeginSession();//宏 -> #define CORE_CONFIGURATION(Configuration)

	// Do the dialog dance :)
	// <5>	运行
	pDMCore->Execute();

	// Terminate the dialog core
	// <6>	终结对话Core
	TerminateDialogCore();

	// Finally, send a message to signal that this session of the Dialog Core is over
	// <7>	最后，发送消息以表示Dialog Core会话结束
	PostThreadMessage(g_idDMInterfaceThread, WM_DIALOGFINISHED, 0, 0);
	return 0;
}

