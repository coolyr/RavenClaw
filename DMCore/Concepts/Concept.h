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
// CONCEPT.H   - implementation of the CConcept base class
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
//   [2006-06-15] (antoine): merged with Calista belief updating framework
//							 (from RavenClaw 1)
//	 [2005-11-07] (antoine): added support for partial concept update
//   [2004-12-23] (dbohus):  added GetSmallName
//   [2004-06-02] (dbohus):  added definition of pOwnerConcept, concepts now
//                            check with parent if unclear if they are
//                            grounded
//   [2004-05-30] (dbohus):  changed definition of IsGrounded
//   [2004-05-19] (dbohus):  major makeover on concepts, introduced frame 
//                            concepts and structure concepts which support 
//                            grounding
//   [2004-05-15] (dbohus):  cleaned up concepts code, added Update as well as
//                            various types of update methods
//   [2004-04-20] (jsherwan):added InsertAt()
//   [2004-04-27] (dbohus):  added ConveyanceAsString()
//   [2004-04-25] (dbohus):  added WaitingConveyance flag and functionality
//   [2004-04-18] (dbohus):  renamed MergedHistory to 
//                            CreateMergedHistoryConcept and added a 
//                            MergeHistory function which merges the history
//                            up into the current value for a concept
//   [2004-04-17] (dbohus):  added support for declaring the grounding models
//                            subsumed by the concept
//   [2004-04-16] (dbohus):  removed grounding models from constructors and 
//                            added them as a separate method
//   [2003-04-28] (antoine): fixed == operator for ValConfSet so that two empty
//                            concepts are now equal
//   [2003-04-14] (dbohus):  added support for OwnerDialogAgent
//   [2003-04-10] (dbohus):  added Restore on concepts
//   [2003-04-03] (dbohus):  added support for concept grounding models
//   [2003-04-02] (dbohus):  introduced support for grounding models
//   [2003-04-01] (dbohus):  added support for iTurnLastUpdated
//   [2003-03-19] (dbohus):  eliminated ValueToString, transformed 
//                            ValueToStringWithConf into ValueSetToString
//   [2003-03-13] (antoine): added CConcept::ModeValueToString to return a 
//                            string representing the value of the mode of the
//                            value/confidence distribution of the concept
//   [2003-01-29] (dbohus):  added equality and comparison operators
//   [2002-12-09] (dbohus):  major revamp on concepts: added support for 
//                            reopening concepts and maintaing a history of
//                            previous values 
//   [2002-12-01] (dbohus):  added CValConfSet::GetModeValConf to obtain the 
//                            mode of the distribution
//   [2002-12-01] (dbohus):  added SetValueFromString and UpdateValueFromString
//                            functions on concepts
//   [2002-11-26] (dbohus):  added CValConfSet and changed concepts to work 
//                            with it
//   [2002-11-26] (dbohus):  fixed bug in assign operator when assigning to 
//                            self
//   [2002-05-25] (dbohus):  deemed preliminary stable version 0.5
//	 [2002-04-10] (agh)   :  added Conveyance type and CConcept conveyance 
// 							  member
//   [2002-01-08] (dbohus):  started working on this
//
//-----------------------------------------------------------------------------

#pragma once
#ifndef __CONCEPT_H__
#define __CONCEPT_H__

#include "../../Utils/Utils.h"
#include "../../DMCore/Grounding/Grounding.h"

//-----------------------------------------------------------------------------
// Definitions of concept types
//-----------------------------------------------------------------------------
//定义concept的类型 - 枚举
typedef enum
{
	ctUnknown, ctInt, ctBool, ctString, ctFloat, ctStruct, ctFrame, ctArray
} TConceptType;

// D: and definitions for corresponding strings (used for logging purposes)
static string ConceptTypeAsString[] = { "Unknown", "Int", "Bool", "String",
"Float", "Struct", "Frame", "Array" };

// D: type for specifing the concept source: is it a user or a system concept
// 定义concept的类别
typedef enum { csUser, csSystem } TConceptSource;

// A: type for specifying whether a concept has been conveyed to the user
//	类型，用于指定是否已将概念传达给用户 [传达、 没传达、失败]
typedef enum { cConveyed, cNotConveyed, cFailed } TConveyance;

