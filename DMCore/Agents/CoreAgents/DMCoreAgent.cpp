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
// DMCOREAGENT.CPP - implements the core dialog management agent, that handles
//				     execution and input passes (agenda generation, bindings, 
//				     etc)
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
//   [2007-03-05] (antoine): changed Execute so that grounding and dialog agents
//							 are only executed once the floor is free and all 
//							 pending prompt notifications have been received 
//							 (previously, only the floor mattered)
//   [2005-06-20] (antoine): copied DMCoreAgent
//
//-----------------------------------------------------------------------------

#include "../../../DMInterfaces/DMInterface.h"
#include "../../../DMCore/Agents/Registry.h"
#include "../../../DMCore/Core.h"
#ifdef GALAXY
#include "DMCore/Events/GalaxyInteractionEvent.h"
#endif
#include "DMCoreAgent.h"


// *** *** BIG QUESTION: What core stuff do we log, and where ?
// 1. We need to log the compiled agenda at each input pass
// 2. Maybe it would be nice to log the execution stack, or at least a trace
//    of it. 
// 3. Not only the open concepts, but also the bindings...

// *** Concept Binding and generally the Input PASS need to be revisited 
//     since they were not implemented completely

// A: A vector of strings containing labels for TFloorStatus variables
// 包含 TFloorStatus 变量标签的字符串向量 [全局变量]
vector<string> vsFloorStatusLabels;


//-----------------------------------------------------------------------------
// Constructors and Destructors
//-----------------------------------------------------------------------------

// D: constructor
//默认 - 构造函数
CDMCoreAgent::CDMCoreAgent(string sAName, string sAConfiguration, string sAType) : CAgent(sAName, sAConfiguration, sAType)
{
	bFocusClaimsPhaseFlag = false;				// indicates whether we should		焦点声明标志
	fsFloorStatus = fsSystem;					// indicates who has the floor		Floor状态 - 枚举
	iTurnNumber = 0;							// stores the current turn number	记录当前turn数
	csoStartOverFunct = NULL;					// a custom start over function		定义用户定制的重新启动的 函数[指针]
	//全局变量 - 定义floor状态
	vsFloorStatusLabels.push_back("unknown");	// 包含TFloorStatus变量标签的字符串向量
	vsFloorStatusLabels.push_back("user");
	vsFloorStatusLabels.push_back("system");
	vsFloorStatusLabels.push_back("free");
}

// D: virtual destructor - does nothing so far
//虚拟 析构函数
CDMCoreAgent::~CDMCoreAgent()
{
}

//-----------------------------------------------------------------------------
// Static function for dynamic agent creation
////静态方法动态创建Agent
//-----------------------------------------------------------------------------
CAgent* CDMCoreAgent::AgentFactory(string sAName, string sAConfiguration)
{
	return new CDMCoreAgent(sAName, sAConfiguration);
}

//-----------------------------------------------------------------------------
// CAgent class overwritten methods
//-----------------------------------------------------------------------------

// D: the overwritten Reset method
//重写【覆盖】的复位方法
void CDMCoreAgent::Reset()
{
	// clear the class members
	esExecutionStack.clear();
	ehExecutionHistory.clear();
	bhBindingHistory.clear();
	eaAgenda.celSystemExpectations.clear();
	eaAgenda.vCompiledExpectations.clear();
}

//-----------------------------------------------------------------------------
//
// CoreAgent specific methods
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
// DIALOG CORE EXECUTION
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// D: this function does the actual dialog task execution, by executing the
//    agents that are on the stack, and issuing input passes when appropriate
//	  此函数通过执行堆栈上的agent来执行实际的对话任务DT，并在适当时发出输入阶段
//-----------------------------------------------------------------------------

void CDMCoreAgent::Execute()
{

	//		create & initialize the dialog task
	// <1>	创建 和 初始化 Dialog Task Tree
	pDTTManager->CreateDialogTree();

	//		The floor is initially free
	// <2>	初始化floor状态 '自由'       枚举：[未知、系统、用户、自由]
	SetFloorStatus(fsFree); // 'fsFree'
	   
	//		put the root of the dialog task on the execution stack
	// <3>	把根节点放入堆栈
	Log(DMCORE_STREAM, "Starting Dialog Task execution.");
	ContinueWith(this, pDTTManager->GetDialogTaskTreeRoot());

	//		creates the initial dialog state
	// <4>	初始化[更新]对话状态		[1-重新组装agenda   2-记录当前状态	3-解析input line cofig，作为状态一部分??]
	pStateManager->UpdateState();

	//		do the while loop for execution
	// <5>	做while循环执行 - 当执行栈非空时，继续执行.
	while (!esExecutionStack.empty())//while (!esExecutionStack.empty())
	{
		//#############################################################################################################
		
		// <6>	获取sTurnId
		string sTurnId = "User:???";
		if (pInteractionEventManager->GetLastInput())	//获取最近user输入事件指针
			sTurnId = "User:" + pInteractionEventManager->GetLastInput()->GetStringProperty("[uttid]");

		Log(DMCORE_STREAM, "Starting DMCore processing [%s]", sTurnId.c_str());

		//#############################################################################################################
		
		//		eliminate all the agents that have completed from the execution stack
		// <7>	所有执行完成的agent弹出栈  => agenda修改过 -> 重新组装
		popCompletedFromExecutionStack();
		Log(DMCORE_STREAM, "Eliminated completed agents from stack [%s]", sTurnId.c_str());
		
		//#############################################################################################################
		
		//		if the execution stack is empty now, break the loop
		// <8>	如果栈空 - 退出循环 [DTT执行完毕]
		if (esExecutionStack.empty())
			break;
		
		//#############################################################################################################
		
		//		Performs grounding only when the floor is free and we got all our notifications
		//		只有当floor是free，我们得到了所有的通知[RecentOutputs为空]，才执行grouding
		// <9>	如果返回为“”，说明我们都得到通知
		if ((GetFloorStatus() == fsFree) && (pOutputManager->GetPromptsWaitingForNotification() == ""))//执行接地grounding
		{
			/*
			// D: type describing a concept grounding request
			// D：描述概念接受请求的类型
				#define GRS_UNPROCESSED 0		// the unprocessed status for a grounding request	未处理态			@
				#define GRS_PENDING     1		// the pending status for a grounding request		未下最终论断		@
				#define GRS_READY       2       // the ready status for a grounding request			准备态
				#define GRS_SCHEDULED   3		// the scheduled status for a grounding request		调度态			@
				#define GRS_EXECUTING   5		// the executing status for a grounding request		执行态
				#define GRS_DONE        6   	// the completed status for a grounding request		完成态
			*/

			//Turn Grouding	【不理解】:							bTurnGroundingRequest			=> HasPendingRequests
			//concept Grouding 【request阶段没有得到需要的slot】: GRS_UNPROCESSED, GRS_PENDING	=> HasPendingRequests
			//concept Grouding:									 GRS_SCHEDULED				    => HasScheduledConceptGroundingRequests
			if (pGroundingManager->HasPendingRequests() || pGroundingManager->HasScheduledConceptGroundingRequests())
			{
				//#############################################执行 Grouding####################################################
				// <10>	执行接地 【bTurnGroundingRequest=True || GRS_UNPROCESSED || GRS_PENDING || GRS_SCHEDULED】
				pGroundingManager->Run();//执行接地
				//#############################################执行 Grouding####################################################
			}

			//		now pop completed
			// <11>	删除执行完成的agent
			int iPopped = popCompletedFromExecutionStack();

			Log(DMCORE_STREAM, "Performed grounding [%s]", sTurnId.c_str());

			// now while new grounding requests appear, keep running the grounding process
			// <12>	现在当[新的接地请求]出现时，继续运行接地过程
			while ((iPopped > 0) && pGroundingManager->HasUnprocessedConceptGroundingRequests())	// D：确定是否存在未处理的概念接收请求
			{
				// run it
				// 继续运行接地操作【删除完成执行的agent,可能导致新的接地请求？】
				pGroundingManager->Run();
				// eliminate all the agents that have completed (potentially as a 
				// result of the grounding phase) from the execution stack
				iPopped = popCompletedFromExecutionStack();
			}

			Log(DMCORE_STREAM, "Completed grounding on [%s]", sTurnId.c_str());
		}//执行接地grounding

		//#############################################################################################################
		
		//		now, run the focus analysis process if the core was flagged to
		//		do so, and if there are no scheduled grounding activities
		// <13>	现在，如果DMCore设置为执行焦点分析过程，并且没有调度的grouding Action，则运行焦点分析[Foucs Analysis]过程
		if (bFocusClaimsPhaseFlag)// ===>>  通过Triiger触发的Agent会把bFocusClaimsPhaseFlag置成True??  【AcquireNextEvent中修改过】
		{
			//		Analyze the need for a focus shift, and resolve it if necessary
			// <14>	分析是否需要focus shift，如果需要则解决
			if (assembleFocusClaims())
			{
				// <15>	把焦点声明agent放到栈顶
				resolveFocusShift(); //把焦点声明agent放到栈顶
			}
			//		reset the flag
			// <16>	重置标志Flag=False
			bFocusClaimsPhaseFlag = false;
		}
		//#############################################################################################################
		//		if the execution stack is empty now, break the loop
		// <17>	如果栈为空， 结束循环
		if (esExecutionStack.empty())
			break;
		//#############################################################################################################
		//		grab the first (executable) dialog agent from the stack
		// <18>	从堆栈中抓取第一个（可执行）对话Agent
		CDialogAgent* pdaAgentInFocus = GetAgentInFocus();

		//		check that we found a proper one (if there's nothing else to be executed, we're done)
		// <17>	检查我们找到一个合适的（如果没有其他要执行，我们退出while）
		if (!pdaAgentInFocus) break;
		//#############################################################################################################
		//	if the floor is not free (or we're still waiting for notifications), do not execute agents that require the floor, just wait for the next event
		// <18>	如果Floor不free的（或者我们还在等待通知），不要执行需要Floor的agent，只需等待下一个event
		// request 和 inform 在执行期间始终需要Floor[true]
		if (pdaAgentInFocus->RequiresFloor() &&
			!((GetFloorStatus() == fsFree) && (pOutputManager->GetPromptsWaitingForNotification() == ""))
			)
		{
			AcquireNextEvent();	// <17>	等待下一个event
			continue;			// <18>	继续下次循环
		}
		//#############################################################################################################
		// and execute it
		// 执行agent
		Log(DMCORE_STREAM, "Executing dialog agent %s [%s]", pdaAgentInFocus->GetName().c_str(), sTurnId.c_str());
		//		mark the time it was executed
		// <19>	记录实际被执行的时间
		ehExecutionHistory[esExecutionStack.front().iEHIndex].vtExecutionTimes.push_back(GetTime());
		//		execute it
		// <20>	实际执行
		//########################################实际 执行  Execute########################################################
		TDialogExecuteReturnCode dercResult = pdaAgentInFocus->Execute();
		//########################################实际 执行  Execute########################################################
		ehExecutionHistory[esExecutionStack.front().iEHIndex].bExecuted = true;//有历史项被执行
		//#############################################################################################################

		/*
		// D: definition for return codes on dialog agent execution
		// D：对话代理执行时 【返回码】的定义
		typedef enum
		{
			dercContinueExecution,		// continue the execution			<1>	继续执行
			dercYieldFloor,				// gives the floor to the user		<2>	把floor给user
			dercTakeFloor,				// takes the floor					<3>	保持floor
			dercWaitForEvent,			// waits for a real-world event		<4>	等待 [真实世界] 事件
			dercFinishDialog,			// terminate the dialog				<5>	终止对话
			dercFinishDialogAndCloseSession,// terminate the dialog
			// and sends a close session message to the hub					<6>	终止对话并向集线器发送关闭会话消息
			dercRestartDialog            // restart the dialog				<6>	重启回话
		} TDialogExecuteReturnCode;
		*/
		//################################################################################################################
		//		and now analyze the return
		// <21>	分析执行 return Code
		switch (dercResult)
		{
		case dercContinueExecution:  //[Agency, Expect, Execute]   [help, NonUnderstanding]
			// continue the execution		继续执行
			break;//执行栈下一个Agent

		case dercFinishDialog:		//[Terminate]
			// finish the dialog			终止对话
			Log(DMCORE_STREAM, "Dialog Task Execution completed. Dialog finished");
			return;

		case dercFinishDialogAndCloseSession:   //[quit, TerminateAndCloseSession, Timeout, Giveup,..]
			// tell the hub to close the session	终止对话框并向集线器发送关闭会话消息
			Log(DMCORE_STREAM, "Sending close_session to the hub");
			DMI_SendEndSession();
			// finish the dialog
			Log(DMCORE_STREAM, "Dialog Task Execution completed. Dialog finished");
			return;

		case dercRestartDialog:		//[startOver, ...]
			// call the start over routine		重启对话
			StartOver();
			break;

		case dercYieldFloor:			//[help, YieldTurn,..]

			// gives the floor to the user		把floor给user
			SetFloorStatus(fsUser);
			//#####################################################################
			// wait for the next event			等待事件
			AcquireNextEvent();
			//#####################################################################
			break;

		case dercTakeFloor:		//[Inform,  Request]  [InformHelp, nounderstanding, repeat, suspend, timeout, askrepeat, askrephrase, moveon, whatCanISay..]

			// gives the floor to the system	把floor给system
			SetFloorStatus(fsSystem);
			//#####################################################################
			// wait for the next event			等待事件
			AcquireNextEvent();
			//#####################################################################
			break;

		case dercWaitForEvent:					
			//#####################################################################
			// wait for the next event		    等待事件
			AcquireNextEvent();
			//#####################################################################
			break;
		}
	}//while (!esExecutionStack.empty())

	// tell the hub to close the session
	Log(DMCORE_STREAM, "Sending close_session to the hub");
	DMI_SendEndSession();
	Log(DMCORE_STREAM, "Dialog Task Execution completed. Dialog finished.");
}

