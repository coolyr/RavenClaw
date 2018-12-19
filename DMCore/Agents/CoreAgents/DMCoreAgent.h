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
// DMCoreAgent.H - defines the core dialog management agent, that handles
//				   execution and input passes (agenda generation, bindings, 
//				   etc)
//                 ** THIS IS A DEVELOPMENT VERSION THAT NEEDS TO BE MERGED 
//                    WITH DMCOREAGENT WHEN IT'S STABLE **
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
//   [2004-12-29] (antoine): added method GetNumberNonUnderstandings
//   [2004-12-23] (antoine): modified constructor and agent factory to handle
//							  configurations
//   [2004-04-06] (dbohus):  rearranged the broadcast agenda into a new, 
//                            compressed format
//   [2004-03-25] (dbohus):  moved functionality for broadcastign the 
//                            expectation agenda into the state manager where
//                            it actually belong
//   [2004-03-23] (dbohus):  added functionality for broadcasting the 
//                            expectation agenda
//   [2004-03-22] (dbohus):  fixed binding counting and developed bindings
//                            description and history routines
//   [2004-03-03] (dbohus):  added RestartTopic
//   [2004-03-03] (dbohus):  fixed bug in popFromExecutionStack so that indeed
//                            all the children are removed. Added 
//                            PopAgentFromExecutionStack() and
//                            PopTopicFromExecutionStack()
//   [2004-03-03] (dbohus):  added access to binding history datastructure
//   [2004-02-16] (dbohus):  added binding history datastructure (maintains a 
//                            history of features regarding the binding
//                            process)
//   [2003-11-09] (dbohus):  moved the grounding and focus claims phase out of 
//                            the input phase, into the execution cycle; now 
//                            they happen at every clock tick, when the core
//                            agent has been signaled to do them (added the 
//                            SignalFocusClaimsPhase and SignalGroundingPhase
//                            methods)
//   [2003-07-11] (antoine): added support for state-dependent language models
//							  (function updateLanguageModel called in 
//							  doInputPass)
//   [2003-04-29] (dbohus):  fixed small bug in Execute (checking for empty
//                            execution stack after popCompleted)
//   [2003-04-25] (dbohus,
//                 antoine): checking for completion of agents changed to 
//                            before execution; added PopAgentFromExecutionStack
//   [2003-04-15] (dbohus,
//                 antoine): introduced support for *-type expectations
//   [2003-04-13] (dbohus):  added friend class CGroundingManagerAgent
//                           delete counters for bindings in 2nd phase, since
//                            there's no more second phase
//   [2003-04-08] (dbohus):  added support for logging the execution stack
//   [2003-03-24] (dbohus):  changed resolveFocusShifts so that all the 
//                            claiming agents are pushed on the stack
//   [2003-02-13] (dbohus):  added support for dercRestartDialog 
//   [2003-01-15] (dbohus):  changed so that conflicting expectations are not
//                            created when the slots map to the same concept
//                            from different agents
//   [2002-12-01] (dbohus):  added confidence to concept binding
//   [2002-11-20] (dbohus):  added support for and binding through custom 
//                            binding filters
//   [2002-10-19] (dbohus):  added basic skeleton code for non-understanding 
//                            processing
//   [2002-07-15] (dbohus):  fixed bug with binding on multiple expectations on
//							  a given level
//   [2002-05-25] (dbohus):  deemed preliminary stable version 0.5
//   [2002-04-12] (dbohus):  decided to transform it into a core agent, which
//                            therefore took over all the functionality that 
//							  we previously had implemented in DMCore.h (cpp). 
//   [2002-01-26] (dbohus):  started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __DMCOREAGENT_H__
#define __DMCOREAGENT_H__

#include "../../../Utils/Utils.h"
#include "../../../DMCore/Agents/Agent.h"
#include "../../../DMCore/Agents/DialogAgents/DialogAgent.h"

// D: when ALWAYS_CONFIDENT is defined, the binding on concepts will ignore the
//    confidence scores on the input and will be considered "always confident"
// #define ALWAYS_CONFIDENT

