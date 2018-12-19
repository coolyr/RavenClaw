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
// GAASKREPEAT.CPP - implementation of the AskRepeat and NotifyAndAskRepeat 
//                   grounding action classes; this grounding action asks the 
//                   user to repeat what they said
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
//   [2004-12-28] (antoine): added constructor with configuration
//   [2004-03-01] (dbohus): added GNotifyAndAskRepeat
//   [2004-02-16] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#include "GAAskRepeat.h"
#include "../../../DialogTask/DialogTask.h"

//-----------------------------------------------------------------------------
//
// D: Specification/implementation of the ask repeat agency
//
//-----------------------------------------------------------------------------

DEFINE_EXECUTE_AGENT(_CAskRepeat,
	IS_NOT_DTS_AGENT() //非 DTS agent

	private:
		// the agent for which we are doing the ask repeat prompt
		// 我们正在为其执行请求重复提示的代理
		CMARequest *pRequestAgent;

	public:
		// D: set the request agent
		void SetRequestAgent(CMARequest *pARequestAgent)
		{
			pRequestAgent = pARequestAgent;
		}

		// D: return the request agent
		CMARequest* GetRequestAgent()
		{
			return pRequestAgent;
		}

		// D: the execute routine
		// D：执行例程
		virtual TDialogExecuteReturnCode Execute()
		{

			// if we need to do notification, do it
			// 如果我们需要做通知，做它
			if (GetParameterValue("notify") == "true")
			{
				if ((pDMCore->GetBindingHistorySize() > 1) && (pDMCore->GetBindingResult(-2).bNonUnderstanding))
				{
					// issue the subsequent non-understanding prompt
					// 发出 [后续] 的非理解提示
					pOutputManager->Output(this,	"inform subsequent_nonunderstanding", fsSystem);
				}
				else
				{
					// issue the inform non-understanding prompt
					//发出通知不理解提示
					pOutputManager->Output(this, "inform nonunderstanding", fsSystem);
				}
			}

			// issue the ask repeat prompt
			// 发出ask repeat提示
			pOutputManager->Output(this,	"request nonunderstanding_askrepeat", fsUser);
			// increment the execute counter on that agent
			// 增加该代理上的执行计数器
			pRequestAgent->IncrementExecuteCounter();
			// set this agent as completed
			// 将此代理设置为完成
			SetCompleted(ctSuccess);
			// and clean it off the execution stack (hacky)
			//并清除它的执行堆栈（hacky）
			pDMCore->PopAgentFromExecutionStack(this);
			// finally, return a request for a new input pass
			return dercTakeFloor;
		}//virtual TDialogExecuteReturnCode Execute()
)//DEFINE_EXECUTE_AGENT(_CAskRepeat,

			//-----------------------------------------------------------------------------
			//
			// D: Implementation of the ask repeat action
			//
			//-----------------------------------------------------------------------------

			// Constructor with configuration
			CGAAskRepeat::CGAAskRepeat(string sNewConfiguration) :	CGroundingAction(sNewConfiguration)
			{
			}

		// D: Return the name of the ask repeat action
		string CGAAskRepeat::GetName()
		{
			return "ASK_REPEAT";
		}

		// D: Run the action
		// D：运行操作
#pragma warning (disable:4100)
		void CGAAskRepeat::Run(void *pParams)
		{

			// look for the agency for doing the ask repeat
			// 寻找 agency 做询问重复
			_CAskRepeat *pAskRepeatAgent = (_CAskRepeat *)AgentsRegistry["/_AskRepeat"];
			if (!pAskRepeatAgent)
			{
				// if not found, create it
				// type : _CAskRepeat
				// name : /_AskRepeat
				pAskRepeatAgent = (_CAskRepeat *)
					AgentsRegistry.CreateAgent("_CAskRepeat", "/_AskRepeat");
				// initialize it
				pAskRepeatAgent->Initialize();
				// and register it
				pAskRepeatAgent->Register();
			}
			else
			{
				// o/w just reset it
				pAskRepeatAgent->Reset();
			}

			// set the dynamic agent ID to the name of the agent
			pAskRepeatAgent->SetDynamicAgentID(
				((CMARequest *)pParams)->GetName());
			// sets the agent for which we are doing the ask_repeat
			//设置我们正在执行ask_repeat的代理
			pAskRepeatAgent->SetRequestAgent((CMARequest *)pParams);
			// sets the configuration
			//设置配置
			pAskRepeatAgent->SetConfiguration(s2sConfiguration);

			// and add the ask repeat agent on the stack
			// 在执行堆栈上push一个新的对话代理
			pDMCore->ContinueWith(pGroundingManager, pAskRepeatAgent);
		}//run
#pragma warning (default:4100)

		// D: Register the agent used by this grounding action
		void CGAAskRepeat::RegisterDialogAgency()
		{
			if (!AgentsRegistry.IsRegisteredAgentType("_CAskRepeat"))
				AgentsRegistry.RegisterAgentType("_CAskRepeat",
				_CAskRepeat::AgentFactory);
		}
