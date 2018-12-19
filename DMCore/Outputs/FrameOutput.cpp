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
// FRAMEOUTPUT.CPP - implementation of the CFrameOutput class. This class is 
//					 derived off COutput and encapsulates a Rosetta-type output 
//					 for the dialog manager.
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
//   [2005-06-02] (antoine): added the possibility to have a "prompt_header" and
//                           "prompt_ending" parameters in the OutputManager's 
//                           configuration, the values of which get appended 
//                           after resp. before and after each prompt, before
//                           they get parsed by the Create method
//   [2005-01-11] (antoine): changed ToString so that it includes a slot giving
//							  the number of times the output has been repeated
//   [2004-02-24] (dbohus):  changed outputs so that we no longer clone 
//                            concepts but use them directly
//   [2003-10-08] (dbohus):  fixed the prompt parser so that it can accept 
//                            values in quotes
//   [2003-03-13] (antoine): modified CFrameOutput::Create so that it uses only
//                            the value of the mode of the confidence distr.
//                            of each concept (instead of the list of all 
//                            values)
//   [2003-03-13] (dbohus,
//                 antoine): fixed concept cloning in FrameOutput::Create
//                            (set concept name)
//   [2003-01-15] (dbohus):  added support for sending merged history concepts
//                            i.e. name<@concept
//   [2002-10-10] (dbohus):  rewritten Create function so that it's cleaner
//   [2002-06-25] (dbohus):  unified Create and CreateWithClones in a single
//							  create function which takes a bool parameter
//   [2002-06-17] (dbohus):  deemed preliminary stable version 0.5
//	 [2002-06-14] (dbohus):  started working on this
// 
//-----------------------------------------------------------------------------

#include "FrameOutput.h"

// D: Check that the compiler configuration is correct (the Frame-type outputs
//    are used only in a Galaxy configuration so far)
#ifndef GALAXY
#error FrameOutput.cpp should be compiled only in the galaxy version
#endif

#include "../../DMCore/Log.h"
#include "../../DMCore/Agents/CoreAgents/OutputManagerAgent.h"

// D: a pointer to the actual Output Manager agent
extern COutputManagerAgent* pOutputManager;

//-----------------------------------------------------------------------------
// D: Constructors and Destructor
//-----------------------------------------------------------------------------
// D: Constructor - does nothing so far
CFrameOutput::CFrameOutput()
{
}

// D: Virtual destructor - does nothing so far
CFrameOutput::~CFrameOutput()
{
}

//-----------------------------------------------------------------------------
// D: COutput-Specific public methods
//-----------------------------------------------------------------------------
// D: virtual function for cloning an output 
COutput* CFrameOutput::Clone(int iNewOutputId)
{
	// create the new output
	CFrameOutput* pfoClone = new CFrameOutput;
	// copy the slot-value information
	pfoClone->vsSlotNames = vsSlotNames;
	pfoClone->vsValues = vsValues;
	// copy the rest of the stuff using the auxiliary COutput::clone() method
	clone(pfoClone, iNewOutputId);
	// finally return the cloned output
	return pfoClone;
}