//-----------------------------------------------------------------------------
// D: Auxiliary type definitions for the expectation agenda
//    These definitions are also reflected into Helios, since Helios uses the 
//    agenda to simulate this binding process in advance in order to compute
//    expectation features. Any changes to these structures, need to be 
//    also reflected in Helios
//-----------------------------------------------------------------------------
// D: a compiled concept expectation: implemented as a mapping between a 
//    string (the grammar concept), and a vector of integers (pointers in the 
//	  concept expectation list [celSystemExpectations]. This representation is used for faster 
//    checking of grammar concept expectations
//D���������������ʵ��Ϊ�ַ������﷨������������������������б��е�ָ�룩֮���ӳ�䡣
//   ���ֱ�ʾ�����ڸ���ؼ���﷨��������
//   slotName -> (vector)slotIndex
typedef map <string, TIntVector, less <string>, allocator <TIntVector> >
TMapCE;

typedef struct
{
	TMapCE mapCE;					// the hash of compiled expectations		���������Ĺ�ϣ
	CDialogAgent* pdaGenerator;		// the agent that represents that level		����ǰlevel�����Ĵ���
	//  of expectations
} TCompiledExpectationLevel;

// D: the struct representation for the compiled expectation agenda. It 
//    contains the expectation list gathered from the dialog task tree, 
//    and a "compiled" representation for each of the levels
// D�������������̵�struct��ʾ�� �������ӶԻ��������ռ��������б��Լ�ÿ������ġ��ѱ��롱��ʾ
typedef struct
{
	// the full system expectations, as gathered from the dialog task tree
	// �ӶԻ��������ռ�������ϵͳ����
	TConceptExpectationList celSystemExpectations;

	// an array holding the expectations of different levels (on index 0, 
	// are the expectations of the focused agent, on index 1 those of the
	// immediate upper level agent, etc)
	// ���ֲ�ͬlevel�����������飨������0�ϣ���focus agent��������������1�ϵ�ֱ���ϲ�agent�������ȣ�
	vector <TCompiledExpectationLevel, allocator <TCompiledExpectationLevel> > 	vCompiledExpectations;
} TExpectationAgenda;


//-----------------------------------------------------------------------------
// D: Auxiliary type definitions for the execution stack and history
//-----------------------------------------------------------------------------

// D: structure holding an execution stack item
// �ṹ����ִ�ж�ջ��
typedef struct
{
	CDialogAgent* pdaAgent;			// the agent that is on the stack					agentָ��
	int iEHIndex;					// an index into it's correspondent history entry	��ʷ����
} TExecutionStackItem;

// D: definition of the execution stack type - essentially a list 
// ����ִ�ж�ջ���� - ��������һ���б�
typedef list <TExecutionStackItem, allocator<TExecutionStackItem> >
TExecutionStack;

// D: structure holding a execution history item
// ����ִ����ʷ item ��Ľṹ��
typedef struct
{
	string sCurrentAgent;			// the name of the agent that is executed	Ҫִ�е�Agent������
	string sCurrentAgentType;		// the type of that agent					Agent ����
	string sScheduledBy;			// the agent that scheduled this one for    ���ȵ�ǰagent�ĸ�Agent
	//   execution
	bool bScheduled;				// has the history item been scheduled		�Ƿ��б�����ִ����ʷ
	//   for execution?
	bool bExecuted;					// has the history item been executed?		����ʷ�ִ�У�
	bool bCommitted;				// has the history item been committed		����ʷ���ѱ��ύ��History?
	//   to history?
	bool bCanceled;					// has the history item been canceled		��ִ��ǰ����ʷitem�Ƿ�ȡ��
	//   before being committed?
	_timeb timeScheduled;			// the time when the agent was scheduled	�����ȵ�ʱ�䡾һ����
	//   for execution
	vector<_timeb, allocator<_timeb> >
		vtExecutionTimes;			// the times when the agent was actually	agent��ʵ��ִ�е�ʱ�䡾�����
	//   executed
	_timeb timeTerminated;			// the time when the agent completed		agent��ɵ�ʱ�䡾һ����
	//   execution
	int iStateHistoryIndex;			// the index in the history of dialog		ִ�д���ʱdialog state����ʷ��¼�е�����
	// states when the agent was executed
} TExecutionHistoryItem;

// A: definition of the execution history class - an extended vector
class CExecutionHistory : public vector<TExecutionHistoryItem, allocator<TExecutionHistoryItem> >
{


};

//-----------------------------------------------------------------------------
// D: Auxiliary type definitions for bindings
//-----------------------------------------------------------------------------

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
	//  the expectation was disabled
} TBinding;

