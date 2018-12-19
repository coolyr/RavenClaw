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
// ���� TFloorStatus ������ǩ���ַ������� [ȫ�ֱ���]
vector<string> vsFloorStatusLabels;


//-----------------------------------------------------------------------------
// Constructors and Destructors
//-----------------------------------------------------------------------------

// D: constructor
//Ĭ�� - ���캯��
CDMCoreAgent::CDMCoreAgent(string sAName, string sAConfiguration, string sAType) : CAgent(sAName, sAConfiguration, sAType)
{
	bFocusClaimsPhaseFlag = false;				// indicates whether we should		����������־
	fsFloorStatus = fsSystem;					// indicates who has the floor		Floor״̬ - ö��
	iTurnNumber = 0;							// stores the current turn number	��¼��ǰturn��
	csoStartOverFunct = NULL;					// a custom start over function		�����û����Ƶ����������� ����[ָ��]
	//ȫ�ֱ��� - ����floor״̬
	vsFloorStatusLabels.push_back("unknown");	// ����TFloorStatus������ǩ���ַ�������
	vsFloorStatusLabels.push_back("user");
	vsFloorStatusLabels.push_back("system");
	vsFloorStatusLabels.push_back("free");
}

// D: virtual destructor - does nothing so far
//���� ��������
CDMCoreAgent::~CDMCoreAgent()
{
}

//-----------------------------------------------------------------------------
// Static function for dynamic agent creation
////��̬������̬����Agent
//-----------------------------------------------------------------------------
CAgent* CDMCoreAgent::AgentFactory(string sAName, string sAConfiguration)
{
	return new CDMCoreAgent(sAName, sAConfiguration);
}

//-----------------------------------------------------------------------------
// CAgent class overwritten methods
//-----------------------------------------------------------------------------

// D: the overwritten Reset method
//��д�����ǡ��ĸ�λ����
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
//	  �˺���ͨ��ִ�ж�ջ�ϵ�agent��ִ��ʵ�ʵĶԻ�����DT�������ʵ�ʱ��������׶�
//-----------------------------------------------------------------------------

