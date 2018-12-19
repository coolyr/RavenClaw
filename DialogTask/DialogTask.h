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
// DIALOGTASK.H   - This module contains the description of the dialog task, 
//                  (i.e. all the user defined agencies, etc, etc)
// DIALOGTASK.H - ��ģ������Ի�������������������û�����Ĵ���ȵȣ�
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
//   [2004-04-24] (dbohus):  added REOPEN_ON_COMPLETION
//   [2004-04-12] (dbohus):  added macro for reopening topics
//   [2003-04-09] (dbohus, 
//                 antoine): added macro for logging
//   [2003-03-17] (dbohus):  added macro for specifying server name and port
//   [2002-11-21] (dbohus):  added macros for specifying usage of binding 
//                            filters in the core configuration
//   [2002-03-15] (dbohus):  started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __DIALOGTASK_H__
#define __DIALOGTASK_H__

#include "../DMCore/Core.h"
#include "../DMCore/Agents/AllAgents.h"

// D: function that will get called whenever a new session starts. It gives 
//    the developer the opportunity to customize part of the core agents, 
//    (for instance alter logging behavior, declare multiple output streams
//    etc), and do some other initialization work 
//	D��ÿ���»Ự��ʼʱ���õĺ����� 
//	��Ϊ������Ա�ṩ�˶��ƺ��Ĵ����һ���ֵĻ��ᣨ�����޸���־��Ϊ���������������ȣ�������һЩ������ʼ������
void DialogTaskOnBeginSession();

//-----------------------------------------------------------------------------
// 
// A set of macros that can be used to specify various configuration options 
// in the DialogTaskOnBeginSession() function
//
//-----------------------------------------------------------------------------

// D: the define for the galaxy server configuration
#define DMSERVER_CONFIGURATION(Name, Port)\
    extern "C" {\
        char* lpszDMServerName = Name;\
        int iDMServerPort = Port;\
    }\


// D: the define for the DialogTaskOnBeginSession function
#define CORE_CONFIGURATION(Configuration)\
	void DialogTaskOnBeginSession() {\
        Configuration\
	}\


// D: define the name of the slot containing the confidence information from 
// helios
// TK: DEPRECATED
#define DEFINE_CONFIDENCE_SLOTNAME(Name)\
    ;\

// D: define for declaring output devices
// D��define������������豸
#define USE_OUTPUT_DEVICES(Devices)\
	{\
		Devices\
	}\

// D: define for declaring a certain output device
#define OUTPUT_DEVICE(Name, ServerCall, Params)\
	pOutputManager->RegisterOutputDevice(Name, ServerCall, Params);\

// D: define for declaring the default output device
// D��define��������Ĭ������豸
#define DEFAULT_OUTPUT_DEVICE(Name, ServerCall, Params)\
	pOutputManager->RegisterOutputDevice(Name, ServerCall, Params);\
	pOutputManager->SetDefaultOutputDevice(Name);\