//-----------------------------------------------------------------------------
// A: Waits for and processes the next real-world event
// A：等待并处理下一个真实世界事件
//-----------------------------------------------------------------------------
#pragma warning (disable:4127)
void CDMCoreAgent::AcquireNextEvent()//等待下一个事件：AcquireNextEvent()
{
	// <1>	等待交互事件从交互管理器到达[ 当前（未处理）事件的队列为空,等待一个事件，并添加到未处理队列 ]
	pInteractionEventManager->WaitForEvent();
	//		Unqueue event
	// <2>	从当前（未处理）事件的队列获取下一个事件[并把获取的event加入到History]
	CInteractionEvent *pieEvent = pInteractionEventManager->GetNextEvent();

	//##################################绑定 concept###########################################
	//		try and bind concepts 
	// <3>	尝试绑定concept
	TBindingsDescr bdBindings;
	bindConcepts(bdBindings);
	//##################################绑定 concept###########################################

	//		add the binding results to history
	// <4>	将绑定结果添加到历史记录
	bhBindingHistory.push_back(bdBindings);

	//		Set the bindings index on the focused agent
	// <5>	在关注的代理上设置绑定索引
	GetAgentInFocus()->SetLastBindingsIndex(bhBindingHistory.size() - 1);

	//		signal the need for a focus claims phase
	// <6>	发信号 需要一个焦点声明阶段[focus claims phase]//###################??????????
	SignalFocusClaimsPhase();

	Log(DMCORE_STREAM, "Acquired new %s event.", pieEvent->GetType().c_str());

	/*
	typedef enum
	{
		fsUnknown,		// floor owner is unknown		未知
		fsUser,			// floor owner is user			用户
		fsSystem,		// floor owner is system		系统
		fsFree,			// floor is not owned by anyone	自由
	} TFloorStatus;
	*/
	//		update the floor status if the event specifies it
	// <7>	如果事件指定了Floor Status，更新Floor状态为事件指定的
	if (pieEvent->HasProperty("[floor_status]"))
	{
		SetFloorStatus(StringToFloorStatus(pieEvent->GetStringProperty("[floor_status]")));
	}

	/*
		#define IET_DIALOG_STATE_CHANGE	"dialog_state_change"		//对话状态改变
		#define IET_USER_UTT_START	"user_utterance_start"			//用户开始
		#define IET_USER_UTT_END	"user_utterance_end"			//用户结束
		#define IET_PARTIAL_USER_UTT "partial_user_utterance"
		#define IET_SYSTEM_UTT_START	"system_utterance_start"	//系统开始
		#define IET_SYSTEM_UTT_END	"system_utterance_end"			//系统结束
		#define IET_SYSTEM_UTT_CANCELED	"system_utterance_canceled"	
		#define IET_FLOOR_OWNER_CHANGES "floor_owner_changes"
		#define IET_SESSION "session"
		#define IET_GUI "gui"										//界面
	*/
	//		Process event
	// <8>	根据不同的事件类型，处理event
	if (pieEvent->GetType() == IET_USER_UTT_START)// (1)	用户开始 user_utterance_start
	{

	}
	else if (pieEvent->GetType() == IET_USER_UTT_END || pieEvent->GetType() == IET_GUI)//(2)	用户结束 user_utterance_end
	{

		if (pieEvent->IsComplete())//event完成
		{
			// Set the last input on the focused agent
			// 设置焦点agent上的最后一个input
			GetAgentInFocus()->SetLastInputIndex(iTurnNumber);// 最后输入TurnNumber
			GetAgentInFocus()->IncrementTurnsInFocusCounter();// 表示agent自上次【重置/重新打开】以来有多少次获得焦点 iTurnsInFocusCounter

			iTurnNumber++;//TurnNumber加一

			//###############################Turn Grouding#########################################
			// signal the need for a turn grounding
			// 标志需要Turn接地  【?】
			pGroundingManager->RequestTurnGrounding();
			//###############################Turn Grouding#########################################

			Log(DMCORE_STREAM, "Processed new input [User:%s]", pieEvent->GetStringProperty("[uttid]").c_str());

			return;
		}

	}
	else if (pieEvent->GetType() == IET_SYSTEM_UTT_START)//(3)	系统开始 system_utterance_start
	{

		// sends notification information to the OutputManager
		// void COutputManagerAgent::PreliminaryNotify(int iOutputId, string sTaggedUtt)
		// 将通知信息发送到OutputManager    [Preliminary:初步]
		// 标记的输出话语 =>  sTaggedUtt 
		pOutputManager->PreliminaryNotify(
			pieEvent->GetIntProperty("[utt_count]"),
			pieEvent->GetStringProperty("[tagged_prompt]"));

		Log(DMCORE_STREAM, "Processed preliminary output notification.");
	}
	else if (pieEvent->GetType() == IET_SYSTEM_UTT_END)// (4)	系统结束 system_utterance_end
	{

		// sends notification information to the OutputManager
		// 将通知信息发送到OutputManager
		pOutputManager->Notify(pieEvent->GetIntProperty("[utt_count]"),
			pieEvent->GetIntProperty("[bargein_pos]"),
			pieEvent->GetStringProperty("[conveyance]"),
			pieEvent->GetStringProperty("[tagged_prompt]"));

		Log(DMCORE_STREAM, "Processed output notification.");
	}
	else if (pieEvent->GetType() == IET_SYSTEM_UTT_CANCELED)// (5)	系统结束 system_utterance_canceled
	{

		// sends notification information to the OutputManager
		//将通知信息发送到OutputManager
		pOutputManager->Notify(pieEvent->GetIntProperty("[utt_count]"), 0, "", "");

		Log(DMCORE_STREAM, "Output cancel notification processed.");
	}
	else if (pieEvent->GetType() == IET_DIALOG_STATE_CHANGE)// (6)	对话状态改变 dialog_state_change
	{

		pStateManager->UpdateState();		//更新状态 [执行Stack改变]
		pStateManager->BroadcastState();	//将状态广播到系统中的其他组件

	}
	else
	{
		//#define IET_PARTIAL_USER_UTT "partial_user_utterance"  部分用户说话
		//#define IET_FLOOR_OWNER_CHANGES "floor_owner_changes"  Floor所有者改变
		//#define IET_SESSION "session"							 会话session
	}
}//等待下一个事件：AcquireNextEvent()




#pragma warning (default:4127)

// J:	Updates the speech recognizer's configuration to the one
//		specified by the agent on the top of the stack.
//		For instance, the hash [(set_lm=my_lm),(set_dtmf_len=4)] will be 
//		converted into a GalFrame [..., :set_lm=my_lm, :set_dtmf_len=4, ...]
void CDMCoreAgent::updateInputLineConfiguration()
{
	// grab the first dialog agent from the stack
	CDialogAgent* pdaCurrent = esExecutionStack.front().pdaAgent;

	// obtain the hash describing the entire configuration
	STRING2STRING s2sConfig = pdaCurrent->GetInputLineConfiguration();

	// log it
	Log(DMCORE_STREAM, "Input line configuration dumped below.\n%s",
		S2SHashToString(s2sConfig, "\n").c_str());

	if (s2sConfig.size() > 0)
	{

		// if we are in a Galaxy configuration, send requests through the Galaxy 
		// interface
#ifdef GALAXY	
		TGIGalaxyCall gcGalaxyCall;
		gcGalaxyCall.sModuleFunction = "sphinx.set_config";
		gcGalaxyCall.bBlockingCall = false;     
		if (s2sConfig.size() > 0)
		{
			STRING2STRING::iterator iPtr;
			for (iPtr = s2sConfig.begin();
				iPtr != s2sConfig.end();
				iPtr++)
			{

				gcGalaxyCall.s2sInputs.insert(STRING2STRING::value_type(":" + iPtr->first,
					iPtr->second));
			}
		}
		// retrieve the current thread id
		DWORD dwThreadId = GetCurrentThreadId();

		// send the message to the Galaxy Interface Thread
		PostThreadMessage(g_idDMInterfaceThread, WM_GALAXYCALL,
			(WPARAM)&gcGalaxyCall, dwThreadId);

		// and wait for a reply
		MSG Message;
		GetMessage(&Message, NULL, WM_ACTIONFINISHED, WM_ACTIONFINISHED);
#endif // GALAXY

	}
}

// D: Registers a customized binding filter
void CDMCoreAgent::RegisterBindingFilter(string sBindingFilterName,
	TBindingFilterFunct bffFilter)
{
	// check if it's already in the map
	if (s2bffFilters.find(sBindingFilterName) != s2bffFilters.end())
	{
		Warning(FormatString("Could not add binding filter %s, as it already "\
			"exists.", sBindingFilterName.c_str()));
	}
	else
	{
		// add it if not already in the map
		s2bffFilters.insert(
			STRING2BFF::value_type(sBindingFilterName, bffFilter));
	}
}

//---------------------------------------------------------------------
// A: Methods for accessing Interface-specific variables
//---------------------------------------------------------------------

int CDMCoreAgent::GetIntSessionID()
{
	return DMI_GetIntSessionID();
}

//-----------------------------------------------------------------------------
// D: Expectation Agenda functions
//-----------------------------------------------------------------------------

// D: collects and logs the concepts that undergo grounding
void CDMCoreAgent::dumpConcepts()
{

	// the string in which we construct the concepts dump
	string sConceptsDump;

	// first collect the list of concepts that undergo grounding 
	TConceptPointersVector cpvConcepts;
	// use a set to record the seen grounding models and avoid duplicates
	TConceptPointersSet cpsExcludeConcepts;
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin();
		iPtr != esExecutionStack.end();
		iPtr++)
	{
		// gather the concepts from each element on the stack               
		iPtr->pdaAgent->DeclareConcepts(cpvConcepts, cpsExcludeConcepts);
	}

	// now go through the list of concept, and for all the ones that have a 
	// grounding model, log them
	for (unsigned int i = 0; i < cpvConcepts.size(); i++)
	{
		if (cpvConcepts[i]->GetGroundingModel())
		{
			sConceptsDump += FormatString(
				"Concept: %s\n%s",
				cpvConcepts[i]->GetAgentQualifiedName().c_str(),
				cpvConcepts[i]->HypSetToString().c_str());
		}
	}

	// now log the concepts
	Log(CONCEPT_STREAM, "Grounding-relevant concepts dumped below:\n%s",
		sConceptsDump.c_str());
}


// D: dumps the execution stack to the log
void CDMCoreAgent::dumpExecutionStack()
{
	Log(DMCORE_STREAM, "Execution stack dumped below:\n%s",
		executionStackToString().c_str());
}

// A: converts the current execution stack into a string representation
string CDMCoreAgent::executionStackToString()
{
	return executionStackToString(esExecutionStack);
}

// D: converts the given execution stack into a string representation
string CDMCoreAgent::executionStackToString(TExecutionStack es)
{
	string sResult;

	// iterate through the execution stack
	TExecutionStack::iterator iPtr;
	for (iPtr = es.begin();
		iPtr != es.end();
		iPtr++)
	{
		sResult += FormatString("  %s\n", iPtr->pdaAgent->GetName().c_str());
	}

	// finally, return the string
	return sResult;
}

// D: this method clears the current system action
void CDMCoreAgent::clearCurrentSystemAction()
{
	saSystemAction.setcpRequests.clear();
	saSystemAction.setcpExplicitConfirms.clear();
	saSystemAction.setcpImplicitConfirms.clear();
	saSystemAction.setcpUnplannedImplicitConfirms.clear();
}

// D: this method computes the current system action
void CDMCoreAgent::computeCurrentSystemAction()
{

	// now go through the levels of the agenda, and look for implicit 
	// confirms, explicit confirms and requests
	bool bFoundFocus = false;
	for (unsigned int l = 0;
		!bFoundFocus && (l < eaAgenda.vCompiledExpectations.size());
		l++)
	{

		// set that we have found the focus
		bFoundFocus = true;

		TMapCE::iterator iPtr;
		// iterate through the compiled expectations from that level
		for (iPtr = eaAgenda.vCompiledExpectations[l].mapCE.begin();
			iPtr != eaAgenda.vCompiledExpectations[l].mapCE.end();
			iPtr++)
		{

			string sSlotExpected = iPtr->first;
			TIntVector& rvIndices = iPtr->second;

			// go through the indices
			for (unsigned int i = 0; i < rvIndices.size(); i++)
			{

				// grab the expectation
				TConceptExpectation& rceExpectation =
					eaAgenda.celSystemExpectations[rvIndices[i]];

				// find the concept name
				CConcept* pConcept = &(rceExpectation.pDialogAgent->
					C(rceExpectation.sConceptName));

				// find the agent name
				string sAgentName = rceExpectation.pDialogAgent->GetName();

				// now check if the agent is an implicit confirmation
				if (sAgentName.find("_ImplicitConfirmExpect") != -1)
				{
					// find the name of the confirmed concept
					string sFoo, sConfirmedConcept;
					SplitOnFirst(sAgentName, "[", sFoo, sConfirmedConcept);
					sConfirmedConcept = TrimRight(sConfirmedConcept, "]");

					// signal that we are implicitly confirming that concept
					SignalImplicitConfirmOnConcept(
						&(pDTTManager->GetDialogTaskTreeRoot()->C(sConfirmedConcept)));

					// then we have an implicit confirmation on top of the 
					// stack, mark that this was actually not the focus level
					bFoundFocus = false;
					// then continue
					continue;
				}

				// now check if the agent is an explicit confirmation
				if (sAgentName.find("_ExplicitConfirm") != -1)
				{
					// find the name of the confirmed concept
					string sFoo, sConfirmedConcept;
					SplitOnFirst(sAgentName, "[", sFoo, sConfirmedConcept);
					sConfirmedConcept = TrimRight(sConfirmedConcept, "]");

					// signal that we are implicitly confirming that concept
					SignalExplicitConfirmOnConcept(
						&(pDTTManager->GetDialogTaskTreeRoot()->C(sConfirmedConcept)));
				}

				// if the expectation is open 
				if (!rceExpectation.bDisabled)
				{
					// then mark this concept as requested
					saSystemAction.setcpRequests.insert(pConcept);
					// and also go through the other concepts in the expectation and 
					// mark them as requested
					for (unsigned int i = 0;
						i < rceExpectation.vsOtherConceptNames.size(); i++)
					{
						saSystemAction.setcpRequests.insert(&(rceExpectation.pDialogAgent->
							C(rceExpectation.vsOtherConceptNames[i])));
					}
				}
			}
		}
	}

	// now log the current system action 
	Log(DMCORE_STREAM, FormatString("System action dumped below.\n%s",
		currentSystemActionToString().c_str()));

}

// D: this method creates a string representation of the system action   
string CDMCoreAgent::currentSystemActionToString()
{
	return systemActionToString(saSystemAction);
}

// D: this method creates a string representation of the system action   
string CDMCoreAgent::systemActionToString(TSystemAction saASystemAction)
{

	// now log the current system action
	string sResult = "REQUEST(";
	set<CConcept *>::iterator iPtr;
	for (iPtr = saASystemAction.setcpRequests.begin();
		iPtr != saASystemAction.setcpRequests.end();
		iPtr++)
	{
		sResult += FormatString("%s,", (*iPtr)->GetAgentQualifiedName().c_str());
	}
	sResult = TrimRight(sResult, ",");
	sResult += ")\nEXPL_CONF(";
	for (iPtr = saASystemAction.setcpExplicitConfirms.begin();
		iPtr != saASystemAction.setcpExplicitConfirms.end();
		iPtr++)
	{
		sResult += FormatString("%s,", (*iPtr)->GetAgentQualifiedName().c_str());
	}
	sResult = TrimRight(sResult, ",");
	sResult += ")\nIMPL_CONF(";
	for (iPtr = saASystemAction.setcpImplicitConfirms.begin();
		iPtr != saASystemAction.setcpImplicitConfirms.end();
		iPtr++)
	{
		sResult += FormatString("%s,", (*iPtr)->GetAgentQualifiedName().c_str());
	}
	sResult = TrimRight(sResult, ",");
	sResult += ")\nUNPLANNED_IMPL_CONF(";
	for (iPtr = saASystemAction.setcpUnplannedImplicitConfirms.begin();
		iPtr != saASystemAction.setcpUnplannedImplicitConfirms.end();
		iPtr++)
	{
		sResult += FormatString("%s,", (*iPtr)->GetAgentQualifiedName().c_str());
	}
	sResult = TrimRight(sResult, ",");
	sResult += ")";

	return sResult;
}

