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
// GAEXPLICITCONFIRM.CPP - implementation of the explicit confirm grounding 
//                         action class; this grounding action explicitly 
//                         confirms the top value of a concept
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
//   [2007-03-09] (antoine): fixed a _CRequestConfirm so that it takes its
//							 LM- and DTMF-related parameters from the
//							 configuration of its parent agency
//   [2007-01-01] (antoine): changed the _CExplicitConfirm agent into an
//							 agency with one RequestAgent and one InformAgent
//							 (for acknowledgement of the confirmation)
//   [2004-12-30] (antoine): allowed the configuration of the confirmation
//                           language model and the use of DTMF
//   [2004-12-28] (antoine): added constructor with configuration
//   [2003-02-13] (dbohus):  started working on this
// 
//-----------------------------------------------------------------------------

#include "GAExplicitConfirm.h"
#include "../../../DialogTask/DialogTask.h"

//-----------------------------------------------------------------------------
//
// D: Specification/implementation of the explicit confirm agency
//
//-----------------------------------------------------------------------------

DEFINE_AGENCY(_CExplicitConfirm,
	IS_NOT_DTS_AGENT()

	private:
		// D: the concept and value that are explicitly confirmed
		CConcept* pConfirmedConcept;

	public:
		// D: member function for setting the concept that is confirmed
		void SetConfirmedConcept(CConcept* pAConfirmedConcept)
		{
			pConfirmedConcept = pAConfirmedConcept;
		}

		// D: member function for accessing the concept that is confirmed
		CConcept* GetConfirmedConcept()
		{
			if (!pConfirmedConcept)
				FatalError("Confirmed concept in agent " + GetName() +
				" is NULL.");

			return pConfirmedConcept;
		}

		// D: member function for accessing the hypothesis that is confirmed
		// D：用于访问已确认的hypothesis的成员函数
		CHyp* GetConfirmedHyp()
		{
			if (!pConfirmedConcept)
				FatalError("Confirmed concept in agent " + GetName() +
				" is NULL.");
			else if (!pConfirmedConcept->GetTopHyp())
				FatalError("Confirmed hypothesis of concept " +
				pConfirmedConcept->GetAgentQualifiedName() + " is NULL.");

			return pConfirmedConcept->GetTopHyp();
		}

		// D: overwritten virtual function for concepts localization
		// D：覆盖虚函数用于概念本地化
		virtual CConcept& LocalC(string sConceptName)
		{
			if (sConceptName == pConfirmedConcept->GetName())
				return *pConfirmedConcept;
			else return CDialogAgency::LocalC(sConceptName);
		}

		// D: the confirm concept holds the response to the confirmation
		// D：confirm concept 保存对确认的响应
		DEFINE_CONCEPTS(
			BOOL_USER_CONCEPT(confirm, "none")
			)

			DEFINE_SUBAGENTS(
			SUBAGENT(RequestConfirm, _CRequestConfirm, "none")
			SUBAGENT(AcknowledgeConfirm, _CAcknowledgeConfirm, "none")
			)

			ON_INITIALIZATION(
			// (antoine) This is very hacky... The configuration does not 
			// contain the grounding model specification at creation time,  
			// only when Initialize is called by the Run method of the 
			// ExplicitConfirm grounding action, so we create the 
			// grounding model now instead of in the SUBAGENT directive
			//（antoine）这是非常hacky ...配置在创建时不包含接地模型规范，
			// 只有当Initialize由ExplicitConfirm接地动作的Run方法调用时，所以我们现在创建接地模型，而不是 SUBAGENT指令
		if (HasParameter("grounding_model"))
			SubAgents[0]->CreateGroundingModel(GetParameterValue("grounding_model"));
		)

			SUCCEEDS_WHEN(
			SUCCEEDED(RequestConfirm) &&
			(!IS_TRUE(confirm) || SUCCEEDED(AcknowledgeConfirm))
			)

			// D: on completion, update the confirmed concept value 
			//    accordingly
			// D：完成后，相应地更新确认的概念值
			ON_COMPLETION(
			// signal that the concept grounding has completed
			//表示概念接地已完成
			pGroundingManager->ScheduleConceptGrounding(pConfirmedConcept);
		)
			)//DEFINE_AGENCY(_CExplicitConfirm,
			//###############################################################################################################

			DEFINE_REQUEST_AGENT(_CRequestConfirm,
			IS_NOT_DTS_AGENT()

			// D: we're requesting the confirm concept
			REQUEST_CONCEPT(confirm)

			// A: the number of attempts before failing the ExplicitConfirm
			//    can be specified in the action's configuration
			virtual int GetMaxExecuteCounter()
		{
				if (!pdaParent->HasParameter("max_attempts"))
					return 10000;
				else
					return atoi(pdaParent->GetParameterValue("max_attempts").c_str());
			}

		// D: the prompt we're requesting it with is 
		// D：我们请求的提示
		virtual string Prompt()
		{
			_CExplicitConfirm* pdaEC = (_CExplicitConfirm*)pdaParent;
			return FormatString("{explicit_confirm %s %s=\"%s\"}",
				pdaEC->GetConfirmedConcept()->GetAgentQualifiedName().c_str(),
				pdaEC->GetConfirmedConcept()->GetName().c_str(),
				pdaEC->GetConfirmedHyp()->ValueToString().c_str());
		}

		// D: on execute, set the state of the grounding request to executing
		// D：on execute，将接收请求的状态设置为执行
		virtual TDialogExecuteReturnCode Execute()
		{
			_CExplicitConfirm* pdaEC = (_CExplicitConfirm*)pdaParent;
			// signal the core about the explicit confirm
			pDMCore->SignalExplicitConfirmOnConcept(pdaEC->GetConfirmedConcept());
			// and call the inherited execute
			// 实际执行函数
			return CMARequest::Execute();
		}

		// A: expects a yes or a no (or the equivalent DTMF keys)
	public:
		virtual string GrammarMapping()
		{
			string sGrammarMappingAsString = "![Yes]>true, ![No]>false";
			// Adds a DTMF key to the trigger expectation if specified
			// in the agent configuration
			if (pdaParent->HasParameter("dtmf_yes"))
			{
				sGrammarMappingAsString += ", ![" + pdaParent->GetParameterValue("dtmf_yes") + "]>true";
			}
			if (pdaParent->HasParameter("dtmf_no"))
			{
				sGrammarMappingAsString += ", ![" + pdaParent->GetParameterValue("dtmf_no") + "]>false";
			}

			return sGrammarMappingAsString;
		}

		// A: If a language model was specified in the configuration, use it
		//    otherwise use that of the parent agent
		// J: Renamed LanguageModel -> InputLineConfigurationInitString
		string InputLineConfigurationInitString()
		{
			string sLM = "";
			if (pdaParent->HasParameter("confirmation_lm"))
			{
				// LM from the grounding action configuration
				sLM = "set_lm=" + pdaParent->GetParameterValue("confirmation_lm");
			}

			// If DTMF is in use, expect 1 key at this point
			if (pdaParent->HasParameter("dtmf_yes"))
			{
				if (sLM.length() > 0) sLM += ", ";
				sLM += "set_dtmf_len=1";
			}

			return sLM;
		}

		SUCCEEDS_WHEN(
			!((_CExplicitConfirm*)pdaParent)->GetConfirmedConcept()->IsSealed() &&
			!pDMCore->LastTurnNonUnderstanding()
			//		(GetLastInputIndex() != -1) &&
			//		!pDMCore->LastTurnNonUnderstanding()
			)

			ON_COMPLETION(
			// This ensures that the agent remembers it's been completed once the
			// parent agency checks for success criteria (the conditions in SUCCEEDS_WHEN
			// might no longer hold then)
			SetCompleted(ctSuccess);
		)
			)//DEFINE_REQUEST_AGENT(_CRequestConfirm,
			//###################################################################################

			DEFINE_INFORM_AGENT(_CAcknowledgeConfirm,
			IS_NOT_DTS_AGENT()

			PRECONDITION(IS_TRUE(confirm))
			PROMPT("{ inform confirm_okay }")
			)

			//-----------------------------------------------------------------------------
			//
			// D: Implementation of the explicit confirm action
			//
			//-----------------------------------------------------------------------------

			// A: Constructor with configuration
			CGAExplicitConfirm::CGAExplicitConfirm(string sNewConfiguration) :
			CGroundingAction(sNewConfiguration)
		{
			}

		// D: Return the name of the grounding action
		string CGAExplicitConfirm::GetName()
		{
			return "EXPL_CONF";
		}

		// D: Run the action  -concept
		// 执行接地Action
		void CGAExplicitConfirm::Run(void *pParams)
		{

			// look for the agency for doing the explicit confirm
			// 寻找明确确认的agency
			string sAgencyName = FormatString("/_ExplicitConfirm[%s]", ((CConcept *)pParams)->GetAgentQualifiedName().c_str());
			_CExplicitConfirm *pExplicitConfirmAgency = (_CExplicitConfirm *)AgentsRegistry[sAgencyName];
			if (!pExplicitConfirmAgency)
			{
				// if not found, create it
				pExplicitConfirmAgency = (_CExplicitConfirm *)
					AgentsRegistry.CreateAgent("_CExplicitConfirm", sAgencyName);
				// create the grounding model
				if (s2sConfiguration.find("grounding_model") != s2sConfiguration.end())
				{
					pExplicitConfirmAgency->SetParameter("grounding_model", s2sConfiguration["grounding_model"]);
				}
				else
				{
					pExplicitConfirmAgency->SetParameter("grounding_model", "request_default");
				}
				// initialize it
				pExplicitConfirmAgency->Initialize();
				// and register it
				pExplicitConfirmAgency->Register();
			}
			else
			{
				// o/w reset it
				pExplicitConfirmAgency->Reset();
			}

			// sets the context to be the latest agent in focus
			//将上下文设置为焦点中的最新代理
			pExplicitConfirmAgency->SetContextAgent(
				pDMCore->GetAgentInFocus()->GetContextAgent());
			// set the dynamic agent ID to the name of the concept
			//将动态代理ID设置为概念的名称
			pExplicitConfirmAgency->SetDynamicAgentID(
				((CConcept *)pParams)->GetAgentQualifiedName());
			// sets the concept
			//设置概念
			pExplicitConfirmAgency->SetConfirmedConcept((CConcept *)pParams);
			// sets the configuration
			//设置配置
			pExplicitConfirmAgency->SetConfiguration(s2sConfiguration);

			// and add the explicit confirm agency on the stack
			// 并在堆栈中添加显式确认代理
			// 把动态创建的当前 显示确认Agency压栈执行
			pDMCore->ContinueWith(pGroundingManager, pExplicitConfirmAgency);

			// and set the grounding request to on_stack
			// 并将接收请求设置为on_stack
			pGroundingManager->SetConceptGroundingRequestStatus(
				(CConcept *)pParams, GRS_EXECUTING);
		}

		// D: Register the agencies used by this grounding action
		void CGAExplicitConfirm::RegisterDialogAgency()
		{
			if (!AgentsRegistry.IsRegisteredAgentType("_CExplicitConfirm"))
				AgentsRegistry.RegisterAgentType("_CExplicitConfirm",
				_CExplicitConfirm::AgentFactory);
			if (!AgentsRegistry.IsRegisteredAgentType("_CRequestConfirm"))
				AgentsRegistry.RegisterAgentType("_CRequestConfirm",
				_CRequestConfirm::AgentFactory);
			if (!AgentsRegistry.IsRegisteredAgentType("_CAcknowledgeConfirm"))
				AgentsRegistry.RegisterAgentType("_CAcknowledgeConfirm",
				_CAcknowledgeConfirm::AgentFactory);
		}