// D: define for the output class
#define USE_OUTPUT_CLASS(OutputClass)\
    pOutputManager->SetOutputClass(#OutputClass);\

// D: define the default timeout period
#define DEFAULT_TIMEOUT_PERIOD(Period)\
	pDMCore->SetDefaultTimeoutPeriod(Period);\

// D: define the library agents to be used
// D������Ҫʹ�õ� Library Agents
#define USE_LIBRARY_AGENTS(Agents)\
    {\
        Agents\
    }\

// D: declare which library agents to be used
// D������Ҫʹ����Щ�����
#define LIBRARY_AGENT(AgentType, AgentName, RegistrationFunction, ConfigurationString)\
	pDTTManager->Use(#AgentType, #AgentName, &RegistrationFunction, ConfigurationString);

// D: define the grounding model types to use
// D������Ҫʹ�õĽӵ�ģ������
#define USE_GROUNDING_MODEL_TYPES(ModelTypes)\
    {\
        ModelTypes\
    }\

// D: define the grounding actions to be used
// D������Ҫʹ�õĽӵز���
#define USE_GROUNDING_ACTIONS(Actions)\
    {\
        Actions\
    }\

// D: define for using all the grounding model types
// D������ʹ�����еĽӵ�ģ������
#define USE_ALL_GROUNDING_MODEL_TYPES\
    USE_GROUNDING_MODEL_TYPES(ALL_GROUNDING_MODEL_TYPES)


// D: declare which grounding models are used
// D������ʹ����Щ�ӵ�ģ��
#define GROUNDING_MODEL_TYPE(Name, FactoryMethod)\
    pGroundingManager->RegisterGroundingModelType(#Name, FactoryMethod);

// D: define for using all the grounding actions
// D��defineʹ�����еĽӵض���
#define USE_ALL_GROUNDING_ACTIONS(Configuration)\
    USE_GROUNDING_ACTIONS(ALL_GROUNDING_ACTIONS(Configuration))

// D: declare which grounding actions are to be used
// D������Ҫʹ����Щ�ӵش�ʩ
//void CGroundingManagerAgent::UseGroundingAction(string sActionName, CGroundingAction* pgaAGroundingAction)
//      GROUNDING_ACTION(NO_ACTION, NO_ACTION, Configuration)
//		#define NO_ACTION CGANoAction
#define GROUNDING_ACTION(Name, GAType, Configuration)\
    pGroundingManager->UseGroundingAction(#Name, new GAType((string)Configuration));

// D: declare the binding filters to be used
#define USE_BINDING_FILTERS(BindingFilters)\
    {\
        BindingFilters\
    }\

// D: declare customized binding filters to be user
#define BINDING_FILTER(Name, Filter)\
    pDMCore->RegisterBindingFilter(Name, Filter);\
    
// D: declare a customized start over routine
#define CUSTOM_START_OVER(Funct)\
    pDMCore->RegisterCustomStartOver(Funct);\

// D: macro for logging from the dialog task log
#define LOG(X)\
    Log(DIALOGTASK_STREAM, X);\

//-----------------------------------------------------------------------------
// 
// Other macros to increase readability of dialog task specification
//
//-----------------------------------------------------------------------------

// D: shortcuts for definiting items in structures
#define INT_ITEM(Name) \
    ITEM(Name, CIntConcept)
#define BOOL_ITEM(Name) \
    ITEM(Name, CBoolConcept)
#define STRING_ITEM(Name) \
    ITEM(Name, CStringConcept)
#define FLOAT_ITEM(Name) \
    ITEM(Name, CFloatConcept)
#define CUSTOM_ITEM(Name, Type) \
    ITEM(Name, Type)

// D: shortcuts for defining concepts
// D�����ڶ������Ŀ�ݷ�ʽ
#define INT_SYSTEM_CONCEPT(Name) SYSTEM_CONCEPT(Name, CIntConcept)
#define BOOL_SYSTEM_CONCEPT(Name) SYSTEM_CONCEPT(Name, CBoolConcept)
#define STRING_SYSTEM_CONCEPT(Name) SYSTEM_CONCEPT(Name, CStringConcept)
#define FLOAT_SYSTEM_CONCEPT(Name) SYSTEM_CONCEPT(Name, CFloatConcept)
#define CUSTOM_SYSTEM_CONCEPT(Name, Type) SYSTEM_CONCEPT(Name, Type)

// D: shortcuts for various agent definitions
#define RESET_ON_COMPLETION ON_COMPLETION(RESET)
#define REOPEN_ON_COMPLETION ON_COMPLETION(REOPEN)

#define INT_USER_CONCEPT(Name, GroundingModelSpec) \
    USER_CONCEPT(Name, CIntConcept, GroundingModelSpec)
#define BOOL_USER_CONCEPT(Name, GroundingModelSpec) \
    USER_CONCEPT(Name, CBoolConcept, GroundingModelSpec)
#define STRING_USER_CONCEPT(Name, GroundingModelSpec) \
    USER_CONCEPT(Name, CStringConcept, GroundingModelSpec)
#define FLOAT_USER_CONCEPT(Name, GroundingModelSpec) \
    USER_CONCEPT(Name, CFloatConcept, GroundingModelSpec)
#define CUSTOM_USER_CONCEPT(Name, Type, GroundingModelSpec) \
    USER_CONCEPT(Name, Type, GroundingModelSpec)

// D: shortcuts for various agent check functions
// D�����ִ����鹦�ܵĿ�ݷ�ʽ
#define COMPLETED(Agent) (A(#Agent).HasCompleted())
#pragma warning (disable:4005)
#define FAILED(Agent) (A(#Agent).HasFailed())
#define SUCCEEDED(Agent) (A(#Agent).HasSucceeded())
#pragma warning (default:4005)
#define IS_ACTIVE_TOPIC(Agent) (pDMCore->AgentIsActive(&(A(#Agent))))
#define IS_FOCUSED(Agent) (pDMCore->IsFocused(&(A(#Agent))))

// D: shortcuts for commands on agents
#define SET_COMPLETED(Agent) {A(#Agent).SetCompleted();}
#define FINISH(Agent) {A(#Agent).SetCompleted(); \
    pDMCore->PopAgentFromExecutionStack(&A(#Agent));}
#define RESET_AGENT(Agent) {A(#Agent).Reset();}
#define REOPEN_AGENT(Agent) {A(#Agent).ReOpen();}
#define REOPEN_TOPIC(Agent) {A(#Agent).ReOpenTopic();}

// D: shortcuts for various concept check functions
// D�����ڸ��ָ����麯���Ŀ�ݷ�ʽ
#define AVAILABLE(Concept) (C(#Concept).IsAvailableAndGrounded())
#define UNAVAILABLE(Concept) (!(C(#Concept).IsAvailableAndGrounded()))
#define UPDATED(Concept) (C(#Concept).IsUpdatedAndGrounded())
#define NOT_UPDATED(Concept) (!(C(#Concept).IsUpdatedAndGrounded()))
#define INVALIDATED(Concept) (C(#Concept).IsInvalidated())
#define IS_TRUE(Concept) (AVAILABLE(Concept) && (bool)C(#Concept))
#define IS_FALSE(Concept) (AVAILABLE(Concept) && !(bool)C(#Concept))
#define HAS_HISTORY(Concept) (C(#Concept).GetHistorySize() > 0)
#define TURNS_SINCE_LAST_UPDATED(Concept) (C(#Concept).GetTurnsSinceLastUpdated())
#define UPDATED_IN_LAST_TURN(Concept) ((C(#Concept).GetTurnsSinceLastUpdated())==0)
#define UPDATED_IN_PENULTIMATE_TURN(Concept) ((C(#Concept).GetTurnsSinceLastUpdated())==1)

// D: shortcuts for methods on concepts
// D�������ϵķ����Ŀ�ݷ�ʽ
#define REOPEN_CONCEPT(Concept) {C(#Concept).ReOpen();}
#define RESTORE_CONCEPT(Concept) {C(#Concept).Restore();}
#define CLEAR(Concept) {C(#Concept).Clear();}
#define SIZE(Concept) (C(#Concept).GetSize())

#endif // __DIALOGTASK_H__
