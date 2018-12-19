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
// OUTPUTMANAGERAGENT.CPP - implements the output manager agent class. This 
//                          agent forwards the output through the bridge to
//                          the output media
// 
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

#include "../../../DMInterfaces/DMInterface.h"
#include "OutputManagerAgent.h"
#include "../../../DMCore/Agents/Registry.h"
#include "../../../DMCore/Core.h"

#ifdef GALAXY
#include "DMCore/Outputs/FrameOutput.h"
#endif

#ifdef OAA
#include "DMCore/Outputs/LFOutput.h"
#endif

//-----------------------------------------------------------------------------
//
// A: COutputHistory class methods
//
//-----------------------------------------------------------------------------

static TDialogState *ds;

//-----------------------------------------------------------------------------
// A: Constructors and destructors
//-----------------------------------------------------------------------------
// A: Default constructor
COutputHistory::COutputHistory()
{
}

// A: virtual destructor
COutputHistory::~COutputHistory()
{
	Clear();
}

//-----------------------------------------------------------------------------
// COutputHistory public methods 
//-----------------------------------------------------------------------------
// DA: Generates a string representation of the history out outputs
string COutputHistory::ToString()
{
	// build the string in sResult
	string sResult = "OUTPUT HISTORY\nid\t utterance\n";
	sResult += "-----------------------------------------------------------"\
		"---------------------\n";

	// add all the utterances in the history in reverse chronological order
	for (int i = vsUtterances.size() - 1;
		(i >= 0) && (5 >= vsUtterances.size() - i); i--)
		sResult += FormatString("%d\t%s\n", vopOutputs[i]->iOutputId,
		vsUtterances[i].c_str());
	return sResult;
}

// A: appends request to history as sent to the external output agent/server.
//    Should get called on DM Interface thread
// A：将请求附加到发送到 外部输出代理/服务器 的历史记录。 应该在DM接口线程上调用
unsigned int COutputHistory::AddOutput(COutput* pOutput, string sUtterance)
{

	// push the output and actual utterance on the history vectors
	//推送历史向量上的输出和实际话语
	vopOutputs.push_back(pOutput);
	vsUtterances.push_back(sUtterance);
	uiCurrentID++;

	// return the current id
	return uiCurrentID;
}

// A: deletes history and starts a new one, beginning with ID uiNewStart
//    should be called on DM Core thread
// A：删除历史记录并启动一个新的，ID从uiNewStart开始， 应该在DM Core线程上调用
void COutputHistory::Clear()
{
	// destroy all the output objects in the history
	// 删除所有的历史输出对象
	for (unsigned int i = 0; i < vopOutputs.size(); i++)
		delete vopOutputs[i];

	vopOutputs.clear(); // 输出历史列表清空
	vsUtterances.clear(); //句子历史清空
	uiCurrentID = 0; //先一个要添加的历史
}

// DA: return the size of the output history
unsigned int COutputHistory::GetSize()
{
	return vopOutputs.size();
}

// DA: return the utterance at a particular index, starting from the
//     most recent
string COutputHistory::GetUtteranceAt(unsigned int iIndex)
{
	int iSize = vsUtterances.size();
	if ((int)(iSize - 1 - iIndex) < 0)
	{
		FatalError(FormatString("Invalid index in output utterance history: "\
			"index = %d, history_size = %d", iSize, iIndex));
		return "";
	}
	else return vsUtterances[iSize - 1 - iIndex];
}

// DA: accesses the indexth element of the array, counting 0 as the
//     most recent.
COutput* COutputHistory::GetOutputAt(unsigned int iIndex)
{
	int iSize = vopOutputs.size();
	if ((int)(iSize - 1 - iIndex) < 0)
	{
		FatalError(FormatString("Invalid index in output utterance history: "\
			"index = %d, history_size = %d", iSize, iIndex));
		return NULL;
	}
	else return vopOutputs[iSize - 1 - iIndex];
}

// D: a shortcut operator for the GetOutputAt 
inline COutput* COutputHistory::operator [](unsigned int iIndex)
{
	return GetOutputAt(iIndex);
}