// D: assembles the expectation agenda
// 组装期望agenda
void CDMCoreAgent::assembleExpectationAgenda()
{

	Log(DMCORE_STREAM, "Expectation Agenda Assembly Phase initiated.");

	//		first collect and compile the expectation agenda
	// <1>	首先收集和编译期望Agenda
	compileExpectationAgenda();

	//		then enforce the binding policies as specified on each level
	// <2>	然后强制执行每个level上指定的绑定策略
	enforceBindingPolicies();

	//		dump agenda to the log
	// <3>	log记录agenda
	Log(EXPECTATIONAGENDA_STREAM, "Concept expectation agenda dumped below:"
		+ expectationAgendaToString());

	Log(DMCORE_STREAM, "Expectation Agenda Assembly Phase completed "\
		"(%d levels).", eaAgenda.vCompiledExpectations.size());
}

// D: gathers the expectations and compiles them in an fast accessible form
// D: definition of an internal type: a set of pointers to an agent

//	D：收集期望, 并组织成快速访问的结构
//	D：内部类型的定义：一组指向代理的指针
typedef set<CDialogAgent*, less <CDialogAgent*>, allocator <CDialogAgent*> >
TDialogAgentSet;

void CDMCoreAgent::compileExpectationAgenda()
{

	// log the activity
	Log(DMCORE_STREAM, "Compiling Expectation Agenda ...");

	//		first clear up the last agenda
	// <1>	清空上次的agenda
	eaAgenda.celSystemExpectations.clear();
	eaAgenda.vCompiledExpectations.clear();

	// get the list of system expectations. To do this, we traverse 
	// the execution stack, and add expectations from all the agents, each 
	// on the appropriate level; also keep track of the expectations
	// that are already declared so as not to duplicate them by this
	// traversal
	// 获得系统期望的列表。 为了做到这一点，我们遍历执行堆栈，
	// 并添加所有的代理的期望，每个在适当的水平; 也跟踪已经声明的期望，以便不被这个遍历重复
	int iLevel = 0;
	TDialogAgentSet setPreviouslySeenAgents;	// the set of agents already	前一层次可见agent
	// seen on the previous levels
	TDialogAgentSet setCurrentlySeenAgents;		// the set of agents seen		当前层次可见的agent
	// on the current level

	// <2>	遍历执行堆栈
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin(); iPtr != esExecutionStack.end(); iPtr++)//遍历栈
	{

		//		remember how big the system expectation agenda was so far
		// <3>	记录当前Agenda的大小size
		int iStartIndex = eaAgenda.celSystemExpectations.size();


		//################### DeclareExpectations ##############################
		//		gather expectations of the agent on the stack indicated by iPtr
		// <4>	收集当前指针指向的代理的期望 [会递归收集当前agent的所有 subagent]
		iPtr->pdaAgent->DeclareExpectations(eaAgenda.celSystemExpectations);// => 重载 子类实现
		//################### DeclareExpectations ##############################


		//		now go thourgh those new expectations and compile them (create 
		//		the corresponding entry into the vCompiledExpectations array)
		// <5>	现在遍历这些新的期望并编译它们（在vCompiledExpectations数组中创建相应的条目）
		//		保持不同level的期望的数组（在索引0上，是focus agent的期望，在索引1上的直接上层agent的期望等）
		TCompiledExpectationLevel celLevel;
		// set the agent that generated this level
		celLevel.pdaGenerator = iPtr->pdaAgent;
		// <6>	遍历当前agent添加的expection
		for (unsigned int i = iStartIndex; i < eaAgenda.celSystemExpectations.size(); i++)//遍历当前agent添加的expection
		{

			//		check that the agent was not already seen on the previous
			//		level (in this case, avoid duplicating its expectation)
			// <7>	检查代理在上一级别上是否已经看到（在这种情况下，避免重复其期望）
			if (setPreviouslySeenAgents.find(eaAgenda.celSystemExpectations[i].pDialogAgent) != setPreviouslySeenAgents.end())
			{
				continue;
			}

			//		insert this agent in the list of currently seen agents
			// <8>	在当前查看的代理列表中插入此代理
			setCurrentlySeenAgents.insert(eaAgenda.celSystemExpectations[i].pDialogAgent);

			string sSlotExpected = eaAgenda.celSystemExpectations[i].sGrammarExpectation;//slotName 带着【】？

			TMapCE::iterator iPtr2;
			// <9>	当前层celLevel有slot Name
			if ((iPtr2 = celLevel.mapCE.find(sSlotExpected)) != celLevel.mapCE.end())
			{
				// if this grammar slot is already expected at this level
				// just add to the vector of pointers
				// <10>		如果这个语法槽已经预期在这个级别, 只是添加到index的向量
				TIntVector& rvIndices = (*iPtr2).second;
				rvIndices.push_back(i);
			}
			else
			{
				//		if the concept is NOT already expected at this level
				//		then add it to the hash of compiled expectations
				// <11>	如果该概念尚未在此level，则将其添加到编译期望的哈希
				TIntVector ivTemp;
				ivTemp.push_back(i);
				celLevel.mapCE.insert(TMapCE::value_type(sSlotExpected, ivTemp));
			}
		}//遍历当前agent添加的expection

		//		finally, we have assembled and compiled this level of expectations,
		//		push it on the array, 
		// <12>	最后，我们组装并编译了这个level的期望，推送到array上，
		eaAgenda.vCompiledExpectations.push_back(celLevel);

		//		update the set of already seen agents
		// <13>	更新的代理集 = 防止重复填充
		setPreviouslySeenAgents.insert(setCurrentlySeenAgents.begin(), setCurrentlySeenAgents.end());

		// <14>	and move to the next level
		iLevel++;
	}//遍历栈

	// log the activity
	Log(DMCORE_STREAM, "Compiling Expectation Agenda completed.");
}

// D: goes through the compiled agenda, and modifies it according to the 
//    binding policies as specified by each level's generator agents
// D：遍历编译的议程，并根据每个level的生成器代理指定的绑定策略修改它
void CDMCoreAgent::enforceBindingPolicies()
{

	// log the activity
	Log(DMCORE_STREAM, "Enforcing binding policies ...");

	// at this point, this only consists of blocking the upper levels if a 
	// WITHIN_TOPIC_ONLY policy is detected
	// 在这一点上，如果检测到WITHIN_TOPIC_ONLY策略，这只包括阻塞上层
	for (unsigned int i = 0; i < eaAgenda.vCompiledExpectations.size(); i++)//遍历每层【每个agent】
	{
		// get the binding policy for this level
		/*
		D: Topic-initiative: bind concepts only within the current topic / focus
		Topic-initiative：仅在当前 topic/focus 内绑定概念
		#define WITHIN_TOPIC_ONLY "bind-this-only"

		D: Mixed-initiative: bind anything
		#define MIXED_INITIATIVE "bind-anything"
		*/
		string sBindingPolicy = eaAgenda.vCompiledExpectations[i].pdaGenerator->DeclareBindingPolicy();
		if (sBindingPolicy == WITHIN_TOPIC_ONLY)//"WITHIN_TOPIC_ONLY"
		{
			// if WITHIN_TOPIC_ONLY, then all the expectations from the upper
			// levels of the agenda are disabled
			// 如果WITHIN_TOPIC_ONLY，那么upper level议程的所有期望都被禁用
			// 所有比当前level大的都设置为disable
			for (unsigned int l = i + 1; l < eaAgenda.vCompiledExpectations.size(); l++)//upper level
			{
				// go through the whole level and disable all expectations
				// 遍历所有的level, disable期望
				TMapCE::iterator iPtr;
				for (iPtr = eaAgenda.vCompiledExpectations[l].mapCE.begin();
					iPtr != eaAgenda.vCompiledExpectations[l].mapCE.end();
					iPtr++)
				{
					// access the indices 
					TIntVector& rivTemp = iPtr->second;
					for (unsigned int ii = 0; ii < rivTemp.size(); ii++)
					{
						int iIndex = rivTemp[ii];
						// don't disable it if it's a *-type expectation
						if (eaAgenda.celSystemExpectations[iIndex].sExpectationType != "*")
						{
							eaAgenda.celSystemExpectations[iIndex].bDisabled = true;
							eaAgenda.celSystemExpectations[iIndex].sReasonDisabled = "within-topic binding policy";
							// *** add on what
						}
					}
				}
			}//upper level
			// break the for loop since it already doesn't matter what the 
			// policy is on upper contexts
			// 跳出循环，因为已经不关心上层的policy是什么了 [因为他是从找到的第一个WITHIN_TOPIC_ONLY，
			// 到最顶层的所有的concept都置为了disable, 而且agenda是每次更新栈的时候都重新创建的。]
			break;
		}//"WITHIN_TOPIC_ONLY"
	}//遍历每层【每个agent】

	// log the activity
	Log(DMCORE_STREAM, "Enforcing binding policies completed.");
}

// D: generates a string representation of the expectation agenda
//    this string is used so far only for logging purposes
// D：生成期望日程的字符串表示,
//    此字符串到目前为止仅用于日志目的
string CDMCoreAgent::expectationAgendaToString()
{
	string sResult;
	// go through all the levels of the agenda
	for (unsigned int l = 0; l < eaAgenda.vCompiledExpectations.size(); l++)
	{
		sResult += FormatString("\n Level %d: generated by %s", l,
			eaAgenda.vCompiledExpectations[l].pdaGenerator->GetName().c_str());
		TMapCE::iterator iPtr;
		// iterate through the compiled expectations from that level
		for (iPtr = eaAgenda.vCompiledExpectations[l].mapCE.begin();
			iPtr != eaAgenda.vCompiledExpectations[l].mapCE.end();
			iPtr++)
		{
			string sSlotExpected = iPtr->first;
			TIntVector& rvIndices = iPtr->second;
			// convert expectations to string description
			for (unsigned int i = 0; i < rvIndices.size(); i++)
			{
				TConceptExpectation& rceExpectation =
					eaAgenda.celSystemExpectations[rvIndices[i]];
				sResult += (rceExpectation.bDisabled) ? "\n  X " : "\n  O ";
				sResult += rceExpectation.sGrammarExpectation + " -> (" +
					rceExpectation.pDialogAgent->GetName() + ")" +
					rceExpectation.sConceptName;
				if (rceExpectation.bDisabled)
				{
					sResult += " [X-" + rceExpectation.sReasonDisabled + "]";
				}
			}
		}
	}
	// finally, return the string
	return sResult;
}

// A: generates a string representation of the current agenda
string CDMCoreAgent::expectationAgendaToBroadcastString()
{
	return expectationAgendaToBroadcastString(eaAgenda);
}

// D: generates a string representation of the expectation agenda
//    that is used to broadcast it to the outside world
string CDMCoreAgent::expectationAgendaToBroadcastString(TExpectationAgenda eaBAgenda)
{
	string sResult;
	STRING2STRING s2sAllOpenGrammarExpectations;
	// go through all the levels of the agenda
	for (unsigned int l = 0; l < eaBAgenda.vCompiledExpectations.size(); l++)
	{
		sResult += FormatString("\n%d:", l);
		TMapCE::iterator iPtr;
		// iterate through the compiled expectations from that level
		for (iPtr = eaBAgenda.vCompiledExpectations[l].mapCE.begin();
			iPtr != eaBAgenda.vCompiledExpectations[l].mapCE.end();
			iPtr++)
		{
			string sSlotExpected = iPtr->first;
			TIntVector& rvIndices = iPtr->second;

			TIntVector vOpenIndices;
			set<CConcept *> scpOpenConcepts;
			TIntVector vClosedIndices;
			set<CConcept *> scpClosedConcepts;

			for (unsigned int i = 0; i < rvIndices.size(); i++)
			{
				TConceptExpectation& rceExpectation =
					eaBAgenda.celSystemExpectations[rvIndices[i]];
				// determine the concept under consideration
				CConcept* pConcept =
					&(rceExpectation.pDialogAgent->C(
					rceExpectation.sConceptName));

				// test that the expectation is not disabled
				if (!eaBAgenda.celSystemExpectations[rvIndices[i]].bDisabled)
				{
					if (scpOpenConcepts.find(pConcept) == scpOpenConcepts.end())
					{
						// add it to the open indices list
						vOpenIndices.push_back(rvIndices[i]);
						// add the concept to the open concepts list
						scpOpenConcepts.insert(pConcept);
						// if by any chance it's already in the closed concepts, 
						set<CConcept *>::iterator iPtr;
						if ((iPtr = scpClosedConcepts.find(pConcept)) !=
							scpClosedConcepts.end())
						{
							// remove it from there
							scpClosedConcepts.erase(iPtr);
						}
					}
				}
				else
				{
					// o/w if the expectation is disabled
					if ((scpClosedConcepts.find(pConcept) == scpClosedConcepts.end()) &&
						(scpOpenConcepts.find(pConcept) == scpOpenConcepts.end()))
					{
						// add it to the closed indices list
						vClosedIndices.push_back(rvIndices[i]);
						// add the concept to the closed concepts list
						scpClosedConcepts.insert(pConcept);
					}
				}
			}

			// now add the first one in the open indices, if there is any
			// in there

			if (vOpenIndices.size() > 0)
			{
				TConceptExpectation& rceExpectation =
					eaBAgenda.celSystemExpectations[vOpenIndices[0]];
				sResult += "\n"; // (air)
				sResult += "O" + rceExpectation.sGrammarExpectation;

				sResult +=
					(rceExpectation.bmBindMethod == bmExplicitValue) ? "V," : "S,";
			}

			// finally, add all the blocked ones
			for (unsigned int i = 0; i < vClosedIndices.size(); i++)
			{
				TConceptExpectation& rceExpectation =
					eaBAgenda.celSystemExpectations[vClosedIndices[i]];
				sResult += "\n"; // (air)
				sResult += "X" + rceExpectation.sGrammarExpectation;
				sResult +=
					(rceExpectation.bmBindMethod == bmExplicitValue) ? "V," : "S,";
			}
		}
		// cut the last comma
		sResult = TrimRight(sResult, ",");
	}
	// finally, return the string
	return Trim(sResult, "\n");
}