// D: and definitions for corresponding strings
static string ConveyanceAsString[] = { "Conveyed", "Not conveyed", "Failed" };

//-----------------------------------------------------------------------------
// Definitions of separators used in conversions. These cannot appear in the
//  values of concepts (i.e. most dangerous are string concepts)
// 用于转换的分隔符的定义。 这些不能出现在概念的值中（即最危险的是字符串概念）
//-----------------------------------------------------------------------------
#define VAL_CONF_SEPARATOR "|"
#define HYPS_SEPARATOR ";"
#define CONCEPT_EQUALS_SIGN "= \t"

#define ABSTRACT_CONCEPT "<ABSTRACT>\n"
#define UNDEFINED_CONCEPT "<UNDEFINED>\n"
#define INVALIDATED_CONCEPT "<INVALIDATED>\n"
#define UNDEFINED_VALUE "<UNDEF_VAL>"

//-----------------------------------------------------------------------------
// CHyp class - this is the base class for the hierarchy of hypothesis
//              classes. It essentially implements a type and an associated 
//              value + confidence score
// CHyp类 - 这是假设类层次结构的基类。 它本质上实现了类型和相关联的值+置信度得分
//-----------------------------------------------------------------------------
class CHyp
{

protected:

	//---------------------------------------------------------------------
	// Protected member variables 
	//---------------------------------------------------------------------
	//
	TConceptType ctHypType;			// the concept (hypothesis) type	//concept类型 => [ctUnknown, ctInt, ctBool, ctString, ctFloat, ctStruct, ctFrame, ctArray]
	float fConfidence;              // the confidence score				//置信度得分

public:

	//---------------------------------------------------------------------
	// Constructors and Destructors
	//---------------------------------------------------------------------
	//
	CHyp();
	CHyp(CHyp& rAHyp);
	virtual ~CHyp();

	//---------------------------------------------------------------------
	// Access to member variables
	// 访问成员变量
	//---------------------------------------------------------------------

	// Access to concept (hypothesis) type 
	TConceptType GetHypType();

	// Access to confidence information
	//
	float GetConfidence();

	//---------------------------------------------------------------------
	// Virtual functions to be overridden by the derived hyp classes
	// 虚函数被派生的hyp类重写
	//---------------------------------------------------------------------

	// Assignment operator
	virtual CHyp& operator = (CHyp& rAHyp);

	// Set confidence score
	virtual void SetConfidence(float fAConfidence);

	// Comparison operators (ignores confidence scores)
	//
	virtual bool operator == (CHyp& rAHyp);
	virtual bool operator < (CHyp& rAHyp);
	virtual bool operator > (CHyp& rAHyp);
	virtual bool operator <= (CHyp& rAHyp);
	virtual bool operator >= (CHyp& rAHyp);

	// Indexing operator
	//
	virtual CHyp* operator [](string sItem);

	// String conversion functions
	//
	virtual string ValueToString();
	virtual string ToString();
	virtual void FromString(string sString);
};


//-----------------------------------------------------------------------------
// CConcept class - this is the base of the hierarchy of concept classes. It 
//                  implements the basic properties and functionality of 
//                  a concept
//-----------------------------------------------------------------------------

// D: definition of default cardinality
#define DEFAULT_HYPSET_CARDINALITY 1000

// D: definition of the probability mass that always remains free (is assigned
//    to others)
#define FREE_PROB_MASS ((float)0.05)

// D: forward declaration
class CDialogAgent;

// D: definition of vector type for concept pointers
// D：概念指针的向量类型的定义
typedef vector <CConcept *> TConceptPointersVector;

// D: definition for a set of concept pointers
typedef set < CConcept *, less < CConcept * >,
	allocator < CConcept * > > TConceptPointersSet;


// D: define concept update types
// D：定义概念更新类型
#define CU_ASSIGN_FROM_STRING		"assign_from_string"
#define CU_ASSIGN_FROM_CONCEPT		"assign_from_concept"
#define CU_UPDATE_WITH_CONCEPT		"update_with_concept"
#define CU_COLLAPSE_TO_MODE         "collapse_to_mode"
#define CU_PARTIAL_FROM_STRING		"partial_from_string"