//-----------------------------------------------------------------------------
//
// Methods for the COutputManagerAgent class
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// AD: Constructors and destructors
//-----------------------------------------------------------------------------
// AD: constructor
COutputManagerAgent::COutputManagerAgent(string sAName, string sAConfiguration, string sAType) : CAgent(sAName, sAConfiguration, sAType)
{

	// set an invalid value for the output device since we don't have 
	// any output devices registered yet
	//为输出设备设置无效的值，因为我们还没有注册任何输出设备
	iDefaultOutputDevice = 0;

	// initialize prompt ids
	//初始化输出计数为 0
	iOutputCounter = 0;

	// create the critical section object
	// 创建临界区对象 - [C++库]
	InitializeCriticalSection(&csCriticalSection);

	// initialize the output class by default to frameoutput
	// 默认情况下将输出类: FrameOutput
	sOutputClass = "FrameOutput";
}

// AD: Virtual destructor
//虚拟 析构函数
COutputManagerAgent::~COutputManagerAgent()
{
	// delete all the outputs that are still in the RecentOutputs list (if any)
	// (the outputs that have been moved to the history will be destroyed by the COutputHistory destructor)
	// 删除仍在“最近输出”列表（如果有）中的所有输出（已移动到历史记录的输出将被COutputHistory析构函数销毁）
	// 最后输出的列表， 并且没有得到成功的通知
	for (unsigned int i = 0; i < vopRecentOutputs.size(); i++)
		delete vopRecentOutputs[i];

	// delete the critical section object
	// 删除临界区对象
	DeleteCriticalSection(&csCriticalSection);
}

//-----------------------------------------------------------------------------
// D: Static function for dynamic agent creation
//-----------------------------------------------------------------------------
CAgent* COutputManagerAgent::AgentFactory(string sAName, string sAConfiguration)
{
	return new COutputManagerAgent(sAName, sAConfiguration);
}

//-----------------------------------------------------------------------------
// D: CAgent class overwritten methods
//-----------------------------------------------------------------------------

// D: the overwritten Reset method
// 覆盖的 重置 方法
void COutputManagerAgent::Reset()
{
	// reinitialize everything
	// 重新初始化 一切
	ohHistory.Clear();//输出历史记录清空
	iOutputCounter = 0; //输出计数清零

	// delete the list of recent outputs
	// 删除最近输出列表
	for (unsigned int i = 0; i < vopRecentOutputs.size(); i++)
		delete vopRecentOutputs[i];
	vopRecentOutputs.clear();
}

//-----------------------------------------------------------------------------
// D: Method for setting the output class
//-----------------------------------------------------------------------------
void COutputManagerAgent::SetOutputClass(string sAOutputClass)
{
	sOutputClass = sAOutputClass;
}

//-----------------------------------------------------------------------------
// A: Methods for registering output devices
//-----------------------------------------------------------------------------

// A: adds output device to list of registered output devices. Output devices
//    have to be registered before they are used
// A：将输出设备添加到注册的输出设备列表。 输出设备在使用前必须进行注册
//	DEFAULT_OUTPUT_DEVICE("nlg", "nlg.launch_query", 1)
bool COutputManagerAgent::RegisterOutputDevice(string sName, string sServerCall, int iParams)
{
	// check if the device is not already registered
	for (unsigned int i = 0; i < vodOutputDevices.size(); i++)
	if (vodOutputDevices[i].sName == sName) // "nlg"
	{
		Log(OUTPUTMANAGER_STREAM, "Device %s already registered", sName.c_str());
		return false;
	}

	/*
		typedef struct
		{
			string sName;			// name of the output device							输出设备名		"nlg"
			string sServerCall;		// external server name (module.function for galaxy)	外部服务器名		"nlg.launch_query"
			int iParams;			// other parameters for the output device				输出设备参数		"1"
		} TOutputDevice;
	*/
	// if not, add it to the list; start by constructing the appropriate structure
	//	DEFAULT_OUTPUT_DEVICE("nlg", "nlg.launch_query", 1)
	TOutputDevice od;
	od.sName = sName;
	od.sServerCall = sServerCall;
	od.iParams = iParams;

	// finally, add it to the list
	vodOutputDevices.push_back(od);
	return true;
}