// D: generates a string representation of the bindings description
// D：生成绑定描述的字符串表示
string CDMCoreAgent::bindingsDescrToString(TBindingsDescr& rbdBindings)
{
	string sResult;
	// go through all the attempted bindings
	for (unsigned int i = 0; i < rbdBindings.vbBindings.size(); i++)
	{
		if (rbdBindings.vbBindings[i].bBlocked)
			sResult += FormatString("Fail:    Level.%d\t%s->(%s)%s [%s]\n",
			rbdBindings.vbBindings[i].iLevel,
			rbdBindings.vbBindings[i].sGrammarExpectation.c_str(),
			rbdBindings.vbBindings[i].sAgentName.c_str(),
			rbdBindings.vbBindings[i].sConceptName.c_str(),
			rbdBindings.vbBindings[i].sReasonDisabled.c_str());
		else
			sResult += FormatString("Success: Level.%d\t%s(%s%s%f)->(%s)%s\n",
			rbdBindings.vbBindings[i].iLevel,
			rbdBindings.vbBindings[i].sGrammarExpectation.c_str(),
			rbdBindings.vbBindings[i].sValue.c_str(),
			VAL_CONF_SEPARATOR,
			rbdBindings.vbBindings[i].fConfidence,
			rbdBindings.vbBindings[i].sAgentName.c_str(),
			rbdBindings.vbBindings[i].sConceptName.c_str());
	}
	// go through all the forced updates
	for (unsigned int i = 0; i < rbdBindings.vfcuForcedUpdates.size(); i++)
	{
		if (rbdBindings.vfcuForcedUpdates[i].iType == FCU_EXPLICIT_CONFIRM)
			sResult += FormatString("Forced update [explicit_confirm] on %s: %s",
			rbdBindings.vfcuForcedUpdates[i].sConceptName.c_str(),
			rbdBindings.vfcuForcedUpdates[i].bUnderstanding ?
			"understanding" : "non-understanding");
	}

	// finally, return
	return sResult;
}

// D: binds the concepts from the input parse into the agenda according to the
//    current interaction policy, then returns a charaterization of binding
//    success/failure in bhiResults
// D：根据当前交互策略将输入解析的概念绑定到议程，
//    然后在bhiResults中返回绑定成功/失败的特征
void CDMCoreAgent::bindConcepts(TBindingsDescr& rbdBindings)
{
	/*
	typedef struct
	{
		string sEventType;				// the type of event 						event类型 "IET_USER_UTT_START", ...
		bool bNonUnderstanding;         // was the turn a non-understanding?		是否no-understanding [例如：没有concept绑定]
		int iConceptsBound;             // the number of bound concepts				绑定的concept数目
		int iConceptsBlocked;           // the number of blocked concepts			阻塞Concept数
		int iSlotsMatched;				// the number of slots that matched			匹配的slot数
		int iSlotsBlocked;				// the number of slots that were blocked	阻塞的slot数


		vector<TBinding> vbBindings;	// the vector of bindings					绑定vector
		vector<TForcedConceptUpdate> vfcuForcedUpdates;								强制更新的向量
		// the vector of forced updates
	} TBindingsDescr;
	*/

	Log(DMCORE_STREAM, "Concepts Binding Phase initiated.");

	//		initialize to zero the number of concepts bound and blocked
	// <1>	将绑定和阻塞的概念数初始化为零
	rbdBindings.sEventType = pInteractionEventManager->GetLastEvent()->GetType(); //event类型 "IET_USER_UTT_START", ...
	rbdBindings.iConceptsBlocked = 0;
	rbdBindings.iConceptsBound = 0;
	rbdBindings.iSlotsMatched = 0;
	rbdBindings.iSlotsBlocked = 0;

	// hash which stores the slots that matched and how many times they did so
	// hash存储匹配的slot和相应的次数
	map<string, int> msiSlotsMatched;

	// hash which stores the slots that were blocked and how many times they were blocked
	// hash存储阻塞的slot和它们的次数
	map<string, int> msiSlotsBlocked;

	//		go through each concept expectation level and try to bind things
	// <2>	遍历每个概念期望level并尝试绑定
	for (unsigned int iLevel = 0; iLevel < eaAgenda.vCompiledExpectations.size(); iLevel++)//每层：vCompiledExpectations
	{

		/*
		typedef struct
		{
			TMapCE mapCE;					// the hash of compiled expectations	编译期望的哈希
			CDialogAgent* pdaGenerator;		// the agent that represents that level	代表当前level期望的代理
		} TCompiledExpectationLevel;
		*/
		//		go through the hash of expected slots at that level
		// <3>	遍历该level的预期slot的散列
		TMapCE::iterator iPtr;
		for (iPtr = eaAgenda.vCompiledExpectations[iLevel].mapCE.begin();
			iPtr != eaAgenda.vCompiledExpectations[iLevel].mapCE.end();
			iPtr++)
		{
			//   slotName -> slotIndex[Vecor<int>]
			string sSlotExpected = iPtr->first;	    // the grammar slot expected
			TIntVector& rvIndices = iPtr->second;	// indices in the system expectation list

			//		if the slot actually exists in the parse, then try to bind it
			// <4>	如果slot实际上存在于input解析中，则尝试绑定它
			//######################################################################################################################
			if (pInteractionEventManager->LastEventMatches(sSlotExpected))//最后event属性匹配了当前的 sSlotExpected[slotName]
			{

				Log(DMCORE_STREAM, "Event matches %s.", sSlotExpected.c_str());

				// go through the array of indices and construct another array
				// which contains only the indices of "open" expectations, 
				// excluding any expectations that redundanly match to the same concept
				//遍历索引数组并构造另一个仅包含“open”期望的索引的数组，不包括任何与相同概念冗余匹配的期望
				TIntVector vOpenIndices;
				set<CConcept *> scpOpenConcepts;

				// also construct another array which contains the indices of 
				// "closed" expectations, excluding any expectations that 
				// redundantly match the same concept
				// 也构造另一个数组，其包含“closed”期望的索引，排除任何期望冗余地匹配相同的概念
				TIntVector vClosedIndices;
				set<CConcept *> scpClosedConcepts;

				// <5>	遍历当前slot的index vector
				for (unsigned int i = 0; i < rvIndices.size(); i++)//只一个？？【slot为单位？】
				{
					//		determine the concept under consideration
					// <6>	获取slot所绑定的的concept
					CConcept* pConcept =
						&(eaAgenda.celSystemExpectations[rvIndices[i]].pDialogAgent->C(eaAgenda.celSystemExpectations[rvIndices[i]].sConceptName));

					// test that the expectation is not disabled
					// <7>	测试期望未禁用 - 可用
					if (!eaAgenda.celSystemExpectations[rvIndices[i]].bDisabled)
					{
						if (scpOpenConcepts.find(pConcept) == scpOpenConcepts.end())
						{
							// add it to the open indices list
							//将其添加到open的索引列表
							vOpenIndices.push_back(rvIndices[i]);
							// add the concept to the open concepts list
							//将概念添加到open的概念列表中
							scpOpenConcepts.insert(pConcept);
							// if by any chance it's already in the closed concepts, 
							// 如果任何机会它已经在close的概念，
							set<CConcept *>::iterator iPtr;
							if ((iPtr = scpClosedConcepts.find(pConcept)) != scpClosedConcepts.end())
							{
								// remove it from there
								// 移除
								scpClosedConcepts.erase(iPtr);
							}
						}
					}//可用
					else// <8>	不可用
					{
						// o/w if the expectation is disabled
						// o / w如果期望被禁用
						if ((scpClosedConcepts.find(pConcept) == scpClosedConcepts.end()) &&
							(scpOpenConcepts.find(pConcept) == scpOpenConcepts.end()))
						{
							// add it to the closed indices list
							// 将其添加到封闭索引列表中
							vClosedIndices.push_back(rvIndices[i]);
							// add the concept to the closed concepts list
							scpClosedConcepts.insert(pConcept);
						}
					}//不可用
				}//for (unsigned int i = 0; i < rvIndices.size(); i++)//只一个？？【slot为单位？】

				// the slot value to be bound
				// 要绑定的槽值
				string sSlotValue;

				//		and the confidence score
				// <9>	event置信度得分 [默认设置为 1.0f]
				float fConfidence = pInteractionEventManager->GetLastEventConfidence();

				// <10>	如果匹配到可用concept
				if (vOpenIndices.size() > 0)//如果匹配到可用concept
				{
					//		check that the confidence is strictly above the current nonunderstanding threshold
					// <11>	检查置信度是否严格高于当前的不可忽略阈值 => event的confidence大于fNonunderstandingThreshold
					if (fConfidence > fNonunderstandingThreshold)//event的confidence大于fNonunderstandingThreshold
					{
						//		check for multiple bindings on a level
						// <12>	如果可以binding多个
						if (vOpenIndices.size() > 1)
						{
							// if there are multiple bindings possible, log that 
							// as a warning for now *** later we need to deal with 
							// this by adding disambiguation agencies
							//如果有多个绑定可能，记录作为警告现在***后，我们需要通过添加消歧agencies
							string sAgents;
							for (unsigned int i = 0; i < vOpenIndices.size(); i++)
							{
								sAgents +=
									eaAgenda.celSystemExpectations[vOpenIndices[i]].\
									pDialogAgent->GetName() +
									" tries to bind to " +
									eaAgenda.celSystemExpectations[vOpenIndices[i]].\
									sConceptName +
									"\n";
							}
							Warning(FormatString("Multiple binding for grammar "\
								"concept %s. Agents dumped below. Binding "\
								"performed just for the first agent.\n%s",
								sSlotExpected.c_str(), sAgents.c_str()));
						}/// 如果可以binding多个 => 只绑定到第一个agent

						//		now bind the grammar concept to the first agent expecting this slot; obtain the value for that grammar slot
						// <13>	现在将语法概念绑定到期望此slot的第一个agent; 
						//		从event中获取该slot的值
						sSlotValue = pInteractionEventManager->GetValueForExpectation(sSlotExpected);//sSlotExpected[slotName]

						//####################################执行 binding###################################################
						//		do the actual concept binding
						// <14>	做实际的概念绑定 [绑定到期望此slot的第一个agent的concept]
						performConceptBinding(
							sSlotExpected,
							sSlotValue,
							fConfidence,
							vOpenIndices[0],
							pInteractionEventManager->LastEventIsComplete());
						//####################################执行 bingding###################################################

						//		now that we've bound at this level, invalidate this expected slot on all the other levels
						// <15>	现在我们已经绑定在这个级别，使所有其他级别的此预期槽slot失效
						for (unsigned int iOtherLevel = iLevel + 1; iOtherLevel < eaAgenda.vCompiledExpectations.size(); iOtherLevel++)
						{
							eaAgenda.vCompiledExpectations[iOtherLevel].mapCE.erase(sSlotExpected);// Agenda格式？？？？？？？？？
						}
					}//if(fConfidence > fNonunderstandingThreshold)//event的confidence大于fNonunderstandingThreshold
					else
					{
						// o/w the confidence is below the nonunderstanding
						// threshold, so we will reject this utterance by
						// basically not binding anything
						// <16>	信心低于非不理解阈值，因此我们将通过基本上不绑定任何东西来拒绝这个话语
					}
				}//if (vOpenIndices.size() > 0)//如果匹配到可用concept

				//		write the open binding description (only for the first concept, the one that actually bound)
				// <17>	写open的绑定描述（只针对第一个concept，实际绑定的那个）
				for (unsigned int i = 0; i < vOpenIndices.size(); i++)
				{
					if (i == 0)//只针对第一个concept
					{
						//		check that the confidence is strictly above the current nonunderstanding threshold
						// <18>	检查置信度是否严格高于当前的不可忽略阈值
						if (fConfidence > fNonunderstandingThreshold)
						{
							/*
							// D: structure describing one particular binding
							// D：描述一种特定结合的结构
							typedef struct
							{
								bool bBlocked;					// indicates whether the binding was			是否阻塞
								//  blocked or not
								string sGrammarExpectation;		// the expected grammar slot					期望的语法槽
								string sValue;					// the value in the binding						绑定的值
								float fConfidence;				// the confidence score for the binding			绑定的置信分
								int iLevel;						// the level in the agenda						agenda的level
								string sAgentName;				// the name of the agent that declared the		声明期望的agent
								//  expectation
								string sConceptName;			// the name of the concept that will bind		绑定的concept名
								string sReasonDisabled;			// if the binding was blocked, the reason		如果disable,说明原因
							} TBinding;
							*/
							TBinding bBinding;
							bBinding.bBlocked = false;
							bBinding.iLevel = iLevel;
							bBinding.fConfidence = fConfidence;
							bBinding.sAgentName = eaAgenda.celSystemExpectations[vOpenIndices[i]].pDialogAgent->GetName();
							bBinding.sConceptName = eaAgenda.celSystemExpectations[vOpenIndices[i]].sConceptName;
							bBinding.sGrammarExpectation = eaAgenda.celSystemExpectations[vOpenIndices[i]].sGrammarExpectation;
							bBinding.sValue = sSlotValue;
							rbdBindings.vbBindings.push_back(bBinding);
							rbdBindings.iConceptsBound++;//绑定的concept数目
							// add the slot to the list of matched slots 
							// 将该slot添加到匹配的slot列表
							msiSlotsMatched[bBinding.sGrammarExpectation] = 1;	// hash存储匹配的slot和相应的次数
							// in case we find it in the blocked slots, (it could have gotten
							// there on an earlier level) delete it from there
							// 如果我们发现它在阻塞的slot，（它可能已经得到了在较早的level）删除它从那里
							map<string, int>::iterator iPtr;
							if ((iPtr = msiSlotsBlocked.find(bBinding.sGrammarExpectation)) != msiSlotsBlocked.end())
							{
								msiSlotsBlocked.erase(iPtr);	// hash存储阻塞的slot和它们的次数
							}
						}
						else
						{
							//		o/w if the confidence is not above the threshold
							// <19>	低于阈值
							TBinding bBlockedBinding;
							bBlockedBinding.bBlocked = true;
							bBlockedBinding.iLevel = iLevel;
							bBlockedBinding.fConfidence = fConfidence;
							bBlockedBinding.sAgentName = eaAgenda.celSystemExpectations[vOpenIndices[i]].pDialogAgent->GetName();
							bBlockedBinding.sConceptName = eaAgenda.celSystemExpectations[vOpenIndices[i]].sConceptName;
							bBlockedBinding.sGrammarExpectation = eaAgenda.celSystemExpectations[vOpenIndices[i]].sGrammarExpectation;
							bBlockedBinding.sReasonDisabled = "confidence below nonunderstanding threshold";//被禁用原因【confidence过低】
							bBlockedBinding.sValue = sSlotValue;
							rbdBindings.vbBindings.push_back(bBlockedBinding);
							rbdBindings.iConceptsBlocked++; //阻塞Concept数
							// add the slot to the list of matched slots 
							// 将该slot添加到匹配的slot列表
							msiSlotsMatched[bBlockedBinding.sGrammarExpectation] = 1;	// hash存储匹配的slot和相应的次数
							// in case we find it in the blocked slots, (it could have gotten
							// there on an earlier level) delete it from there
							map<string, int>::iterator iPtr;
							if ((iPtr = msiSlotsBlocked.find(bBlockedBinding.sGrammarExpectation)) != msiSlotsBlocked.end())
							{
								msiSlotsBlocked.erase(iPtr);	// hash存储阻塞的slot和它们的次数
							}
						}
					}//if (i == 0)//只针对第一个concept
				}//填充绑定TBindingsDescr:  for (unsigned int i = 0; i < vOpenIndices.size(); i++)

				//		write the blocked bindings description
				// <20>	写阻塞的绑定描述TBindingsDescr
				for (unsigned int i = 0; i < vClosedIndices.size(); i++)
				{
					TBinding bBlockedBinding;
					bBlockedBinding.bBlocked = true;
					bBlockedBinding.iLevel = iLevel;
					bBlockedBinding.fConfidence = fConfidence;
					bBlockedBinding.sAgentName = eaAgenda.celSystemExpectations[vClosedIndices[i]].pDialogAgent->GetName();
					bBlockedBinding.sConceptName = eaAgenda.celSystemExpectations[vClosedIndices[i]].sConceptName;
					bBlockedBinding.sGrammarExpectation = eaAgenda.celSystemExpectations[vClosedIndices[i]].sGrammarExpectation;
					bBlockedBinding.sReasonDisabled = eaAgenda.celSystemExpectations[vClosedIndices[i]].sReasonDisabled;//被禁用原因【slot被阻塞原因】
					bBlockedBinding.sValue = sSlotValue;
					rbdBindings.vbBindings.push_back(bBlockedBinding);
					rbdBindings.iConceptsBlocked++;//阻塞Concept数
					// add it to the list of blocked slots, if it's not already
					// in the one of matched slots
					if (msiSlotsMatched.find(bBlockedBinding.sGrammarExpectation) == msiSlotsMatched.end())
					{
						msiSlotsBlocked[bBlockedBinding.sGrammarExpectation] = 1;//hash存储阻塞的slot和它们的次数
					}
				}// 写阻塞的绑定描述TBindingsDescr



			}//if (pInteractionEventManager->LastEventMatches(sSlotExpected))
		}//for (iPtr = eaAgenda.vCompiledExpectations[iLevel].mapCE.begin();
	}//for (unsigned int iLevel = 0; iLevel < eaAgenda.vCompiledExpectations.size(); iLevel++)

	//		for user inputs, update the non-understanding flag
	// <21>	对于用户输入，更新非理解标志bNonUnderstanding
	if (pInteractionEventManager->GetLastEvent()->GetType() == IET_USER_UTT_END ||
		pInteractionEventManager->GetLastEvent()->GetType() == IET_GUI)
	{
		rbdBindings.bNonUnderstanding = (rbdBindings.iConceptsBound == 0);//不理解 ： 是否有concept绑定
	}
	else
	{
		rbdBindings.bNonUnderstanding = false;
	}


	// update the slots matched and blocked information
	// 更新插槽匹配和阻塞的信息
	rbdBindings.iSlotsMatched = msiSlotsMatched.size(); // 匹配的slot数
	rbdBindings.iSlotsBlocked = msiSlotsBlocked.size(); // 阻塞的slot数



	/*
		#define IET_DIALOG_STATE_CHANGE	"dialog_state_change"
		#define IET_USER_UTT_START	"user_utterance_start"
		#define IET_USER_UTT_END	"user_utterance_end"
		#define IET_PARTIAL_USER_UTT "partial_user_utterance"
		#define IET_SYSTEM_UTT_START	"system_utterance_start"
		#define IET_SYSTEM_UTT_END	"system_utterance_end"
		#define IET_SYSTEM_UTT_CANCELED	"system_utterance_canceled"
		#define IET_FLOOR_OWNER_CHANGES "floor_owner_changes"
		#define IET_SESSION "session"
		#define IET_GUI "gui"
	*/

	// finally, for user inputs, check if the statistics match what helios 
	// predicted would happen (the helios binding features)
	// finally，对于用户输入，检查统计是否匹配helios的预测（helios绑定功能）
	if (pInteractionEventManager->GetLastEvent()->GetType() == IET_USER_UTT_END)
	{
		if ((pInteractionEventManager->LastEventMatches("[slots_blocked]")) &&
			(pInteractionEventManager->LastEventMatches("[slots_matched]")))
		{
			bool bHeliosMatch = true;
			string sH4SlotsBlocked = pInteractionEventManager->GetValueForExpectation("[slots_blocked]");
			string sH4SlotsMatched = pInteractionEventManager->GetValueForExpectation("[slots_matched]");
			if ((sH4SlotsBlocked != "N/A") && (atoi(sH4SlotsBlocked.c_str()) != rbdBindings.iSlotsBlocked))
				bHeliosMatch = false;
			if ((sH4SlotsMatched != "N/A") && (atoi(sH4SlotsMatched.c_str()) != rbdBindings.iSlotsMatched))
				bHeliosMatch = false;
			if (!bHeliosMatch)
			{
				Warning("Helios binding features are different from RavenClaw obtained values.");
			}
		}
		//##########################强制概念更新##############################################
		// finally，执行强制概念更新
		// finally, perform the forced concept updates
		performForcedConceptUpdates(rbdBindings);
		//##########################强制概念更新##############################################
	}
	else if (pInteractionEventManager->GetLastEvent()->GetType() == IET_GUI)
	{
		//###########################强制概念更新#############################################
		performForcedConceptUpdates(rbdBindings);
		//###########################强制概念更新#############################################
	}

	// and finally log the attempted bindings
	Log(DMCORE_STREAM, "Attempted bindings dumped below:\n%s",
		bindingsDescrToString(rbdBindings).c_str());

	// and the general statistics
	Log(DMCORE_STREAM, "Concepts Binding Phase completed (%d concept(s) "\
		"bound, %d concept(s) blocked out).",
		rbdBindings.iConceptsBound,
		rbdBindings.iConceptsBlocked);
}

