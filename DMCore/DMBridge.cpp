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
// DMBRIDGE.CPP - implements the functions which bridge between the dialog
//                management core and the DM interface
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

#include "../DMInterfaces/DMInterface.h"
#include "../DMCore/Core.h"

// D: structure holding the RavenClaw initialization parameters
TRavenClawConfigParams rcpRavenClawInitParams;

// D: structure saving temporarily the session initialization parameters
TRavenClawConfigParams rcpSessionInitParams;

//-----------------------------------------------------------------------------
// Handles for the Core and DMInterface threads
// Core��DMInterface�̵߳ľ��
//-----------------------------------------------------------------------------
HANDLE g_hDMCoreThread = NULL;
DWORD g_idDMInterfaceThread = 0;

//-----------------------------------------------------------------------------
// Constructors for the RavenClaw configuration parameters
//-----------------------------------------------------------------------------
// D: default constructor for RavenClaw configuration parameters
//#RavenClaw������Ĭ�Ϲ��캯��
TRavenClawConfigParams::TRavenClawConfigParams()
{
	// initialize the Dialog Manager Interface verbosity to 2
	//#��ʼ��Dialog Manage Interface����ϸ�̶�Ϊ2
	Set(RCP_DMI_VERBOSITY, "2");
	// initialize all the logging streams to be visible and logged
	//#��ʼ�����пɼ��ͼ�¼����־��¼��
	Set(RCP_LOGGED_STREAMS, sAllLoggingStreams); //��¼��
	Set(RCP_DISPLAYED_STREAMS, sAllLoggingStreams); //�ɼ���
	// initialize so that we exit on a fatal error
	//#��ʼ����ʹ���Ǵ���һ�����������Ƿ�ֱ���˳�
	Set(RCP_EXIT_ON_FATAL_ERROR, "true");

	/*
		expl_impl = expl_impl.pol
		expl = expl.pol
		request_default = request_default.pol
		request_lr = request_lr.pol
	*/
	// initialize the grounding policies file
	//#��ʼ���ӵز����ļ�
	Set(RCP_GROUNDING_POLICIES_FILE, ".\\grounding.policies"); //Ŀǰȱʧ��
	// initialize the dialog states file
	//	#��ʼ���Ի�״̬�ļ�
	Set(RCP_DIALOG_STATES_FILE, "");
	// initialize the grounding configuration
	//#��ʼ���ӵ�����
	Set(RCP_GROUNDING_MANAGER_CONFIGURATION, "full_grounding");// ??
	// initialize the default timeout
	//#��ʼ��Ĭ�ϳ�ʱʱ��
	Set(RCP_DEFAULT_TIMEOUT, "8");
	// initialize the default nonunderstanding threshold
	//#��ʼ��Ĭ�Ϸ������ֵ
	Set(RCP_DEFAULT_NONUNDERSTANDING_THRESHOLD, "0");
}

// D: copy constructor for RavenClaw configuration parameters
//�������캯����ʼ��
TRavenClawConfigParams::TRavenClawConfigParams(TRavenClawConfigParams& rARCP)
{
	// copies the hash of parameters
	STRING2STRING::iterator iPtr;
	for (iPtr = rARCP.s2sParams.begin(); iPtr != rARCP.s2sParams.end(); iPtr++)
	{
		s2sParams.insert(STRING2STRING::value_type(iPtr->first, iPtr->second));
	}
}

// D: assignment operator for RavenClaw configuration parameters
//��������� ��=�� ��ʼ��
TRavenClawConfigParams& TRavenClawConfigParams::operator =(
	TRavenClawConfigParams& rARCP)
{
	if (&rARCP != this)
	{
		// cleans the hash
		s2sParams.clear();
		// and copies the parameters from the other one
		STRING2STRING::iterator iPtr;
		for (iPtr = rARCP.s2sParams.begin(); iPtr != rARCP.s2sParams.end(); iPtr++)
		{
			s2sParams.insert(STRING2STRING::value_type(iPtr->first, iPtr->second));
		}
	}
	return *this;
}