// A: Sets the default output device. Will invoke a fatal error if device 
//    does not exist.
// A：设置默认输出设备。 如果设备不存在，将调用致命错误。
void COutputManagerAgent::SetDefaultOutputDevice(string sName)
{
	// look for the device in the list of registered devices
	// 在注册设备列表中查找设备
	for (unsigned int i = 0; i < vodOutputDevices.size(); i++)
	if (vodOutputDevices[i].sName == sName)
	{
		iDefaultOutputDevice = i;
		return;
	}
	FatalError("Cannot set default output device to " + sName + ". Device "\
		"not registered.");
}

// A: looks output device up in registered list; returns null if it's not found
// A：在注册列表中查找输出设备。 如果没有找到，则返回null
TOutputDevice *COutputManagerAgent::GetOutputDevice(string sName)
{
	// if no name specified, return the default output device
	if (sName == "")
		return GetDefaultOutputDevice();
	// check through the the list of output devices
	for (unsigned int i = 0; i < vodOutputDevices.size(); i++)
	if (sName == vodOutputDevices[i].sName)
		return &(vodOutputDevices[i]);
	return NULL;
}

// AD: returns pointer to default output device, used when no device was 
//     specified in a prompt
TOutputDevice *COutputManagerAgent::GetDefaultOutputDevice()
{
	if ((iDefaultOutputDevice >= 0) &&
		(iDefaultOutputDevice < vodOutputDevices.size()))
	{
		return &(vodOutputDevices[iDefaultOutputDevice]);
	}
	else
	{
		return NULL;
	}
}

// D: returns the name of the default output device
// D：返回默认输出设备的名称
string COutputManagerAgent::GetDefaultOutputDeviceName()
{
	if ((iDefaultOutputDevice >= 0) &&
		(iDefaultOutputDevice < vodOutputDevices.size()))
	{
		return vodOutputDevices[iDefaultOutputDevice].sName;
	}
	else
	{
		return "";
	}
}

//-----------------------------------------------------------------------------
// A: Output methods
//-----------------------------------------------------------------------------