// D: structure describing a particular forced update
#define FCU_EXPLICIT_CONFIRM	1			//ǿ��	- ��ʾȷ��
#define FCU_IMPLICIT_CONFIRM	2			//ǿ��	- ��ʽȷ��
#define FCU_UNPLANNED_IMPLICIT_CONFIRM  3	//�޼ƻ�	- ��ʽȷ��
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
// D: structure holding a binding history item, describing the bindings in a turn
// D���������ʷ��¼��Ľṹ������һ���غ��еİ�
typedef struct
{
	//    int iTurnNumber;                // the input turn number      
	string sEventType;				// the type of event to which this			event���� "IET_USER_UTT_START", ...
	// binding corresponds
	bool bNonUnderstanding;         // was the turn a non-understanding?		�Ƿ�no-understanding [���磺û��concept��]
	// i.e. no concepts bound
	int iConceptsBound;             // the number of bound concepts				�󶨵�concept��Ŀ
	int iConceptsBlocked;           // the number of blocked concepts			����Concept��
	int iSlotsMatched;				// the number of slots that matched			ƥ���slot��
	int iSlotsBlocked;				// the number of slots that were blocked	������slot��	
	vector<TBinding> vbBindings;	// the vector of bindings					��vector
	vector<TForcedConceptUpdate> vfcuForcedUpdates;								//ǿ�Ƹ��µ�����
	// the vector of forced updates
} TBindingsDescr;

// D: definition of the binding history - a vector
// D������ʷ�Ķ��� - һ������
typedef vector<TBindingsDescr, allocator<TBindingsDescr> >
TBindingHistory;

// D: definition of function type for performing customized bindings 
//     (binding filter functions)
// �����û����ư󶨵ĺ��� [slotName, slotValue]
typedef string(*TBindingFilterFunct)(string sSlotName, string sSlotValue);

// D: auxiliary definition for mapping from binding filter names to the actual functions
// �������壬���ڴӰ󶨹��������Ƶ�ʵ�ʺ�����ӳ��
typedef map<string, TBindingFilterFunct,
	less<string>, allocator<TBindingFilterFunct> > STRING2BFF;

// D: structure maintaining a description of the current system action on the
//    various concepts
// D�����ֵ�ǰϵͳaction�Ը��ָ���������Ľṹ
typedef struct
{
	set<CConcept *> setcpRequests;					//Turn Grouding
	set<CConcept *> setcpExplicitConfirms;			//��ʾȷ��
	set<CConcept *> setcpImplicitConfirms;			//��ʽȷ��
	set<CConcept *> setcpUnplannedImplicitConfirms;	//����Concept
} TSystemAction;

// D: structure describing system actions taken on a particular concept
// D���������ض������ȡ��ϵͳ�����Ľṹ
#define SA_REQUEST	"REQ"
#define SA_EXPL_CONF	"EC"
#define SA_IMPL_CONF	"IC"
#define SA_UNPLANNED_IMPL_CONF  "UIC"
#define SA_OTHER	"OTH"
typedef struct
{
	string sSystemAction;
	// *** this might need to be elaborated with other system actions
	// �������Ҫ������ϵͳ����һ����в���
} TSystemActionOnConcept;

// D: definition for customized start over routine
// һ���Զ���Ŀ�ʼ����
typedef void(*TCustomStartOverFunct)();

//-----------------------------------------------------------------------------
//
// D: CDMCoreAgent class -
//      implements core dialog management functionalities execution, input 
//      passes, expectation agenda, etc
//
//-----------------------------------------------------------------------------

// A: Floor status constants
// floor ״̬����
typedef enum
{
	fsUnknown,		// floor owner is unknown		δ֪
	fsUser,			// floor owner is user			�û�
	fsSystem,		// floor owner is system		ϵͳ
	fsFree,			// floor is not owned by anyone	����
} TFloorStatus;

extern vector<string> vsFloorStatusLabels;

class CDMCoreAgent : public CAgent
{

private:

	//---------------------------------------------------------------------
	// Private members
	//---------------------------------------------------------------------
	//
	TExecutionStack esExecutionStack;		// the execution stack				//ִ��ջ
	CExecutionHistory ehExecutionHistory;	// the execution history			//ִ����ʷ
	TBindingHistory bhBindingHistory;       // the binding history				//����ʷ
	TExpectationAgenda eaAgenda;			// the expectation agenda			//����agenda
	TFocusClaimsList fclFocusClaims;		// the list of focus claims			//���������б�
	TSystemAction saSystemAction;			// the current system action		//��ǰϵͳ����

