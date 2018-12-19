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
//D：编译概念期望：实现为字符串（语法概念）和整数向量（概念期望列表中的指针）之间的映射。
//   这种表示法用于更快地检查语法概念期望
//   slotName -> (vector)slotIndex
typedef map <string, TIntVector, less <string>, allocator <TIntVector> >
TMapCE;

typedef struct
{
	TMapCE mapCE;					// the hash of compiled expectations		编译期望的哈希
	CDialogAgent* pdaGenerator;		// the agent that represents that level		代表当前level期望的代理
	//  of expectations
} TCompiledExpectationLevel;

// D: the struct representation for the compiled expectation agenda. It 
//    contains the expectation list gathered from the dialog task tree, 
//    and a "compiled" representation for each of the levels
// D：编译的期望议程的struct表示。 它包含从对话任务树收集的期望列表，以及每个级别的“已编译”表示
typedef struct
{
	// the full system expectations, as gathered from the dialog task tree
	// 从对话任务树收集的完整系统期望
	TConceptExpectationList celSystemExpectations;

	// an array holding the expectations of different levels (on index 0, 
	// are the expectations of the focused agent, on index 1 those of the
	// immediate upper level agent, etc)
	// 保持不同level的期望的数组（在索引0上，是focus agent的期望，在索引1上的直接上层agent的期望等）
	vector <TCompiledExpectationLevel, allocator <TCompiledExpectationLevel> > 	vCompiledExpectations;
} TExpectationAgenda;


//-----------------------------------------------------------------------------
// D: Auxiliary type definitions for the execution stack and history
//-----------------------------------------------------------------------------

// D: structure holding an execution stack item
// 结构保存执行堆栈项
typedef struct
{
	CDialogAgent* pdaAgent;			// the agent that is on the stack					agent指针
	int iEHIndex;					// an index into it's correspondent history entry	历史索引
} TExecutionStackItem;

// D: definition of the execution stack type - essentially a list 
// 定义执行堆栈类型 - 本质上是一个列表
typedef list <TExecutionStackItem, allocator<TExecutionStackItem> >
TExecutionStack;

// D: structure holding a execution history item
// 保存执行历史 item 项的结构体
typedef struct
{
	string sCurrentAgent;			// the name of the agent that is executed	要执行的Agent的名称
	string sCurrentAgentType;		// the type of that agent					Agent 类型
	string sScheduledBy;			// the agent that scheduled this one for    调度当前agent的父Agent
	//   execution
	bool bScheduled;				// has the history item been scheduled		是否有被调度执行历史
	//   for execution?
	bool bExecuted;					// has the history item been executed?		有历史项被执行？
	bool bCommitted;				// has the history item been committed		有历史项已被提交到History?
	//   to history?
	bool bCanceled;					// has the history item been canceled		在执行前，历史item是否被取消
	//   before being committed?
	_timeb timeScheduled;			// the time when the agent was scheduled	被调度的时间【一个】
	//   for execution
	vector<_timeb, allocator<_timeb> >
		vtExecutionTimes;			// the times when the agent was actually	agent被实际执行的时间【多个】
	//   executed
	_timeb timeTerminated;			// the time when the agent completed		agent完成的时间【一个】
	//   execution
	int iStateHistoryIndex;			// the index in the history of dialog		执行代理时dialog state的历史记录中的索引
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
	//  the expectation was disabled
} TBinding;