// D: Perform the concept binding
// D：执行概念绑定
void CDMCoreAgent::performConceptBinding(string sSlotName, string sSlotValue,
	float fConfidence, int iExpectationIndex, bool bIsComplete)
{

	//		obtain a reference to the expectation structure
	// <1>	获得对期望结构的引用
	TConceptExpectation& ceExpectation = eaAgenda.celSystemExpectations[iExpectationIndex];

	//		compute the value we need to bind to that concept
	// <2>	计算我们需要绑定到那个概念的值
	string sValueToBind = "";
	if (ceExpectation.bmBindMethod == bmSlotValue)			// (1)	GRAMMAR_MAPPING("[QueryCategory.Origin]")
	{
		// bind the slot value
		sValueToBind = sSlotValue;
	}
	else if (ceExpectation.bmBindMethod == bmExplicitValue)	//(2)	GRAMMAR_MAPPING("![Yes]>true," "![No]>false" )
	{
		// bind the explicit value
		sValueToBind = ceExpectation.sExplicitValue;
	}
	else													//(3)	[value1|confidence1; value2|confidence] >: filter_function
	{
		// bind through a binding function
		// 通过filter函数绑定
		STRING2BFF::iterator iPtr;
		if ((iPtr = s2bffFilters.find(ceExpectation.sBindingFilterName)) == s2bffFilters.end())
		{
			FatalError(FormatString("Could not find binding filter :%s for "\
				"expectation %s generated by agent %s.",
				ceExpectation.sBindingFilterName.c_str(),
				ceExpectation.sGrammarExpectation.c_str(),
				ceExpectation.pDialogAgent->GetName().c_str()));
		}
		// if the binding filter was found, call it
		// 如果绑定过滤器找到，调用它
		sValueToBind = (*(iPtr->second))(sSlotName, sSlotValue);
	}

	// reset the confidence to 1, if ALWAYS_CONFIDENT is defined
#ifdef ALWAYS_CONFIDENT
	fConfidence = 1.0;
#endif

	// <3>	格式： slotValue|confidence    ==>  value/confidence
	string sBindingString = FormatString("%s%s%f", sValueToBind.c_str(), VAL_CONF_SEPARATOR, fConfidence);

	// now bind that particular value/confidence
	// 现在绑定该特定值/置信度
	// bIsComplete = pInteractionEventManager->LastEventIsComplete()  最后一个事件是否完成
	if (bIsComplete)
	{
		// first, create a temporary concept for that
		// 创建一个临时的concept
		CConcept *pTempConcept = ceExpectation.pDialogAgent->C(ceExpectation.sConceptName).EmptyClone();
		// assign it from the string
		// <4>	通过string赋值concept
		//		sBindingString =>  格式： slotValue|confidence    ==>  value/confidence
		pTempConcept->Update(CU_ASSIGN_FROM_STRING, &sBindingString);

		CConcept &c = ceExpectation.pDialogAgent->C(ceExpectation.sConceptName);

		//		first if the concept has an undergoing grounding request, remove it
		// <5>	首先如果概念有一个正在进行的接地请求，删除它
		if (c.IsUndergoingGrounding())
			pGroundingManager->RemoveConceptGroundingRequest(&c);

		//############################ 实际更新 ###########################################
		//		now call the binding method 
		// <6>	现在调用绑定方法,利用临时变量pTempConcept绑定实际的concept
		c.Update(CU_UPDATE_WITH_CONCEPT, pTempConcept);
		//############################ 实际更新 ###########################################

		// finally, deallocate the temporary concept
		// 析构
		delete pTempConcept;
	}//if (bIsComplete) 最后event完成
	else
	{
		// perform a partial (temporary) binding
		// 执行部分（临时）绑定
		ceExpectation.pDialogAgent->C(ceExpectation.sConceptName).Update(CU_PARTIAL_FROM_STRING, &sBindingString);
	}

	// log it
	Log(DMCORE_STREAM, "Slot %s(%s) bound to concept (%s)%s.",
		sSlotName.c_str(), sBindingString.c_str(),
		ceExpectation.pDialogAgent->GetName().c_str(),
		ceExpectation.sConceptName.c_str());
}// D：执行概念绑定


// D: processes non-understandings
void CDMCoreAgent::processNonUnderstanding()
{
	Log(DMCORE_STREAM, "Process Non-Understanding Phase initiated.");

	// sets meta information on the input, specifying it's a non-understanding
	Log(DMCORE_STREAM, "Non-understanding %s detected.", NON_UNDERSTANDING);
	pInteractionEventManager->GetLastInput()->SetProperty("["NON_UNDERSTANDING"]", "true");

	Log(DMCORE_STREAM, "Process Non-Understanding Phase completed.");
}

// D: performs the forced updates for the concepts
// D：执行概念的强制更新
void CDMCoreAgent::performForcedConceptUpdates(TBindingsDescr& rbdBindings)
{

	Log(DMCORE_STREAM, "Performing forced concept updates ...");

	// now go through the concepts that are waiting to be explicitly 
	// confirmed and perform the corresponding updates on them
	// 遍历等待显示确认的concept,执行相应更新
	set<CConcept *>::iterator iPtr;
	for (iPtr = saSystemAction.setcpExplicitConfirms.begin(); iPtr != saSystemAction.setcpExplicitConfirms.end(); iPtr++)//遍历当前系统动作中的 Explicit Concept
	{
		// check that the concept is still sealed (no update was performed  on it yet)
		if ((*iPtr)->IsSealed())//如果concept是sealed
		{
			/*
			typedef struct
			{
			string sConceptName;			// the name of the concept that had a	具有强制更新的概念的名称
			//  forced update
			int iType;						// the type of the forced update		强制更新的类型
			bool bUnderstanding;			// the update changed the concept
			//  enough that the grounding action
			//  on it is different, and therefore
			//  we consider that we have an actual
			//  understanding occuring on that
			//  concept
			// 更新改变了足够的概念，对它的基础动作是不同的，因此我们认为我们有一个实际的理解发生在那个概念
			} TForcedConceptUpdate;
			*/
			// then perform an forced update	
			// 执行强制更新
			TForcedConceptUpdate fcu;
			fcu.sConceptName = (*iPtr)->GetName();
			fcu.iType = FCU_EXPLICIT_CONFIRM;
			fcu.bUnderstanding = false;
			CHyp *phOldTopHyp = (*iPtr)->GetTopHyp();

			// log the update
			Log(DMCORE_STREAM, "Performing forced concept update on %s ...", (*iPtr)->GetName().c_str());
			Log(DMCORE_STREAM, "Concept grounding status: %d", pGroundingManager->GetConceptGroundingRequestStatus((*iPtr)));
			(*iPtr)->Update(CU_UPDATE_WITH_CONCEPT, NULL);// ????pUpdateData=NULL

			Log(DMCORE_STREAM, "Concept grounding status: %d", pGroundingManager->GetConceptGroundingRequestStatus((*iPtr)));
			// now, if this update has desealed the concept, then we need
			// to run grounding on this concept 
			// 如果更新解封了 concept, 则运行接地Action
			if (!((*iPtr)->IsSealed()) && (pGroundingManager->GetConceptGroundingRequestStatus(*iPtr) != GRS_EXECUTING))
			{
				// now schedule grounding on this concept
				string sAction = pGroundingManager->ScheduleConceptGrounding(*iPtr);//// D：强制接地管理器为某个概念安排接地
				// if the action scheduled is still explicit confirm
				if (sAction != "EXPL_CONF")
				{
					// then mark that we have an understanding
					// 理解
					fcu.bUnderstanding = true;
					rbdBindings.bNonUnderstanding = false;
				}
				else if ((*iPtr)->GetTopHyp() == phOldTopHyp)
				{
					// if we are still on an explicit confirm on the same hypothesis, 
					// seal it back
					// 在相同concept上执行 explicit confirm,则seal它
					(*iPtr)->Seal();
				}
			}

			// finally, push this into the bindings
			rbdBindings.vfcuForcedUpdates.push_back(fcu);//添加到强制更新的向量
		}//if ((*iPtr)->IsSealed())//如果concept是sealed
	}//遍历当前系统动作中的 Explicit Concept

	// now go through the concepts that are waiting to be implicitly 
	// confirmed and perform the corresponding updates on them
	// 遍历等待隐式确认的concept,执行相应更新
	for (iPtr = saSystemAction.setcpImplicitConfirms.begin(); iPtr != saSystemAction.setcpImplicitConfirms.end(); iPtr++)
	{
		// check that the concept is still sealed (no update was performed 
		// on it yet)
		if ((*iPtr)->IsSealed())
		{

			// then perform an forced update			
			TForcedConceptUpdate fcu;
			fcu.sConceptName = (*iPtr)->GetName();
			fcu.iType = FCU_IMPLICIT_CONFIRM;
			fcu.bUnderstanding = false;

			// log the update
			Log(DMCORE_STREAM, FormatString("Performing forced concept update on %s ...", (*iPtr)->GetName().c_str()));
			(*iPtr)->Update(CU_UPDATE_WITH_CONCEPT, NULL);

			// now, if this update has desealed the concept, then we need
			// to run grounding on this concept 
			if (!((*iPtr)->IsSealed()) &&
				(pGroundingManager->GetConceptGroundingRequestStatus(*iPtr) != GRS_EXECUTING))
			{
				// first check that it was not already scheduled
				if (pGroundingManager->GetScheduledGroundingActionOnConcept(*iPtr) == "")
					// if not scheduled already, schedule it now				
					pGroundingManager->ScheduleConceptGrounding(*iPtr);
				// then mark that we have an understanding
				fcu.bUnderstanding = true;
				rbdBindings.bNonUnderstanding = false;
			}

			// finally, push this into the bindings
			rbdBindings.vfcuForcedUpdates.push_back(fcu);
		}
	}// 遍历等待隐式确认的concept,执行相应更新

	// finally, go through the concepts that have unplanned implicit confirmations
	// on them and perform the corresponding updates on them
	// 遍历【unplanned】等待隐式确认的concept,执行相应更新
	for (iPtr = saSystemAction.setcpUnplannedImplicitConfirms.begin(); iPtr != saSystemAction.setcpUnplannedImplicitConfirms.end(); iPtr++)
	{
		// check that the concept is still sealed (no update was performed 
		// on it yet)
		if ((*iPtr)->IsSealed())
		{

			// then perform an forced update			
			TForcedConceptUpdate fcu;
			fcu.sConceptName = (*iPtr)->GetName();
			fcu.iType = FCU_UNPLANNED_IMPLICIT_CONFIRM;
			fcu.bUnderstanding = false;

			// log the update
			Log(DMCORE_STREAM, FormatString("Performing forced concept update on %s ...", (*iPtr)->GetName().c_str()));
			(*iPtr)->Update(CU_UPDATE_WITH_CONCEPT, NULL);

			// now, if this update has desealed the concept, then we need
			// to run grounding on this concept 
			if (!((*iPtr)->IsSealed()))
			{
				// first check that it was not already scheduled
				if (pGroundingManager->GetScheduledGroundingActionOnConcept(*iPtr) == "")
					// if not scheduled already, schedule it now				
					pGroundingManager->ScheduleConceptGrounding(*iPtr);
				// then mark that we have an understanding
				fcu.bUnderstanding = true;
				rbdBindings.bNonUnderstanding = false;
			}

			// finally, push this into the bindings
			rbdBindings.vfcuForcedUpdates.push_back(fcu);
		}
	}// 遍历【unplanned】等待隐式确认的concept,执行相应更新

	// log completion of this phase
	Log(DMCORE_STREAM, "Forced concept updates completed.");
}


