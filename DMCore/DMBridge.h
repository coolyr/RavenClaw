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
// DMBRIDGE.CPP - functions which bridge between the dialog
//                management core and the Galaxy interface
// 
// Galaxy :  a centralized message-passing infrastructure. Each component is
//	implemented as a separate process that connects to a centralized traffic router – the Galaxy hub.The messages
//	are sent through the hub, which forwards them to the appropriate destination.The routing logic is described
// by a configuration script.
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
//   [2002-12-03] (dbohus): fixed code so that bInSession is reset once dialog
//                           terminates
//   [2002-10-20] (dbohus): added core initialization parameters structure
//	 [2002-06-07] (dbohus): added GALAXY/OAA conditionals for compilation
//                           of GALAXY/OAA-specific parts of the bridge
//   [2002-05-25] (dbohus): deemed preliminary stable version 0.5
//   [2002-01-23] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __DMBRIDGE_H__
#define __DMBRIDGE_H__

#include <windows.h>
#include <winuser.h>
#include "../Utils/Utils.h"

//-----------------------------------------------------------------------------
// IDs for messages sent from the DialogCore thread to the DMInterface thread
//-----------------------------------------------------------------------------

#define WM_DLGBASE			(WM_USER+100)		// the base id for the messages
#define WM_DLGMAX			(WM_DLGBASE+5)		// the max id for the messages

#define WM_WAITINPUT		(WM_DLGBASE)		// wait for input
#define WM_GALAXYCALL		(WM_DLGBASE+1)		// do a call to a galaxy module
#define WM_OAACALL			(WM_DLGBASE+1)		// do a call to an OAA module
#define WM_DIALOGFINISHED	(WM_DLGBASE+2)		// finish the dialog
#define WM_ACTIONFINISHED	(WM_DLGBASE+3)		// notify that action was 
//  completed
#define WM_WAITINTERACTIONEVENT	(WM_DLGBASE+4)	// wait for an interaction event 等待一个交互event
#define WM_GALAXYACTIONCALL		(WM_DLGBASE+5)	// do a call to a galaxy module

#define ACTION_SUCCESS 1						// the action was completed 
//	successfully
#define ACTION_FAILED 0							// the action terminated with a 
//  failure

//-----------------------------------------------------------------------------
// Handle declaration for the DMCore and DMInterface threads
//-----------------------------------------------------------------------------
extern HANDLE g_hDMCoreThread;
extern DWORD g_idDMInterfaceThread;

//-----------------------------------------------------------------------------
// D: Data structure holding the RavenClaw framework configuration parameters
//-----------------------------------------------------------------------------

// D: first, a number of defines for the names of the parameters
// 首先，为参数的名称定义了一些宏
#define RCP_DMI_VERBOSITY "dmi_verbosity"
#define RCP_LOGGED_STREAMS "logged_streams"
#define RCP_DISPLAYED_STREAMS "displayed_streams"
#define RCP_EXIT_ON_FATAL_ERROR "exit_on_fatal_error"
#define RCP_GROUNDING_POLICIES "grounding_policies"
#define RCP_GROUNDING_POLICIES_FILE "grounding_policies_file"
#define RCP_DIALOG_STATES_FILE "dialog_states_file"
#define RCP_GROUNDING_MANAGER_CONFIGURATION "grounding_manager_configuration"
#define RCP_LOG_DIR "log_dir"
#define RCP_LOG_PREFIX ""
#define RCP_DEFAULT_TIMEOUT "default_timeout"
#define RCP_DEFAULT_NONUNDERSTANDING_THRESHOLD "default_nonunderstanding_threshold"
#define RCP_SERVER_NAME "server_name"
#define RCP_SERVER_PORT "server_port"

//配置参数
struct TRavenClawConfigParams
{

	// hash containing the parameters
	// #typedef map <string, string> STRING2STRING;
	// #Hash参数   map <string, string>
	STRING2STRING s2sParams;

public:
	// constructor for the RavenClaw initialization parameters (sets defaults)
	// #默认构造函数
	TRavenClawConfigParams();

	// copy constructor
	//#拷贝构造函数
	TRavenClawConfigParams(TRavenClawConfigParams& rARCP);

	// assignment operator
	//重载运算符 =
	TRavenClawConfigParams& operator = (TRavenClawConfigParams& rARCP);

	// reads the prameters from a configuration file
	//#从文件中读取配置
	void ReadFromFile(FILE* fid);

	// sets and gets parameters
	//#设置和获取配置参数
	void Set(string sParam, string sValue);
	string Get(string sParam);

	// converts the list of parameters to a string 
	//把参数输出
	string ToString();
};

//-----------------------------------------------------------------------------
// D: the RavenClaw initialization parameters; these parameters are read from
//    the ravenclaw configuration file, and are the starting subset of 
//    parameters valid across all sessions in an instantiation of the dialog
//    manager
//-----------------------------------------------------------------------------
extern TRavenClawConfigParams rcpRavenClawInitParams;

//-----------------------------------------------------------------------------
// Initialization functions
//-----------------------------------------------------------------------------
// D: initialize the whole dialog system. This function will get called
//    only once, when the dialog manager is started up
void InitializeSystem(string sConfigurationFileName);

// D: initialize a new dialog session. This function gets called each time
//    a new session starts
void InitializeDialogSession(TRavenClawConfigParams rcpSessionParams);

//-----------------------------------------------------------------------------
// Dialog flow function. This function synchronizes the DialogCore with the 
// DMInterface threads and ensures the dialog flow
//-----------------------------------------------------------------------------
#define TIMEOUT_ELAPSED "[turn_timeout:timeout]"
#define VOID_INPUT "VoidInput"
#define START_SESSION "StartSession"
#define TERMINATE_SESSION "TerminateSession"
#define NON_UNDERSTANDING "NonUnderstanding"
void DoDialogFlow();

#endif // __DM_BRIDGE_H__