// D: structure describing a particular forced update
#define FCU_EXPLICIT_CONFIRM	1			//强制	- 显示确认
#define FCU_IMPLICIT_CONFIRM	2			//强制	- 隐式确认
#define FCU_UNPLANNED_IMPLICIT_CONFIRM  3	//无计划	- 隐式确认
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
// D：保存绑定历史记录项的结构，描述一个回合中的绑定
typedef struct
{
	//    int iTurnNumber;                // the input turn number      
	string sEventType;				// the type of event to which this			event类型 "IET_USER_UTT_START", ...
	// binding corresponds
	bool bNonUnderstanding;         // was the turn a non-understanding?		是否no-understanding [例如：没有concept绑定]
	// i.e. no concepts bound
	int iConceptsBound;             // the number of bound concepts				绑定的concept数目
	int iConceptsBlocked;           // the number of blocked concepts			阻塞Concept数
	int iSlotsMatched;				// the number of slots that matched			匹配的slot数
	int iSlotsBlocked;				// the number of slots that were blocked	阻塞的slot数	
	vector<TBinding> vbBindings;	// the vector of bindings					绑定vector
	vector<TForcedConceptUpdate> vfcuForcedUpdates;								//强制更新的向量
	// the vector of forced updates
} TBindingsDescr;

// D: definition of the binding history - a vector
// D：绑定历史的定义 - 一个向量
typedef vector<TBindingsDescr, allocator<TBindingsDescr> >
TBindingHistory;

// D: definition of function type for performing customized bindings 
//     (binding filter functions)
// 定义用户定制绑定的函数 [slotName, slotValue]
typedef string(*TBindingFilterFunct)(string sSlotName, string sSlotValue);

// D: auxiliary definition for mapping from binding filter names to the actual functions
// 辅助定义，用于从绑定过滤器名称到实际函数的映射
typedef map<string, TBindingFilterFunct,
	less<string>, allocator<TBindingFilterFunct> > STRING2BFF;

// D: structure maintaining a description of the current system action on the
//    various concepts
// D：保持当前系统action对各种概念的描述的结构
typedef struct
{
	set<CConcept *> setcpRequests;					//Turn Grouding
	set<CConcept *> setcpExplicitConfirms;			//显示确认
	set<CConcept *> setcpImplicitConfirms;			//隐式确认
	set<CConcept *> setcpUnplannedImplicitConfirms;	//正常Concept
} TSystemAction;

// D: structure describing system actions taken on a particular concept
// D：描述对特定概念采取的系统操作的结构
#define SA_REQUEST	"REQ"
#define SA_EXPL_CONF	"EC"
#define SA_IMPL_CONF	"IC"
#define SA_UNPLANNED_IMPL_CONF  "UIC"
#define SA_OTHER	"OTH"
typedef struct
{
	string sSystemAction;
	// *** this might need to be elaborated with other system actions
	// 这可能需要与其他系统操作一起进行阐述
} TSystemActionOnConcept;

// D: definition for customized start over routine
// 一个自定义的开始函数
typedef void(*TCustomStartOverFunct)();

//-----------------------------------------------------------------------------
//
// D: CDMCoreAgent class -
//      implements core dialog management functionalities execution, input 
//      passes, expectation agenda, etc
//
//-----------------------------------------------------------------------------

// A: Floor status constants
// floor 状态常量
typedef enum
{
	fsUnknown,		// floor owner is unknown		未知
	fsUser,			// floor owner is user			用户
	fsSystem,		// floor owner is system		系统
	fsFree,			// floor is not owned by anyone	自由
} TFloorStatus;

extern vector<string> vsFloorStatusLabels;

class CDMCoreAgent : public CAgent
{

private:

	//---------------------------------------------------------------------
	// Private members
	//---------------------------------------------------------------------
	//
	TExecutionStack esExecutionStack;		// the execution stack				//执行栈
	CExecutionHistory ehExecutionHistory;	// the execution history			//执行历史
	TBindingHistory bhBindingHistory;       // the binding history				//绑定历史
	TExpectationAgenda eaAgenda;			// the expectation agenda			//期望agenda
	TFocusClaimsList fclFocusClaims;		// the list of focus claims			//焦点声明列表
	TSystemAction saSystemAction;			// the current system action		//当前系统动作