//-----------------------------------------------------------------------------
// D: Focus Claims functions
//-----------------------------------------------------------------------------
// D: assembles a list of focus claims, and returns the size of that list
// D：收集焦点声明的列表，并返回该列表的大小
int CDMCoreAgent::assembleFocusClaims()
{
	Log(DMCORE_STREAM, "Focus Claims Assembly Phase initiated.");

	/*
	// D: structure describing a focus claim
	// D：描述焦点声明的结构
		typedef struct
		{
			string sAgentName;				// the name of the agent that claims focus		// 声明claim的agent的名称
			bool bClaimDuringGrounding;     // indicates whether or not the focus is		// 是否在Grouding期间声明Claim的
			//  claimed during grounding
		} TFocusClaim;
	*/

	//		gather the focus claims, starting with the root of the dialog task tree
	// <1>	收集焦点声明，从对话框任务树的根Root开始
	int iClaims = 0;
	TFocusClaimsList fclTempFocusClaims;
	iClaims = pDTTManager->GetDialogTaskTreeRoot()->DeclareFocusClaims(fclTempFocusClaims);//从root节点开始递归收集满足#define TRIGGERED_BY(Condition)条件的Agent【主要】

	// log the list of claiming agents
	// <2>	记录声明代理的列表
	string sLogString;
	if (fclTempFocusClaims.size() == 0)
		sLogString = "0 agent(s) claiming focus.";
	else
		sLogString = FormatString("%d agent(s) claiming focus (dumped below):\n", fclTempFocusClaims.size());

	for (unsigned int i = 0; i < fclTempFocusClaims.size(); i++)
		sLogString += FormatString("  %s\n", fclTempFocusClaims[i].sAgentName.c_str());
	Log(DMCORE_STREAM, sLogString.c_str());

	//		now prune the claims of agents that have their completion criteria satisfied
	// <3>	现在删除满足【完成】标准的agent的声明
	Log(DMCORE_STREAM, "Pruning Focus Claims list.");
	// check if we undergoing some grounding action
	// 检查我们是否正在进行一些 grouding action
	bool bDuringGrounding = pGroundingManager->HasScheduledConceptGroundingRequests()/* ||
							pGroundingManager->HasExecutingConceptGroundingRequests()*/;

	int iClaimsEliminated = 0;
	fclFocusClaims.clear(); //CDMCoreAgent的焦点声明列表清空
	sLogString = "";
	// <4>	遍历收集到的fclTempFocusClaims
	for (unsigned int i = 0; i < fclTempFocusClaims.size(); i++)//遍历收集到的fclTempFocusClaims
	{
		// <5>	获取声明focus的Agent
		CDialogAgent* pdaFocusClaimingAgent = (CDialogAgent *)AgentsRegistry[fclTempFocusClaims[i].sAgentName];

		if (pdaFocusClaimingAgent->SuccessCriteriaSatisfied() ||
			pdaFocusClaimingAgent->FailureCriteriaSatisfied() ||
			AgentIsActive(pdaFocusClaimingAgent) ||
			(!fclTempFocusClaims[i].bClaimDuringGrounding && bDuringGrounding))
		{
			// eliminate the agent from the list of agents claiming focus
			// if they already have the success criteria satisfied or if
			// they are already in focus, or if they cannot trigger during
			// grounding
			//<6>	如果agent
			//			1	-	已经完成，
			//			2	-	或者它们已经在stack中，
			//			3	-	或者如果它们在接地期间不能触发Focus　shift，
			//	则从代理声明焦点的代理列表中消除
			iClaimsEliminated++;
			// and mark it in the log string
			sLogString += FormatString("  %s\n", fclTempFocusClaims[i].sAgentName.c_str());
		}
		else
		{
			//		o/w add it to the real list of claims
			// <7>	添加到CDMCoreAgent的实际声明列表里
			fclFocusClaims.push_back(fclTempFocusClaims[i]);
		}
	}//for (unsigned int i = 0; i < fclTempFocusClaims.size(); i++)//遍历收集到的fclTempFocusClaims

	// finally, add the prefix for the log string of eliminated agents
	// finally，添加消除代理的日志字符串的前缀
	if (iClaimsEliminated == 0)
		sLogString = "0 agents(s) eliminated from the focus claims list.";
	else
		sLogString =
		FormatString("%d agent(s) eliminated from the focus claims list "\
		"(dumped below):\n%s", iClaimsEliminated, sLogString.c_str());

	Log(DMCORE_STREAM, sLogString.c_str());

	Log(DMCORE_STREAM, "Focus Claims Assembly Phase completed "\
		"(%d claim(s)).", iClaims);
	return iClaims;
}

// D: resolves focus shifts
// D：解决Focus偏移
void CDMCoreAgent::resolveFocusShift()
{

	//		send out a warning if we have a multiple focus shift
	// <1>	如果我们有一个多focus Shift，发出警告
	if (fclFocusClaims.size() > 1)
	{
		string sMessage = "Ambiguous focus shift (claiming agents dump below).\n";
		for (unsigned int i = 0; i < fclFocusClaims.size(); i++)
			sMessage += fclFocusClaims[i].sAgentName + "\n";
		Warning(sMessage.c_str());
	}

	//		put the agents on the stack 
	// <2>	将代理放在堆栈上	=>	只有一个？！！
	for (unsigned int i = 0; i < fclFocusClaims.size(); i++)
	{
		string sClaimingAgent = fclFocusClaims[i].sAgentName;
		Log(DMCORE_STREAM,
			"Adding focus-claiming agent %s on the execution stack.",
			sClaimingAgent.c_str());
		ContinueWith(this, (CDialogAgent *)AgentsRegistry[sClaimingAgent]);
	}
}

//-----------------------------------------------------------------------------
//
// TIMEOUT RELATED METHODS
//
//-----------------------------------------------------------------------------
// D: sets the timeout period
void CDMCoreAgent::SetTimeoutPeriod(int iATimeoutPeriod)
{
	iTimeoutPeriod = iATimeoutPeriod;
	DMI_SetTimeoutPeriod(iTimeoutPeriod);
}

// D: returns the current timeout period
// D：返回当前的超时时间
int CDMCoreAgent::GetTimeoutPeriod()
{
	return iTimeoutPeriod;
}

// D: sets the default timeout period
void CDMCoreAgent::SetDefaultTimeoutPeriod(int iADefaultTimeoutPeriod)
{
	iDefaultTimeoutPeriod = iADefaultTimeoutPeriod;
}

// D: returns the current timeout period
int CDMCoreAgent::GetDefaultTimeoutPeriod()
{
	return iDefaultTimeoutPeriod;
}

//-----------------------------------------------------------------------------
//
// METHODS FOR ACCESSING THE NONUNDERSTANDING THRESHOLD
//
//-----------------------------------------------------------------------------

// D: sets the nonunderstanding threshold
void CDMCoreAgent::SetNonunderstandingThreshold(float fANonunderstandingThreshold)
{
	fNonunderstandingThreshold = fANonunderstandingThreshold;
}

// D: gets the nonunderstanding threshold
float CDMCoreAgent::GetNonunderstandingThreshold()
{
	return fNonunderstandingThreshold;
}

// D: sets the default nonunderstanding threshold
void CDMCoreAgent::SetDefaultNonunderstandingThreshold(float fANonuThreshold)
{
	fDefaultNonunderstandingThreshold = fANonuThreshold;
}

// D: returns the default nonunderstanding threshold
// D：返回默认的nonunderstanding阈值
float CDMCoreAgent::GetDefaultNonunderstandingThreshold()
{
	return fDefaultNonunderstandingThreshold;
}

//---------------------------------------------------------------------
// METHODS FOR SIGNALING FLOOR CHANGES
//---------------------------------------------------------------------

void CDMCoreAgent::SetFloorStatus(TFloorStatus fsaFloorStatus)
{
	Log(DMCORE_STREAM, "Set floor status to %s", vsFloorStatusLabels[fsaFloorStatus].c_str());
	fsFloorStatus = fsaFloorStatus;
}

void CDMCoreAgent::SetFloorStatus(string sAFloorStatus)
{
	SetFloorStatus(StringToFloorStatus(sAFloorStatus));
}

TFloorStatus CDMCoreAgent::GetFloorStatus()
{
	return fsFloorStatus;
}

string CDMCoreAgent::FloorStatusToString(TFloorStatus fsAFloor)
{
	return vsFloorStatusLabels[fsAFloor];
}

TFloorStatus CDMCoreAgent::StringToFloorStatus(string sAFloor)
{
	for (unsigned int i = 0; i < vsFloorStatusLabels.size(); i++)
	{
		if (vsFloorStatusLabels[i] == sAFloor)
			return (TFloorStatus)i;
	}
	return fsUnknown;
}


//-----------------------------------------------------------------------------
//
// METHOD FOR SIGNALING THE NEED FOR A FOCUS CLAIMS PHASE
//
//-----------------------------------------------------------------------------

// D: signal a focus claims phase - set a flag so that the core agent will run
//    a focus claims phase the next chance it gets
// D：signal a focus claims phase - 设置一个标志，以便DMCore agent将运行焦点声明阶段下一个机会, 默认为True
// void SignalFocusClaimsPhase(bool bAFocusClaimsPhaseFlag = true);
void CDMCoreAgent::SignalFocusClaimsPhase(bool bAFocusClaimsPhaseFlag)
{
	bFocusClaimsPhaseFlag = bAFocusClaimsPhaseFlag;
}

//-----------------------------------------------------------------------------
//
// ACCESS TO VARIOUS PRIVATE FIELDS (mostly state information for the 
// State Manager Agent)
//
//-----------------------------------------------------------------------------

// D: returns the number of concepts bound in the last input pass
int CDMCoreAgent::LastTurnGetConceptsBound()
{
	if (bhBindingHistory.size() == 0)
		return -1;
	else
		return bhBindingHistory.back().iConceptsBound;
}