// D: now, the CConcept class
class CConcept
{

protected:

	//---------------------------------------------------------------------
	// Protected CConcept class members
	// 保护成员
	//---------------------------------------------------------------------

	// concept type and source
	//
	//# typedef enum { ctUnknown, ctInt, ctBool, ctString, ctFloat, ctStruct, ctFrame, ctArray} TConceptType;
	//定义concept的类型 - 枚举
	TConceptType ctConceptType;
	//# typedef enum { csUser, csSystem } TConceptSource;
	TConceptSource csConceptSource;

	// concept name
	// 概念名
	string sName;

	// the owner dialog agent
	// 拥有者agent
	CDialogAgent* pOwnerDialogAgent;

	// the owner concept
	// 拥有者concept
	CConcept* pOwnerConcept;

	// the grounding model
	// grouding接地模型
	CGMConcept* pGroundingModel;

	// the grounded flag
	//接地标志
	bool bGrounded;

	//the invalidated flag (indicates that the concept was available, 
	//but then invalidated
	// 无效标志（指示概念可用，但随后无效)
	bool bInvalidated;

	// the restored_for_grounding flag (indicates that the concept was
	// reopened, but the grounding engined restored it because the 
	// value that was pushed in history needs to undergo grounding; when
	// this flag is set the current hyp undergoes grounding, and, when
	// the current hyp is grounded, it is compared with the history hyp;
	// if they are the same, then the current hyp is deleted, and basically
	// the concept is reopened back)
	bool bRestoredForGrounding;

	// the sealed flag (can be set through the Seal() function, and is 
	//  set to false by all operations that alter the concept)
	// 密封标志（可以通过Seal（）函数设置，并且被改变概念的所有操作设置为false）
	bool bSealed;

	// the change notification flag (when this is set to false, the 
	// concept no longer notifies changes)
	// 更改通知标志（当此设置为false时，概念不再通知更改）
	bool bChangeNotification;

	// the set of hypotheses, number of valid hypotheses, and cardinality
	// 假设集合，有效假设数量和基数
	vector<CHyp*, allocator<CHyp*> > vhCurrentHypSet;
	int iNumValidHyps;

	// the set of partial hypotheses
	// 该组部分假设
	vector<CHyp*, allocator<CHyp*> > vhPartialHypSet;
	int iNumValidPartialHyps;

	int iCardinality; //基数


	// information about when the last concept update was performed
	// 关于何时执行最后概念更新的信息
	int iTurnLastUpdated;

	// conveyance information (has the concept been conveyed to the user?)
	// 传送信息（该概念是否已传达给用户？）
	TConveyance cConveyance;
	bool bWaitingConveyance;

	// history information 
	// 历史信息
	CConcept* pPrevConcept;
	bool bHistoryConcept;

	// store the hypothesis that has already been explicitly confirmed 
	// for this concept (as a string);
	// 存储已经为此概念明确确认的假设（作为字符串）;
	string sExplicitlyConfirmedHyp;

	// store the hypothesis that has already been explicitly disconfirmed 
	// for this concept (as a string);
	// 存储已经为此概念明确否定的假设（作为字符串）;
	string sExplicitlyDisconfirmedHyp;

public:

	//---------------------------------------------------------------------
	// Constructors and destructors 
	//---------------------------------------------------------------------

	// default constructor
	CConcept(string sAName = "NONAME",
		TConceptSource csAConceptSource = csUser,
		int iACardinality = DEFAULT_HYPSET_CARDINALITY);

	// virtual destructor
	virtual ~CConcept();

	//---------------------------------------------------------------------
	// Methods for overall concept manipulation
	//整体概念操纵的方法
	//---------------------------------------------------------------------