	int iTimeoutPeriod;						// the current timeout period		//当前超时阶段
	int iDefaultTimeoutPeriod;				// the default timeout period		//默认超时阶段

	float fNonunderstandingThreshold;       // the current nonunderstanding		//当前不理解阈值
	//  threshold
	float fDefaultNonunderstandingThreshold;// the default nonunderstanding		//默认不理解阈值
	//  threshold                                                 

	STRING2BFF s2bffFilters;                // the register of binding			//绑定过滤器名称到实际函数的映射
	//  filters

	bool bFocusClaimsPhaseFlag;             // indicates whether we should		//是否运行焦点声明
	//  run focus claims
	bool bAgendaModifiedFlag;				// indicates if the agenda should	//agenda是否修改过【重组装】
	// be recompiled

	TFloorStatus fsFloorStatus;             // indicates who has the floor		// floor 状态常量[未知、系统、用户、自由]
	int iTurnNumber;						// stores the current turn number	//当前的turn数
	TCustomStartOverFunct csoStartOverFunct;// a custom start over function		//自定义的重新开始函数

	//---------------------------------------------------------------------
	// The grounding manager needs access to internals, so it is declared
	// as a friend class
	//---------------------------------------------------------------------

	//Grouding 需要访问内部成员，声明为友元类
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
	// 静态工厂动态创建agent对象
	static CAgent* AgentFactory(string sAName, string sAConfiguration);

	//---------------------------------------------------------------------
	// CAgent Class overwritten methods 
	//---------------------------------------------------------------------
	//虚 - 重置
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
	// 执行对话任务
	void Execute();

	//---------------------------------------------------------------------
	// Method for performing an input pass (and related)
	//---------------------------------------------------------------------

	// Waits for and processes the next real world event
	// 等待和处理下一个 事件
	void AcquireNextEvent();

	// Registers a customized binding filter
	// 注册自定义绑定过滤器
	void RegisterBindingFilter(string sBindingFilterName, TBindingFilterFunct bffFilter);

	//---------------------------------------------------------------------
	// Methods for accessing Interface-specific variables
	// 访问接口特定变量的方法
	//---------------------------------------------------------------------

	int GetIntSessionID();

	//---------------------------------------------------------------------
	// Timeout related methods
	// 超时相关方法
	//---------------------------------------------------------------------

	// Sets and gets the timeout for the system 
	// 设置获取系统超时
	void SetTimeoutPeriod(int iATimeoutPeriod);
	int GetTimeoutPeriod();

	// Sets the default timeout period for the system
	// 设置获取系统默认超时
	void SetDefaultTimeoutPeriod(int iADefaultTimeoutPeriod);
	int GetDefaultTimeoutPeriod();

	//---------------------------------------------------------------------
	// Methods for accessing the nonunderstanding threshold
	// 访问非理解阈值的方法
	//---------------------------------------------------------------------

	// Sets and gets the nonunderstanding threshold for the system
	// 设置获取 不理解阈值
	void SetNonunderstandingThreshold(float fANonunderstandingThreshold);
	float GetNonunderstandingThreshold();

	// Sets the default nonunderstanding threshold for the system
	// 设置获取系统默认不理解阈值
	void SetDefaultNonunderstandingThreshold(float fANonuThreshold);
	float GetDefaultNonunderstandingThreshold();

	//---------------------------------------------------------------------
	// Signaling the need for running a focus claims phase
	// 发信号通知需要运行焦点声明阶段
	//---------------------------------------------------------------------

	void SignalFocusClaimsPhase(bool bAFocusClaimsPhaseFlag = true);

	//---------------------------------------------------------------------
	// Signaling floor changes
	// floor 改变
	//---------------------------------------------------------------------

	void SetFloorStatus(TFloorStatus fsaFloorStatus);
	TFloorStatus GetFloorStatus();
	void CDMCoreAgent::SetFloorStatus(string sAFloorStatus);