// D: returns true if the last turn was a non-understanding
// D：如果最后一个turn是不理解，则返回true
bool CDMCoreAgent::LastTurnNonUnderstanding()
{
	// <1>	从后往前查找 => 绑定历史
	for (int i = bhBindingHistory.size() - 1; i >= 0; i--)
	{
		/*
			#define IET_DIALOG_STATE_CHANGE	"dialog_state_change"
			#define IET_USER_UTT_START	"user_utterance_start"
			#define IET_USER_UTT_END	"user_utterance_end"
			#define IET_PARTIAL_USER_UTT "partial_user_utterance"
			#define IET_SYSTEM_UTT_START	"system_utterance_start"
			#define IET_SYSTEM_UTT_END	"system_utterance_end"
			#define IET_SYSTEM_UTT_CANCELED	"system_utterance_canceled"
			#define IET_FLOOR_OWNER_CHANGES "floor_owner_changes"
			#define IET_SESSION "session"
			#define IET_GUI "gui"
		*/
		// <2>	user说话结束 | GUI［界面］
		if (bhBindingHistory[i].sEventType == IET_USER_UTT_END ||
			bhBindingHistory[i].sEventType == IET_GUI)
		{
			if (bhBindingHistory[i].bNonUnderstanding)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	return false;
}

// A: returns the number of consecutive non-understandings so far
// 返回到目前为止的连续不理解的数量
int CDMCoreAgent::GetNumberNonUnderstandings()
{
	int iNumNonunderstandings = 0;
	for (int i = bhBindingHistory.size() - 1; i >= 0; i--)
	{
		if (bhBindingHistory[i].sEventType == IET_USER_UTT_END ||
			bhBindingHistory[i].sEventType == IET_GUI)
		{
			if (bhBindingHistory[i].bNonUnderstanding)
			{
				iNumNonunderstandings++;
			}
			else
			{
				break;
			}
		}
	}
	return iNumNonunderstandings;
}

// A: returns the total number of non-understandings in the current dialog
//    so far
//返回当前对话框中迄今为止不可理解的总数
int CDMCoreAgent::GetTotalNumberNonUnderstandings()
{
	int iNumNonunderstandings = 0;
	for (int i = 0; i < (int)bhBindingHistory.size(); i++)
	{
		if (bhBindingHistory[i].bNonUnderstanding)
		{
			iNumNonunderstandings++;
		}
	}
	return iNumNonunderstandings;
}

//-----------------------------------------------------------------------------
//
// EXECUTION STACK AND HISTORY METHODS
//
//-----------------------------------------------------------------------------

// D: Pushes a new dialog agent on the execution stack 
// 在执行堆栈上push一个新的对话代理
void CDMCoreAgent::ContinueWith(CAgent* paPusher, CDialogAgent* pdaDialogAgent)
{

	//		check that the agent is not already on top of the stack
	// <1>	检查 是否已经在栈顶
	if (!esExecutionStack.empty() && (esExecutionStack.begin()->pdaAgent == pdaDialogAgent))
	{
		// 如果已经在栈顶，请求被忽略
		Log(DMCORE_STREAM, "Agent %s already on top of the execution stack. ContinueWith request ignored.", pdaDialogAgent->GetName().c_str());
		return;
	}

	//		add an entry in the history; fill in all the slots
	// <2>	在历史中添加条目; 填写所有插槽
	TExecutionHistoryItem ehi;
	ehi.sCurrentAgent = pdaDialogAgent->GetName();		//name
	ehi.sCurrentAgentType = pdaDialogAgent->GetType();	//type
	ehi.bScheduled = true;
	ehi.sScheduledBy = paPusher->GetName();				//被谁调度
	ehi.timeScheduled = GetTime();						//调度的时间
	ehi.bExecuted = false;
	ehi.bCommitted = false;
	ehi.bCanceled = false;
	ehi.iStateHistoryIndex = -1;
	// ***ehi.timeTerminated = 0;
	ehExecutionHistory.push_back(ehi); //存入执行历史中

	//		and put it on the stack
	// <3>	实际实行的Agent压入堆栈
	TExecutionStackItem esi;
	esi.pdaAgent = pdaDialogAgent;
	esi.iEHIndex = ehExecutionHistory.size() - 1;	//记录在执行历史中的索引
	esExecutionStack.push_front(esi);				//在开始位置[栈顶]增加一个元素======在开始位置增加一个元素

	//		stores the execution index in the agent
	// <4>	将执行的[历史]index存储在代理中
	pdaDialogAgent->SetLastExecutionIndex(esi.iEHIndex);

	//		signals that the agenda needs to be recomputed
	// <5>	表明agenda需要被重新更新	=> 执行栈stack改变就需要更新Agenda
	bAgendaModifiedFlag = true;

	Log(DMCORE_STREAM, "Agent %s added on the execution stack by %s.",
		ehi.sCurrentAgent.c_str(), ehi.sScheduledBy.c_str());
}

// D: Restarts a topic
//重新启动主题
void CDMCoreAgent::RestartTopic(CDialogAgent* pdaDialogAgent)
{
	// first, locate the agent
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin();
		iPtr != esExecutionStack.end();
		iPtr++)
	{
		if (iPtr->pdaAgent == pdaDialogAgent) break;
	}

	// if the agent was nothere in the list, trigger a fatal error
	if (iPtr == esExecutionStack.end())
	{
		Warning("Cannot restart the " + pdaDialogAgent->GetName() +
			" agent. Agent not found on execution stack.");
		return;
	}

	Log(DMCORE_STREAM, "Restarting agent %s.", pdaDialogAgent->GetName().c_str());

	// store the planner of this agent
	CDialogAgent* pdaScheduler = (CDialogAgent *)
		AgentsRegistry[ehExecutionHistory[iPtr->iEHIndex].sScheduledBy];

	// now clean it off the execution stack
	PopTopicFromExecutionStack(pdaDialogAgent);

	// reopen it
	pdaDialogAgent->ReOpen();

	// and readd it to the stack
	ContinueWith(pdaScheduler, pdaDialogAgent);
}

// D: Registers a custom start over function 
void CDMCoreAgent::RegisterCustomStartOver(TCustomStartOverFunct csoAStartOverFunct)
{
	csoStartOverFunct = csoAStartOverFunct;
}

// D: Restarts the dialog 
// D：重新启动对话框
void CDMCoreAgent::StartOver()
{
	if (csoStartOverFunct == NULL)
	{
		// restart the dialog clear the execution stack
		// 清空执行栈 【野指针？】
		esExecutionStack.clear();
		// destroy the dialog task tree
		// 销毁对话树
		pDTTManager->DestroyDialogTree();
		// recreate the dialog task tree
		// 重新创建 DTT
		pDTTManager->ReCreateDialogTree();
		// restart the execution by putting the root on the stack
		// 根节点放入栈顶，重启会话
		Log(DMCORE_STREAM, "Restarting Dialog Task execution.");
		ContinueWith(this, pDTTManager->GetDialogTaskTreeRoot());
		//更新状态
		pStateManager->UpdateState();
	}
	else
	{
		(*csoStartOverFunct)();
	}
}

// D: Pops all the completed agents (and all the agents they have ever planned
//	  for off the execution stack
// 所有完成的agent出栈
int CDMCoreAgent::popCompletedFromExecutionStack()
{
	TExecutionStack::iterator iPtr;
	bool bFoundCompleted;	// indicates if completed agents were still found

	TStringVector vsAgentsEliminated;
	// when no more completed agents can be found, return
	do
	{
		bFoundCompleted = false;
		//		go through the execution stack
		// <1>	遍历执行栈
		for (iPtr = esExecutionStack.begin(); iPtr != esExecutionStack.end(); iPtr++)
		{

			//		if you find an agent that has completed
			// <2>	是否完成 【成功， 失败】
			if (iPtr->pdaAgent->HasCompleted())
			{
				//##########################popTopicFromExecutionStack#############################
				//		pop it off the execution stack
				// <3>	弹出栈 - [弹出所有子agent]
				popTopicFromExecutionStack(iPtr->pdaAgent, vsAgentsEliminated);
				bFoundCompleted = true;
				break;
				//##########################popTopicFromExecutionStack#############################
			}
		}
	} while (bFoundCompleted);

	//		when no more completed agents can be found, log and return
	// <4>	log记录
	if (vsAgentsEliminated.size() != 0)
	{
		string sAgents;
		for (unsigned int i = 0; i < vsAgentsEliminated.size(); i++)
			sAgents += FormatString("%s\n", vsAgentsEliminated[i].c_str());

		sAgents = TrimRight(sAgents);

		Log(DMCORE_STREAM, "Eliminated %d completed agent(s) (dumped below) "
			"from the execution stack.\n%s",
			vsAgentsEliminated.size(), sAgents.c_str());
	}

	//		return the number of agents eliminated
	// <5>	返回删除的agent数目
	return vsAgentsEliminated.size();
}

// D: Pops a dialog agent from the execution stack
void CDMCoreAgent::popAgentFromExecutionStack(CDialogAgent* pdaADialogAgent,
	TStringVector& rvsAgentsEliminated)
{
	// check for empty stack condition
	if (esExecutionStack.empty())
	{
		FatalError("Cannot pop the " + pdaADialogAgent->GetName() +
			" agent off the execution stack. Stack is empty.");
	}

	// first, locate the agent
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin();
		iPtr != esExecutionStack.end();
		iPtr++)
	{
		if (iPtr->pdaAgent == pdaADialogAgent) break;
	}

	// if the agent was nothere in the list, trigger a fatal error
	if (iPtr == esExecutionStack.end())
	{
		FatalError("Cannot pop the " + pdaADialogAgent->GetName() +
			" agent off the execution stack. Agent not found.");
	}

	// mark the time this agent's execution was terminated
	ehExecutionHistory[iPtr->iEHIndex].timeTerminated = GetTime();

	// call the agent's OnCompletion method
	iPtr->pdaAgent->OnCompletion();

	// and add it to the list of eliminated agents
	rvsAgentsEliminated.push_back(iPtr->pdaAgent->GetName());

	// eliminate the agent from the stack
	esExecutionStack.erase(iPtr);

	// signals that the agenda needs to be recompiled
	bAgendaModifiedFlag = true;
}

// D: Pops a dialog agent from the execution stack, together with all the 
//    other agents it has ever planned for execution
// D：从执行堆栈中弹出一个对话代理，以及它计划执行的所有其他代理
void CDMCoreAgent::popTopicFromExecutionStack(CDialogAgent* pdaADialogAgent, TStringVector& rvsAgentsEliminated)
{
	// check for empty stack condition
	if (esExecutionStack.empty())
	{
		FatalError("Cannot pop the " + pdaADialogAgent->GetName() +
			" agent off the execution stack. Stack is empty.");
	}

	//		first, locate the agent
	// <1>	定位栈中的位置
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin();
		iPtr != esExecutionStack.end();
		iPtr++)
	{
		if (iPtr->pdaAgent == pdaADialogAgent) break;
	}

	//		if the agent was nothere in the list, trigger a fatal error
	// <2>	如果代理不在列表中，则触发致命错误
	if (iPtr == esExecutionStack.end())
	{
		FatalError("Cannot pop the " + pdaADialogAgent->GetName() + " agent off the execution stack. Agent not found.");
	}

	//		the set of eliminated agents
	// <3>	清除agent的set集合
	set<CDialogAgent*, less<CDialogAgent*> > sEliminatedAgents;

	// initialize it with the starting agent
	// 要删除的当前agent
	sEliminatedAgents.insert(iPtr->pdaAgent);

	//		mark the time this agent's execution was terminated
	// <4>	标记此代理的执行被终止的时间
	ehExecutionHistory[iPtr->iEHIndex].timeTerminated = GetTime();

	// 调用 <3>	OnCompletion()方法####################################@重要@#################################
	// <5>	call the agent's OnCompletion method
	iPtr->pdaAgent->OnCompletion();//宏 #define ON_COMPLETION(DO_STUFF)
	// 调用 <3>	OnCompletion()方法####################################@重要@#################################

	//		and add it to the list of eliminated agents
	// <6>	加入到删除列表
	rvsAgentsEliminated.push_back(iPtr->pdaAgent->GetName());

	//		eliminate the agent from the stack
	// <7>	堆栈中删除当前agent
	esExecutionStack.erase(iPtr);

	// now enter in a loop going through the stack repeatedly until 
	// we didn't find anything else to remove
	// <8>	循环检查堆栈，直到我们没有找到任何其他要删除
	bool bFoundAgentToRemove = true;
	while (bFoundAgentToRemove)
	{

		bFoundAgentToRemove = false;

		//		now traverse the stack
		// <9>	遍历执行栈 ： 删除子Agent
		for (iPtr = esExecutionStack.begin(); iPtr != esExecutionStack.end(); iPtr++)
		{

			//		check to see who planned the current agent
			// <10>	查看当前代理的scheduler调用者
			CDialogAgent* pdaScheduler = (CDialogAgent *) AgentsRegistry[ehExecutionHistory[iPtr->iEHIndex].sScheduledBy];
			
			// <11>	如果当前的agent的父agent在删出列表里，则也要删除当前的agent
			if (sEliminatedAgents.find(pdaScheduler) != sEliminatedAgents.end())
			{
				// then we need to eliminate this one; so first add it to the 
				// list of eliminated agents
				// 如果删除一个agent,则所有的子agent都是要删除的。 =>  不需要检查完成标志
				// 实际删除是从当前agent递归查找删除它的所有的子agent
				sEliminatedAgents.insert(iPtr->pdaAgent);
				// mark the time this agent execution was terminated
				// 记录终止时间
				ehExecutionHistory[iPtr->iEHIndex].timeTerminated = GetTime();
				//################################################################
				// call the agent's OnCompletion method
				iPtr->pdaAgent->OnCompletion();
				//################################################################
				// and add it to the list of eliminated agents
				//添加到删除列表
				rvsAgentsEliminated.push_back(iPtr->pdaAgent->GetName());
				// eliminate the agent from the stack
				// 堆栈中删除 - 实际操作
				esExecutionStack.erase(iPtr);
				// set found one to true
				bFoundAgentToRemove = true;
				// and break the for loop
				break;
			}
		}
	}//while (bFoundAgentToRemove)

	// agenda修改过 - 重新组装#####################################!!!
	bAgendaModifiedFlag = true;
	// agenda修改过 - 重新组装#####################################!!!
}

// A: Pops all grounding agents from the execution stack
void CDMCoreAgent::popGroundingAgentsFromExecutionStack(TStringVector& rvsAgentsEliminated)
{
	// check for empty stack condition
	if (esExecutionStack.empty())
	{
		Log(DMCORE_STREAM, "Execution stack empty: no grounding agent popped");
		return;
	}

	// the set of eliminated agents
	set<CAgent*, less<CAgent*> > sEliminatedAgents;

	// initialize it with the grounding manager agent since it
	// schedules all root grounding agents
	sEliminatedAgents.insert(pGroundingManager);

	// now enter in a loop going through the stack repeatedly until 
	// we didn't find anything else to remove
	bool bFoundAgentToRemove = true;

	while (bFoundAgentToRemove)
	{

		bFoundAgentToRemove = false;

		// now traverse the stack
		for (TExecutionStack::iterator iPtr = esExecutionStack.begin();
			iPtr != esExecutionStack.end();
			iPtr++)
		{

			// check to see who planned the current agent
			CDialogAgent* pdaScheduler = (CDialogAgent *)
				AgentsRegistry[ehExecutionHistory[iPtr->iEHIndex].sScheduledBy];
			if (sEliminatedAgents.find(pdaScheduler) != sEliminatedAgents.end())
			{
				// then we need to eliminate this one; so first add it to the 
				// list of eliminated agents
				sEliminatedAgents.insert(iPtr->pdaAgent);
				// mark the time this agent execution was terminated
				ehExecutionHistory[iPtr->iEHIndex].timeTerminated = GetTime();
				// call the agent's OnCompletion method
				iPtr->pdaAgent->OnCompletion();
				// and add it to the list of eliminated agents
				rvsAgentsEliminated.push_back(iPtr->pdaAgent->GetName());
				// eliminate the agent from the stack
				esExecutionStack.erase(iPtr);
				// set found one to true
				bFoundAgentToRemove = true;
				// and break the for loop
				break;
			}
		}
	}

	bAgendaModifiedFlag = true;
}