	int iTimeoutPeriod;						// the current timeout period		//��ǰ��ʱ�׶�
	int iDefaultTimeoutPeriod;				// the default timeout period		//Ĭ�ϳ�ʱ�׶�

	float fNonunderstandingThreshold;       // the current nonunderstanding		//��ǰ�������ֵ
	//  threshold
	float fDefaultNonunderstandingThreshold;// the default nonunderstanding		//Ĭ�ϲ������ֵ
	//  threshold                                                 

	STRING2BFF s2bffFilters;                // the register of binding			//�󶨹��������Ƶ�ʵ�ʺ�����ӳ��
	//  filters

	bool bFocusClaimsPhaseFlag;             // indicates whether we should		//�Ƿ����н�������
	//  run focus claims
	bool bAgendaModifiedFlag;				// indicates if the agenda should	//agenda�Ƿ��޸Ĺ�������װ��
	// be recompiled

	TFloorStatus fsFloorStatus;             // indicates who has the floor		// floor ״̬����[δ֪��ϵͳ���û�������]
	int iTurnNumber;						// stores the current turn number	//��ǰ��turn��
	TCustomStartOverFunct csoStartOverFunct;// a custom start over function		//�Զ�������¿�ʼ����

	//---------------------------------------------------------------------
	// The grounding manager needs access to internals, so it is declared
	// as a friend class
	//---------------------------------------------------------------------

	//Grouding ��Ҫ�����ڲ���Ա������Ϊ��Ԫ��
	friend class CGroundingManagerAgent;
	friend class CStateManagerAgent;

public:

	//---------------------------------------------------------------------
	// Constructor and destructor
	//---------------------------------------------------------------------
	//
	// Default constructor
	CDMCoreAgent(string sAName,
		string sAConfiguration = "",
		string sAType = "CAgent:CDMCoreAgent");

	// Virtual destructor
	virtual ~CDMCoreAgent();

	// static function for dynamic agent creation
	// ��̬������̬����agent����
	static CAgent* AgentFactory(string sAName, string sAConfiguration);

	//---------------------------------------------------------------------
	// CAgent Class overwritten methods 
	//---------------------------------------------------------------------
	//�� - ����
	virtual void Reset();					// overwritten Reset

public:

	//---------------------------------------------------------------------
	//
	// DMCoreAgent class specific public methods
	//
	//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	// Execution
	//---------------------------------------------------------------------

	// Executes the dialog task
	// ִ�жԻ�����
	void Execute();

	//---------------------------------------------------------------------
	// Method for performing an input pass (and related)
	//---------------------------------------------------------------------

	// Waits for and processes the next real world event
	// �ȴ��ʹ�����һ�� �¼�
	void AcquireNextEvent();

	// Registers a customized binding filter
	// ע���Զ���󶨹�����
	void RegisterBindingFilter(string sBindingFilterName, TBindingFilterFunct bffFilter);

	//---------------------------------------------------------------------
	// Methods for accessing Interface-specific variables
	// ���ʽӿ��ض������ķ���
	//---------------------------------------------------------------------

	int GetIntSessionID();

	//---------------------------------------------------------------------
	// Timeout related methods
	// ��ʱ��ط���
	//---------------------------------------------------------------------

	// Sets and gets the timeout for the system 
	// ���û�ȡϵͳ��ʱ
	void SetTimeoutPeriod(int iATimeoutPeriod);
	int GetTimeoutPeriod();

	// Sets the default timeout period for the system
	// ���û�ȡϵͳĬ�ϳ�ʱ
	void SetDefaultTimeoutPeriod(int iADefaultTimeoutPeriod);
	int GetDefaultTimeoutPeriod();

	//---------------------------------------------------------------------
	// Methods for accessing the nonunderstanding threshold
	// ���ʷ������ֵ�ķ���
	//---------------------------------------------------------------------

	// Sets and gets the nonunderstanding threshold for the system
	// ���û�ȡ �������ֵ
	void SetNonunderstandingThreshold(float fANonunderstandingThreshold);
	float GetNonunderstandingThreshold();

	// Sets the default nonunderstanding threshold for the system
	// ���û�ȡϵͳĬ�ϲ������ֵ
	void SetDefaultNonunderstandingThreshold(float fANonuThreshold);
	float GetDefaultNonunderstandingThreshold();

	//---------------------------------------------------------------------
	// Signaling the need for running a focus claims phase
	// ���ź�֪ͨ��Ҫ���н��������׶�
	//---------------------------------------------------------------------

