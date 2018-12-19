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
// OUTPUT.H - definition of the COutput base class. This class represents an 
//			  output from the dialog manager, and is used by the COutputManager
//			  core agent. The class is the base of a hierarchy.
// OUTPUT.H - COutput基类的定义。 此类表示对话管理器的输出，并由输出管理器核心代理使用。 类是层次结构的基础。
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
//	 [2007-02-08] (antoine): added bIsFinalOutput
//   [2005-10-22] (antoine): added method GetGeneratorAgentName
//   [2005-10-20] (antoine): added a sDialogState field sent along with the
//                           output (this is for the InteractionManager)
//   [2005-01-11] (antoine): added methods for computing and getting a counter
//                            on the number of repeats for a given output
//   [2004-02-25] (dbohus):  added methods for cancelling and changing concept
//                            notification requests
//   [2004-02-24] (dbohus):  changed outputs so that we no longer clone 
//                            concepts but use them directly
//   [2003-10-20] (dbohus):  fixed a bug in clone so that the vector of 
//                            concepts is cloned correctly even when there are
//                            nulls in there
//   [2003-04-10] (dbohus,  
//                 antoine): fixed bug in clone so that the vector of concepts
//                            is indeed cloned
//   [2002-12-03] (dbohus):  fixed bug in FindConceptByName
//   [2002-10-15] (dbohus):  added destructor so that concepts are deleted
//   [2002-10-14] (dbohus):  added GetOutputDevice, GetAct, GetObject, moved 
//                            all members to protected; removed all the Get 
//							  functions put OutputManagerAgent as friend class
//   [2002-10-13] (dbohus):  changed iParams to iFlags, moved flags definitions
//                            here, added CheckFlags function
//   [2002-10-12] (dbohus):  removed FindConcept, since not used so far
//   [2002-06-25] (dbohus):  unified Create and CreateWithClones in a single
//							  create function which takes a bool parameter
//   [2002-06-17] (dbohus):  deemed preliminary stable version 0.5
//	 [2002-06-17] (dbohus):  added method for finding concepts
//	 [2002-06-14] (dbohus):  drafted this class
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include "../../Utils/Utils.h"
#include "../../DMCore/Agents/CoreAgents/DMCoreAgent.h"
#include "../../DMCore/Concepts/Concept.h"

//-----------------------------------------------------------------------------
// D: The COutput class.
//		COutput基类的定义。 此类表示对话管理器的输出，并由输出管理器核心代理使用。 类是层次结构的基础。
//-----------------------------------------------------------------------------

//###########	prompt => [FLAGS] ACT OBJECT [PARAMETERS]
class COutput
{

	//友元类
	friend class COutputHistory;		// output history has friend access
	friend class COutputManagerAgent;	// output manager has friend access 

protected:
	// class members 
	//
	string sGeneratorAgentName;			// name of the agent that ordered this			订购此代理的代理的名称
	// output
	int iOutputId;						// the output id								输出id
	int iExecutionIndex;				// the index of the execution item				执行项的索引 [与产生这个output相关]
	// corresponding to the generation 
	// of this output

	string sDialogState;				// a string representing the dialog state		表示发出此提示时的对话状态的字符串
	
	//at which this prompt was issued 
	//###################################################################################################
	string sAct;						// the act (dialog move)						动作
	string sObject;					    // the object (acted on or with)				对象
	//###################################################################################################

	vector<CConcept *> vcpConcepts;		// the list of concepts referred in this output	在此输出中引用的概念列表
	vector<bool> vbNotifyConcept;       // parallel vector indicating whether the concept conveyance should be 
	//  notified or not																	指示concept运输是否应该被通知的 <平行向量>