//AD: Constructs the appropriate output for each prompt and sends it to device
//    specified in sPrompts.  Also sends out any enqueued prompts first.
//    syntax for sPrompts: {...prompt1...}{...prompt2...}...{...promptn...}
//    see documentation for internal details of prompts
/*
AD：为每个提示构造适当的输出output，并将其发送到sPrompts中指定的设备。 还会首先发送任何入队提示。
sPrompts的语法：{... prompt1 ...} {... prompt2 ...} ... {... promptn ...}, 查看提示内部详细信息的文档

//pOutputManager->Output(this, "inform nonunderstanding", fsSystem);

sPrompts   =>  "inform nonunderstanding"
*/
vector<COutput*> COutputManagerAgent::Output(CDialogAgent* pGeneratorAgent, string sPrompts, TFloorStatus fsFinalFloorStatus)
{

	//		check that we know which dialog agent sent ordered this output
	// <1>	检查我们知道哪个对话代理发送排序这个输出
	if (pGeneratorAgent == NULL)
		FatalError(FormatString("No generator agent specified for output "\
		"(dump below).\n%s", sPrompts.c_str()));

	//		check that the prompts are correctly enclosed in braces {}
	// <2>	检查提示是否正确包含在大括号{}
	sPrompts = Trim(sPrompts);
	if ((sPrompts != "") && (sPrompts[0] != '{'))
	{
		sPrompts = "{" + sPrompts + "}";
	}
	// vector of pointers to the generated output objects to return to the calling agent
	// 指向生成的输出对象的指针的向量，以返回到调用代理
	vector <COutput *> voOutputs;

	//		while there are still prompts, process them and send them out
	// <3>	当还有提示时，处理它们并将它们发送出去
	while (sPrompts != "")
	{

		//		get the first prompt from the queue
		// <4>	从队列中获取 【第一个】提示{}对象 sFirstPrompt
		int iFirstPromptLength = FindClosingQuoteChar(sPrompts, 1, '{', '}');
		string sFirstPrompt = sPrompts.substr(0, iFirstPromptLength);
		sPrompts = sPrompts.substr(iFirstPromptLength, sPrompts.size() - iFirstPromptLength);

		//		check that the prompt is not empty; if so, ignore it and continue looping
		// <5>	检查提示不是空的; 如果是，则忽略它并继续循环
		if (sFirstPrompt == "{}") continue;

		//		Sets the post-prompt floor status
		// <6>	设置后提示floor状态,默认为fsSystem
		TFloorStatus fsFloor = fsSystem;
		// <7>	如果prompt为空，则设置为参数值fsFinalFloorStatus
		if (sPrompts == "")
			fsFloor = fsFinalFloorStatus;

		// log the prompt
		Log(OUTPUTMANAGER_STREAM, "Processing output prompt %d from %s. (dump "\
			"below)\n%s", iOutputCounter, pGeneratorAgent->GetName().c_str(),
			sFirstPrompt.c_str());

		// create the new output; if we are in a Galaxy configuration, 
		// it's a CFrameOutput; if in an OAA configuration, it's a 
		// CLFOutput
		// 创建新输出; 
		// 如果我们在一个Galaxy配置，它是一个CFrameOutput; 
		// 如果在OAA配置中，它是一个CLFOutput
		COutput *pOutput = NULL;

#ifdef GALAXY
		if (sOutputClass == "FrameOutput")
			pOutput = (COutput*)(new CFrameOutput);
		else
		{
			FatalError(FormatString("Output manager configured with an unknown "
				"output class: %s", sOutputClass.c_str()));
		}
#endif

#ifdef OAA
		if (sOutputClass == "LFOutput")
			COutput *pOutput = (COutput*)(new CLFOutput);
		else
		{
			FatalError(FormatString("Output manager configured with an unknown "
				"output class: %s", sOutputClass.c_str()));
		}
#endif
		//########################################################创建 output################################################################
		// <8>	创建数处对象output, 强prompt解析成output对象
		//bool CFrameOutput::Create(string sAGeneratorAgentName, int iAExecutionIndex, string sAOutput, TFloorStatus fsAFloor, int iAOutputId)
		if (!pOutput->Create(pGeneratorAgent->GetName(), pStateManager->GetStateHistoryLength() - 1, sFirstPrompt, fsFloor, iOutputCounter))
		{
			// if the output could not be created, deallocate it and ignore
			// 如果output创建失败，则析构对象，并继续循环
			delete pOutput;
			continue;
		}
		//########################################################创建 output################################################################

		//		store a pointer to the created output to send back to the calling agent
		// <9>	存储指向创建的输出的指针以发送回调用代理
		voOutputs.push_back(pOutput);

		if (pOutput->GetAct() == "repeat")
		{
			Repeat();
			continue;
		}

		/*
		start{
				act		**
				object		**
				_repeat_counter		**
				slotName[0]		slotValue[0]
				slotName[1]		slotValue[1]
				...
		}
		end
		*/
		//################################实际输出####################################
		//		send the output
		// <10>	实际输出
		string sOutputSpecification = output(pOutput);
		//################################实际输出###################################

		// log the activity, if it has successfully taken place
		string sTurnId = "User:???";
		if (pInteractionEventManager->GetLastInput())
		{
			sTurnId = "User:" + pInteractionEventManager->GetLastInput()->GetStringProperty("[uttid]");
		}
		if (sOutputSpecification != "")
		{
			Log(OUTPUTMANAGER_STREAM,"Processed output prompt %d (state %d) and sent it to %s (dump below) [%s]\n%s",
				iOutputCounter,
				pStateManager->GetStateHistoryLength() - 1,
				pOutput->sOutputDeviceName.c_str(), sTurnId.c_str(),
				sOutputSpecification.c_str());
		}

		// and finally increase the output counter
		// <11>	增加输出计数器
		iOutputCounter++;
	}//while (sPrompts != "")

	return voOutputs;
}//Output(CDialogAgent* pGeneratorAgent,	string sPrompts, TFloorStatus fsFinalFloorStatus)