// D: reads the RavenClaw configurations parameters from a file, specified by
//    a handler
//�������ļ���ȡ��ʼ�� [key=value����ʽ]
void TRavenClawConfigParams::ReadFromFile(FILE* fid)
{
	// read the file line by line
	char lpszLine[STRING_MAX];
	while (fgets(lpszLine, STRING_MAX, fid) != NULL)
	{
		string sAttribute;
		string sValue;
		// check for comments 
		//�����ע�ͣ������
		if ((lpszLine[0] == '#') || ((lpszLine[0] == '/') && lpszLine[1] == '/'))
			continue;

		// check for Attribute = Value pair
		//���� ����=ֵ 
		if (SplitOnFirst((string)lpszLine, "=", sAttribute, sValue))
		{
			// now check the attribute
			sAttribute = ToLowerCase(Trim(sAttribute));
			sValue = Trim(sValue);
			Set(sAttribute, sValue);
		}
	}
}

// D: sets the value for a ravenclaw configuration parameter
//����ֵ
void TRavenClawConfigParams::Set(string sParam, string sValue)
{
	s2sParams[sParam] = sValue;
}

// D: obtains the value of a ravenclaw configuration parameter
//��ȡֵ
string TRavenClawConfigParams::Get(string sParam)
{
	STRING2STRING::iterator iPtr;
	if ((iPtr = s2sParams.find(sParam)) != s2sParams.end())
	{
		// if found, return
		return iPtr->second;
	}
	else
	{
		// o/w return empty
		return "";
	}
}

// D: converts the list of parameters to a string
// #��hash�еĲ���ת����String�ַ������͸�ʽ
string TRavenClawConfigParams::ToString()
{
	return S2SHashToString(s2sParams, "\n") + "\n";
}

//-----------------------------------------------------------------------------
// Initialization functions
//-----------------------------------------------------------------------------
// D: initialize the whole RavenClaw system. This function will get called
//    only once, when RavenClaw is started up
void InitializeSystem(string sConfigurationFileName)
{

	DMI_DisplayMessage("Initializing Ravenclaw ...", 0);

	// Obtain the thread id for the DMInterface thread
	g_idDMInterfaceThread = GetCurrentThreadId();

	// Read the configuration file
	if (sConfigurationFileName == "")
	{
		// if configuration file not specified 
		DMI_DisplayMessage("No configuration file specified. Using defaults.",
			0);
	}
	else
	{
		// o/w process the configuration file
		DMI_DisplayMessage("Processing configuration file ...", 0);
		FILE* fid = fopen(sConfigurationFileName.c_str(), "r");
		if (fid)
		{
			// read the configuration from the file
			rcpRavenClawInitParams.ReadFromFile(fid);
			// finally close the file
			fclose(fid);
		}
		else
		{
			// if configuration file not specified or cannot be opened
			DMI_DisplayMessage(
				FormatString("Could not read configuration file \"%s\". Using "\
				"defaults.", sConfigurationFileName.c_str()).c_str(), 0);
		}
	}

	// apply global RavenClaw initialization parameters (for now only deal with 
	// DMI verbosity)
	DMI_SetVerbosity(atoi(rcpRavenClawInitParams.Get(RCP_DMI_VERBOSITY).c_str()));

	DMI_DisplayMessage("Ravenclaw initialization completed.", 0);
}

// D: initialize a new dialog session. This function gets called each time
//    a new dialog session starts (in the Galaxy configuration, this typically
//    happens on the arrival of a begin_session message
void InitializeDialogSession(TRavenClawConfigParams rcpSessionParams)
{

	// first, wait for the DialogCore thread to finish, just in case it's
	// not finished yet for whatever reasons
	if (g_hDMCoreThread != NULL)
	{
		WaitForSingleObject(g_hDMCoreThread, INFINITE);
		g_hDMCoreThread = NULL;
	}

	// create a new DialogCore thread, which will run the core
	DWORD ThreadId;
	// save the session initialization parameters in a global variable
	rcpSessionInitParams = rcpSessionParams;
	// and create the thread, passing a pointer to that variable
	g_hDMCoreThread =
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DialogCoreThread,
		&rcpSessionInitParams, 0, &ThreadId);
	// check that it was created successfully
	assert(g_hDMCoreThread);
}

