//=============================================================================
//
//   Copyright (c) 2000-2005, Carnegie Mellon University.  
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
// GALAXYINTERACTIONEVENT.H - encapsulates an interaction event such as a notification
//							of prompt delivery, a new user input, barge-in...
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
//   [2005-06-22] (antoine): started this
//
//-----------------------------------------------------------------------------

#include "GalaxyInteractionEvent.h"

// D: Include galaxy stuff
extern "C" {
#pragma warning (disable : 4100)	// disable a warning for unused params
#pragma warning (disable: 4117)		// disable warning for redefines
#include <galaxy/galaxy_all.h>
#pragma warning (default: 4117)
#pragma warning (default: 4100)
}

//---------------------------------------------------------------------
// Constructor and destructor
//---------------------------------------------------------------------
//

// A: Default constructor
CGalaxyInteractionEvent::CGalaxyInteractionEvent() {}

// A: Constructs an event from a Galaxy frame
CGalaxyInteractionEvent::CGalaxyInteractionEvent(void *gfAEvent)
{
	//		identify the type of event
	// <1>	获取事件类型
	sType = (string)Gal_GetString((Gal_Frame)gfAEvent, ":event_type");

	//		check if this is a complete or partial event notification
	// <2>	检查这是否 完成 或 部分事件 通知
	bComplete = ((int)Gal_GetInt((Gal_Frame)gfAEvent, ":event_complete") == 1);

	// <3>	转换成EventFrame
	Gal_Frame gfEventFrame = Gal_CopyFrame((Gal_Frame)gfAEvent);//获取时间属性Hash

	// fills in the properties hash of the event
	s2sProperties.clear();
	int nProperties = 0;
	char** ppszProperties = NULL;

	// <4>	解析出属性gfProperties， 转换成ppszProperties
	Gal_Frame gfProperties = Gal_GetFrame(gfEventFrame, ":properties");
	ppszProperties = Gal_GetProperties(gfProperties, &nProperties);

	//		now go through all the keys, and add them 
	// <5>	遍历属性，添加属性 <name , key>
	for (int i = 0; i < nProperties; i++)//填充事件属性
	{
		string sPropertyName = (string)ppszProperties[i];

		//		adds brackets around the feature name so that it can be handled the same way as a grammar slot by the system (antoine)
		// <6>	在feature名称周围添加方括号[]，以便系统（antoine）可以按照与语法槽slot相同的方式处理它
		sPropertyName = Trim(sPropertyName, ": ");
		// <7>	属性两边添加【】
		if (!sPropertyName.empty() && sPropertyName.at(0) != '[')
			sPropertyName = "[" + sPropertyName + "]";
		// <8>	添加属性 key value
		s2sProperties.insert(STRING2STRING::value_type(
			sPropertyName,
			(string)Gal_GetString(gfProperties, ppszProperties[i])));
	}

	// by default, events get a confidence of 1.0
	// NB: should we change this and use a fixed slot in the Galaxy frame for confidence?
	// <8>	默认event的执行度为 1.0f
	fConfidence = 1.0f;//系统在 检测/理解 事件时的 置信度 = 1.0f

	// finally, deallocate ppszProperties (allocation was done by 
	// Gal_GetProperties
	if (ppszProperties != NULL)
		free(ppszProperties);

	// <9>	添加解析出的事件，指针gfEvent存储在CInteractionEvent的子类中
	gfEvent = gfEventFrame;
}

// A: Destructor
CGalaxyInteractionEvent::~CGalaxyInteractionEvent()
{
	if (gfEvent)
	{
		Gal_FreeFrame((Gal_Frame)gfEvent);
	}
}

//---------------------------------------------------------------------
// CInteractionEvent-specific methods
//---------------------------------------------------------------------
//

// A: Returns the Galaxy frame for the event
void *CGalaxyInteractionEvent::GetEventFrame()
{
	return gfEvent;
}