	void SignalFocusClaimsPhase(bool bAFocusClaimsPhaseFlag = true);

	//---------------------------------------------------------------------
	// Signaling floor changes
	// floor �ı�
	//---------------------------------------------------------------------

	void SetFloorStatus(TFloorStatus fsaFloorStatus);
	TFloorStatus GetFloorStatus();
	void CDMCoreAgent::SetFloorStatus(string sAFloorStatus);

	string FloorStatusToString(TFloorStatus fsAFloor);
	TFloorStatus StringToFloorStatus(string sAFloor);

	//---------------------------------------------------------------------
	// Access to various private fields
	// ˽�ж������
	//---------------------------------------------------------------------

	// Gets the number of concepts bound in the last input phase
	// �������׶ΰ󶨵�concept��Ŀ
	int LastTurnGetConceptsBound();

	// Returns true if there was a non-understanding in the last input phase
	// �Ƿ�����������
	bool LastTurnNonUnderstanding();

	// Returns the number of consecutive non-understandings so far
	// �����������Ŀ
	int GetNumberNonUnderstandings();

	// Returns the total number non-understandings so far
	// �ܲ������
	int GetTotalNumberNonUnderstandings();

	//---------------------------------------------------------------------
	// Methods for execution and access to the execution stack, history, and binding history
	// ִ�кͷ���ִ�ж�ջ����ʷ�Ͱ���ʷ�ķ���
	//---------------------------------------------------------------------

	// Plans a new agent for execution (pushes it on the execution stack)
	// ������һ��ִ�е�agent [�ŵ�ջ��]
	void ContinueWith(CAgent* paPusher, CDialogAgent* pdaDialogAgent);

	// Restarts an agent
	// ��������һ��agent
	void RestartTopic(CDialogAgent* pdaDialogAgent);

	// Registers a customized start over routine
	// ע���Զ������������
	void RegisterCustomStartOver(TCustomStartOverFunct csoAStartOverFunct);

	// The start over function
	// ��������
	void StartOver();

	// Returns the agent on top of the execution stack (the focused one)
	// ����ջ��agent
	CDialogAgent* GetAgentInFocus();

	// Returns the dialog task specification agent in focus (closest to the top of the execution stack
	// ����DTT�Ľ���agent
	CDialogAgent* GetDTSAgentInFocus();

	// Returns true if the agent is in focus
	// agent�Ƿ��ǽ���
	bool AgentIsInFocus(CDialogAgent* pdaDialogAgent);

	// Returns the previously focused agent (previous with respect to the incoming parameter)
	// ����ջ��ǰһ������agent
	CDialogAgent* GetAgentPreviouslyInFocus(CDialogAgent* pdaDialogAgent = NULL);

	// Returns the previously focused dialog task specification agent 
	// (previous with respect to the incoming parameter)
	// ����DTT�е�ǰһ������agent
	CDialogAgent* GetDTSAgentPreviouslyInFocus(CDialogAgent* pdaDialogAgent = NULL);

	// Returns the current main topic agent
	// ���ص�ǰtopic��agent
	CDialogAgent* GetCurrentMainTopicAgent();

	// Returns true if the agent is currently active (i.e. it is 
	// somewhere on the execution stack)
	// �ж�agent�Ƿ��Ծ
	bool AgentIsActive(CDialogAgent* pdaDialogAgent);

	// Eliminates an agent from the execution stack
	// ɾ��ջ��agent
	void PopAgentFromExecutionStack(CDialogAgent* pdaADialogAgent);

	// Eliminates an agent from the execution stack, together with all
	// the subagents it has planned
	// ɾ��ջ��topic agent�� ͬʱɾ������ ��agent
	void PopTopicFromExecutionStack(CDialogAgent* pdaADialogAgent);

	// Eliminates all grounding agents from the execution stack
	// ɾ������grouding agent
	void PopGroundingAgentsFromExecutionStack();

	// Returns the size of the binding history
	// ���ذ���ʷ��size
	int GetBindingHistorySize();

	// Returns a reference to the binding history item
	// ͨ��������������
	const TBindingsDescr& GetBindingResult(int iBindingHistoryIndex);

	// Returns the number of the last input turn
	// ������������turn��
	int GetLastInputTurnNumber();

	// Returns a description of the system action on a particular concept
	// ���ض��ض������ϵͳ����������
	TSystemActionOnConcept GetSystemActionOnConcept(CConcept* pConcept);