	// a set of virtual operators on concepts
	//
	virtual CConcept& operator = (CConcept& rAConcept);
	virtual CConcept& operator = (int iAValue);
	virtual CConcept& operator = (bool bAValue);
	virtual CConcept& operator = (float fAValue);
	virtual CConcept& operator = (double dAValue);
	virtual CConcept& operator = (string sAValue);
	virtual CConcept& operator = (const char* lpszAValue);
	virtual bool operator == (CConcept& rAConcept);
	virtual bool operator != (CConcept& rAConcept);
	virtual bool operator < (CConcept& rAConcept);
	virtual bool operator > (CConcept& rAConcept);
	virtual bool operator <= (CConcept& rAConcept);
	virtual bool operator >= (CConcept& rAConcept);
	virtual CConcept& operator [](int iIndex);
	virtual CConcept& operator [](string sIndex);
	virtual CConcept& operator [](const char* lpszIndex);
	virtual operator int();
	virtual operator float();
	virtual operator bool();
	virtual operator string();

	// Clear the contents of the concept (completely)
	virtual void Clear();

	// Clear the current contents of the concept
	virtual void ClearCurrentValue();

	// Clone the concept
	virtual CConcept* Clone(bool bCloneHistory = true);

	// Construct an empty clone of the concept
	virtual CConcept* EmptyClone();

	// update the concept
	virtual void Update(string sUpdateType, void* pUpdateData);

	//---------------------------------------------------------------------
	// Virtual methods implementing various types of updates in the naive
	// probabilistic update scheme (NPU)
	// 在【朴素概率更新】方案（NPU）中实现各种类型更新的虚拟方法
	//---------------------------------------------------------------------

	// assign from another string
	virtual void Update_NPU_AssignFromString(void* pUpdateData);

	// assign from another concept
	virtual void Update_NPU_AssignFromConcept(void* pUpdateData);

	// update with another concept
	virtual void Update_NPU_UpdateWithConcept(void* pUpdateData);

	// update the value of a concept, so that it collapses to the mode
	// with probability 0.95 (this is used mostly on reopens)
	virtual void Update_NPU_CollapseToMode(void* pUpdateData);

	// assign the partial value of a concept from a string
	virtual void Update_PartialFromString(void* pUpdateData);
	//---------------------------------------------------------------------
	// Virtual methods implementing various types of updates in the Calista
	// belief updating model
	// 在Calista信仰更新模型中实现各种类型更新的虚拟方法
	//---------------------------------------------------------------------

	// assign from another string
	virtual void Update_Calista_AssignFromString(void* pUpdateData);

	// assign from another concept
	virtual void Update_Calista_AssignFromConcept(void* pUpdateData);

	// update with another concept
	virtual void Update_Calista_UpdateWithConcept(void* pUpdateData);

	// update the value of a concept, so that it collapses to the mode
	// with probability 0.95 (this is used mostly on reopens)
	virtual void Update_Calista_CollapseToMode(void* pUpdateData);

	//---------------------------------------------------------------------
	// Methods implementing various flags (state information) 
	// on the concept
	// 对概念实现各种标志操作（状态信息）的方法
	//---------------------------------------------------------------------

	// check if the concept is updated (there is at least a current value)
	virtual bool IsUpdated();

	// check if the concept is updated and grounded
	virtual bool IsUpdatedAndGrounded();

	// check if the concept is available (we have a current value or a 
	// history value)
	virtual bool IsAvailable();

	// check that the concept is available and grounded
	virtual bool IsAvailableAndGrounded();

	// checks that the concept has partial hypothesis values
	virtual bool HasPartialUpdate();

	// check if the concept is grounded
	virtual bool IsGrounded();

	// check if the concept is currently undergoing grounding
	virtual bool IsUndergoingGrounding();

	// check if the concept is invalidated
	virtual bool IsInvalidated();

	// check if the concept is currently restored for grounding
	virtual bool IsRestoredForGrounding();

	//---------------------------------------------------------------------
	// Methods implementing conversions to string format
	//	实现转换为字符串格式的方法
	//---------------------------------------------------------------------

	// Generate a string representation of the grounded hypothesis (the top
	// one, if the concept is grounded)
	virtual string GroundedHypToString();

	// Generate a string representation of the top hypothesis
	virtual string TopHypToString();

	// Generate a string representation of the set of hypotheses
	virtual string HypSetToString();

