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
// OUTPUT.H - COutput����Ķ��塣 �����ʾ�Ի������������������������������Ĵ���ʹ�á� ���ǲ�νṹ�Ļ�����
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
//		COutput����Ķ��塣 �����ʾ�Ի������������������������������Ĵ���ʹ�á� ���ǲ�νṹ�Ļ�����
//-----------------------------------------------------------------------------

//###########	prompt => [FLAGS] ACT OBJECT [PARAMETERS]
class COutput
{

	//��Ԫ��
	friend class COutputHistory;		// output history has friend access
	friend class COutputManagerAgent;	// output manager has friend access 

protected:
	// class members 
	//
	string sGeneratorAgentName;			// name of the agent that ordered this			�����˴���Ĵ��������
	// output
	int iOutputId;						// the output id								���id
	int iExecutionIndex;				// the index of the execution item				ִ��������� [��������output���]
	// corresponding to the generation 
	// of this output

	string sDialogState;				// a string representing the dialog state		��ʾ��������ʾʱ�ĶԻ�״̬���ַ���
	
	//at which this prompt was issued 
	//###################################################################################################
	string sAct;						// the act (dialog move)						����
	string sObject;					    // the object (acted on or with)				����
	//###################################################################################################

	vector<CConcept *> vcpConcepts;		// the list of concepts referred in this output	�ڴ���������õĸ����б�
	vector<bool> vbNotifyConcept;       // parallel vector indicating whether the concept conveyance should be 
	//  notified or not																	ָʾconcept�����Ƿ�Ӧ�ñ�֪ͨ�� <ƽ������>

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
	vector<string> vsFlags;				// flags for the output						�����־
	string sOutputDeviceName;			// the name of the device this output		�����Ӧ���򵽵��豸������
	//  should be directed to
	TConveyance cConveyance;			// whether the output was fully conveyed	����Ƿ���ȫ����������� ö�٣�[��� û���ʧ��]
	// to the recipient
	int iRepeatCounter;					// the number of times this output has		������ѱ������Ĵ�����������
	//  been uttered (consecutively)
	TFloorStatus fsFinalFloorStatus;	// the floor status at the end of this output	���������Ľ���ʱ���floor״̬ ö�٣���λ�á�ϵͳ���û������ɡ�



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
	// ��Ҫ���Ƿ��ķ���
	//---------------------------------------------------------------------

	// Pure virtual method which creates a certain output from a given
	// string-represented prompt
	//���鷽���Ӹ������ַ�����ʾ����һ���ض������
	virtual bool Create(string sAGeneratorAgentName, int iAExecutionIndex, string sAPrompt, TFloorStatus fsAFloor, int iAOutputId) = 0;

	// Pure virtual method which generates a string representation for the
	// output that will be sent to the external output component (i.e. 
	// NLG in the Galaxy framework, Gemini in the OAA framework)
	//���鷽�����ɱ�ʾ������ַ���, �����͵��ⲿ��������������Galaxy����е�NLG����OAA����е�Gemini��
	virtual string ToString() = 0;

	// Pure virtual method for cloning an output
	// ���麯�� - ��¡һ���������
	virtual COutput* Clone(int iNewOutputId) = 0;

	//---------------------------------------------------------------------
	// Public methods 
	// public����
	//---------------------------------------------------------------------

	// Get name of the agent that generated this output
	// ��ȡ���ɴ�����Ĵ��������
	string GetGeneratorAgentName();

	// Set and Get dialog state information for this output
	// ���úͻ�ȡ������ĶԻ�״̬��Ϣ
	int GetDialogStateIndex();
	void SetDialogStateIndex(int iAExecutionIndex);
	string GetDialogState();
	void SetDialogState(string sADialogState);

	// Set and Get conveyance information on the output
	// ���úͻ�ȡ�����������Ϣ
	void SetConveyance(TConveyance cAConveyance);
	TConveyance GetConveyance();

	// Set and Get the act for this output
	//���ò���ȡ������Ķ���
	void SetAct(string sAAct);
	string GetAct();

	// Set and Get the final floor status for this output
	// ���ò���ȡ����������� floor ״̬
	void SetFinalFloorStatus(TFloorStatus fsAFloor);
	TFloorStatus GetFinalFloorStatus();
	string GetFinalFloorStatusLabel();

	// Checks if a certain flag is set
	// ���flag�Ƿ�����
	bool CheckFlag(string sFlag);

	// Notifies a concept of conveyance information
	// ֪ͨ��ͨ��Ϣ�ĸ���
	void NotifyConceptConveyance(string sConceptName, TConveyance cAConveyance);

	// Returns a pointer to the concept, given the concept name
	// ����һ��ָ������ָ�룬������������
	virtual CConcept* GetConceptByName(string sConceptName);

	// Cancels the notification request for one of the concepts
	// ȡ������һ�������֪ͨ����
	void CancelConceptNotificationRequest(CConcept* pConcept);

	// Changes the pointers for one of the concepts (this happens on 
	// reopens and other operations which change the concept pointers)
	// ��������һ�������ָ�루�ⷢ�������´򿪺��������ĸ���ָ��Ĳ�����
	void ChangeConceptNotificationPointer(CConcept* pOldConcept, CConcept* pNewConcept);

	// Gets the number of times this prompt has been uttered
	// ��ȡ����ʾ�ѷ����Ĵ���
	int GetRepeatCounter();
	// Increments the repeat counter
	// �����ظ���������
	void IncrementRepeatCounter();

protected:
	//---------------------------------------------------------------------
	// Protected methods 
	//---------------------------------------------------------------------

	// base clone helper function to be called by Clone() in derived classes
	// ������¡�����������������е�Clone��������
	virtual void clone(COutput* pOutput, int iNewOutputId);
};

#endif // __OUTPUT_H__
