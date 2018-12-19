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

//�Լ���ӵ���Ϣ
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
	//#log��¼��ʼ������
	Log(CORETHREAD_STREAM, "Initializing Core ...");
	Log(CORETHREAD_STREAM, "Core initialization parameters dumped below:\n%s", rcpParams.ToString().c_str());

	// Initialize the random number gererator
	//#��ʼ�������������
	_timeb theTime = GetTime(); // ??
	srand((unsigned)(theTime.millitm + theTime.time * 1000));

	//		Create a new Dialog Management Core Agent, and register it
	//<1>	����һ���µ�Dialog Management Core Agent����ע����
	AgentsRegistry.Clear();
	AgentsRegistry.RegisterAgentType("CDMCoreAgent", CDMCoreAgent::AgentFactory);		//ע�� <agentType, create Function>
	pDMCore = (CDMCoreAgent*)AgentsRegistry.CreateAgent("CDMCoreAgent", "DMCoreAgent"); //����agent
	if (!pDMCore)
		FatalError("Could not create DMCore agent.");
	pDMCore->Initialize(); //��ʼ�� agent
	pDMCore->Register();	   //ע��agent[thisָ��]

	// set the default core configuration 
	//����Ĭ�����ò���
	pDMCore->SetDefaultTimeoutPeriod(atoi(rcpParams.Get(RCP_DEFAULT_TIMEOUT).c_str()));//Ĭ�ϳ�ʱ
	pDMCore->SetDefaultNonunderstandingThreshold((float)atof(rcpParams.Get(RCP_DEFAULT_NONUNDERSTANDING_THRESHOLD).c_str()));//Ĭ�ϲ������ֵ

	//#######################################################################
	// create all the other dialog core agents
	// ���������ĸ��� �Ի�����agent
	Log(CORETHREAD_STREAM, "Creating auxiliary core dialog core agents ...");


	//		create the interaction event manager
	//<2>	�����¼�������
	AgentsRegistry.RegisterAgentType("CInteractionEventManagerAgent", CInteractionEventManagerAgent::AgentFactory);
	pInteractionEventManager = (CInteractionEventManagerAgent *)AgentsRegistry.CreateAgent("CInteractionEventManagerAgent", "InteractionEventManagerAgent");
	if (!pInteractionEventManager)
		FatalError("Could not create InteractionEventManager agent.");
	pInteractionEventManager->Initialize(); //��ʼ��agent
	pInteractionEventManager->Register();	//ע��agent[thisָ��] 


	//		create the output manager
	//<3>	���������
	AgentsRegistry.RegisterAgentType("COutputManagerAgent", COutputManagerAgent::AgentFactory);
	pOutputManager = (COutputManagerAgent *)AgentsRegistry.CreateAgent("COutputManagerAgent", "OutputManagerAgent");
	if (!pOutputManager)
		FatalError("Could not create OutputManager agent.");
	pOutputManager->Initialize();	//��ʼ��agent
	pOutputManager->Register();		//ע��agent[thisָ��]


	//		create the galaxy stub
	//<4>	���� galaxy ��Ϣ��Ŧ
	AgentsRegistry.RegisterAgentType("CTrafficManagerAgent", CTrafficManagerAgent::AgentFactory);
	pTrafficManager = (CTrafficManagerAgent *)AgentsRegistry.CreateAgent("CTrafficManagerAgent", "TrafficManagerAgent");
	if (!pTrafficManager)
		FatalError("Could not create TrafficManager agent.");
	pTrafficManager->Initialize();			//��ʼ��agent
	pTrafficManager->Register();			//ע��agent[thisָ��]


	//		create the state manager
	//<5>	����״̬������
	AgentsRegistry.RegisterAgentType("CStateManagerAgent", CStateManagerAgent::AgentFactory);
	pStateManager = (CStateManagerAgent *)AgentsRegistry.CreateAgent("CStateManagerAgent", "StateManagerAgent");
	if (!pStateManager)
		FatalError("Could not create StateManager agent.");
	pStateManager->Initialize();			//��ʼ��agent
	pStateManager->Register();				//ע��agent[thisָ��]
	// set the state broadcast address
	// ����״̬�㲥��ַ
	// ����״̬�� ��agentName -> stateName��
	pStateManager->LoadDialogStateNames(rcpParams.Get(RCP_DIALOG_STATES_FILE));


	//		create the dialog task tree manager
	//<6>	�����Ի���������[ DTT ]������
	AgentsRegistry.RegisterAgentType("CDTTManagerAgent", CDTTManagerAgent::AgentFactory);
	pDTTManager = (CDTTManagerAgent *)AgentsRegistry.CreateAgent("CDTTManagerAgent", "DTTManagerAgent");
	if (!pDTTManager)
		FatalError("Could not create DTTManager agent.");
	pDTTManager->Initialize();			//��ʼ��agent
	pDTTManager->Register();			//ע��agent[thisָ��]


	//		create the grounding manager
	//<7>	�����ӵع�����
	AgentsRegistry.RegisterAgentType("CGroundingManagerAgent", CGroundingManagerAgent::AgentFactory);
	pGroundingManager = (CGroundingManagerAgent *)AgentsRegistry.CreateAgent("CGroundingManagerAgent", "GroundingManagerAgent");
	if (!pGroundingManager)
		FatalError("Could not create GroundingManager agent.");
	pGroundingManager->Initialize();		// ��ʼ��agent
	pGroundingManager->Register();		// ע��agent[thisָ��]

	//#####################################################################################
	// set the configuration
	// �������ò���
	pGroundingManager->SetConfiguration(rcpParams.Get(RCP_GROUNDING_MANAGER_CONFIGURATION));
	// �ӽӵ�file�м���polices => ��ֻ�Ǵ��ж�����Ҫ�����ݣ���string����ʽ���洢��policy��hash����.��
	//������ʵ����Agent��ʱ������Create������ʱ������policy Hash���в��ҵ����õ�Groudingģ�ͣ�����policy�ַ����Ľ�����������Ӧ�Ķ���
	// and load the models specifications from the grounding policies file
	// �ӽӵز����ļ�����ģ�Ͳ��� <policy Hash��>
	// hash holding the grounding models policies 
	// (key = model_name, value= model policy string)
	// ����policy Hash
	// STRING2STRING s2sPolicies;
	/*
		expl_impl = expl_impl.pol �ļ�����
		expl = expl.pol	�ļ�����
		request_default = request_default.pol �ļ�����
		request_lr = request_lr.pol �ļ�����
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

	//ɾ�����к��ĵ�Agent
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
	// ���ע���
	AgentsRegistry.Clear();

	// and log that the core terminated successfully
	Log(CORETHREAD_STREAM, "Core terminated successfully.");
}

//-----------------------------------------------------------------------------
// THE DIALOG CORE THREAD FUNCTION
// �����̺߳���
//-----------------------------------------------------------------------------
//LPVOID��һ��û�����͵�ָ�룬Ҳ����˵����Խ��������͵�ָ�븳ֵ��LPVOID���͵ı�����һ����Ϊ�������ݣ���Ȼ����ʹ�õ�ʱ����ת��������
DWORD WINAPI DialogCoreThread(LPVOID pParams)
{
	// <1>	��ʼ�����ò���
	TRavenClawConfigParams rcpParams = *(TRavenClawConfigParams *)pParams;
	// <2>	��־��ʼ��
	InitializeLogging(
		rcpParams.Get(RCP_LOG_DIR),
		rcpParams.Get(RCP_LOG_PREFIX),
		DEFAULT_LOG_FILENAME,
		rcpParams.Get(RCP_LOGGED_STREAMS),
		rcpParams.Get(RCP_DISPLAYED_STREAMS),
		rcpParams.Get(RCP_EXIT_ON_FATAL_ERROR)
		);

	// Print out Olympus version stuff
	// ��¼Olympus�汾��
	Log(CORETHREAD_STREAM, "Olympus Branch: %s\nOlympus Revision: %s", OLYMPUS_SVN_BRANCH, OLYMPUS_SVN_REVISION);

	// Initialize the core: create all the core agents
	// <3>	��ʼ��core���������к���Agent
	InitializeDialogCore(rcpParams);

	// Call the dialog task initialize function 
	// <4>	���öԻ������ʼ������
	//		ÿ���»Ự��ʼʱ���õĺ���,�����߶���һЩ������� [�궨�����]
	DialogTaskOnBeginSession();//�� -> #define CORE_CONFIGURATION(Configuration)

	// Do the dialog dance :)
	// <5>	����
	pDMCore->Execute();

	// Terminate the dialog core
	// <6>	�ս�Ի�Core
	TerminateDialogCore();

	// Finally, send a message to signal that this session of the Dialog Core is over
	// <7>	��󣬷�����Ϣ�Ա�ʾDialog Core�Ự����
	PostThreadMessage(g_idDMInterfaceThread, WM_DIALOGFINISHED, 0, 0);
	return 0;
}