	//---------------------------------------------------------------------
	// Methods providing access to concept type and source
	// 提供对概念类型和源的访问的方法
	//---------------------------------------------------------------------

	// access to the concept type
	//
	virtual TConceptType GetConceptType();
	virtual void SetConceptType(TConceptType ctAConceptType);

	// access to the concept source
	// 
	virtual TConceptSource GetConceptSource();
	virtual void SetConceptSource(TConceptSource csAConceptSource);

	//---------------------------------------------------------------------
	// Methods providing access to concept name
	// 提供访问概念名称的方法
	//---------------------------------------------------------------------

	// set the concept name
	virtual void SetName(string sAName);

	// return the concept name
	string GetName();

	// return the small concept name (if the concept is part of a structure
	// or a frame, it returns only the actual name)
	string GetSmallName();

	// return the concept name, qualified with the owner agent's name
	string GetAgentQualifiedName();

	//---------------------------------------------------------------------
	// Methods providing access to the owner dialog agent
	// 提供对所有者对话代理的访问的方法
	//---------------------------------------------------------------------

	// set the owner dialog agent
	virtual void SetOwnerDialogAgent(CDialogAgent* pADialogAgent);

	// return the owner dialog agent
	CDialogAgent* GetOwnerDialogAgent();

	//---------------------------------------------------------------------
	// Methods providing access to the owner concept
	//---------------------------------------------------------------------

	// set the owner concept
	virtual void SetOwnerConcept(CConcept* pAConcept);

	// return the owner concept
	CConcept* GetOwnerConcept();

	//---------------------------------------------------------------------
	// Methods related to the grounding model
	// 与接地模型相关的方法
	//---------------------------------------------------------------------

	// create a grounding model for this concept
	virtual void CreateGroundingModel(string sGroundingModelSpec);

	// return a pointer to the grounding model for this concept
	CGMConcept* GetGroundingModel();

	// set the grounded internal flag
	virtual void SetGroundedFlag(bool bAGrounded = true);

	// return the grounded internal flag
	virtual bool GetGroundedFlag();

	// declare the list of grounding models that this concept subsumes
	virtual void DeclareGroundingModels(
		TGroundingModelPointersVector& rgmpvModels,
		TGroundingModelPointersSet& rgmpsExclude);

	// declare the list of concepts that this concept subsumes
	virtual void DeclareConcepts(
		TConceptPointersVector& rcpvConcepts,
		TConceptPointersSet& rcpsExclude);

	//---------------------------------------------------------------------
	// Methods related to invalidating a concept
	// 与使概念无效相关的方法
	//---------------------------------------------------------------------

	// set the invalidated flag
	virtual void SetInvalidatedFlag(bool bAInvalidated = true);

	// get the invalidated flag
	virtual bool GetInvalidatedFlag();

	//---------------------------------------------------------------------
	// Methods related to restoring a concept for grounding
	//---------------------------------------------------------------------

	// set the restored for grounding flag
	virtual void SetRestoredForGroundingFlag(
		bool bARestoredForGrounding = true);

	// get the restored for grounding flag
	virtual bool GetRestoredForGroundingFlag();

	//---------------------------------------------------------------------
	// Methods related to sealing 
	// 与密封相关的方法
	//---------------------------------------------------------------------

	// seal the concept
	virtual void Seal();

	// break the seal (set seal flag to false)
	virtual void BreakSeal();

	// check if the concept is sealed
	virtual bool IsSealed();

	//---------------------------------------------------------------------
	// Methods related to signaling the concept change
	// 与信号传递概念变化相关的方法
	//---------------------------------------------------------------------

	// enable / disable concept change notifications
	// 
	virtual void DisableChangeNotification();
	virtual void EnableChangeNotification();
	virtual void SetChangeNotification(bool bAChangeNotification = true);

	// processing that happens whenever the concept value is changed
	virtual void NotifyChange();

	//---------------------------------------------------------------------
	// Methods related to the current hypotheses set and belief manipulation
	// 与当前假设集合和信念操纵相关的方法
	//---------------------------------------------------------------------

	// factory method for hypotheses
	virtual CHyp* HypFactory();