	// Signal explicit and implicit confirms on a certain concept
	// �ź���ʽ����ʽȷ��ĳһ����
	void SignalExplicitConfirmOnConcept(CConcept* pConcept);
	void SignalImplicitConfirmOnConcept(CConcept* pConcept);
	void SignalUnplannedImplicitConfirmOnConcept(int iState, CConcept* pConcept);
	void SignalAcceptOnConcept(CConcept* pConcept);

	//---------------------------------------------------------------------
	// Methods for floor handling
	//---------------------------------------------------------------------



private:

	//---------------------------------------------------------------------
	// DMCoreManagerAgent private methods related to the execution stack
	// ����ִ��ջ��˽�з���
	//---------------------------------------------------------------------

	// Pops all the completed agents from the execution stack, and returns
	// the number popped
	// ɾ����ɵ�agent,������Ŀ
	int popCompletedFromExecutionStack();

	// Pops a dialog agent from the execution stack
	// ɾ��һ��agent
	void popAgentFromExecutionStack(CDialogAgent *pdaADialogAgent, TStringVector& rvsAgentsEliminated);

	// Pops a dialog agent from the execution stack, together with all the
	// sub-agents (recursively) that it has planned for execution
	// ɾ��topic agent ������������agent��
	void popTopicFromExecutionStack(CDialogAgent *pdaADialogAgent, TStringVector& rvsAgentsEliminated);

	// Eliminates all grounding agents from the execution stack
	// ɾ������grouding agent
	void popGroundingAgentsFromExecutionStack(TStringVector& rvsAgentsEliminated);

	//---------------------------------------------------------------------
	// DMCoreManagerAgent private methods related to the input pass
	// �����˽�з���
	//---------------------------------------------------------------------

	// Method for logging the concepts
	//log��¼concept
	void dumpConcepts();

	// Methods for logging the execution stack
	//log��¼ִ��ջ
	void dumpExecutionStack();
	string executionStackToString();
	string executionStackToString(TExecutionStack es);

	// Methods for computing the current system action 
	// ���㵱ǰϵͳ��action
	void clearCurrentSystemAction();
	void computeCurrentSystemAction();
	string systemActionToString(TSystemAction saASystemAction);
	string currentSystemActionToString();

	// Methods for assembling, logging, and broadcasting the expectation agenda
	// ��װ����¼���㲥����agenda
	void assembleExpectationAgenda();
	void compileExpectationAgenda();
	void enforceBindingPolicies();
	void broadcastExpectationAgenda();
	string expectationAgendaToString();
	string expectationAgendaToBroadcastString();
	string expectationAgendaToBroadcastString(TExpectationAgenda eaBAgenda);

	// Method for logging the bindings description
	//log��¼binding ���η�
	string bindingsDescrToString(TBindingsDescr& rbdBindings);

	// J:  Updates the speech recognizer's configuration according to the execution stack
	//����ִ�ж�ջ��������ʶ��������
	void updateInputLineConfiguration();

	// Assembles the list of focus claims
	// ��װ���������б�
	int assembleFocusClaims();

	// Binds the concepts from the input, according to an agenda
	// ͨ��agenda��concept
	void bindConcepts(TBindingsDescr& rbdBindings);

	// Helper function which performs an actual binding to a concept
	// ������������ִ�жԸ����ʵ�ʰ�
	void performConceptBinding(string sSlotName, string sSlotValue, float fConfidence, int iExpectationIndex, bool bIsComplete);

	// Helper function that performs a binding through a customized binding filter
	//����������ͨ���Զ���󶨹�����ִ�а�
	void performCustomConceptBinding(int iExpectationIndex);

	// Helper function that performs the forced concept updates (if 
	// there are concepts that are being explicitly or implicitly 
	// confirmed and they are not being updated in the binding phase
	// then force an update on these concepts)
	//��������������ִ��ǿ�Ƹ�����£����������ʽ����ʽȷ�ϵĸ�����������ڰ󶨽׶�δ���£���ǿ�ƶ���Щ������и��£�
	void performForcedConceptUpdates(TBindingsDescr& rbdBindings);

	// Process non-understandings
	// ���� �����
	void processNonUnderstanding();

	// Analyze the need for and resolve focus shifts
	// ��������ͽ�����ת��
	void resolveFocusShift();

	// Rolls back to a previous dialog state (e.g. after a user barge-in)
	//�ع�����һ���Ի���״̬�����磬���û�����֮��
	void rollBackDialogState(int iState);
};

#endif // __DMCOREAGENT_H__