	string FloorStatusToString(TFloorStatus fsAFloor);
	TFloorStatus StringToFloorStatus(string sAFloor);

	//---------------------------------------------------------------------
	// Access to various private fields
	// 私有对象访问
	//---------------------------------------------------------------------

	// Gets the number of concepts bound in the last input phase
	// 最后输入阶段绑定的concept数目
	int LastTurnGetConceptsBound();

	// Returns true if there was a non-understanding in the last input phase
	// 是否理解最后输入
	bool LastTurnNonUnderstanding();

	// Returns the number of consecutive non-understandings so far
	// 连续不理解数目
	int GetNumberNonUnderstandings();

	// Returns the total number non-understandings so far
	// 总不理解数
	int GetTotalNumberNonUnderstandings();

	//---------------------------------------------------------------------
	// Methods for execution and access to the execution stack, history, and binding history
	// 执行和访问执行堆栈，历史和绑定历史的方法
	//---------------------------------------------------------------------

	// Plans a new agent for execution (pushes it on the execution stack)
	// 决定下一个执行的agent [放到栈顶]
	void ContinueWith(CAgent* paPusher, CDialogAgent* pdaDialogAgent);

	// Restarts an agent
	// 重新启动一个agent
	void RestartTopic(CDialogAgent* pdaDialogAgent);

	// Registers a customized start over routine
	// 注册自定义的启动过程
	void RegisterCustomStartOver(TCustomStartOverFunct csoAStartOverFunct);

	// The start over function
	// 重新启动
	void StartOver();

	// Returns the agent on top of the execution stack (the focused one)
	// 返回栈顶agent
	CDialogAgent* GetAgentInFocus();

	// Returns the dialog task specification agent in focus (closest to the top of the execution stack
	// 返回DTT的焦点agent
	CDialogAgent* GetDTSAgentInFocus();

	// Returns true if the agent is in focus
	// agent是否是焦点
	bool AgentIsInFocus(CDialogAgent* pdaDialogAgent);

	// Returns the previously focused agent (previous with respect to the incoming parameter)
	// 返回栈中前一个焦点agent
	CDialogAgent* GetAgentPreviouslyInFocus(CDialogAgent* pdaDialogAgent = NULL);

	// Returns the previously focused dialog task specification agent 
	// (previous with respect to the incoming parameter)
	// 返回DTT中的前一个焦点agent
	CDialogAgent* GetDTSAgentPreviouslyInFocus(CDialogAgent* pdaDialogAgent = NULL);

	// Returns the current main topic agent
	// 返回当前topic　agent
	CDialogAgent* GetCurrentMainTopicAgent();

	// Returns true if the agent is currently active (i.e. it is 
	// somewhere on the execution stack)
	// 判断agent是否活跃
	bool AgentIsActive(CDialogAgent* pdaDialogAgent);

	// Eliminates an agent from the execution stack
	// 删除栈顶agent
	void PopAgentFromExecutionStack(CDialogAgent* pdaADialogAgent);

	// Eliminates an agent from the execution stack, together with all
	// the subagents it has planned
	// 删除栈顶topic agent， 同时删除所有 子agent
	void PopTopicFromExecutionStack(CDialogAgent* pdaADialogAgent);

	// Eliminates all grounding agents from the execution stack
	// 删除所有grouding agent
	void PopGroundingAgentsFromExecutionStack();

	// Returns the size of the binding history
	// 返回绑定历史的size
	int GetBindingHistorySize();

	// Returns a reference to the binding history item
	// 通过索引返回引用
	const TBindingsDescr& GetBindingResult(int iBindingHistoryIndex);

	// Returns the number of the last input turn
	// 返回最后输入的turn数
	int GetLastInputTurnNumber();

	// Returns a description of the system action on a particular concept
	// 返回对特定概念的系统操作的描述
	TSystemActionOnConcept GetSystemActionOnConcept(CConcept* pConcept);