	// add a hypothesis to the current set (return the index)
	virtual int AddHyp(CHyp* pAHyp);

	// add a new hypothesis to the current set (return the index)
	virtual int AddNewHyp();

	// add a NULL hypothesis to the current set (return the index)
	virtual int AddNullHyp();

	// sets a hypothesis into a location (location has to contain a 
	// null-hyp and the operation actually copies)
	virtual void SetHyp(int iIndex, CHyp* pHyp);

	// sets a hypothesis into a location
	virtual void SetNullHyp(int iIndex);

	// deletes a hypothesis at a given index
	virtual void DeleteHyp(int iIndex);

	// return the hypothesis at a given index
	virtual CHyp* GetHyp(int iIndex);

	// finds the index of a given hypothesis
	virtual int GetHypIndex(CHyp* pHyp);

	// return the confidence of a certain hypothesis (specified by index)
	virtual float GetHypConfidence(int iIndex);

	// set the confidence for a certain hypothesis (specified by index)
	virtual void SetHypConfidence(int iIndex, float fConfidence);

	// return the top hypothesis
	virtual CHyp* GetTopHyp();

	// return the top hyp index
	virtual int GetTopHypIndex();

	// return the index for the second best hyp
	virtual int Get2ndHypIndex();

	// return the confidence score of the top hypothesis
	virtual float GetTopHypConfidence();

	// check if a hypothesis is valid (confidence score > 0; valus is not 
	// NULL)
	virtual bool IsValidHyp(int iIndex);

	// return the total number of hypotheses for a concept (including 
	// NULLs)
	virtual int GetNumHyps();

	// return the number of valid hypotheses for a concept
	virtual int GetNumValidHyps();

	// clear the current set of hypotheses for the concept
	virtual void ClearCurrentHypSet();

	// copies the current hypset from another concept
	virtual void CopyCurrentHypSetFrom(CConcept& rAConcept);

	// sets the cardinality of the hypset
	virtual void SetCardinality(int iACardinality);

	// returns the cardinality of the hypset
	virtual int GetCardinality();

	// returns the prior for a hypothesis
	virtual float GetPriorForHyp(CHyp* pHyp);

	// returns the confusability for a hypothesis
	virtual float GetConfusabilityForHyp(CHyp* pHyp);

	// returns the concept type information for a concept
	virtual string GetConceptTypeInfo();

	// sets the explicitly confirmed and disconfirmed hyps for a concept
	//
	virtual void SetExplicitlyConfirmedHyp(CHyp* pHyp);
	virtual void SetExplicitlyConfirmedHyp(string sAExplicitlyConfirmedHyp);
	virtual void SetExplicitlyDisconfirmedHyp(CHyp* pHyp);
	virtual void SetExplicitlyDisconfirmedHyp(string sAExplicitlyDisconfirmedHyp);

	// returns the explicitly confirmed and disconfirmed hyps for a concept
	// 
	virtual string GetExplicitlyConfirmedHypAsString();
	virtual string GetExplicitlyDisconfirmedHypAsString();

	// clears the explicitly confirmed and disconfirmed hyps 
	// 
	virtual void ClearExplicitlyConfirmedHyp();
	virtual void ClearExplicitlyDisconfirmedHyp();

	//---------------------------------------------------------------------
	// Methods providing access to partially updated values
	// 提供访问部分更新值的方法
	//---------------------------------------------------------------------

	// adds a partial hypothesis to the current set of partial hypotheses
	virtual int AddPartialHyp(CHyp* pAHyp);

	// adds a new partial hypothesis to the current set of partial hypotheses
	virtual int AddNewPartialHyp();

	// adds a null partial hypothesis to the current set of partial hypotheses
	virtual int AddNullPartialHyp();

	// indicates whether a partial hypothesis is currently available 
	virtual bool HasPartialHyp();

	// return a partial hypothesis
	virtual CHyp* GetPartialHyp(int iIndex);

	// return the top partial hypothesis
	virtual CHyp* GetTopPartialHyp();

	// return the top partial hypothesis
	virtual int GetTopPartialHypIndex();

	// return the top partial hypothesis
	virtual float GetTopPartialHypConfidence();