// AD: Constructs and sends prompt that will repeat last utterance
// 构造并发送 [将重复最后一次发音] 的提示
void COutputManagerAgent::Repeat()
{

	// guard for safe access using the critical section 
	// guard 使用临界区安全访问
	EnterCriticalSection(&csCriticalSection);

	unsigned int i;
	unsigned int uiChosen = (unsigned int)-1;

	// check if there is something in history to repeat
	// 检查在历史中是否有东西要重复
	if (ohHistory.GetSize() == 0)
	{
		Warning("Output history is empty, there is nothing to be repeated.");
		// leave the critical section 
		LeaveCriticalSection(&csCriticalSection);
		return;
	}

	// if we have been notified of all outputs
	// 如果我们已经收到所有输出的通知
	if ((vopRecentOutputs.size() == 0) && (ohHistory[0]->GetConveyance() == cConveyed) && !(ohHistory[0]->CheckFlag(":non-repeatable")))
		// and the last one is repeatable, then choose that one
		//最后一个是可重复的，然后选择那个
		uiChosen = 0;
	else
	{
		i = 0;
		if ((vopRecentOutputs.size() == 0) && (ohHistory[0]->GetConveyance() != cConveyed))
			i = 1;

		// now go through history, most recent first, and decide which to repeat
		// 现在经历历史，最近的第一个，并决定重复哪个
		for (; i < ohHistory.GetSize(); i++)
		{
			if (!(ohHistory[i]->CheckFlag(":non-repeatable")))// 非 ":non-repeatable"
			{
				uiChosen = i;
				break;
			}
		}
	}

	// if nothing got chosen so far
	if (uiChosen == -1)
	{
		Log(OUTPUTMANAGER_STREAM,
			"No repeatable outputs found.  Repeating most recent.");
		if ((vopRecentOutputs.size()) != 0 ||
			(ohHistory[0]->GetConveyance() == cConveyed) || (ohHistory.GetSize() == 1))
			uiChosen = 0;
		else
			uiChosen = 1;
	}

	// log the decision 
	Log(OUTPUTMANAGER_STREAM, "Repeating: %s", ohHistory.GetUtteranceAt(uiChosen).c_str());

	// clone old output and give it a new output id
	// 克隆，赋值新id
	COutput *opToRepeat = ohHistory[uiChosen]->Clone(iOutputCounter);

	// set the dialog state to the current one
	//	opToRepeat->SetDialogStateIndex(pStateManager->GetStateHistoryLength()-1);
	//	opToRepeat->SetDialogState(pStateManager->GetStateAsString());

	// leave the critical section 
	LeaveCriticalSection(&csCriticalSection);

	// send out the output
	// 发出输出
	output(opToRepeat);

	// increment the output counter
	iOutputCounter++;
}