// A: Rolls back to a previous dialog state (e.g. after a user barge-in)
void CDMCoreAgent::rollBackDialogState(int iState)
{

	int iLastEHIndex = (*pStateManager)[iState].iEHIndex;

	for (int i = ehExecutionHistory.size() - 1; i > iLastEHIndex; i--)
	{
		TExecutionHistoryItem ehi = ehExecutionHistory[i];
		// if the agents were indeed executed and not yet canceled
		if (ehi.bExecuted && !ehi.bCanceled)
		{
			// Undo the execution of the agent
			CDialogAgent *pdaAgent = (CDialogAgent*)
				AgentsRegistry[ehi.sCurrentAgent];
			pdaAgent->Undo();

			// Mark the execution as canceled
			ehi.bCanceled = true;

			Log(DMCORE_STREAM, "Canceled execution of agent %s (state=%d,"
				"iEHIndex=%d).", pdaAgent->GetName().c_str(),
				iState, i);
		}
	}

	// Now updates the execution stack and the agenda
	TDialogState dsCurrentState = (*pStateManager)[iState];
	fsFloorStatus = dsCurrentState.fsFloorStatus;
	esExecutionStack = dsCurrentState.esExecutionStack;
	eaAgenda = dsCurrentState.eaAgenda;
	saSystemAction = dsCurrentState.saSystemAction;
	// There is no need to recompile the agenda
	bAgendaModifiedFlag = false;

	// And updates the current state
	pStateManager->UpdateState();

	// Log the rollback
	Log(DMCORE_STREAM, "Rolled back to state %d. Current agenda dumped"
		" below\n%s", iState, expectationAgendaToString().c_str());

}

// D: Returns the size of the binding history
// D：返回绑定历史记录的大小
int CDMCoreAgent::GetBindingHistorySize()
{
	return bhBindingHistory.size();
}

// D: Returns a pointer to the binding result from history
// D：返回指向历史记录的绑定结果的指针
const TBindingsDescr& CDMCoreAgent::GetBindingResult(int iBindingHistoryIndex)
{
	// check that the index is within bounds
	if ((iBindingHistoryIndex >= 0) ||
		(iBindingHistoryIndex < -(int)bhBindingHistory.size()))
	{
		FatalError("Out of bounds index on access to binding history.");
		return bhBindingHistory[0];
	}
	else
	{
		return bhBindingHistory[bhBindingHistory.size() + iBindingHistoryIndex];
	}
}

// D: Returns a description of the system action taken on a particular concept
//    (***** this function will need to be elaborated more *****)
// D：返回对特定概念执行的系统操作的描述
//（*****这个函数需要详细说明*****）
TSystemActionOnConcept CDMCoreAgent::GetSystemActionOnConcept(CConcept* pConcept)
{
	TSystemActionOnConcept saoc;

	// check if the concept is among the requested concepts
	// request类型
	if (saSystemAction.setcpRequests.find(pConcept) != saSystemAction.setcpRequests.end())
	{
		// then we have a request
		saoc.sSystemAction = SA_REQUEST;
	}

	// check if the concept is among the explicitly confirmed concepts
	// 显示确认
	else if (saSystemAction.setcpExplicitConfirms.find(pConcept) != saSystemAction.setcpExplicitConfirms.end())
	{
		// then we have an explicit confirm
		saoc.sSystemAction = SA_EXPL_CONF;
	}

	// check if the concept is among the implicitly confirmed concepts
	// 隐式确认
	else if (saSystemAction.setcpImplicitConfirms.find(pConcept) != saSystemAction.setcpImplicitConfirms.end())
	{
		// then we have an implicit confirm
		saoc.sSystemAction = SA_IMPL_CONF;
	}

	// check if the concept is among the unplanned implicitly confirmed concepts
	// unplanned隐式确认
	else if (saSystemAction.setcpUnplannedImplicitConfirms.find(pConcept) != saSystemAction.setcpUnplannedImplicitConfirms.end())
	{
		// then we have an implicit confirm
		saoc.sSystemAction = SA_UNPLANNED_IMPL_CONF;
	}

	// check if the concept is among the implicitly confirmed concepts
	// 其他类型
	else
	{
		// then we have an "other" type request
		saoc.sSystemAction = SA_OTHER;
	}

	// return 
	return saoc;
}

// D: Signal an explicit confirmation on a concept
// D：表示一个概念的显式确认
void CDMCoreAgent::SignalExplicitConfirmOnConcept(CConcept* pConcept)
{
	if (saSystemAction.setcpUnplannedImplicitConfirms.find(pConcept) !=
		saSystemAction.setcpUnplannedImplicitConfirms.end())
	{
		// if the concept is currently marked under an unplanned implicit 
		// confirm, then delete it from there 
		saSystemAction.setcpUnplannedImplicitConfirms.erase(pConcept);
	}
	// finally add it to the list
	saSystemAction.setcpExplicitConfirms.insert(pConcept);
}

// D: Signal an implicit confirmation on a concept
void CDMCoreAgent::SignalImplicitConfirmOnConcept(CConcept* pConcept)
{
	if (saSystemAction.setcpUnplannedImplicitConfirms.find(pConcept) !=
		saSystemAction.setcpUnplannedImplicitConfirms.end())
	{
		// if the concept is currently marked under an unplanned implicit 
		// confirm, then delete it from there 
		saSystemAction.setcpUnplannedImplicitConfirms.erase(pConcept);
	}
	// finally add it to the list
	saSystemAction.setcpImplicitConfirms.insert(pConcept);
}

// D: Signal an unplanned implicit confirmation on a concept
// D：表示对概念的计划外隐式确认
void CDMCoreAgent::SignalUnplannedImplicitConfirmOnConcept(int iState,	CConcept* pConcept)
{

	if (iState >= 0)
	{
		if (((*pStateManager)[iState].saSystemAction.setcpImplicitConfirms.find(pConcept) == (*pStateManager)[iState].saSystemAction.setcpImplicitConfirms.end()) 
			&&
			((*pStateManager)[iState].saSystemAction.setcpExplicitConfirms.find(pConcept) == (*pStateManager)[iState].saSystemAction.setcpExplicitConfirms.end()))
		{
			// if it's not already marked as being explicitly or implicitly confirmed
			// now log the current system action 
			// 如果它没有被标记为显式或隐式确认, 现在记录当前系统action
			Log(DMCORE_STREAM, FormatString("System action dumped below.\n%s",
				systemActionToString((*pStateManager)[iState].saSystemAction).c_str()));

			(*pStateManager)[iState].saSystemAction.setcpUnplannedImplicitConfirms.insert(pConcept);
		}
	}
	else
	{
		if ((saSystemAction.setcpImplicitConfirms.find(pConcept) ==	saSystemAction.setcpImplicitConfirms.end()) &&
			(saSystemAction.setcpExplicitConfirms.find(pConcept) ==	saSystemAction.setcpExplicitConfirms.end()))
		{
			// if it's not already marked as being explicitly or implicitly confirmed
			// now log the current system action 
			// 如果它没有被标记为显式或隐式确认, 现在记录当前系统操作
			Log(DMCORE_STREAM, FormatString("System action dumped below.\n%s",
				systemActionToString(saSystemAction).c_str()));

			saSystemAction.setcpUnplannedImplicitConfirms.insert(pConcept);
		}
	}

}

// D: Signal an accept action on a concept
//通知 我们在一个concept上做一个accept动作
void CDMCoreAgent::SignalAcceptOnConcept(CConcept* pConcept)
{
	// erase that agent from the explicit confirmations
	// but not from the implicit confirmations, since once an IC
	// is done, we want to perform an IC update on that concept
	// 从显式确认中删除该代理，但不从隐式确认中删除该代理，
	// 因为一旦IC完成，我们要对该概念执行IC更新
	saSystemAction.setcpExplicitConfirms.erase(pConcept);
}

// D: Returns the agent on top of the execution stack
// 返回执行栈栈顶的agent
CDialogAgent* CDMCoreAgent::GetAgentInFocus()
{
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin(); iPtr != esExecutionStack.end(); iPtr++)
	{
		if (iPtr->pdaAgent->IsExecutable())//可执行
		{
			return iPtr->pdaAgent;
		}
	}

	// o/w return NULL
	return NULL;
}

// D: Returns the task agent closest to the top of the execution stack
// 返回最靠近执行堆栈顶部的任务代理
CDialogAgent* CDMCoreAgent::GetDTSAgentInFocus()
{
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin();
		iPtr != esExecutionStack.end();
		iPtr++)
	{
		if (iPtr->pdaAgent->IsDTSAgent() && iPtr->pdaAgent->IsExecutable())
			return iPtr->pdaAgent;
	}
	// o/w if no task agent found, return NULL
	return NULL;
}

// D: Returns true if the specified agent is in focus
// D：如果指定的代理程序处于焦点，则返回true
bool CDMCoreAgent::AgentIsInFocus(CDialogAgent* pdaDialogAgent)
{

	// if it's not executable, return false
	if (!pdaDialogAgent->IsExecutable())
		return false;

	// if it's not a task agent
	if (!pdaDialogAgent->IsDTSAgent())
	{
		return GetAgentInFocus() == pdaDialogAgent;
	}

	// if it is a task agent then get the task agent in focus and check 
	// against that
	return GetDTSAgentInFocus() == pdaDialogAgent;
}

// D: Returns the agent that is previous on the stack
//    to the specified agent (i.e. the previously focused dts agent)
CDialogAgent* CDMCoreAgent::GetAgentPreviouslyInFocus(
	CDialogAgent* pdaDialogAgent)
{
	if (esExecutionStack.empty())
		return NULL;
	else
	{
		// start from the beginning
		TExecutionStack::iterator iPtr = esExecutionStack.begin();
		if (pdaDialogAgent == NULL)
			pdaDialogAgent = GetAgentInFocus();
		// advance till we find the agent
		while ((iPtr != esExecutionStack.end()) &&
			(iPtr->pdaAgent != pdaDialogAgent)) iPtr++;
		// if not found, return NULL
		if (iPtr == esExecutionStack.end())
			return NULL;
		// o/w advance till we find an executable one or the end
		iPtr++;
		while ((iPtr != esExecutionStack.end()) &&
			!iPtr->pdaAgent->IsExecutable())
			iPtr++;
		// if we're at the end return NULL
		if (iPtr == esExecutionStack.end())
			return NULL;
		// o/w return that agent
		else return iPtr->pdaAgent;
	}
}

// D: Returns the dialog task specification agent that is previous on the stack
//    to the specified agent (i.e. the previously focused dts agent)
CDialogAgent* CDMCoreAgent::GetDTSAgentPreviouslyInFocus(
	CDialogAgent* pdaDialogAgent)
{
	if (esExecutionStack.empty())
		return NULL;
	else
	{
		// start from the beginning
		TExecutionStack::iterator iPtr = esExecutionStack.begin();
		if (pdaDialogAgent == NULL)
			pdaDialogAgent = GetAgentInFocus();
		// advance till we find the agent
		while ((iPtr != esExecutionStack.end()) &&
			(iPtr->pdaAgent != pdaDialogAgent)) iPtr++;
		// if not found, return NULL
		if (iPtr == esExecutionStack.end())
			return NULL;
		// o/w advance one more
		iPtr++;
		// and do it until we reach a DTS agent
		while ((iPtr != esExecutionStack.end()) &&
			!iPtr->pdaAgent->IsDTSAgent()) iPtr++;
		// if we're at the end return NULL
		if (iPtr == esExecutionStack.end())
			return NULL;
		// o/w return that agent
		else return iPtr->pdaAgent;
	}
}

// D: Returns the current main topic agent
// A: The main topic is now identified by going down the stack
//    instead of using the tree
// D：返回当前主要 主题代理
// A：主题现在通过向下走堆栈而不是使用树来识别
CDialogAgent* CDMCoreAgent::GetCurrentMainTopicAgent()
{
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin();
		iPtr != esExecutionStack.end();
		iPtr++)
	{

		if (iPtr->pdaAgent->IsAMainTopic())
		{
			return iPtr->pdaAgent;
		}
	}

	// No main topic found on the stack (probably an error)
	return NULL;
}

// D: Returns true if the agent is an active topic
// D：如果代理是活动主题，则返回true  => 如果当前节点在执行栈中，返回True
bool CDMCoreAgent::AgentIsActive(CDialogAgent* pdaDialogAgent)
{
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin(); iPtr != esExecutionStack.end(); iPtr++)
	{
		if (iPtr->pdaAgent == pdaDialogAgent)
			return true;
	}
	return false;
}

// D: Eliminates a given agent from the execution stack
// D：从执行堆栈中消除给定代理
void CDMCoreAgent::PopAgentFromExecutionStack(CDialogAgent* pdaADialogAgent)
{

	// call upon the private helper function to eliminate the agent
	TStringVector vsAgentsEliminated;
	popAgentFromExecutionStack(pdaADialogAgent, vsAgentsEliminated);

	// if the agent was found, log and return 
	if (vsAgentsEliminated.size() != 0)
	{
		string sAgents;
		for (unsigned int i = 0; i < vsAgentsEliminated.size(); i++)
			sAgents += FormatString("%s\n", vsAgentsEliminated[i].c_str());

		sAgents = TrimRight(sAgents);

		Log(DMCORE_STREAM, "Eliminated %d agent(s) (dumped below) from the "\
			"execution stack.\n%s", vsAgentsEliminated.size(), sAgents.c_str());
	}
}

// D: Eliminates a given agent from the execution stack, together with all the
//    agents it has planned for execution
// D：从执行堆栈中删除给定代理，以及它计划执行的所有代理
void CDMCoreAgent::PopTopicFromExecutionStack(CDialogAgent* pdaADialogAgent)
{

	// call upon the private helper function to eliminate the agent
	// 调用私人帮助函数来消除代理
	TStringVector vsAgentsEliminated;
	popTopicFromExecutionStack(pdaADialogAgent, vsAgentsEliminated);

	// if the agent was found, log and return 
	if (vsAgentsEliminated.size() != 0)
	{
		string sAgents;
		for (unsigned int i = 0; i < vsAgentsEliminated.size(); i++)
			sAgents += FormatString("%s\n", vsAgentsEliminated[i].c_str());

		sAgents = TrimRight(sAgents);

		Log(DMCORE_STREAM, "Eliminated %d agent(s) (dumped below) from the "\
			"execution stack.\n%s", vsAgentsEliminated.size(), sAgents.c_str());
	}
}

// A: Eliminates all grounding agents (i.e. all agents schedules by the 
//    grounding manager) from the execution stack
void CDMCoreAgent::PopGroundingAgentsFromExecutionStack()
{

	// call upon the private helper function to eliminate the agent
	TStringVector vsAgentsEliminated;
	popGroundingAgentsFromExecutionStack(vsAgentsEliminated);

	// if the agent was found, log and return 
	if (vsAgentsEliminated.size() != 0)
	{
		string sAgents;
		for (unsigned int i = 0; i < vsAgentsEliminated.size(); i++)
			sAgents += FormatString("%s\n", vsAgentsEliminated[i].c_str());

		sAgents = TrimRight(sAgents);

		Log(DMCORE_STREAM, "Eliminated %d grounding agent(s) (dumped below) from the "\
			"execution stack.\n%s", vsAgentsEliminated.size(), sAgents.c_str());
	}
}

// A: Returns the last input turn number
int CDMCoreAgent::GetLastInputTurnNumber()
{
	return iTurnNumber;
}