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
// OUTPUTMANAGERAGENT.H   - definition of the output manager agent class. This 
//                          agent forwards the output thourgh the brige onto
//                          the NLG/Graphical Interface, etc
//OUTPUTMANAGERAGENT.H - 输出管理器代理类的定义。 此代理将输出通过网桥转发到NLG /图形界面等 
// ----------------------------------------------------------------------------
// 
// BEFORE MAKING CHANGES TO THIS CODE, please read the appropriate 
// documentation, available in the Documentation folder. 
//
// ANY SIGNIFICANT CHANGES made should be reflected back in the documentation
// file(s)
//
// ANY CHANGES made (even small bug fixed, should be reflected in the history
// below, in reverse chronological order
// 
// HISTORY --------------------------------------------------------------------
//
//   [2006-06-15] (antoine): merged with latest RavenClaw1 version
//   [2005-01-26] (antoine): modified output so that it handles the 
//                           ":non-listening" flag
//   [2005-01-11] (antoine): modified Notify so that it increments the repeat
//							  counter of the output
//   [2004-12-23] (antoine): modified constructor, agent factory to handle
//							  configurations
//   [2004-02-25] (dbohus):  added methods for cancelling and changing concept
//                            notification requests
//   [2002-10-25] (dbohus): Redid the concept conveyance notifications
//   [2002-10-21] (dbohus): Added output classes 
//   [2002-10-15] (dbohus): Cleaned up Repeat and Notify
//   [2002-10-14] (dbohus): Eliminated the promptsqueue, no longer needed
//   [2002-10-12] (dbohus): Changed concurrency control to work with
//                          WinAPI criticalsection objects
//   [2002-10-11] (dbohus): Removed concurrency control for COutputHistory
//							as it is not needed
//   [2002-10-10] (dbohus): Unified methods for adding to the history of 
//                          outputs. There's a single one now: AddOutput
//   [2002-10-10] (dbohus): Started revisioning/understanding Drew's code
//   [2002-09-25] (dbohus): Fixed bug when outputting empty prompts
//   [2002-07-14] (agh)   : Added Repeat() functionality
//   [2002-06-25] (dbohus): replaced COutput::CreateWithClones() with the 
//							corresponding parametrized version of Create()
//	 [2002-06-20] (agh)   : robustified Notify() and added use of 
//							CreateWithClones() function.
//   [2002-06-14] (dbohus): started polishing this, changing it to support
//                          the introduction of the COutput class
//	 [2002-05-30] (agh)   : Baseline version complete. Included thread safety.
//   [2002-05-24] (agh)   : Finished basic functionality, Notification, added
//							support for different output media
//	 [2002-04-03] (agh)   : added COutputHistory() class
//   [2002-03-28] (agh)   : began output functions
//   [2002-02-03] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __OUTPUTMANAGERAGENT_H__
#define __OUTPUTMANAGERAGENT_H__

#include <windows.h>
#include <winuser.h>
#include "../../../Utils/Utils.h"
#include "../../../DMCore/Agents/Agent.h"
#include "../../../DMCore/Agents/DialogAgents/DialogAgent.h"
#include "../../../DMCore/Concepts/AllConcepts.h"
#include "../../../DMCore/Outputs/Output.h"


// A: Output Device parameters
// if defined, device will send notifications back to output manager
// A：输出设备参数
// 如果定义，设备将发送通知回到输出管理器
#define OD_NOTIFIES 0x1

//-----------------------------------------------------------------------------
// A: Describes destination for a prompt (output device) sent to be outputted 
//    with a key name, an external server call, and any parameters that modify 
//    how the Output Manager should handle the output
//	  DEFAULT_OUTPUT_DEVICE("nlg", "nlg.launch_query", 1)
//-----------------------------------------------------------------------------
typedef struct
{
	string sName;			// name of the output device							输出设备名		"nlg"
	string sServerCall;		// external server name (module.function for galaxy)	外部服务器名		"nlg.launch_query"
	int iParams;			// other parameters for the output device				输出设备参数		"1"
} TOutputDevice;


//-----------------------------------------------------------------------------
// A: COutputHistory Class - 
//     Keeps record of outputs sent to generation and utterances built, and
//	   status of success of outputting those utterances
//     记录将输出发送到生成器，以及输出这些发音的成功状态
//-----------------------------------------------------------------------------
class COutputHistory
{

private:
	// private members
	// 私有成员

	vector<string> vsUtterances;		// history of utterances as strings		句子的历史   字符串
	vector<COutput*> vopOutputs;		// history of outputs					输出的历史

	unsigned int uiCurrentID;			// next id to be added to history		下一个要添加到历史的id

public:
	//---------------------------------------------------------------------
	// Constructors and destructor
	//---------------------------------------------------------------------
	//
	// Default constructor
	// 默认构造函数
	COutputHistory();

	// Destructor
	// 虚拟析构函数
	virtual ~COutputHistory();

public:
	//---------------------------------------------------------------------
	// public methods
	//---------------------------------------------------------------------