void CDMCoreAgent::Execute()
{

	//		create & initialize the dialog task
	// <1>	���� �� ��ʼ�� Dialog Task Tree
	pDTTManager->CreateDialogTree();

	//		The floor is initially free
	// <2>	��ʼ��floor״̬ '����'       ö�٣�[δ֪��ϵͳ���û�������]
	SetFloorStatus(fsFree); // 'fsFree'
	   
	//		put the root of the dialog task on the execution stack
	// <3>	�Ѹ��ڵ�����ջ
	Log(DMCORE_STREAM, "Starting Dialog Task execution.");
	ContinueWith(this, pDTTManager->GetDialogTaskTreeRoot());

	//		creates the initial dialog state
	// <4>	��ʼ��[����]�Ի�״̬		[1-������װagenda   2-��¼��ǰ״̬	3-����input line cofig����Ϊ״̬һ����??]
	pStateManager->UpdateState();

	//		do the while loop for execution
	// <5>	��whileѭ��ִ�� - ��ִ��ջ�ǿ�ʱ������ִ��.
	while (!esExecutionStack.empty())//while (!esExecutionStack.empty())
	{
		//#############################################################################################################
		
		// <6>	��ȡsTurnId
		string sTurnId = "User:???";
		if (pInteractionEventManager->GetLastInput())	//��ȡ���user�����¼�ָ��
			sTurnId = "User:" + pInteractionEventManager->GetLastInput()->GetStringProperty("[uttid]");

		Log(DMCORE_STREAM, "Starting DMCore processing [%s]", sTurnId.c_str());

		//#############################################################################################################
		
		//		eliminate all the agents that have completed from the execution stack
		// <7>	����ִ����ɵ�agent����ջ  => agenda�޸Ĺ� -> ������װ
		popCompletedFromExecutionStack();
		Log(DMCORE_STREAM, "Eliminated completed agents from stack [%s]", sTurnId.c_str());
		
		//#############################################################################################################
		
		//		if the execution stack is empty now, break the loop
		// <8>	���ջ�� - �˳�ѭ�� [DTTִ�����]
		if (esExecutionStack.empty())
			break;
		
		//#############################################################################################################
		
		//		Performs grounding only when the floor is free and we got all our notifications
		//		ֻ�е�floor��free�����ǵõ������е�֪ͨ[RecentOutputsΪ��]����ִ��grouding
		// <9>	�������Ϊ������˵�����Ƕ��õ�֪ͨ
		if ((GetFloorStatus() == fsFree) && (pOutputManager->GetPromptsWaitingForNotification() == ""))//ִ�нӵ�grounding
		{
			/*
			// D: type describing a concept grounding request
			// D����������������������
				#define GRS_UNPROCESSED 0		// the unprocessed status for a grounding request	δ����̬			@
				#define GRS_PENDING     1		// the pending status for a grounding request		δ�������۶�		@
				#define GRS_READY       2       // the ready status for a grounding request			׼��̬
				#define GRS_SCHEDULED   3		// the scheduled status for a grounding request		����̬			@
				#define GRS_EXECUTING   5		// the executing status for a grounding request		ִ��̬
				#define GRS_DONE        6   	// the completed status for a grounding request		���̬
			*/

			//Turn Grouding	������⡿:							bTurnGroundingRequest			=> HasPendingRequests
			//concept Grouding ��request�׶�û�еõ���Ҫ��slot��: GRS_UNPROCESSED, GRS_PENDING	=> HasPendingRequests
			//concept Grouding:									 GRS_SCHEDULED				    => HasScheduledConceptGroundingRequests
			if (pGroundingManager->HasPendingRequests() || pGroundingManager->HasScheduledConceptGroundingRequests())
			{
				//#############################################ִ�� Grouding####################################################
				// <10>	ִ�нӵ� ��bTurnGroundingRequest=True || GRS_UNPROCESSED || GRS_PENDING || GRS_SCHEDULED��
				pGroundingManager->Run();//ִ�нӵ�
				//#############################################ִ�� Grouding####################################################
			}

			//		now pop completed
			// <11>	ɾ��ִ����ɵ�agent
			int iPopped = popCompletedFromExecutionStack();

			Log(DMCORE_STREAM, "Performed grounding [%s]", sTurnId.c_str());

			// now while new grounding requests appear, keep running the grounding process
			// <12>	���ڵ�[�µĽӵ�����]����ʱ���������нӵع���
			while ((iPopped > 0) && pGroundingManager->HasUnprocessedConceptGroundingRequests())	// D��ȷ���Ƿ����δ����ĸ����������
			{
				// run it
				// �������нӵز�����ɾ�����ִ�е�agent,���ܵ����µĽӵ����󣿡�
				pGroundingManager->Run();
				// eliminate all the agents that have completed (potentially as a 
				// result of the grounding phase) from the execution stack
				iPopped = popCompletedFromExecutionStack();
			}

			Log(DMCORE_STREAM, "Completed grounding on [%s]", sTurnId.c_str());
		}//ִ�нӵ�grounding

		//#############################################################################################################
		
		//		now, run the focus analysis process if the core was flagged to
		//		do so, and if there are no scheduled grounding activities
		// <13>	���ڣ����DMCore����Ϊִ�н���������̣�����û�е��ȵ�grouding Action�������н������[Foucs Analysis]����
		if (bFocusClaimsPhaseFlag)// ===>>  ͨ��Triiger������Agent���bFocusClaimsPhaseFlag�ó�True??  ��AcquireNextEvent���޸Ĺ���
		{
			//		Analyze the need for a focus shift, and resolve it if necessary
			// <14>	�����Ƿ���Ҫfocus shift�������Ҫ����
			if (assembleFocusClaims())
			{
				// <15>	�ѽ�������agent�ŵ�ջ��
				resolveFocusShift(); //�ѽ�������agent�ŵ�ջ��
			}
			//		reset the flag
			// <16>	���ñ�־Flag=False
			bFocusClaimsPhaseFlag = false;
		}
		//#############################################################################################################
		//		if the execution stack is empty now, break the loop
		// <17>	���ջΪ�գ� ����ѭ��
		if (esExecutionStack.empty())
			break;
		//#############################################################################################################
		//		grab the first (executable) dialog agent from the stack
		// <18>	�Ӷ�ջ��ץȡ��һ������ִ�У��Ի�Agent
		CDialogAgent* pdaAgentInFocus = GetAgentInFocus();

		//		check that we found a proper one (if there's nothing else to be executed, we're done)
		// <17>	��������ҵ�һ�����ʵģ����û������Ҫִ�У������˳�while��
		if (!pdaAgentInFocus) break;
		//#############################################################################################################
		//	if the floor is not free (or we're still waiting for notifications), do not execute agents that require the floor, just wait for the next event
		// <18>	���Floor��free�ģ��������ǻ��ڵȴ�֪ͨ������Ҫִ����ҪFloor��agent��ֻ��ȴ���һ��event
		// request �� inform ��ִ���ڼ�ʼ����ҪFloor[true]
		if (pdaAgentInFocus->RequiresFloor() &&
			!((GetFloorStatus() == fsFree) && (pOutputManager->GetPromptsWaitingForNotification() == ""))
			)
		{
			AcquireNextEvent();	// <17>	�ȴ���һ��event
			continue;			// <18>	�����´�ѭ��
		}
		//#############################################################################################################
		// and execute it
		// ִ��agent
		Log(DMCORE_STREAM, "Executing dialog agent %s [%s]", pdaAgentInFocus->GetName().c_str(), sTurnId.c_str());
		//		mark the time it was executed
		// <19>	��¼ʵ�ʱ�ִ�е�ʱ��
		ehExecutionHistory[esExecutionStack.front().iEHIndex].vtExecutionTimes.push_back(GetTime());
		//		execute it
		// <20>	ʵ��ִ��
		//########################################ʵ�� ִ��  Execute########################################################
		TDialogExecuteReturnCode dercResult = pdaAgentInFocus->Execute();
		//########################################ʵ�� ִ��  Execute########################################################
		ehExecutionHistory[esExecutionStack.front().iEHIndex].bExecuted = true;//����ʷ�ִ��
		//#############################################################################################################

		/*
		// D: definition for return codes on dialog agent execution
		// D���Ի�����ִ��ʱ �������롿�Ķ���
		typedef enum
		{
			dercContinueExecution,		// continue the execution			<1>	����ִ��
			dercYieldFloor,				// gives the floor to the user		<2>	��floor��user
			dercTakeFloor,				// takes the floor					<3>	����floor
			dercWaitForEvent,			// waits for a real-world event		<4>	�ȴ� [��ʵ����] �¼�
			dercFinishDialog,			// terminate the dialog				<5>	��ֹ�Ի�
			dercFinishDialogAndCloseSession,// terminate the dialog
			// and sends a close session message to the hub					<6>	��ֹ�Ի������������͹رջỰ��Ϣ
			dercRestartDialog            // restart the dialog				<6>	�����ػ�
		} TDialogExecuteReturnCode;
		*/
		//################################################################################################################
		//		and now analyze the return
		// <21>	����ִ�� return Code
		switch (dercResult)
		{
		case dercContinueExecution:  //[Agency, Expect, Execute]   [help, NonUnderstanding]
			// continue the execution		����ִ��
			break;//ִ��ջ��һ��Agent

		case dercFinishDialog:		//[Terminate]
			// finish the dialog			��ֹ�Ի�
			Log(DMCORE_STREAM, "Dialog Task Execution completed. Dialog finished");
			return;

		case dercFinishDialogAndCloseSession:   //[quit, TerminateAndCloseSession, Timeout, Giveup,..]
			// tell the hub to close the session	��ֹ�Ի������������͹رջỰ��Ϣ
			Log(DMCORE_STREAM, "Sending close_session to the hub");
			DMI_SendEndSession();
			// finish the dialog
			Log(DMCORE_STREAM, "Dialog Task Execution completed. Dialog finished");
			return;

		case dercRestartDialog:		//[startOver, ...]
			// call the start over routine		�����Ի�
			StartOver();
			break;

		case dercYieldFloor:			//[help, YieldTurn,..]

			// gives the floor to the user		��floor��user
			SetFloorStatus(fsUser);
			//#####################################################################
			// wait for the next event			�ȴ��¼�
			AcquireNextEvent();
			//#####################################################################
			break;

		case dercTakeFloor:		//[Inform,  Request]  [InformHelp, nounderstanding, repeat, suspend, timeout, askrepeat, askrephrase, moveon, whatCanISay..]

			// gives the floor to the system	��floor��system
			SetFloorStatus(fsSystem);
			//#####################################################################
			// wait for the next event			�ȴ��¼�
			AcquireNextEvent();
			//#####################################################################
			break;

		case dercWaitForEvent:					
			//#####################################################################
			// wait for the next event		    �ȴ��¼�
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
// A���ȴ���������һ����ʵ�����¼�
//-----------------------------------------------------------------------------
#pragma warning (disable:4127)
void CDMCoreAgent::AcquireNextEvent()//�ȴ���һ���¼���AcquireNextEvent()
{
	// <1>	�ȴ������¼��ӽ�������������[ ��ǰ��δ�����¼��Ķ���Ϊ��,�ȴ�һ���¼�������ӵ�δ������� ]
	pInteractionEventManager->WaitForEvent();
	//		Unqueue event
	// <2>	�ӵ�ǰ��δ�����¼��Ķ��л�ȡ��һ���¼�[���ѻ�ȡ��event���뵽History]
	CInteractionEvent *pieEvent = pInteractionEventManager->GetNextEvent();

	//##################################�� concept###########################################
	//		try and bind concepts 
	// <3>	���԰�concept
	TBindingsDescr bdBindings;
	bindConcepts(bdBindings);
	//##################################�� concept###########################################

	//		add the binding results to history
	// <4>	���󶨽����ӵ���ʷ��¼
	bhBindingHistory.push_back(bdBindings);

	//		Set the bindings index on the focused agent
	// <5>	�ڹ�ע�Ĵ��������ð�����
	GetAgentInFocus()->SetLastBindingsIndex(bhBindingHistory.size() - 1);

	//		signal the need for a focus claims phase
	// <6>	���ź� ��Ҫһ�����������׶�[focus claims phase]//###################??????????
	SignalFocusClaimsPhase();

	Log(DMCORE_STREAM, "Acquired new %s event.", pieEvent->GetType().c_str());

	/*
	typedef enum
	{
		fsUnknown,		// floor owner is unknown		δ֪
		fsUser,			// floor owner is user			�û�
		fsSystem,		// floor owner is system		ϵͳ
		fsFree,			// floor is not owned by anyone	����
	} TFloorStatus;
	*/
	//		update the floor status if the event specifies it
	// <7>	����¼�ָ����Floor Status������Floor״̬Ϊ�¼�ָ����
	if (pieEvent->HasProperty("[floor_status]"))
	{
		SetFloorStatus(StringToFloorStatus(pieEvent->GetStringProperty("[floor_status]")));
	}

	/*
		#define IET_DIALOG_STATE_CHANGE	"dialog_state_change"		//�Ի�״̬�ı�
		#define IET_USER_UTT_START	"user_utterance_start"			//�û���ʼ
		#define IET_USER_UTT_END	"user_utterance_end"			//�û�����
		#define IET_PARTIAL_USER_UTT "partial_user_utterance"
		#define IET_SYSTEM_UTT_START	"system_utterance_start"	//ϵͳ��ʼ
		#define IET_SYSTEM_UTT_END	"system_utterance_end"			//ϵͳ����
		#define IET_SYSTEM_UTT_CANCELED	"system_utterance_canceled"	
		#define IET_FLOOR_OWNER_CHANGES "floor_owner_changes"
		#define IET_SESSION "session"
		#define IET_GUI "gui"										//����
	*/
	//		Process event
	// <8>	���ݲ�ͬ���¼����ͣ�����event
	if (pieEvent->GetType() == IET_USER_UTT_START)// (1)	�û���ʼ user_utterance_start
	{

	}
	else if (pieEvent->GetType() == IET_USER_UTT_END || pieEvent->GetType() == IET_GUI)//(2)	�û����� user_utterance_end
	{

		if (pieEvent->IsComplete())//event���
		{
			// Set the last input on the focused agent
			// ���ý���agent�ϵ����һ��input
			GetAgentInFocus()->SetLastInputIndex(iTurnNumber);// �������TurnNumber
			GetAgentInFocus()->IncrementTurnsInFocusCounter();// ��ʾagent���ϴΡ�����/���´򿪡������ж��ٴλ�ý��� iTurnsInFocusCounter

			iTurnNumber++;//TurnNumber��һ

			//###############################Turn Grouding#########################################
			// signal the need for a turn grounding
			// ��־��ҪTurn�ӵ�  ��?��
			pGroundingManager->RequestTurnGrounding();
			//###############################Turn Grouding#########################################

			Log(DMCORE_STREAM, "Processed new input [User:%s]", pieEvent->GetStringProperty("[uttid]").c_str());

			return;
		}

	}
	else if (pieEvent->GetType() == IET_SYSTEM_UTT_START)//(3)	ϵͳ��ʼ system_utterance_start
	{

		// sends notification information to the OutputManager
		// void COutputManagerAgent::PreliminaryNotify(int iOutputId, string sTaggedUtt)
		// ��֪ͨ��Ϣ���͵�OutputManager    [Preliminary:����]
		// ��ǵ�������� =>  sTaggedUtt 
		pOutputManager->PreliminaryNotify(
			pieEvent->GetIntProperty("[utt_count]"),
			pieEvent->GetStringProperty("[tagged_prompt]"));

		Log(DMCORE_STREAM, "Processed preliminary output notification.");
	}
	else if (pieEvent->GetType() == IET_SYSTEM_UTT_END)// (4)	ϵͳ���� system_utterance_end
	{

		// sends notification information to the OutputManager
		// ��֪ͨ��Ϣ���͵�OutputManager
		pOutputManager->Notify(pieEvent->GetIntProperty("[utt_count]"),
			pieEvent->GetIntProperty("[bargein_pos]"),
			pieEvent->GetStringProperty("[conveyance]"),
			pieEvent->GetStringProperty("[tagged_prompt]"));

		Log(DMCORE_STREAM, "Processed output notification.");
	}
	else if (pieEvent->GetType() == IET_SYSTEM_UTT_CANCELED)// (5)	ϵͳ���� system_utterance_canceled
	{

		// sends notification information to the OutputManager
		//��֪ͨ��Ϣ���͵�OutputManager
		pOutputManager->Notify(pieEvent->GetIntProperty("[utt_count]"), 0, "", "");

		Log(DMCORE_STREAM, "Output cancel notification processed.");
	}
	else if (pieEvent->GetType() == IET_DIALOG_STATE_CHANGE)// (6)	�Ի�״̬�ı� dialog_state_change
	{

		pStateManager->UpdateState();		//����״̬ [ִ��Stack�ı�]
		pStateManager->BroadcastState();	//��״̬�㲥��ϵͳ�е��������

	}
	else
	{
		//#define IET_PARTIAL_USER_UTT "partial_user_utterance"  �����û�˵��
		//#define IET_FLOOR_OWNER_CHANGES "floor_owner_changes"  Floor�����߸ı�
		//#define IET_SESSION "session"							 �Ựsession
	}
}//�ȴ���һ���¼���AcquireNextEvent()




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
// ��װ����agenda
void CDMCoreAgent::assembleExpectationAgenda()
{

	Log(DMCORE_STREAM, "Expectation Agenda Assembly Phase initiated.");

	//		first collect and compile the expectation agenda
	// <1>	�����ռ��ͱ�������Agenda
	compileExpectationAgenda();

	//		then enforce the binding policies as specified on each level
	// <2>	Ȼ��ǿ��ִ��ÿ��level��ָ���İ󶨲���
	enforceBindingPolicies();

	//		dump agenda to the log
	// <3>	log��¼agenda
	Log(EXPECTATIONAGENDA_STREAM, "Concept expectation agenda dumped below:"
		+ expectationAgendaToString());

	Log(DMCORE_STREAM, "Expectation Agenda Assembly Phase completed "\
		"(%d levels).", eaAgenda.vCompiledExpectations.size());
}

// D: gathers the expectations and compiles them in an fast accessible form
// D: definition of an internal type: a set of pointers to an agent

//	D���ռ�����, ����֯�ɿ��ٷ��ʵĽṹ
//	D���ڲ����͵Ķ��壺һ��ָ������ָ��
typedef set<CDialogAgent*, less <CDialogAgent*>, allocator <CDialogAgent*> >
TDialogAgentSet;

void CDMCoreAgent::compileExpectationAgenda()
{

	// log the activity
	Log(DMCORE_STREAM, "Compiling Expectation Agenda ...");

	//		first clear up the last agenda
	// <1>	����ϴε�agenda
	eaAgenda.celSystemExpectations.clear();
	eaAgenda.vCompiledExpectations.clear();

	// get the list of system expectations. To do this, we traverse 
	// the execution stack, and add expectations from all the agents, each 
	// on the appropriate level; also keep track of the expectations
	// that are already declared so as not to duplicate them by this
	// traversal
	// ���ϵͳ�������б� Ϊ��������һ�㣬���Ǳ���ִ�ж�ջ��
	// ��������еĴ����������ÿ�����ʵ���ˮƽ; Ҳ�����Ѿ��������������Ա㲻����������ظ�
	int iLevel = 0;
	TDialogAgentSet setPreviouslySeenAgents;	// the set of agents already	ǰһ��οɼ�agent
	// seen on the previous levels
	TDialogAgentSet setCurrentlySeenAgents;		// the set of agents seen		��ǰ��οɼ���agent
	// on the current level

	// <2>	����ִ�ж�ջ
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin(); iPtr != esExecutionStack.end(); iPtr++)//����ջ
	{

		//		remember how big the system expectation agenda was so far
		// <3>	��¼��ǰAgenda�Ĵ�Сsize
		int iStartIndex = eaAgenda.celSystemExpectations.size();


		//################### DeclareExpectations ##############################
		//		gather expectations of the agent on the stack indicated by iPtr
		// <4>	�ռ���ǰָ��ָ��Ĵ�������� [��ݹ��ռ���ǰagent������ subagent]
		iPtr->pdaAgent->DeclareExpectations(eaAgenda.celSystemExpectations);// => ���� ����ʵ��
		//################### DeclareExpectations ##############################


		//		now go thourgh those new expectations and compile them (create 
		//		the corresponding entry into the vCompiledExpectations array)
		// <5>	���ڱ�����Щ�µ��������������ǣ���vCompiledExpectations�����д�����Ӧ����Ŀ��
		//		���ֲ�ͬlevel�����������飨������0�ϣ���focus agent��������������1�ϵ�ֱ���ϲ�agent�������ȣ�
		TCompiledExpectationLevel celLevel;
		// set the agent that generated this level
		celLevel.pdaGenerator = iPtr->pdaAgent;
		// <6>	������ǰagent��ӵ�expection
		for (unsigned int i = iStartIndex; i < eaAgenda.celSystemExpectations.size(); i++)//������ǰagent��ӵ�expection
		{

			//		check that the agent was not already seen on the previous
			//		level (in this case, avoid duplicating its expectation)
			// <7>	����������һ�������Ƿ��Ѿ�����������������£������ظ���������
			if (setPreviouslySeenAgents.find(eaAgenda.celSystemExpectations[i].pDialogAgent) != setPreviouslySeenAgents.end())
			{
				continue;
			}

			//		insert this agent in the list of currently seen agents
			// <8>	�ڵ�ǰ�鿴�Ĵ����б��в���˴���
			setCurrentlySeenAgents.insert(eaAgenda.celSystemExpectations[i].pDialogAgent);

			string sSlotExpected = eaAgenda.celSystemExpectations[i].sGrammarExpectation;//slotName ���š�����

			TMapCE::iterator iPtr2;
			// <9>	��ǰ��celLevel��slot Name
			if ((iPtr2 = celLevel.mapCE.find(sSlotExpected)) != celLevel.mapCE.end())
			{
				// if this grammar slot is already expected at this level
				// just add to the vector of pointers
				// <10>		�������﷨���Ѿ�Ԥ�����������, ֻ����ӵ�index������
				TIntVector& rvIndices = (*iPtr2).second;
				rvIndices.push_back(i);
			}
			else
			{
				//		if the concept is NOT already expected at this level
				//		then add it to the hash of compiled expectations
				// <11>	����ø�����δ�ڴ�level��������ӵ����������Ĺ�ϣ
				TIntVector ivTemp;
				ivTemp.push_back(i);
				celLevel.mapCE.insert(TMapCE::value_type(sSlotExpected, ivTemp));
			}
		}//������ǰagent��ӵ�expection

		//		finally, we have assembled and compiled this level of expectations,
		//		push it on the array, 
		// <12>	���������װ�����������level�����������͵�array�ϣ�
		eaAgenda.vCompiledExpectations.push_back(celLevel);

		//		update the set of already seen agents
		// <13>	���µĴ��� = ��ֹ�ظ����
		setPreviouslySeenAgents.insert(setCurrentlySeenAgents.begin(), setCurrentlySeenAgents.end());

		// <14>	and move to the next level
		iLevel++;
	}//����ջ

	// log the activity
	Log(DMCORE_STREAM, "Compiling Expectation Agenda completed.");
}

// D: goes through the compiled agenda, and modifies it according to the 
//    binding policies as specified by each level's generator agents
// D�������������̣�������ÿ��level������������ָ���İ󶨲����޸���
void CDMCoreAgent::enforceBindingPolicies()
{

	// log the activity
	Log(DMCORE_STREAM, "Enforcing binding policies ...");

	// at this point, this only consists of blocking the upper levels if a 
	// WITHIN_TOPIC_ONLY policy is detected
	// ����һ���ϣ������⵽WITHIN_TOPIC_ONLY���ԣ���ֻ���������ϲ�
	for (unsigned int i = 0; i < eaAgenda.vCompiledExpectations.size(); i++)//����ÿ�㡾ÿ��agent��
	{
		// get the binding policy for this level
		/*
		D: Topic-initiative: bind concepts only within the current topic / focus
		Topic-initiative�����ڵ�ǰ topic/focus �ڰ󶨸���
		#define WITHIN_TOPIC_ONLY "bind-this-only"

		D: Mixed-initiative: bind anything
		#define MIXED_INITIATIVE "bind-anything"
		*/
		string sBindingPolicy = eaAgenda.vCompiledExpectations[i].pdaGenerator->DeclareBindingPolicy();
		if (sBindingPolicy == WITHIN_TOPIC_ONLY)//"WITHIN_TOPIC_ONLY"
		{
			// if WITHIN_TOPIC_ONLY, then all the expectations from the upper
			// levels of the agenda are disabled
			// ���WITHIN_TOPIC_ONLY����ôupper level��̵�����������������
			// ���бȵ�ǰlevel��Ķ�����Ϊdisable
			for (unsigned int l = i + 1; l < eaAgenda.vCompiledExpectations.size(); l++)//upper level
			{
				// go through the whole level and disable all expectations
				// �������е�level, disable����
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
			// ����ѭ������Ϊ�Ѿ��������ϲ��policy��ʲô�� [��Ϊ���Ǵ��ҵ��ĵ�һ��WITHIN_TOPIC_ONLY��
			// ���������е�concept����Ϊ��disable, ����agenda��ÿ�θ���ջ��ʱ�����´����ġ�]
			break;
		}//"WITHIN_TOPIC_ONLY"
	}//����ÿ�㡾ÿ��agent��

	// log the activity
	Log(DMCORE_STREAM, "Enforcing binding policies completed.");
}

// D: generates a string representation of the expectation agenda
//    this string is used so far only for logging purposes
// D�����������ճ̵��ַ�����ʾ,
//    ���ַ�����ĿǰΪֹ��������־Ŀ��
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
// D�����ɰ��������ַ�����ʾ
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
// D�����ݵ�ǰ�������Խ���������ĸ���󶨵���̣�
//    Ȼ����bhiResults�з��ذ󶨳ɹ�/ʧ�ܵ�����
void CDMCoreAgent::bindConcepts(TBindingsDescr& rbdBindings)
{
	/*
	typedef struct
	{
		string sEventType;				// the type of event 						event���� "IET_USER_UTT_START", ...
		bool bNonUnderstanding;         // was the turn a non-understanding?		�Ƿ�no-understanding [���磺û��concept��]
		int iConceptsBound;             // the number of bound concepts				�󶨵�concept��Ŀ
		int iConceptsBlocked;           // the number of blocked concepts			����Concept��
		int iSlotsMatched;				// the number of slots that matched			ƥ���slot��
		int iSlotsBlocked;				// the number of slots that were blocked	������slot��


		vector<TBinding> vbBindings;	// the vector of bindings					��vector
		vector<TForcedConceptUpdate> vfcuForcedUpdates;								ǿ�Ƹ��µ�����
		// the vector of forced updates
	} TBindingsDescr;
	*/

	Log(DMCORE_STREAM, "Concepts Binding Phase initiated.");

	//		initialize to zero the number of concepts bound and blocked
	// <1>	���󶨺������ĸ�������ʼ��Ϊ��
	rbdBindings.sEventType = pInteractionEventManager->GetLastEvent()->GetType(); //event���� "IET_USER_UTT_START", ...
	rbdBindings.iConceptsBlocked = 0;
	rbdBindings.iConceptsBound = 0;
	rbdBindings.iSlotsMatched = 0;
	rbdBindings.iSlotsBlocked = 0;

	// hash which stores the slots that matched and how many times they did so
	// hash�洢ƥ���slot����Ӧ�Ĵ���
	map<string, int> msiSlotsMatched;

	// hash which stores the slots that were blocked and how many times they were blocked
	// hash�洢������slot�����ǵĴ���
	map<string, int> msiSlotsBlocked;

	//		go through each concept expectation level and try to bind things
	// <2>	����ÿ����������level�����԰�
	for (unsigned int iLevel = 0; iLevel < eaAgenda.vCompiledExpectations.size(); iLevel++)//ÿ�㣺vCompiledExpectations
	{

		/*
		typedef struct
		{
			TMapCE mapCE;					// the hash of compiled expectations	���������Ĺ�ϣ
			CDialogAgent* pdaGenerator;		// the agent that represents that level	����ǰlevel�����Ĵ���
		} TCompiledExpectationLevel;
		*/
		//		go through the hash of expected slots at that level
		// <3>	������level��Ԥ��slot��ɢ��
		TMapCE::iterator iPtr;
		for (iPtr = eaAgenda.vCompiledExpectations[iLevel].mapCE.begin();
			iPtr != eaAgenda.vCompiledExpectations[iLevel].mapCE.end();
			iPtr++)
		{
			//   slotName -> slotIndex[Vecor<int>]
			string sSlotExpected = iPtr->first;	    // the grammar slot expected
			TIntVector& rvIndices = iPtr->second;	// indices in the system expectation list

			//		if the slot actually exists in the parse, then try to bind it
			// <4>	���slotʵ���ϴ�����input�����У����԰���
			//######################################################################################################################
			if (pInteractionEventManager->LastEventMatches(sSlotExpected))//���event����ƥ���˵�ǰ�� sSlotExpected[slotName]
			{

				Log(DMCORE_STREAM, "Event matches %s.", sSlotExpected.c_str());

				// go through the array of indices and construct another array
				// which contains only the indices of "open" expectations, 
				// excluding any expectations that redundanly match to the same concept
				//�����������鲢������һ����������open�����������������飬�������κ�����ͬ��������ƥ�������
				TIntVector vOpenIndices;
				set<CConcept *> scpOpenConcepts;

				// also construct another array which contains the indices of 
				// "closed" expectations, excluding any expectations that 
				// redundantly match the same concept
				// Ҳ������һ�����飬�������closed���������������ų��κ����������ƥ����ͬ�ĸ���
				TIntVector vClosedIndices;
				set<CConcept *> scpClosedConcepts;

				// <5>	������ǰslot��index vector
				for (unsigned int i = 0; i < rvIndices.size(); i++)//ֻһ��������slotΪ��λ����
				{
					//		determine the concept under consideration
					// <6>	��ȡslot���󶨵ĵ�concept
					CConcept* pConcept =
						&(eaAgenda.celSystemExpectations[rvIndices[i]].pDialogAgent->C(eaAgenda.celSystemExpectations[rvIndices[i]].sConceptName));

					// test that the expectation is not disabled
					// <7>	��������δ���� - ����
					if (!eaAgenda.celSystemExpectations[rvIndices[i]].bDisabled)
					{
						if (scpOpenConcepts.find(pConcept) == scpOpenConcepts.end())
						{
							// add it to the open indices list
							//������ӵ�open�������б�
							vOpenIndices.push_back(rvIndices[i]);
							// add the concept to the open concepts list
							//��������ӵ�open�ĸ����б���
							scpOpenConcepts.insert(pConcept);
							// if by any chance it's already in the closed concepts, 
							// ����κλ������Ѿ���close�ĸ��
							set<CConcept *>::iterator iPtr;
							if ((iPtr = scpClosedConcepts.find(pConcept)) != scpClosedConcepts.end())
							{
								// remove it from there
								// �Ƴ�
								scpClosedConcepts.erase(iPtr);
							}
						}
					}//����
					else// <8>	������
					{
						// o/w if the expectation is disabled
						// o / w�������������
						if ((scpClosedConcepts.find(pConcept) == scpClosedConcepts.end()) &&
							(scpOpenConcepts.find(pConcept) == scpOpenConcepts.end()))
						{
							// add it to the closed indices list
							// ������ӵ���������б���
							vClosedIndices.push_back(rvIndices[i]);
							// add the concept to the closed concepts list
							scpClosedConcepts.insert(pConcept);
						}
					}//������
				}//for (unsigned int i = 0; i < rvIndices.size(); i++)//ֻһ��������slotΪ��λ����

				// the slot value to be bound
				// Ҫ�󶨵Ĳ�ֵ
				string sSlotValue;

				//		and the confidence score
				// <9>	event���Ŷȵ÷� [Ĭ������Ϊ 1.0f]
				float fConfidence = pInteractionEventManager->GetLastEventConfidence();

				// <10>	���ƥ�䵽����concept
				if (vOpenIndices.size() > 0)//���ƥ�䵽����concept
				{
					//		check that the confidence is strictly above the current nonunderstanding threshold
					// <11>	������Ŷ��Ƿ��ϸ���ڵ�ǰ�Ĳ��ɺ�����ֵ => event��confidence����fNonunderstandingThreshold
					if (fConfidence > fNonunderstandingThreshold)//event��confidence����fNonunderstandingThreshold
					{
						//		check for multiple bindings on a level
						// <12>	�������binding���
						if (vOpenIndices.size() > 1)
						{
							// if there are multiple bindings possible, log that 
							// as a warning for now *** later we need to deal with 
							// this by adding disambiguation agencies
							//����ж���󶨿��ܣ���¼��Ϊ��������***��������Ҫͨ���������agencies
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
						}/// �������binding��� => ֻ�󶨵���һ��agent

						//		now bind the grammar concept to the first agent expecting this slot; obtain the value for that grammar slot
						// <13>	���ڽ��﷨����󶨵�������slot�ĵ�һ��agent; 
						//		��event�л�ȡ��slot��ֵ
						sSlotValue = pInteractionEventManager->GetValueForExpectation(sSlotExpected);//sSlotExpected[slotName]

						//####################################ִ�� binding###################################################
						//		do the actual concept binding
						// <14>	��ʵ�ʵĸ���� [�󶨵�������slot�ĵ�һ��agent��concept]
						performConceptBinding(
							sSlotExpected,
							sSlotValue,
							fConfidence,
							vOpenIndices[0],
							pInteractionEventManager->LastEventIsComplete());
						//####################################ִ�� bingding###################################################

						//		now that we've bound at this level, invalidate this expected slot on all the other levels
						// <15>	���������Ѿ������������ʹ������������Ĵ�Ԥ�ڲ�slotʧЧ
						for (unsigned int iOtherLevel = iLevel + 1; iOtherLevel < eaAgenda.vCompiledExpectations.size(); iOtherLevel++)
						{
							eaAgenda.vCompiledExpectations[iOtherLevel].mapCE.erase(sSlotExpected);// Agenda��ʽ������������������
						}
					}//if(fConfidence > fNonunderstandingThreshold)//event��confidence����fNonunderstandingThreshold
					else
					{
						// o/w the confidence is below the nonunderstanding
						// threshold, so we will reject this utterance by
						// basically not binding anything
						// <16>	���ĵ��ڷǲ������ֵ��������ǽ�ͨ�������ϲ����κζ������ܾ��������
					}
				}//if (vOpenIndices.size() > 0)//���ƥ�䵽����concept

				//		write the open binding description (only for the first concept, the one that actually bound)
				// <17>	дopen�İ�������ֻ��Ե�һ��concept��ʵ�ʰ󶨵��Ǹ���
				for (unsigned int i = 0; i < vOpenIndices.size(); i++)
				{
					if (i == 0)//ֻ��Ե�һ��concept
					{
						//		check that the confidence is strictly above the current nonunderstanding threshold
						// <18>	������Ŷ��Ƿ��ϸ���ڵ�ǰ�Ĳ��ɺ�����ֵ
						if (fConfidence > fNonunderstandingThreshold)
						{
							/*
							// D: structure describing one particular binding
							// D������һ���ض���ϵĽṹ
							typedef struct
							{
								bool bBlocked;					// indicates whether the binding was			�Ƿ�����
								//  blocked or not
								string sGrammarExpectation;		// the expected grammar slot					�������﷨��
								string sValue;					// the value in the binding						�󶨵�ֵ
								float fConfidence;				// the confidence score for the binding			�󶨵����ŷ�
								int iLevel;						// the level in the agenda						agenda��level
								string sAgentName;				// the name of the agent that declared the		����������agent
								//  expectation
								string sConceptName;			// the name of the concept that will bind		�󶨵�concept��
								string sReasonDisabled;			// if the binding was blocked, the reason		���disable,˵��ԭ��
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
							rbdBindings.iConceptsBound++;//�󶨵�concept��Ŀ
							// add the slot to the list of matched slots 
							// ����slot��ӵ�ƥ���slot�б�
							msiSlotsMatched[bBinding.sGrammarExpectation] = 1;	// hash�洢ƥ���slot����Ӧ�Ĵ���
							// in case we find it in the blocked slots, (it could have gotten
							// there on an earlier level) delete it from there
							// ������Ƿ�������������slot�����������Ѿ��õ����ڽ����level��ɾ����������
							map<string, int>::iterator iPtr;
							if ((iPtr = msiSlotsBlocked.find(bBinding.sGrammarExpectation)) != msiSlotsBlocked.end())
							{
								msiSlotsBlocked.erase(iPtr);	// hash�洢������slot�����ǵĴ���
							}
						}
						else
						{
							//		o/w if the confidence is not above the threshold
							// <19>	������ֵ
							TBinding bBlockedBinding;
							bBlockedBinding.bBlocked = true;
							bBlockedBinding.iLevel = iLevel;
							bBlockedBinding.fConfidence = fConfidence;
							bBlockedBinding.sAgentName = eaAgenda.celSystemExpectations[vOpenIndices[i]].pDialogAgent->GetName();
							bBlockedBinding.sConceptName = eaAgenda.celSystemExpectations[vOpenIndices[i]].sConceptName;
							bBlockedBinding.sGrammarExpectation = eaAgenda.celSystemExpectations[vOpenIndices[i]].sGrammarExpectation;
							bBlockedBinding.sReasonDisabled = "confidence below nonunderstanding threshold";//������ԭ��confidence���͡�
							bBlockedBinding.sValue = sSlotValue;
							rbdBindings.vbBindings.push_back(bBlockedBinding);
							rbdBindings.iConceptsBlocked++; //����Concept��
							// add the slot to the list of matched slots 
							// ����slot��ӵ�ƥ���slot�б�
							msiSlotsMatched[bBlockedBinding.sGrammarExpectation] = 1;	// hash�洢ƥ���slot����Ӧ�Ĵ���
							// in case we find it in the blocked slots, (it could have gotten
							// there on an earlier level) delete it from there
							map<string, int>::iterator iPtr;
							if ((iPtr = msiSlotsBlocked.find(bBlockedBinding.sGrammarExpectation)) != msiSlotsBlocked.end())
							{
								msiSlotsBlocked.erase(iPtr);	// hash�洢������slot�����ǵĴ���
							}
						}
					}//if (i == 0)//ֻ��Ե�һ��concept
				}//����TBindingsDescr:  for (unsigned int i = 0; i < vOpenIndices.size(); i++)

				//		write the blocked bindings description
				// <20>	д�����İ�����TBindingsDescr
				for (unsigned int i = 0; i < vClosedIndices.size(); i++)
				{
					TBinding bBlockedBinding;
					bBlockedBinding.bBlocked = true;
					bBlockedBinding.iLevel = iLevel;
					bBlockedBinding.fConfidence = fConfidence;
					bBlockedBinding.sAgentName = eaAgenda.celSystemExpectations[vClosedIndices[i]].pDialogAgent->GetName();
					bBlockedBinding.sConceptName = eaAgenda.celSystemExpectations[vClosedIndices[i]].sConceptName;
					bBlockedBinding.sGrammarExpectation = eaAgenda.celSystemExpectations[vClosedIndices[i]].sGrammarExpectation;
					bBlockedBinding.sReasonDisabled = eaAgenda.celSystemExpectations[vClosedIndices[i]].sReasonDisabled;//������ԭ��slot������ԭ��
					bBlockedBinding.sValue = sSlotValue;
					rbdBindings.vbBindings.push_back(bBlockedBinding);
					rbdBindings.iConceptsBlocked++;//����Concept��
					// add it to the list of blocked slots, if it's not already
					// in the one of matched slots
					if (msiSlotsMatched.find(bBlockedBinding.sGrammarExpectation) == msiSlotsMatched.end())
					{
						msiSlotsBlocked[bBlockedBinding.sGrammarExpectation] = 1;//hash�洢������slot�����ǵĴ���
					}
				}// д�����İ�����TBindingsDescr



			}//if (pInteractionEventManager->LastEventMatches(sSlotExpected))
		}//for (iPtr = eaAgenda.vCompiledExpectations[iLevel].mapCE.begin();
	}//for (unsigned int iLevel = 0; iLevel < eaAgenda.vCompiledExpectations.size(); iLevel++)

	//		for user inputs, update the non-understanding flag
	// <21>	�����û����룬���·�����־bNonUnderstanding
	if (pInteractionEventManager->GetLastEvent()->GetType() == IET_USER_UTT_END ||
		pInteractionEventManager->GetLastEvent()->GetType() == IET_GUI)
	{
		rbdBindings.bNonUnderstanding = (rbdBindings.iConceptsBound == 0);//����� �� �Ƿ���concept��
	}
	else
	{
		rbdBindings.bNonUnderstanding = false;
	}


	// update the slots matched and blocked information
	// ���²��ƥ�����������Ϣ
	rbdBindings.iSlotsMatched = msiSlotsMatched.size(); // ƥ���slot��
	rbdBindings.iSlotsBlocked = msiSlotsBlocked.size(); // ������slot��



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
	// finally�������û����룬���ͳ���Ƿ�ƥ��helios��Ԥ�⣨helios�󶨹��ܣ�
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
		//##########################ǿ�Ƹ������##############################################
		// finally��ִ��ǿ�Ƹ������
		// finally, perform the forced concept updates
		performForcedConceptUpdates(rbdBindings);
		//##########################ǿ�Ƹ������##############################################
	}
	else if (pInteractionEventManager->GetLastEvent()->GetType() == IET_GUI)
	{
		//###########################ǿ�Ƹ������#############################################
		performForcedConceptUpdates(rbdBindings);
		//###########################ǿ�Ƹ������#############################################
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
// D��ִ�и����
void CDMCoreAgent::performConceptBinding(string sSlotName, string sSlotValue,
	float fConfidence, int iExpectationIndex, bool bIsComplete)
{

	//		obtain a reference to the expectation structure
	// <1>	��ö������ṹ������
	TConceptExpectation& ceExpectation = eaAgenda.celSystemExpectations[iExpectationIndex];

	//		compute the value we need to bind to that concept
	// <2>	����������Ҫ�󶨵��Ǹ������ֵ
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
		// ͨ��filter������
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
		// ����󶨹������ҵ���������
		sValueToBind = (*(iPtr->second))(sSlotName, sSlotValue);
	}

	// reset the confidence to 1, if ALWAYS_CONFIDENT is defined
#ifdef ALWAYS_CONFIDENT
	fConfidence = 1.0;
#endif

	// <3>	��ʽ�� slotValue|confidence    ==>  value/confidence
	string sBindingString = FormatString("%s%s%f", sValueToBind.c_str(), VAL_CONF_SEPARATOR, fConfidence);

	// now bind that particular value/confidence
	// ���ڰ󶨸��ض�ֵ/���Ŷ�
	// bIsComplete = pInteractionEventManager->LastEventIsComplete()  ���һ���¼��Ƿ����
	if (bIsComplete)
	{
		// first, create a temporary concept for that
		// ����һ����ʱ��concept
		CConcept *pTempConcept = ceExpectation.pDialogAgent->C(ceExpectation.sConceptName).EmptyClone();
		// assign it from the string
		// <4>	ͨ��string��ֵconcept
		//		sBindingString =>  ��ʽ�� slotValue|confidence    ==>  value/confidence
		pTempConcept->Update(CU_ASSIGN_FROM_STRING, &sBindingString);

		CConcept &c = ceExpectation.pDialogAgent->C(ceExpectation.sConceptName);

		//		first if the concept has an undergoing grounding request, remove it
		// <5>	�������������һ�����ڽ��еĽӵ�����ɾ����
		if (c.IsUndergoingGrounding())
			pGroundingManager->RemoveConceptGroundingRequest(&c);

		//############################ ʵ�ʸ��� ###########################################
		//		now call the binding method 
		// <6>	���ڵ��ð󶨷���,������ʱ����pTempConcept��ʵ�ʵ�concept
		c.Update(CU_UPDATE_WITH_CONCEPT, pTempConcept);
		//############################ ʵ�ʸ��� ###########################################

		// finally, deallocate the temporary concept
		// ����
		delete pTempConcept;
	}//if (bIsComplete) ���event���
	else
	{
		// perform a partial (temporary) binding
		// ִ�в��֣���ʱ����
		ceExpectation.pDialogAgent->C(ceExpectation.sConceptName).Update(CU_PARTIAL_FROM_STRING, &sBindingString);
	}

	// log it
	Log(DMCORE_STREAM, "Slot %s(%s) bound to concept (%s)%s.",
		sSlotName.c_str(), sBindingString.c_str(),
		ceExpectation.pDialogAgent->GetName().c_str(),
		ceExpectation.sConceptName.c_str());
}// D��ִ�и����


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
// D��ִ�и����ǿ�Ƹ���
void CDMCoreAgent::performForcedConceptUpdates(TBindingsDescr& rbdBindings)
{

	Log(DMCORE_STREAM, "Performing forced concept updates ...");

	// now go through the concepts that are waiting to be explicitly 
	// confirmed and perform the corresponding updates on them
	// �����ȴ���ʾȷ�ϵ�concept,ִ����Ӧ����
	set<CConcept *>::iterator iPtr;
	for (iPtr = saSystemAction.setcpExplicitConfirms.begin(); iPtr != saSystemAction.setcpExplicitConfirms.end(); iPtr++)//������ǰϵͳ�����е� Explicit Concept
	{
		// check that the concept is still sealed (no update was performed  on it yet)
		if ((*iPtr)->IsSealed())//���concept��sealed
		{
			/*
			typedef struct
			{
			string sConceptName;			// the name of the concept that had a	����ǿ�Ƹ��µĸ��������
			//  forced update
			int iType;						// the type of the forced update		ǿ�Ƹ��µ�����
			bool bUnderstanding;			// the update changed the concept
			//  enough that the grounding action
			//  on it is different, and therefore
			//  we consider that we have an actual
			//  understanding occuring on that
			//  concept
			// ���¸ı����㹻�ĸ�������Ļ��������ǲ�ͬ�ģ����������Ϊ������һ��ʵ�ʵ���ⷢ�����Ǹ�����
			} TForcedConceptUpdate;
			*/
			// then perform an forced update	
			// ִ��ǿ�Ƹ���
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
			// ������½���� concept, �����нӵ�Action
			if (!((*iPtr)->IsSealed()) && (pGroundingManager->GetConceptGroundingRequestStatus(*iPtr) != GRS_EXECUTING))
			{
				// now schedule grounding on this concept
				string sAction = pGroundingManager->ScheduleConceptGrounding(*iPtr);//// D��ǿ�ƽӵع�����Ϊĳ������Žӵ�
				// if the action scheduled is still explicit confirm
				if (sAction != "EXPL_CONF")
				{
					// then mark that we have an understanding
					// ���
					fcu.bUnderstanding = true;
					rbdBindings.bNonUnderstanding = false;
				}
				else if ((*iPtr)->GetTopHyp() == phOldTopHyp)
				{
					// if we are still on an explicit confirm on the same hypothesis, 
					// seal it back
					// ����ͬconcept��ִ�� explicit confirm,��seal��
					(*iPtr)->Seal();
				}
			}

			// finally, push this into the bindings
			rbdBindings.vfcuForcedUpdates.push_back(fcu);//��ӵ�ǿ�Ƹ��µ�����
		}//if ((*iPtr)->IsSealed())//���concept��sealed
	}//������ǰϵͳ�����е� Explicit Concept

	// now go through the concepts that are waiting to be implicitly 
	// confirmed and perform the corresponding updates on them
	// �����ȴ���ʽȷ�ϵ�concept,ִ����Ӧ����
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
	}// �����ȴ���ʽȷ�ϵ�concept,ִ����Ӧ����

	// finally, go through the concepts that have unplanned implicit confirmations
	// on them and perform the corresponding updates on them
	// ������unplanned���ȴ���ʽȷ�ϵ�concept,ִ����Ӧ����
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
	}// ������unplanned���ȴ���ʽȷ�ϵ�concept,ִ����Ӧ����

	// log completion of this phase
	Log(DMCORE_STREAM, "Forced concept updates completed.");
}


//-----------------------------------------------------------------------------
// D: Focus Claims functions
//-----------------------------------------------------------------------------
// D: assembles a list of focus claims, and returns the size of that list
// D���ռ������������б������ظ��б�Ĵ�С
int CDMCoreAgent::assembleFocusClaims()
{
	Log(DMCORE_STREAM, "Focus Claims Assembly Phase initiated.");

	/*
	// D: structure describing a focus claim
	// D���������������Ľṹ
		typedef struct
		{
			string sAgentName;				// the name of the agent that claims focus		// ����claim��agent������
			bool bClaimDuringGrounding;     // indicates whether or not the focus is		// �Ƿ���Grouding�ڼ�����Claim��
			//  claimed during grounding
		} TFocusClaim;
	*/

	//		gather the focus claims, starting with the root of the dialog task tree
	// <1>	�ռ������������ӶԻ����������ĸ�Root��ʼ
	int iClaims = 0;
	TFocusClaimsList fclTempFocusClaims;
	iClaims = pDTTManager->GetDialogTaskTreeRoot()->DeclareFocusClaims(fclTempFocusClaims);//��root�ڵ㿪ʼ�ݹ��ռ�����#define TRIGGERED_BY(Condition)������Agent����Ҫ��

	// log the list of claiming agents
	// <2>	��¼����������б�
	string sLogString;
	if (fclTempFocusClaims.size() == 0)
		sLogString = "0 agent(s) claiming focus.";
	else
		sLogString = FormatString("%d agent(s) claiming focus (dumped below):\n", fclTempFocusClaims.size());

	for (unsigned int i = 0; i < fclTempFocusClaims.size(); i++)
		sLogString += FormatString("  %s\n", fclTempFocusClaims[i].sAgentName.c_str());
	Log(DMCORE_STREAM, sLogString.c_str());

	//		now prune the claims of agents that have their completion criteria satisfied
	// <3>	����ɾ�����㡾��ɡ���׼��agent������
	Log(DMCORE_STREAM, "Pruning Focus Claims list.");
	// check if we undergoing some grounding action
	// ��������Ƿ����ڽ���һЩ grouding action
	bool bDuringGrounding = pGroundingManager->HasScheduledConceptGroundingRequests()/* ||
							pGroundingManager->HasExecutingConceptGroundingRequests()*/;

	int iClaimsEliminated = 0;
	fclFocusClaims.clear(); //CDMCoreAgent�Ľ��������б����
	sLogString = "";
	// <4>	�����ռ�����fclTempFocusClaims
	for (unsigned int i = 0; i < fclTempFocusClaims.size(); i++)//�����ռ�����fclTempFocusClaims
	{
		// <5>	��ȡ����focus��Agent
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
			//<6>	���agent
			//			1	-	�Ѿ���ɣ�
			//			2	-	���������Ѿ���stack�У�
			//			3	-	������������ڽӵ��ڼ䲻�ܴ���Focus��shift��
			//	��Ӵ�����������Ĵ����б�������
			iClaimsEliminated++;
			// and mark it in the log string
			sLogString += FormatString("  %s\n", fclTempFocusClaims[i].sAgentName.c_str());
		}
		else
		{
			//		o/w add it to the real list of claims
			// <7>	��ӵ�CDMCoreAgent��ʵ�������б���
			fclFocusClaims.push_back(fclTempFocusClaims[i]);
		}
	}//for (unsigned int i = 0; i < fclTempFocusClaims.size(); i++)//�����ռ�����fclTempFocusClaims

	// finally, add the prefix for the log string of eliminated agents
	// finally����������������־�ַ�����ǰ׺
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
// D�����Focusƫ��
void CDMCoreAgent::resolveFocusShift()
{

	//		send out a warning if we have a multiple focus shift
	// <1>	���������һ����focus Shift����������
	if (fclFocusClaims.size() > 1)
	{
		string sMessage = "Ambiguous focus shift (claiming agents dump below).\n";
		for (unsigned int i = 0; i < fclFocusClaims.size(); i++)
			sMessage += fclFocusClaims[i].sAgentName + "\n";
		Warning(sMessage.c_str());
	}

	//		put the agents on the stack 
	// <2>	��������ڶ�ջ��	=>	ֻ��һ��������
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
// D�����ص�ǰ�ĳ�ʱʱ��
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
// D������Ĭ�ϵ�nonunderstanding��ֵ
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
// D��signal a focus claims phase - ����һ����־���Ա�DMCore agent�����н��������׶���һ������, Ĭ��ΪTrue
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
// D��������һ��turn�ǲ���⣬�򷵻�true
bool CDMCoreAgent::LastTurnNonUnderstanding()
{
	// <1>	�Ӻ���ǰ���� => ����ʷ
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
		// <2>	user˵������ | GUI�۽����
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
// ���ص�ĿǰΪֹ����������������
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
//���ص�ǰ�Ի���������Ϊֹ������������
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
// ��ִ�ж�ջ��pushһ���µĶԻ�����
void CDMCoreAgent::ContinueWith(CAgent* paPusher, CDialogAgent* pdaDialogAgent)
{

	//		check that the agent is not already on top of the stack
	// <1>	��� �Ƿ��Ѿ���ջ��
	if (!esExecutionStack.empty() && (esExecutionStack.begin()->pdaAgent == pdaDialogAgent))
	{
		// ����Ѿ���ջ�������󱻺���
		Log(DMCORE_STREAM, "Agent %s already on top of the execution stack. ContinueWith request ignored.", pdaDialogAgent->GetName().c_str());
		return;
	}

	//		add an entry in the history; fill in all the slots
	// <2>	����ʷ�������Ŀ; ��д���в��
	TExecutionHistoryItem ehi;
	ehi.sCurrentAgent = pdaDialogAgent->GetName();		//name
	ehi.sCurrentAgentType = pdaDialogAgent->GetType();	//type
	ehi.bScheduled = true;
	ehi.sScheduledBy = paPusher->GetName();				//��˭����
	ehi.timeScheduled = GetTime();						//���ȵ�ʱ��
	ehi.bExecuted = false;
	ehi.bCommitted = false;
	ehi.bCanceled = false;
	ehi.iStateHistoryIndex = -1;
	// ***ehi.timeTerminated = 0;
	ehExecutionHistory.push_back(ehi); //����ִ����ʷ��

	//		and put it on the stack
	// <3>	ʵ��ʵ�е�Agentѹ���ջ
	TExecutionStackItem esi;
	esi.pdaAgent = pdaDialogAgent;
	esi.iEHIndex = ehExecutionHistory.size() - 1;	//��¼��ִ����ʷ�е�����
	esExecutionStack.push_front(esi);				//�ڿ�ʼλ��[ջ��]����һ��Ԫ��======�ڿ�ʼλ������һ��Ԫ��

	//		stores the execution index in the agent
	// <4>	��ִ�е�[��ʷ]index�洢�ڴ�����
	pdaDialogAgent->SetLastExecutionIndex(esi.iEHIndex);

	//		signals that the agenda needs to be recomputed
	// <5>	����agenda��Ҫ�����¸���	=> ִ��ջstack�ı����Ҫ����Agenda
	bAgendaModifiedFlag = true;

	Log(DMCORE_STREAM, "Agent %s added on the execution stack by %s.",
		ehi.sCurrentAgent.c_str(), ehi.sScheduledBy.c_str());
}

// D: Restarts a topic
//������������
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
// D�����������Ի���
void CDMCoreAgent::StartOver()
{
	if (csoStartOverFunct == NULL)
	{
		// restart the dialog clear the execution stack
		// ���ִ��ջ ��Ұָ�룿��
		esExecutionStack.clear();
		// destroy the dialog task tree
		// ���ٶԻ���
		pDTTManager->DestroyDialogTree();
		// recreate the dialog task tree
		// ���´��� DTT
		pDTTManager->ReCreateDialogTree();
		// restart the execution by putting the root on the stack
		// ���ڵ����ջ���������Ự
		Log(DMCORE_STREAM, "Restarting Dialog Task execution.");
		ContinueWith(this, pDTTManager->GetDialogTaskTreeRoot());
		//����״̬
		pStateManager->UpdateState();
	}
	else
	{
		(*csoStartOverFunct)();
	}
}

// D: Pops all the completed agents (and all the agents they have ever planned
//	  for off the execution stack
// ������ɵ�agent��ջ
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
		// <1>	����ִ��ջ
		for (iPtr = esExecutionStack.begin(); iPtr != esExecutionStack.end(); iPtr++)
		{

			//		if you find an agent that has completed
			// <2>	�Ƿ���� ���ɹ��� ʧ�ܡ�
			if (iPtr->pdaAgent->HasCompleted())
			{
				//##########################popTopicFromExecutionStack#############################
				//		pop it off the execution stack
				// <3>	����ջ - [����������agent]
				popTopicFromExecutionStack(iPtr->pdaAgent, vsAgentsEliminated);
				bFoundCompleted = true;
				break;
				//##########################popTopicFromExecutionStack#############################
			}
		}
	} while (bFoundCompleted);

	//		when no more completed agents can be found, log and return
	// <4>	log��¼
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
	// <5>	����ɾ����agent��Ŀ
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
// D����ִ�ж�ջ�е���һ���Ի������Լ����ƻ�ִ�е�������������
void CDMCoreAgent::popTopicFromExecutionStack(CDialogAgent* pdaADialogAgent, TStringVector& rvsAgentsEliminated)
{
	// check for empty stack condition
	if (esExecutionStack.empty())
	{
		FatalError("Cannot pop the " + pdaADialogAgent->GetName() +
			" agent off the execution stack. Stack is empty.");
	}

	//		first, locate the agent
	// <1>	��λջ�е�λ��
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin();
		iPtr != esExecutionStack.end();
		iPtr++)
	{
		if (iPtr->pdaAgent == pdaADialogAgent) break;
	}

	//		if the agent was nothere in the list, trigger a fatal error
	// <2>	����������б��У��򴥷���������
	if (iPtr == esExecutionStack.end())
	{
		FatalError("Cannot pop the " + pdaADialogAgent->GetName() + " agent off the execution stack. Agent not found.");
	}

	//		the set of eliminated agents
	// <3>	���agent��set����
	set<CDialogAgent*, less<CDialogAgent*> > sEliminatedAgents;

	// initialize it with the starting agent
	// Ҫɾ���ĵ�ǰagent
	sEliminatedAgents.insert(iPtr->pdaAgent);

	//		mark the time this agent's execution was terminated
	// <4>	��Ǵ˴����ִ�б���ֹ��ʱ��
	ehExecutionHistory[iPtr->iEHIndex].timeTerminated = GetTime();

	// ���� <3>	OnCompletion()����####################################@��Ҫ@#################################
	// <5>	call the agent's OnCompletion method
	iPtr->pdaAgent->OnCompletion();//�� #define ON_COMPLETION(DO_STUFF)
	// ���� <3>	OnCompletion()����####################################@��Ҫ@#################################

	//		and add it to the list of eliminated agents
	// <6>	���뵽ɾ���б�
	rvsAgentsEliminated.push_back(iPtr->pdaAgent->GetName());

	//		eliminate the agent from the stack
	// <7>	��ջ��ɾ����ǰagent
	esExecutionStack.erase(iPtr);

	// now enter in a loop going through the stack repeatedly until 
	// we didn't find anything else to remove
	// <8>	ѭ������ջ��ֱ������û���ҵ��κ�����Ҫɾ��
	bool bFoundAgentToRemove = true;
	while (bFoundAgentToRemove)
	{

		bFoundAgentToRemove = false;

		//		now traverse the stack
		// <9>	����ִ��ջ �� ɾ����Agent
		for (iPtr = esExecutionStack.begin(); iPtr != esExecutionStack.end(); iPtr++)
		{

			//		check to see who planned the current agent
			// <10>	�鿴��ǰ�����scheduler������
			CDialogAgent* pdaScheduler = (CDialogAgent *) AgentsRegistry[ehExecutionHistory[iPtr->iEHIndex].sScheduledBy];
			
			// <11>	�����ǰ��agent�ĸ�agent��ɾ���б����ҲҪɾ����ǰ��agent
			if (sEliminatedAgents.find(pdaScheduler) != sEliminatedAgents.end())
			{
				// then we need to eliminate this one; so first add it to the 
				// list of eliminated agents
				// ���ɾ��һ��agent,�����е���agent����Ҫɾ���ġ� =>  ����Ҫ�����ɱ�־
				// ʵ��ɾ���Ǵӵ�ǰagent�ݹ����ɾ���������е���agent
				sEliminatedAgents.insert(iPtr->pdaAgent);
				// mark the time this agent execution was terminated
				// ��¼��ֹʱ��
				ehExecutionHistory[iPtr->iEHIndex].timeTerminated = GetTime();
				//################################################################
				// call the agent's OnCompletion method
				iPtr->pdaAgent->OnCompletion();
				//################################################################
				// and add it to the list of eliminated agents
				//��ӵ�ɾ���б�
				rvsAgentsEliminated.push_back(iPtr->pdaAgent->GetName());
				// eliminate the agent from the stack
				// ��ջ��ɾ�� - ʵ�ʲ���
				esExecutionStack.erase(iPtr);
				// set found one to true
				bFoundAgentToRemove = true;
				// and break the for loop
				break;
			}
		}
	}//while (bFoundAgentToRemove)

	// agenda�޸Ĺ� - ������װ#####################################!!!
	bAgendaModifiedFlag = true;
	// agenda�޸Ĺ� - ������װ#####################################!!!
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
// D�����ذ���ʷ��¼�Ĵ�С
int CDMCoreAgent::GetBindingHistorySize()
{
	return bhBindingHistory.size();
}

// D: Returns a pointer to the binding result from history
// D������ָ����ʷ��¼�İ󶨽����ָ��
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
// D�����ض��ض�����ִ�е�ϵͳ����������
//��*****���������Ҫ��ϸ˵��*****��
TSystemActionOnConcept CDMCoreAgent::GetSystemActionOnConcept(CConcept* pConcept)
{
	TSystemActionOnConcept saoc;

	// check if the concept is among the requested concepts
	// request����
	if (saSystemAction.setcpRequests.find(pConcept) != saSystemAction.setcpRequests.end())
	{
		// then we have a request
		saoc.sSystemAction = SA_REQUEST;
	}

	// check if the concept is among the explicitly confirmed concepts
	// ��ʾȷ��
	else if (saSystemAction.setcpExplicitConfirms.find(pConcept) != saSystemAction.setcpExplicitConfirms.end())
	{
		// then we have an explicit confirm
		saoc.sSystemAction = SA_EXPL_CONF;
	}

	// check if the concept is among the implicitly confirmed concepts
	// ��ʽȷ��
	else if (saSystemAction.setcpImplicitConfirms.find(pConcept) != saSystemAction.setcpImplicitConfirms.end())
	{
		// then we have an implicit confirm
		saoc.sSystemAction = SA_IMPL_CONF;
	}

	// check if the concept is among the unplanned implicitly confirmed concepts
	// unplanned��ʽȷ��
	else if (saSystemAction.setcpUnplannedImplicitConfirms.find(pConcept) != saSystemAction.setcpUnplannedImplicitConfirms.end())
	{
		// then we have an implicit confirm
		saoc.sSystemAction = SA_UNPLANNED_IMPL_CONF;
	}

	// check if the concept is among the implicitly confirmed concepts
	// ��������
	else
	{
		// then we have an "other" type request
		saoc.sSystemAction = SA_OTHER;
	}

	// return 
	return saoc;
}

// D: Signal an explicit confirmation on a concept
// D����ʾһ���������ʽȷ��
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
// D����ʾ�Ը���ļƻ�����ʽȷ��
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
			// �����û�б����Ϊ��ʽ����ʽȷ��, ���ڼ�¼��ǰϵͳaction
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
			// �����û�б����Ϊ��ʽ����ʽȷ��, ���ڼ�¼��ǰϵͳ����
			Log(DMCORE_STREAM, FormatString("System action dumped below.\n%s",
				systemActionToString(saSystemAction).c_str()));

			saSystemAction.setcpUnplannedImplicitConfirms.insert(pConcept);
		}
	}

}

// D: Signal an accept action on a concept
//֪ͨ ������һ��concept����һ��accept����
void CDMCoreAgent::SignalAcceptOnConcept(CConcept* pConcept)
{
	// erase that agent from the explicit confirmations
	// but not from the implicit confirmations, since once an IC
	// is done, we want to perform an IC update on that concept
	// ����ʽȷ����ɾ���ô�����������ʽȷ����ɾ���ô���
	// ��Ϊһ��IC��ɣ�����Ҫ�Ըø���ִ��IC����
	saSystemAction.setcpExplicitConfirms.erase(pConcept);
}

// D: Returns the agent on top of the execution stack
// ����ִ��ջջ����agent
CDialogAgent* CDMCoreAgent::GetAgentInFocus()
{
	TExecutionStack::iterator iPtr;
	for (iPtr = esExecutionStack.begin(); iPtr != esExecutionStack.end(); iPtr++)
	{
		if (iPtr->pdaAgent->IsExecutable())//��ִ��
		{
			return iPtr->pdaAgent;
		}
	}

	// o/w return NULL
	return NULL;
}

// D: Returns the task agent closest to the top of the execution stack
// �������ִ�ж�ջ�������������
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
// D�����ָ���Ĵ�������ڽ��㣬�򷵻�true
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
// D�����ص�ǰ��Ҫ �������
// A����������ͨ�������߶�ջ������ʹ������ʶ��
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
// D����������ǻ���⣬�򷵻�true  => �����ǰ�ڵ���ִ��ջ�У�����True
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
// D����ִ�ж�ջ��������������
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
// D����ִ�ж�ջ��ɾ�����������Լ����ƻ�ִ�е����д���
void CDMCoreAgent::PopTopicFromExecutionStack(CDialogAgent* pdaADialogAgent)
{

	// call upon the private helper function to eliminate the agent
	// ����˽�˰�����������������
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