	/*
	:non-listening 
			the decoder will be set in non-listening mode for the duration of the prompt.
	:non-interruptable 
			the barge-in mechanism will be disabled for the duration of the prompt.
	:non-repeatable 
			this prompt will not be repeated by the output manager, if the user asks the system to repeat.
	:<device> 
			specifies the output device to which the prompt will be sent. By default, prompts are sent to the natural language generation[NLG] device.
	*/
	vector<string> vsFlags;				// flags for the output						输出标志
	string sOutputDeviceName;			// the name of the device this output		此输出应定向到的设备的名称
	//  should be directed to
	TConveyance cConveyance;			// whether the output was fully conveyed	输出是否完全传达给接收者 枚举：[传达、 没传达、失败]
	// to the recipient
	int iRepeatCounter;					// the number of times this output has		该输出已被发出的次数（连续）
	//  been uttered (consecutively)
	TFloorStatus fsFinalFloorStatus;	// the floor status at the end of this output	在这个输出的结束时候的floor状态 枚举：【位置、系统、用户、自由】



public:
	//---------------------------------------------------------------------
	// Constructors and destructors
	//---------------------------------------------------------------------
	// 
	COutput();
	virtual ~COutput();

public:
	//---------------------------------------------------------------------
	// Public abstract methods - to be overwritten by derived classes
	// 需要覆盖发的方法
	//---------------------------------------------------------------------

	// Pure virtual method which creates a certain output from a given
	// string-represented prompt
	//纯虚方法从给定的字符串提示创建一个特定的输出
	virtual bool Create(string sAGeneratorAgentName, int iAExecutionIndex, string sAPrompt, TFloorStatus fsAFloor, int iAOutputId) = 0;

	// Pure virtual method which generates a string representation for the
	// output that will be sent to the external output component (i.e. 
	// NLG in the Galaxy framework, Gemini in the OAA framework)
	//纯虚方法生成表示输出的字符串, 并发送到外部输出组件（即，在Galaxy框架中的NLG，在OAA框架中的Gemini）
	virtual string ToString() = 0;

	// Pure virtual method for cloning an output
	// 纯虚函数 - 克隆一份输出对象
	virtual COutput* Clone(int iNewOutputId) = 0;

	//---------------------------------------------------------------------
	// Public methods 
	// public方法
	//---------------------------------------------------------------------

	// Get name of the agent that generated this output
	// 获取生成此输出的代理的名称
	string GetGeneratorAgentName();

	// Set and Get dialog state information for this output
	// 设置和获取此输出的对话状态信息
	int GetDialogStateIndex();
	void SetDialogStateIndex(int iAExecutionIndex);
	string GetDialogState();
	void SetDialogState(string sADialogState);

	// Set and Get conveyance information on the output
	// 设置和获取输出的输送信息
	void SetConveyance(TConveyance cAConveyance);
	TConveyance GetConveyance();

	// Set and Get the act for this output
	//设置并获取此输出的动作
	void SetAct(string sAAct);
	string GetAct();

	// Set and Get the final floor status for this output
	// 设置并获取此输出的最终 floor 状态
	void SetFinalFloorStatus(TFloorStatus fsAFloor);
	TFloorStatus GetFinalFloorStatus();
	string GetFinalFloorStatusLabel();

	// Checks if a certain flag is set
	// 检查flag是否设置
	bool CheckFlag(string sFlag);

	// Notifies a concept of conveyance information
	// 通知交通信息的概念
	void NotifyConceptConveyance(string sConceptName, TConveyance cAConveyance);

	// Returns a pointer to the concept, given the concept name
	// 返回一个指向概念的指针，给出概念名称
	virtual CConcept* GetConceptByName(string sConceptName);

	// Cancels the notification request for one of the concepts
	// 取消其中一个概念的通知请求
	void CancelConceptNotificationRequest(CConcept* pConcept);

	// Changes the pointers for one of the concepts (this happens on 
	// reopens and other operations which change the concept pointers)
	// 更改其中一个概念的指针（这发生在重新打开和其他更改概念指针的操作）
	void ChangeConceptNotificationPointer(CConcept* pOldConcept, CConcept* pNewConcept);

	// Gets the number of times this prompt has been uttered
	// 获取此提示已发出的次数
	int GetRepeatCounter();
	// Increments the repeat counter
	// 增加重复次数计数
	void IncrementRepeatCounter();

protected:
	//---------------------------------------------------------------------
	// Protected methods 
	//---------------------------------------------------------------------

	// base clone helper function to be called by Clone() in derived classes
	// 基本克隆辅助函数由派生类中的Clone（）调用
	virtual void clone(COutput* pOutput, int iNewOutputId);
};

#endif // __OUTPUT_H__