// AD: Incorporates conveyance information and move the output from the recent
//     list to the history
// AD：合并传送信息, 并将最近列表中的output移动到历史记录
void COutputManagerAgent::Notify(int iOutputId, int iBargeinPos, string sConveyance, string sTaggedUtt)
{

	// log the reception of the notify message
	Log(OUTPUTMANAGER_STREAM, "Received final output notification frame. id: %d; "\
		"bargein: %d; Conveyance Info: (dump below)\n"\
		"%s\nTagged Utterance: (dump below)\n%s",
		iOutputId, iBargeinPos, sConveyance.c_str(),
		sTaggedUtt.c_str());

	//		parse the conveyance information string
	// <1>	解析输送信息字符串
	vector<string> vsParsedConveyance = PartitionString(sConveyance, " ");

	// obtain an index to the corresponding output
	// <2>	获取相应output的索引
	unsigned int iIndex = getRecentOutputIndex(iOutputId);
	if (iIndex == -1)
	{
		Warning(FormatString("Received notification about output %d, which doesn't "\
			"exist is the list of recent outputs. Ignoring it.",
			iOutputId));
		return;
	}

	//		step through concept/position pairs and set conveyance status of concepts
	// <3>	遍历 概念/位置对, 并设置数处对象output的所需要的concept的传送状态[转达， 失败]
	for (unsigned int i = 0; i < vsParsedConveyance.size(); i += 2)
	{
		if ((iBargeinPos == -1) || (atoi(vsParsedConveyance[i + 1].c_str()) < iBargeinPos))
		{
			vopRecentOutputs[iIndex]->NotifyConceptConveyance(vsParsedConveyance[i], cConveyed);
		}
		else
		{
			vopRecentOutputs[iIndex]->NotifyConceptConveyance(vsParsedConveyance[i], cFailed);
		}
	}

	//		set whether or not entire output was conveyed (was if iBargeinPos == -1)
	// <4>	设置整个output对象的传送状态（是if iBargeinPos == -1）
	if (iBargeinPos == -1)
		vopRecentOutputs[iIndex]->SetConveyance(cConveyed);//传达
	else
		vopRecentOutputs[iIndex]->SetConveyance(cFailed);//失败

	//		increments the number of times this prompt has been uttered
	// <5>	递增此提示已被说出的次数
	vopRecentOutputs[iIndex]->IncrementRepeatCounter();

	//		guard for safe access
	// <6>	保护安全访问 - 临界区
	EnterCriticalSection(&csCriticalSection);

	//		move last prompt frame to history
	//		normalize the tagged utterance before
	// <7>	将最后一个prompt移到历史记录
	sTaggedUtt = Trim(sTaggedUtt, " \n");
	ohHistory.AddOutput(vopRecentOutputs[iIndex], sTaggedUtt);//添加到历史记录
	vopRecentOutputs.erase(vopRecentOutputs.begin() + iIndex);

	//		guard for safe access
	// <8>	退出临界区
	LeaveCriticalSection(&csCriticalSection);

	// finally, log the new history on the OUTPUTHISTORY_STREAM
	Log(OUTPUTHISTORY_STREAM, ohHistory.ToString());
}

// D: Gets information about prompts that are about to be sent out
// D：获取有关要发送的提示的信息
void COutputManagerAgent::PreliminaryNotify(int iOutputId, string sTaggedUtt)
{

	// log the reception of the notify message
	Log(OUTPUTMANAGER_STREAM,
		"Received preliminary output notification frame. id: %d; "
		"Tagged Utterance: (dump below)\n%s",
		iOutputId, sTaggedUtt.c_str());

	//		obtain an index to the corresponding output
	// <1>	获取输出列表中相应输出的索引
	unsigned int iIndex = getRecentOutputIndex(iOutputId);
	if (iIndex == -1)
	{
		Warning(FormatString("Received notification about output %d, which doesn't "\
			"exist is the list of recent outputs. Ignoring it.",
			iOutputId));
		return;
	}

	//		now go through the prompt and compute the concepts
	// <2>	现在遍历prompt信息并计算concept
	/*
		 PROMPT ::= [FLAGS] ACT OBJECT [PARAMETERS]
		 FLAGS ::= (see #Flags)
		 ACT ::= alphanumeric string
		 OBJECT ::= alphanumeric string
		 PARAMETERS ::= PARAMETER [PARAMETERS]
		 PARAMETER ::= [ATTR]'<'CONCEPT | ATTR='"'VALUE'"'
		 ATTR ::= alphanumeric string
		 CONCEPT ::= alphanumeric string
		 VALUE ::= alaphanumeric string
	 */
	vector<string> svWords = PartitionString(sTaggedUtt, " "); // [FLAGS] ACT OBJECT [PARAMETERS]
	string sConcept;
	for (unsigned int i = 0; i < svWords.size(); i++)
	{
		string sWord = Trim(svWords[i]);
		if (sWord[0] == '<')// PARAMETER ::= [ATTR]'<'CONCEPT | ATTR='"'VALUE'"'
		{
			//		if we have the beginning of a concept
			// <3>	如果我们有一个概念   [ATTR]'<'CONCEPT | ATTR='"'VALUE'"' 
			sConcept = sWord.substr(1, sWord.length() - 1);
			// and notify that we are doing an ICT on it
			// <4>	通知我们正在做一个ICT[隐式确认]
			//		获取当前concept对象
			CConcept* pConcept = vopRecentOutputs[iIndex]->GetConceptByName(sConcept);
			if (pConcept)
			{
				for (int j = vopRecentOutputs[iIndex]->GetDialogStateIndex();
					(j < pStateManager->GetStateHistoryLength()) && ((*pStateManager)[j].fsFloorStatus != fsUser);
					j++)
				{
					//##################################################################################
					//计划外隐式确认
					pDMCore->SignalUnplannedImplicitConfirmOnConcept(j, pConcept);
					//##################################################################################
				}

				// now log that we found a concept
				Log(OUTPUTMANAGER_STREAM, "Signaling UNPLANNED_IMPL_CONF on concept %s", sConcept.c_str());
			}
		}// PARAMETER :: = [ATTR]'<'CONCEPT | ATTR = '"'VALUE'"'
	}
}