	// Signal explicit and implicit confirms on a certain concept
	// 信号显式和隐式确认某一概念
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
	// 关于执行栈的私有方法
	//---------------------------------------------------------------------

	// Pops all the completed agents from the execution stack, and returns
	// the number popped
	// 删除完成的agent,返回数目
	int popCompletedFromExecutionStack();

	// Pops a dialog agent from the execution stack
	// 删除一个agent
	void popAgentFromExecutionStack(CDialogAgent *pdaADialogAgent, TStringVector& rvsAgentsEliminated);

	// Pops a dialog agent from the execution stack, together with all the
	// sub-agents (recursively) that it has planned for execution
	// 删除topic agent 【包括所以子agent】
	void popTopicFromExecutionStack(CDialogAgent *pdaADialogAgent, TStringVector& rvsAgentsEliminated);

	// Eliminates all grounding agents from the execution stack
	// 删除所有grouding agent
	void popGroundingAgentsFromExecutionStack(TStringVector& rvsAgentsEliminated);

	//---------------------------------------------------------------------
	// DMCoreManagerAgent private methods related to the input pass
	// 输入的私有方法
	//---------------------------------------------------------------------

	// Method for logging the concepts
	//log记录concept
	void dumpConcepts();

	// Methods for logging the execution stack
	//log记录执行栈
	void dumpExecutionStack();
	string executionStackToString();
	string executionStackToString(TExecutionStack es);

	// Methods for computing the current system action 
	// 计算当前系统的action
	void clearCurrentSystemAction();
	void computeCurrentSystemAction();
	string systemActionToString(TSystemAction saASystemAction);
	string currentSystemActionToString();

	// Methods for assembling, logging, and broadcasting the expectation agenda
	// 组装、记录、广播期望agenda
	void assembleExpectationAgenda();
	void compileExpectationAgenda();
	void enforceBindingPolicies();
	void broadcastExpectationAgenda();
	string expectationAgendaToString();
	string expectationAgendaToBroadcastString();
	string expectationAgendaToBroadcastString(TExpectationAgenda eaBAgenda);

	// Method for logging the bindings description
	//log记录binding 修饰符
	string bindingsDescrToString(TBindingsDescr& rbdBindings);

	// J:  Updates the speech recognizer's configuration according to the execution stack
	//根据执行堆栈更新语音识别器配置
	void updateInputLineConfiguration();

	// Assembles the list of focus claims
	// 组装焦点声明列表
	int assembleFocusClaims();

	// Binds the concepts from the input, according to an agenda
	// 通过agenda绑定concept
	void bindConcepts(TBindingsDescr& rbdBindings);

	// Helper function which performs an actual binding to a concept
	// 辅助函数，其执行对概念的实际绑定
	void performConceptBinding(string sSlotName, string sSlotValue, float fConfidence, int iExpectationIndex, bool bIsComplete);

	// Helper function that performs a binding through a customized binding filter
	//帮助函数，通过自定义绑定过滤器执行绑定
	void performCustomConceptBinding(int iExpectationIndex);

	// Helper function that performs the forced concept updates (if 
	// there are concepts that are being explicitly or implicitly 
	// confirmed and they are not being updated in the binding phase
	// then force an update on these concepts)
	//辅助函数，用于执行强制概念更新（如果存在显式或隐式确认的概念，并且它们在绑定阶段未更新，则强制对这些概念进行更新）
	void performForcedConceptUpdates(TBindingsDescr& rbdBindings);

	// Process non-understandings
	// 处理 不理解
	void processNonUnderstanding();

	// Analyze the need for and resolve focus shifts
	// 分析需求和解决检点转移
	void resolveFocusShift();

	// Rolls back to a previous dialog state (e.g. after a user barge-in)
	//回滚到上一个对话框状态（例如，在用户插入之后）
	void rollBackDialogState(int iState);
};

#endif // __DMCOREAGENT_H__