	// check if a partial hypothesis is valid (confidence score > 0; 
	// valus is not NULL)
	bool IsValidPartialHyp(int iIndex);

	// return the total number of partial hypotheses for a concept
	virtual int GetNumPartialHyps();

	// return the total number of valid partial hypotheses for a concept
	virtual int GetNumValidPartialHyps();

	// clear the set of partial hypotheses for the concept
	virtual void ClearPartialHypSet();


	//---------------------------------------------------------------------
	// Methods providing access to turn last updated information
	// 方法提供访问权限以转换最后更新的信息
	//---------------------------------------------------------------------

	// set the turn when the concept was last updated
	virtual void SetTurnLastUpdated(int iTurn);

	// mark that the concept was updated in the current turn
	virtual void MarkTurnLastUpdated();

	// return the turn when the concept was last updated
	virtual int GetTurnLastUpdated();

	// return the number of turns that have elapsed since the concept was
	// last updated
	int GetTurnsSinceLastUpdated();

	//---------------------------------------------------------------------
	// Methods providing access to conveyance information
	// 提供访问交通信息的方法
	//---------------------------------------------------------------------

	// set the waiting_for_conveyance flag
	void SetWaitingConveyance();

	// clear the waiting_for_conveyance flag
	void ClearWaitingConveyance();

	// set the conveyance information
	virtual void SetConveyance(TConveyance cConveyance);

	// return the conveyance information
	TConveyance GetConveyance();

	// clear the concept notification pointer
	virtual void ClearConceptNotificationPointer();

	//---------------------------------------------------------------------
	// Methods for concept history manipulation
	//用于概念历史操作的方法
	//---------------------------------------------------------------------

	// reopen the concept (i.e. push the current value into the history)
	//重新打开概念（即将当前值推入历史）
	virtual void ReOpen();

	// restore the concept (i.e. restore the concept to an older value
	// from its history
	virtual void Restore(int iIndex = -1);

	// clear the history of a concept
	virtual void ClearHistory();

	// construct a new concept by merging over the history of this concept
	virtual CConcept* CreateMergedHistoryConcept();

	// merge history of the concept back into the current value
	virtual void MergeHistory();

	// return the size of the history on the concept
	int GetHistorySize();

	// return a reference to a history version of the concept
	virtual CConcept& GetHistoryVersion(int iIndex);

	// access to the history concept flag
	//
	virtual void SetHistoryConcept(bool bAHistoryConcept = true);
	virtual bool IsHistoryConcept();

	//---------------------------------------------------------------------
	// Methods that are array-specific and will be implemented by arrays
	// 特定于数组且将由数组实现的方法
	//---------------------------------------------------------------------

	// returns the size of the array
	virtual int GetSize();

	// deletes an element at a given index in the array
	virtual void DeleteAt(unsigned int iIndex);

	// inserts an element at a give index in the array
	virtual void InsertAt(unsigned int iIndex, CConcept &rAConcept);
};

// NULL concept: this object is used designate invalid concept references
extern CConcept NULLConcept;

// D: Macro for defining a derived concept type	
#define DEFINE_CONCEPT_TYPE(NewConceptTypeName, BaseConceptTypeName, OTHER_CONTENTS)\
class NewConceptTypeName : public BaseConceptTypeName
{
	\
public:\
	   NewConceptTypeName(string sAName = "NONAME", \
	   TConceptSource csAConceptSource = csUser) :\
	   BaseConceptTypeName(sAName, csAConceptSource) {}; \
	   virtual CConcept* EmptyClone()
	{
		   \
			   return new NewConceptTypeName; \
	   }; \
	   OTHER_CONTENTS; \
}; \

// D: macro for defining the prior for a hypothesis
#define DEFINE_PRIOR(CODE)\
	public:\
	virtual float GetPriorForHyp(CHyp* pHyp)
{
		\
			CODE; \
	}\

	// D: macro for defining the confusability for a hypothesis
#define DEFINE_CONFUSABILITY(CODE)\
	public:\
	virtual float GetConfusabilityForHyp(CHyp* pHyp)
		   {
		\
			CODE; \
	}\


#endif // __CONCEPT_H__