// D: Cancels a concept notification request
// D：取消概念通知请求
void COutputManagerAgent::CancelConceptNotificationRequest(
	CConcept* pConcept)
{
	// guard for safe access
	EnterCriticalSection(&csCriticalSection);
	// remove notification from recent outputs
	for (unsigned int i = 0; i < vopRecentOutputs.size(); i++)
		vopRecentOutputs[i]->CancelConceptNotificationRequest(pConcept);
	// remove notification from history outputs
	for (unsigned int i = 0; i < ohHistory.GetSize(); i++)
		ohHistory[i]->CancelConceptNotificationRequest(pConcept);
	// leave critical section
	LeaveCriticalSection(&csCriticalSection);
}

// D: Changes a concept notification request
void COutputManagerAgent::ChangeConceptNotificationPointer(
	CConcept* pOldConcept, CConcept* pNewConcept)
{
	// guard for safe access
	EnterCriticalSection(&csCriticalSection);
	// remove notification from recent outputs
	for (unsigned int i = 0; i < vopRecentOutputs.size(); i++)
		vopRecentOutputs[i]->ChangeConceptNotificationPointer(
		pOldConcept, pNewConcept);
	// remove notification from history outputs
	for (unsigned int i = 0; i < ohHistory.GetSize(); i++)
		ohHistory[i]->ChangeConceptNotificationPointer(
		pOldConcept, pNewConcept);
	// leave critical section
	LeaveCriticalSection(&csCriticalSection);
}

// D: Return the list of prompts that are waiting for notification
// D：返回等待通知的提示列表
string COutputManagerAgent::GetPromptsWaitingForNotification()
{
	// construct the string
	string sResult = "";
	// guard for safe access
	// 防护安全通道 - 对临界区加锁
	EnterCriticalSection(&csCriticalSection);
	for (unsigned int i = 0; i < vopRecentOutputs.size(); i++)
		sResult = FormatString("%s %d", sResult.c_str(), vopRecentOutputs[i]->iOutputId);

	// leave critical section
	// 退出临界区
	LeaveCriticalSection(&csCriticalSection);

	// return the string 
	return sResult;
}

//-----------------------------------------------------------------------------
// A: COutputManager private (helper) methods
//-----------------------------------------------------------------------------