// D: creates the internal representation starting from a string description
//    of the prompt. Returns false if an output cannot be created
// D：从提示符的字符串描述开始创建内部表示。 如果无法创建输出，则返回false
bool CFrameOutput::Create(string sAGeneratorAgentName,	int iAExecutionIndex, string sAOutput, TFloorStatus fsAFloor, int iAOutputId)
{

	//		set the prompt id and caller agent name
	// <1>	设置提示ID和生成代理名称
	iOutputId = iAOutputId;
	sGeneratorAgentName = sAGeneratorAgentName;
	fsFinalFloorStatus = fsAFloor; //在这个输出的结束时候的floor状态 枚举：【位置、系统、用户、自由】

	//		Set the state index and string representation
	// <2>	设置状态索引和字符串表示
	iExecutionIndex = iAExecutionIndex;
	sDialogState = pStateManager->GetStateAsString(pStateManager->operator[](iExecutionIndex));

	//		gets a reference to the sending agent
	// <3>	获取发送agent的引用
	CDialogAgent *pdaGenerator = (CDialogAgent *)AgentsRegistry[sGeneratorAgentName];
	if (pdaGenerator == NULL)
	{
		// if we can't locate the generator agent
		Warning(FormatString("Could not get a pointer to generator agent %s "\
			"for frame-output (dump below). Output could not"\
			" be created.\n%s",
			sGeneratorAgentName.c_str(),
			sAOutput.c_str()));
		return false;
	}

	//		at starters, the output has not yet been conveyed
	// <4>	在启动时，输出尚未传达
	cConveyance = cNotConveyed;

	//		work on another copy sToParse
	// <5>	输出的prompt {}
	string sToParse = sAOutput;

	// normalize the syntax 
	sToParse = Trim(sToParse, " \n\t}{");

	// check that the prompt is not empty
	if (sToParse == "") return false;

	//		(antoine 06/02/05) If there is a systematic prompt header
	//		in the OutputManager's configuration, append it to the prompt
	// <6>	如果有一个header 标志在OutputManager的配置中，请将其附加到提示符prompt头部
	if (pOutputManager->HasParameter("prompt_header"))
	{
		sToParse = pOutputManager->GetParameterValue("prompt_header")
			+ " " + sToParse;
	}

	//		(antoine 06/02/05) If there is a systematic prompt ending
	//		in the OutputManager's configuration, append it to the prompt
	// <7>	如果有一个系统ender在OutputManager的配置中，请将其附加到提示prompt尾部
	if (pOutputManager->HasParameter("prompt_ending"))
	{
		sToParse += " " + pOutputManager->GetParameterValue("prompt_ending");
	}

	// and a token to be used to read in the prompt bit by bit
	// token 用于在提示符中逐位读取的标记
	string sToken;

	// start splitting the string, but skipping over sequences contained
	// within quotes
	// 开始分割字符串，但跳过包含在引号内的序列
	SplitOnFirst(sToParse, " ", sToken, sToParse);
	sToParse = TrimLeft(sToParse);

	//		at the beginning look for flags
	// <8>	首先解析[FlAG]
	//		[FLAGS] ACT OBJECT [PARAMETERS]
	while (sToken[0] == ':')
	{
		vsFlags.push_back(sToken);// [:non-interruptable , :non-interruptable, ...]
		SplitOnFirst(sToParse, " ", sToken, sToParse);
		sToParse = TrimLeft(sToParse);
	}

	//		next we should have the act 
	//		[FLAGS] ACT OBJECT [PARAMETERS]
	// <9>	接下来解析ACT
	if (sToken == "")
	{
		Warning("Cannot create frame-output: missing act (dump below).\n" +	sAOutput);
		return false;
	}
	else
	{
		// it could be just the act, or something like act = value
		// 它可能是ACT，或者像 act = value
		string sLeftSide, sRightSide;
		if (SplitOnFirst(sToken, "=", sLeftSide, sRightSide))
		{
			if (sLeftSide == "act")
				sAct = Trim(sRightSide);//赋值
			else
			{
				Warning("Cannot create frame-output: missing act (dump below).\n" +	sAOutput);
				return false;
			}
		}
		else
		{
			sAct = Trim(sToken);//赋值
		}
	}

	// continue splitting the string
	// [FLAGS] ACT OBJECT [PARAMETERS]
	// 继续拆分字符串
	SplitOnFirst(sToParse, " ", sToken, sToParse);
	sToParse = TrimLeft(sToParse);

	//		next we should have the object
	// <10>	接下来解析OBJECT
	if (sToken == "")
	{
		Warning("Cannot create frame-output: missing object (dump below).\n" +	sAOutput);
		return false;
	}
	else
	{
		// it could be just the object, or something like object = value
		string sLeftSide, sRightSide;
		if (SplitOnFirst(sToken, "=", sLeftSide, sRightSide, '"'))
		{
			if (sLeftSide == "object")
				sObject = Trim(sRightSide);//赋值
			else
			{
				Warning("Cannot create frame-output: missing object (dump below).\n" +
					sAOutput);
				return false;
			}
		}
		else
		{
			sObject = Trim(sToken);//赋值
		}
	}

	//			now, fill in the rest of the frame, 
	//			[FLAGS] ACT OBJECT [PARAMETERS]
	//	<11>	现在，填充框架的其余部分 [PARAMETERS]
	while (sToParse != "")// [PARAMETERS]
	{
		// continue splitting, this time taking quotes into account, as some
		// of the values might be quoted strings
		// 继续分割，这次考虑引号，因为一些值可能是带引号的字符串
		SplitOnFirst(sToParse, " ", sToken, sToParse, '"');
		sToParse = TrimLeft(sToParse);

		string sLeftSide, sRightSide;
		if (SplitOnFirst(sToken, "<", sLeftSide, sRightSide, '"'))
		{
			// check for form 【slot<concept_name】

			// then look for the concept; 
			// start by identifying getting a pointer to the generator agent
			// 首先通过识别获取指向生成agent的指针
			string sConceptName = Trim(sRightSide);

			//		now identify the concept relative to the generator agent
			// <12>	现在识别相对于agent的concept
			CConcept *pConcept;
			if (sConceptName[0] == '@')
			{
				// then we want a history merged version of the concept   
				// 那么我们想要一个历史合并版本的concept
				sConceptName = sConceptName.substr(1, sConceptName.length() - 1);
				pConcept = pdaGenerator->C(sConceptName).CreateMergedHistoryConcept();
			}
			else
			{
				// o/w simply take a pointer to the concept
				// 只需要一个指针的概念
				pConcept = &(pdaGenerator->C(sConceptName));
			}

			if (pConcept != &NULLConcept)
			{
				// <13>	设置相应的concept
				//##############################设置 concept################################################
				vcpConcepts.push_back(pConcept); //添加到输出列表
				pConcept->SetWaitingConveyance();// D：concept设置等待传送标志
				vbNotifyConcept.push_back(true); // concept 是否被通知=> yes[true]
				//###############################设置 concept###############################################

			}
			else
			{
				FatalError(FormatString("Could not find concept %s relative to "\
					"agent %s for frame-output (dump below).\n%s",
					sConceptName.c_str(), sGeneratorAgentName.c_str(),
					sAOutput.c_str()));
			}

			//		and obtain it's value
			// <14>	并获取concept[slot]的值
			vsValues.push_back(pConcept->GroundedHypToString());

			//		 also set the slot name
			//	<15> 也设置concept[slotValue]名称
			string sSlotName = Trim(sLeftSide);
			if (sSlotName != "")
			{
				// put the slot name
				vsSlotNames.push_back(Trim(sLeftSide));
			}
			else
			{
				// o/w we have only <concept, so we should use the concept name
				//我们只有<概念，所以我们应该使用概念名
				vsSlotNames.push_back(pConcept->GetName());
			}

		}
		else if (SplitOnFirst(sToken, "=", sLeftSide, sRightSide, '"'))
		{
			// <16> 设置非concept输入参数
			//##############################设置 非concept输入参数################################################
			// check for form slot = value
			vsSlotNames.push_back(Trim(sLeftSide));
			vcpConcepts.push_back(NULL); //非concept
			vbNotifyConcept.push_back(false);
			// trim the quotes also off the value (in case there are any)
			// 修剪引号也关闭值（如果有的话）
			vsValues.push_back(Trim(sRightSide, " \n\t\"") + "\n");
			//##############################设置 非concept输入参数################################################

		}

	}//while (sToParse != "")// [PARAMETERS]

	//		set output device (look through the flags for an output device specifier
	// <17>	设置输出设备（查看输出设备说明符的标志
	sOutputDeviceName = "";
	for (unsigned int j = 0; j < vsFlags.size(); j++)
	{
		//TK: should stip leading ':' from flag
		string flag = !vsFlags[j].empty() && vsFlags[j][0] == ':' ?	vsFlags[j].substr(1) : vsFlags[j];
		if (pOutputManager->GetOutputDevice(flag) != NULL)
		{
			sOutputDeviceName = flag;
			break;
		}
	}

	//		if it was not specified in the modifiers, set the output agent to
	//		be the default one
	// <18>	如果没有在修饰符中指定，请将输出代理设置为默认代理
	if (sOutputDeviceName == "")
		sOutputDeviceName = pOutputManager->GetDefaultOutputDeviceName();

	return true;
}//Create(string sAGeneratorAgentName, int iAExecutionIndex, string sAOutput, TFloorStatus fsAFloor, int iAOutputId)

// D: converts the frame-output into an appropriate string representation 
//    to be sent to a Rossetta-type output external agent
// D：将 [帧输出] 转换为适当的字符串表示，以发送到Rossetta类型的输出外部代理
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
string CFrameOutput::ToString()
{

	string sOutputFrame = "start\n{\n";

	// generate act type
	sOutputFrame += "act\t" + sAct + "\n";

	// generate object 
	sOutputFrame += "object\t" + sObject + "\n";

	// adds default indicators
	sOutputFrame += "_repeat_counter\t" + IntToString(iRepeatCounter) + "\n";

	// no flags are generated at this point
	// and now go through the frame and generate all the rest, 
	for (unsigned int i = 0; i < vsSlotNames.size(); i++)
	{
		sOutputFrame += vsSlotNames[i];
		sOutputFrame += "\t";
		sOutputFrame += vsValues[i];
	}

	// terminate
	sOutputFrame += "}\nend\n";

	// and return
	return sOutputFrame;
}