	// Method for generating a string representation of the history of
	// outputs. Used basically for logging purposes
	//用于生成输出历史的字符串表示的方法。 主要用于日志记录
	string ToString();

	// Methods for adding to/removing from the history
	// 添加和删除output对象
	unsigned int AddOutput(COutput* pOutput, string sUtterance);
	void Clear();//清空历史

	// Methods for searching/accessing the history of outputs
	// 查询和获取历史输出对象
	unsigned int GetSize();

	string GetUtteranceAt(unsigned int iIndex);
	COutput* GetOutputAt(unsigned int iIndex);
	COutput* operator[](unsigned int iIndex);
};

//-----------------------------------------------------------------------------
// A: COutputManagerAgent Class - 
//     This class implements the core agent which handles output management in
//	   the RavenClaw dialog manager.
//	处理输出管理的核心agent
//-----------------------------------------------------------------------------

class COutputManagerAgent : public CAgent
{

private:
	//---------------------------------------------------------------------
	// Private members
	//---------------------------------------------------------------------
	//
	// the history of outputs
	// 输出历史
	COutputHistory ohHistory;

	// list of registered output devices
	// 注册的输出设备 【结构体】 
	vector <TOutputDevice> vodOutputDevices;
	
	// index to the default output device
	// 默认的输出设备索引
	unsigned int iDefaultOutputDevice;

	// list of last outputs sent, about which we haven't been notified of success
	// 最后输出的列表， 并且没有通知成功
	vector <COutput *> vopRecentOutputs;

	// counter for generating output IDs
	// 生成输出的id计数
	int iOutputCounter;

	// critical section object for enforcing concurrency control
	// 临界区对象用于执行并发控制
	CRITICAL_SECTION csCriticalSection;

	// the class of outputs to be used
	// 使用的输出类
	string sOutputClass;

public:
	//---------------------------------------------------------------------
	// Constructor and destructor
	//---------------------------------------------------------------------
	//
	// Default constructor
	//默认构造函数
	COutputManagerAgent(string sAName, string sAConfiguration = "", string sAType = "CAgent:COutputManagerAgent");

	// Virtual destructor
	// 虚拟 析构
	virtual ~COutputManagerAgent();

	// static function for dynamic agent creation
	static CAgent* AgentFactory(string sAName, string sAConfiguration);

	//---------------------------------------------------------------------
	// CAgent Class overwritten methods 
	//---------------------------------------------------------------------
	//需要 覆盖的方法 - 重置方法
	virtual void Reset();	// overwritten Reset

public:

	//---------------------------------------------------------------------
	// OutputManagerAgent class specific public methods
	// 特有 public 方法
	//---------------------------------------------------------------------

	// Setting the output class to be used
	// 设置使用的output 类
	void SetOutputClass(string sAOutputClass);

	// Methods for registering and accessing output devices
	// 注册和访问输出设备的方法
	bool RegisterOutputDevice(string sName, string sServerCall, int iParams = 0);
	void SetDefaultOutputDevice(string sName);
	TOutputDevice *GetOutputDevice(string sName);
	TOutputDevice *GetDefaultOutputDevice();
	string GetDefaultOutputDeviceName();


	// Output functions
	// 输出函数

	// Send a set of prompts to the output
	// 向输出发送一组提示
	vector<COutput*> Output(CDialogAgent* pGeneratorAgent, string sPrompts, TFloorStatus fsFinalFloorStatus);

	// Do a repeat
	//重复
	void Repeat();

	// Gets notification back from output server of results of latest sending
	// 从输出服务器返回最新发送结果的通知
	void Notify(int iOutputId, int iBargeinPos, string sConveyance, string sTaggedUtt);

	// Gets a preliminary notification back from output server about 
	// what utterance is about to be spoken
	// 从输出服务器获取关于将要说出什么话语的初步通知
	void PreliminaryNotify(int iOutputId, string sTaggedUtt);

	// Cancels a concept notification request
	// 取消概念通知请求
	void CancelConceptNotificationRequest(CConcept* pConcept);

	// Changes a concept notification request
	// 更改概念通知请求
	void ChangeConceptNotificationPointer(CConcept* pOldConcept, CConcept* pNewConcept);

	// Returns the list of prompts that are waiting for notifications
	// 返回等待通知的提示列表
	string GetPromptsWaitingForNotification();

private:

	//---------------------------------------------------------------------
	// OutputManagerAgent class specific private (helper) methods
	// 特定的私有方法
	//---------------------------------------------------------------------

	// Sends the output to the outside world (i.e. NLG server if in a 
	// Galaxy configuration)
	//将输出发送到外部世界（即NLG服务器，如果在Galaxy配置中）
	string output(COutput* pOutput);

	// Helper methods for Notify
	// 获取最近输出的索引
	unsigned int getRecentOutputIndex(int iConceptId);
};

#endif // __OUTPUTMANAGERAGENT_H__