// A: This is the function that actually sends the output to the interaction  manager
// A：这是实际将输出发送到交互管理器的函数
string COutputManagerAgent::output(COutput* pOutput)
{
	/*
	start{
		act		**
		onbect		**
		_repeat_counter		**
		slotName[0]		slotValue[0]
		slotName[1]		slotValue[1]
		...
	}
	end
	*/
	//		obtain the string representation of the output
	// <1>	获取输出的字符串表示形式
	string sOutput = pOutput->ToString();

	// if we are in a Galaxy configuration, send requests through the Galaxy 
	// interface
//#ifdef GALAXY	

	// <2>	填充调用的参数对象gcGalaxyCall
	TGIGalaxyActionCall gcGalaxyCall;
	gcGalaxyCall.sModuleFunction = "main";
	gcGalaxyCall.sActionType = "system_utterance";
	gcGalaxyCall.s2sProperties.insert(STRING2STRING::value_type(":inframe", sOutput));

	gcGalaxyCall.s2sProperties.insert(STRING2STRING::value_type(":id",
		FormatString("%s:%.3d", DMI_GetSessionID().c_str(), iOutputCounter)));
	gcGalaxyCall.s2sProperties.insert(STRING2STRING::value_type(":utt_count",
		FormatString("%d", iOutputCounter)));
	gcGalaxyCall.s2sProperties.insert(STRING2STRING::value_type(":dialog_state_index",
		FormatString("%d", pOutput->GetDialogStateIndex())));
	gcGalaxyCall.s2sProperties.insert(STRING2STRING::value_type(":dialog_state",
		pOutput->GetDialogState()));
	gcGalaxyCall.s2sProperties.insert(STRING2STRING::value_type(":dialog_act",
		pOutput->GetAct()));
	gcGalaxyCall.s2sProperties.insert(STRING2STRING::value_type(":final_floor_status",
		pOutput->GetFinalFloorStatusLabel()));
	// Adds the output flags to the galaxy frame
	for (unsigned int i = 0; i < pOutput->vsFlags.size(); i++)
	{
		gcGalaxyCall.s2sProperties.insert(STRING2STRING::value_type(
			pOutput->vsFlags[i], "true"));
	}

	//		retrieve the current thread id
	// <3>	检索当前的线程id
	DWORD dwThreadId = GetCurrentThreadId();

	//		send the message to the Galaxy Interface Thread
	// <4>	发送消息到Galaxy Interface Thread
	PostThreadMessage(g_idDMInterfaceThread, WM_GALAXYACTIONCALL,
		(WPARAM)&gcGalaxyCall, dwThreadId);

	// and wait for a reply
	// <5>	等待回复
	MSG Message;
	GetMessage(&Message, NULL, WM_ACTIONFINISHED, WM_ACTIONFINISHED);
//#endif // GALAXY

	// if we are in an OAA configuration, send requests through the OAA
	// interface
#ifdef OAA
	TOIOAACall oaacOAACall;
	oaacOAACall.picltGoal = icl_NewTermFromString((char *)sOutput.c_str());
	// check that the OAA goal was successfully created
	if (oaacOAACall.picltGoal == NULL)
	{
		Error(FormatString("Error creating ICL Term for output (dump below). "\
			"Output will not be sent.\n%s", sOutput.c_str()));
		return "";
	}
	oaacOAACall.picltInitialParams = icl_NewTermFromString("[]");
	oaacOAACall.ppicltOutParams = NULL;
	oaacOAACall.ppicltSolutions = NULL;

	// retrieve the current thread id
	DWORD dwThreadId = GetCurrentThreadId();

	// send the message to the OAA Interface Thread
	PostThreadMessage(g_idDMInterfaceThread, WM_OAACALL,
		(WPARAM)&oaacOAACall, dwThreadId);

	// and wait for a reply
	MSG Message;
	GetMessage(&Message, NULL, WM_ACTIONFINISHED, WM_ACTIONFINISHED);
#endif

	//		record request to await notification, if notifications are enabled for that device
	// <6>	如果为该设备启用了通知，则将请求记录到等待通知的输出列表
	if (GetOutputDevice(pOutput->sOutputDeviceName)->iParams & OD_NOTIFIES)
	{
		// guard access to vopRecentRequests by critical section
		EnterCriticalSection(&csCriticalSection);
		vopRecentOutputs.push_back(pOutput);
		LeaveCriticalSection(&csCriticalSection);
	}
	else
	{
		// o/w move the output directly to the history of outputs...
		// guard for safe access
		EnterCriticalSection(&csCriticalSection);
		// and before moving, also set the output as conveyed, since there
		// will be no future notification
		// <6>	如果不同通知，则将输出添加到输出历史列表，并且设置输出状态cConveyed
		pOutput->SetConveyance(cConveyed);
		ohHistory.AddOutput(pOutput, sOutput);
		// guard for safe access
		LeaveCriticalSection(&csCriticalSection);
	}

	return sOutput;
}

// A: utility function that finds the index of an output in the recent outputs
//    list, based on the output id
// A：效用函数，根据输出ID查找最近输出列表中的输出的索引
unsigned int COutputManagerAgent::getRecentOutputIndex(int iOutputId)
{
	// go through the list and look for that id
	for (unsigned int i = 0; i < vopRecentOutputs.size(); i++)
	{
		if (vopRecentOutputs[i]->iOutputId == iOutputId)
			return i;
	}
	// if not found, return -1
	return (unsigned int)-1;
}