//-----------------------------------------------------------------------------
// Dialog flow function. This function synchronizes the DialogCore with the 
// DMInterface threads and ensures a smooth dialog flow
//-----------------------------------------------------------------------------
#pragma warning (disable:4127)
void DoDialogFlow()
{

	// variable to hold incoming messages from the core thread
	MSG Message;

	// set the incoming input and do the processing but only if the core has 
	// been initialized
	if (g_hDMCoreThread == NULL) return;

	// acquire incoming event
	if (pInteractionEventManager != NULL)
		pInteractionEventManager->SignalInteractionEventArrived();

	while (true)
	{
		// get a message from the dialog core thread. in lParam we will have
		// the caller thread id
		GetMessage(&Message, NULL, WM_DLGBASE, WM_DLGMAX);
		// check that it's not by mistake from somewhere else
		assert(Message.hwnd == NULL);

		// deal with the message coming from the dialog core thread
		switch (Message.message)
		{

			// forwarding galaxy calls to the GalaxyInterface (happens only if 
			// RavenClaw is compiled in a Galaxy configuration)
#ifdef GALAXY	
		case WM_GALAXYCALL: {
								// if the message contains a call to a Galaxy module
								TGIGalaxyCall *pGalaxyCall = (TGIGalaxyCall *)Message.wParam;

								// do the actual call
								bool bResult = CallGalaxyModuleFunction(pGalaxyCall);

								// now check on the result of the call
								if(bResult) {
									// the call was successful, send a message back to the 
									// caller
									PostThreadMessage((DWORD)Message.lParam, WM_ACTIONFINISHED, 
										ACTION_SUCCESS, 0);
								} else {
									// the call was insuccessful; send a message back to the 
									// caller
									PostThreadMessage((DWORD)Message.lParam, WM_ACTIONFINISHED,
										ACTION_FAILED, 0);
								}			
								break;
		}
		case WM_GALAXYACTIONCALL: {
									  // if the message contains a call to a Galaxy module
									  TGIGalaxyActionCall *pGalaxyActionCall = (TGIGalaxyActionCall *)Message.wParam;

									  // do the actual call
									  bool bResult = SendActionThroughHub(pGalaxyActionCall);

									  // now check on the result of the call
									  if(bResult) {
										  // the call was successful, send a message back to the 
										  // caller
										  PostThreadMessage((DWORD)Message.lParam, WM_ACTIONFINISHED, 
											  ACTION_SUCCESS, 0);
									  } else {
										  // the call was insuccessful; send a message back to the 
										  // caller
										  PostThreadMessage((DWORD)Message.lParam, WM_ACTIONFINISHED,
											  ACTION_FAILED, 0);
									  }			
									  break;
		}
#endif
			// forwarding OAA calls to the OAAInterface (happens only if 
			// RavenClaw is compiled in an OAA configuration)
#ifdef OAA
		case WM_OAACALL: {
							 // if the message contains a call to an OAA agent
							 TOIOAACall *pOIOAACall = (TOIOAACall *)Message.wParam;

							 // do the actual call
							 bool bResult = CallOAA(pOIOAACall);

							 // now check on the result of the call
							 if (bResult)
							 {
								 // the call was successful, send a message back to the 
								 // caller
								 PostThreadMessage((DWORD)Message.lParam, WM_ACTIONFINISHED,
									 ACTION_SUCCESS, 0);
							 }
							 else
							 {
								 // the call was insuccessful; send a message back to the 
								 // caller
								 PostThreadMessage((DWORD)Message.lParam, WM_ACTIONFINISHED,
									 ACTION_FAILED, 0);
							 }
							 break;
		}
#endif // OAA

			// handling need for inputs
		case WM_WAITINPUT:
			// if we need an input, break the dialog flow
			return;

			// handling need for inputs
		case WM_WAITINTERACTIONEVENT:
			// if we need an input, break the dialog flow
			return;

			// handling dialog finished
		case WM_DIALOGFINISHED:
			// if the dialog is finished, break the dialog flow
			DMI_SetInSessionFlag(false);
			return;

		default:
			Error("Core/Galaxy Bridge received an unknown message.");
			break;
		}
	}
}

#pragma warning (default:4127)