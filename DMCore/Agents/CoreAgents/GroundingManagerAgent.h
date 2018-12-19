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
// GROUNDINGMANAGERAGENT.H   - defines the grounding management agent. This 
//                             agent encompasses a grounding model, and uses a 
//                             set of plugable strategies/actions to accomplish 
//                             grounding in dialog
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
//   [2006-01-31] (dbohus): added support for dynamically registering grounding
//                          model types
//   [2004-12-23] (antoine): modified constructor, agent factory to handle
//							  configurations
//   [2003-02-08] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __GROUNDINGMANAGERAGENT_H__
#define __GROUNDINGMANAGERAGENT_H__

#include "../../../Utils/Utils.h"
#include "../../../DMCore/Agents/Agent.h"
#include "../../../DMCore/Agents/CoreAgents/DMCoreAgent.h"
#include "../../../DMCore/Grounding/Grounding.h"

// D: auxiliary define for a map holding the list of external policies
// �ⲿ����
typedef map<string, CExternalPolicyInterface*> TExternalPolicies;

// D: auxiliary definition of the grounding manager configuration
// ��������ӵع���������
typedef struct
{
	string sGroundingManagerConfiguration;	//���ò���string
	bool bGroundConcepts;					//�Ƿ�����concept Grouding
	bool bGroundTurns;						//�Ƿ�����Turn Grouding
	string sConceptGM;						//Ĭ�ϵ�concept�ӵ�ģ��
	string sTurnGM;							//Ĭ�ϵ�Turn�ӵ�ģ��
	string sBeliefUpdatingModelName;		//�������ģ�� ["npu", "calista" ]
} TGroundingManagerConfiguration;

// D: type describing a concept grounding request
// D����������������������
#define GRS_UNPROCESSED 0		// the unprocessed status for a grounding request	δ����̬
#define GRS_PENDING     1		// the pending status for a grounding request		δ�������۶�
#define GRS_READY       2       // the ready status for a grounding request			׼��̬
#define GRS_SCHEDULED   3		// the scheduled status for a grounding request		����̬
#define GRS_EXECUTING   5		// the executing status for a grounding request		ִ��̬
#define GRS_DONE        6   	// the completed status for a grounding request		���̬

typedef struct
{
	CConcept* pConcept;				//����
	int iGroundingRequestStatus;	//�ӵ�����״̬
	int iSuggestedActionIndex;		//���鶯������
	int iTurnNumber;				//turn id
} TConceptGroundingRequest;

// D: type for the stack of concept grounding requests
// ���Ͷ��� - �ӵ�����Ķ�ջ
typedef vector <TConceptGroundingRequest, allocator<TConceptGroundingRequest> >
TConceptGroundingRequests;

// D: type for the belief updating model
// ���͵����θ���ģ��
typedef map <string, vector<float> > STRING2FLOATVECTOR;
typedef map <string, STRING2FLOATVECTOR> STRING2STRING2FLOATVECTOR;

// D: the type definition for a grounding action that was run 
// D�����еĻ������������Ͷ���
#define GAT_TURN 0
#define GAT_CONCEPT 1
#define GAT_NONE 2
typedef struct
{
	string sGroundingModelName;         // the name of the grounding model that
	//  took the action
	string sActionName;                 // the name of the grounding action
	int iGroundingActionType;           // the grounding action type ��GAT_TURN��GAT_CONCEPT�� GAT_NONE��
	bool bBargeIn;                      // was there a barge-in on the action
} TGroundingActionHistoryItem;
// D: the type defition for a series of actions
typedef vector <TGroundingActionHistoryItem> TGroundingActionHistoryItems;

// D: definition of function type for creating a grounding model
// D������ Create Grouding Model �ĺ������͵Ķ���
typedef CGroundingModel* (*FCreateGroundingModel)(string);

// D: type definition for the hash containing the grounding models types and factory functions
// �ӵ�ģ������  - hash - �������� 
typedef map <string, FCreateGroundingModel> TGroundingModelsTypeHash;

//-----------------------------------------------------------------------------
//
// D: CGroundingManagerAgent class -
//      implements the grounding model which chooses the right grounding 
//      action based on the current grounding state
// DC �ӵع����������� - ʵ�ֻ��ڵ�ǰ�ӵ�״̬ѡ����ȷ�ӵض����Ľӵ�ģ��
//-----------------------------------------------------------------------------

class CGroundingManagerAgent : public CAgent
{

private:
	//---------------------------------------------------------------------
	// private grounding manager agent specific members
	// �ӵع���������private�ض���Ա
	//---------------------------------------------------------------------

	//#########################################################################
	/*
	concept Grouding:
					expl_impl = expl_impl.pol �ļ�����
					expl = expl.pol	�ļ�����
	Turn Grouding:
					request_default = request_default.pol �ļ�����
					request_lr = request_lr.pol �ļ�����
	*/
	// hash holding the grounding models policies 
	// (key = model_name, value= model policy string)
	// ����policy Hash
	STRING2STRING s2sPolicies;
	//#########################################################################

	// hash holding various constant parameters for feature computation
	// hash������������ĸ��ֳ�������
	STRING2FLOAT s2fConstantParameters;

	// hash holding the belief updating models
	// Hash�������ģ��
	STRING2STRING2FLOATVECTOR s2s2vfBeliefUpdatingModels;

	// hash holding information about the various concepts
	// hash���й��ڸ��� concept ����Ϣ
	STRING2STRING2FLOATVECTOR s2s2vfConceptValuesInfo;

	// hash holding information about the concept type
	// hash���й��ڸ��� concept �� type
	STRING2STRING s2sConceptTypeInfo;

	// hash holding the precomputed belief updating features
	// ��ϣ����Ԥ�ȼ���������������
	STRING2FLOAT s2fBeliefUpdatingFeatures;

	//############################CGroundingAction#########################################
	//      GROUNDING_ACTION(NO_ACTION, NO_ACTION, Configuration)
	//		#define NO_ACTION CGANoAction	=>   ��ʵ����������

	// array holding the grounding actions available 
	// ���ֿ��õĽӵز�����Vector
	vector <CGroundingAction *> vpgaActions;

	// parallel array holding the names of the grounding actions
	// �������飬����ӵض��������� [CGANoAction,     CGAAskRepeat,	EXPL_CONF,    IMPL_CONF, ...]
	TStringVector vsActionNames;
	//###########################CGroundingAction##########################################


	// hash with pointers to the externally implemented policies
	// ��ϣ��ָ���ⲿʵ�ֵĲ��Ե�ָ��
	TExternalPolicies mapExternalPolicies;

	// the grounding manager configuration
	// �ӵع��������� - �ṹ��
	TGroundingManagerConfiguration gmcConfig;

	//############################GroundingRequests##################################
	// flag which indicates if we need to ground turns
	// ��־��ָʾ�����Ƿ���Ҫturns Grouding 
	bool bTurnGroundingRequest;

	// vector implementing the concept grounding stack
	// ʸ��ʵ��concept�ӵض�ջ
	TConceptGroundingRequests vcgrConceptGroundingRequests;

	// flag indicating if the stack is locked
	// ������־ - vcgrConceptGroundingRequests
	bool bLockedGroundingRequests;
	//##########################GroundingRequests####################################

	// the history of grounding actions
	// �ӵز�������ʷ
	vector<TGroundingActionHistoryItems> vgahiGroundingActionsHistory;

	/*
	concept_default			= >		 CGMConcept::GroundingModelFactory

	request_default			= >		 CGMRequestAgent::GroundingModelFactory
	request_lr				= >		 CGMRequestAgent_LR::GroundingModelFactory
	request_handcrafted		= >		 CGMRequestAgent_HandCrafted::GroundingModelFactory
	request_numnonu			= >		 CGMRequestAgent_NumNonu::GroundingModelFactory
	request_experiment		= >		 CGMRequestAgent_Experiment::GroundingModelFactory
	*/
	// the hash containing the grounding model factories
	// hash�����ӵ�ģ�͹���
	TGroundingModelsTypeHash gmthGroundingModelTypeRegistry;

public:

	//---------------------------------------------------------------------
	// Constructor and destructor
	//---------------------------------------------------------------------
	//
	// Default constructor
	CGroundingManagerAgent(string sAName, string sAConfiguration = "", string sAType = "CAgent:CGroundingManagerAgent");

	// Virtual destructor
	virtual ~CGroundingManagerAgent();

	// Static function for dynamic agent creation
	static CAgent* AgentFactory(string sAName, string sAConfiguration);

public:

	//---------------------------------------------------------------------
	//
	// GroundingManagerAgent class specific public methods
	// ���е�spublic����
	//---------------------------------------------------------------------

	//---------------------------------------------------------------------
	// Access to the configuration information
	// ����������Ϣ�ķ���
	//---------------------------------------------------------------------

	// Set the configuration string
	// ����������Ϣ
	void SetConfiguration(string sAGroundingManagerConfiguration);

	// Returns the configuration structure
	// ����������Ϣ - �ṹ��
	TGroundingManagerConfiguration GetConfiguration();

	//---------------------------------------------------------------------
	// Operations with the model data
	// ����ģ�����ݵķ���
	//---------------------------------------------------------------------

	// Load the policies
	// ���ز���
	virtual void LoadPoliciesFromString(string sPolicies);
	virtual void LoadPoliciesFromFile(string sFileName);

	// Return the policy for a certain grounding model
	// ��ĳ���ӵ�ģ�͵Ĳ���
	virtual string GetPolicy(string sModelName);

	// Create an external policy interface
	// �����ⲿ���Խ��
	CExternalPolicyInterface* CreateExternalPolicyInterface(string sAHost);

	// Release all external policy interaces
	// �ͷ������ⲿ����
	void ReleaseExternalPolicyInterfaces();

	//---------------------------------------------------------------------
	// Methods for the belief updating model
	// �������ģ�͵ķ���
	//---------------------------------------------------------------------

	// Load the belief updating model
	// �����������ģ��
	virtual void LoadBeliefUpdatingModel(string sAFileName);

	// Set the belief updating model
	// �����������ģ��
	virtual void SetBeliefUpdatingModelName(string sABeliefUpdatingModelName);

	// Returns the name of the belief updating model
	// �����������ģ����
	virtual string GetBeliefUpdatingModelName();

	// Returns an actual belief updating model, corresponding to a certain system action
	// ���ض�Ӧ��ĳ��ϵͳ������ʵ�����θ���ģ��
	virtual STRING2FLOATVECTOR& GetBeliefUpdatingModelForAction(string sSystemAction);

	// Returns the value of a constant parameter
	// ���س���������ֵ
	virtual float GetConstantParameter(string sParam);

	// Precompute belief updating features
	// Ԥ���������������
	virtual void PrecomputeBeliefUpdatingFeatures(CConcept* pIConcept, CConcept* pNewConcept, string sSystemAction);

	// Returns the value of a feature for the grounding process
	// ���ؽӵع��̵�������ֵ
	virtual float GetGroundingFeature(string sFeatureName);

	// Returns the string value of a feature for the grounding process
	// ���ؽӵع��̵��������ַ���ֵ
	virtual string GetGroundingFeatureAsString(string sFeatureName);

	// Clears the belief updating features
	// ��������������
	virtual void ClearBeliefUpdatingFeatures();

	//---------------------------------------------------------------------
	// Methods for access to concept priors, confusability and concept
	// type information
	// ���ʸ������飬�����Ժ͸���������Ϣ�ķ���
	//---------------------------------------------------------------------

	// Return the prior for a given concept hypothesis
	// ���ظ���������������
	virtual float GetPriorForConceptHyp(string sConcept, string sHyp);

	// Return the confusability score for a concept hypothesis
	// ���ظ������Ļ����Է���
	virtual float GetConfusabilityForConceptHyp(string sConcept, string sHyp);

	// Return the concept type
	// ���ظ������
	virtual string GetConceptTypeInfoForConcept(string sConcept);

	//---------------------------------------------------------------------
	// Methods for identifying grounding actions
	// ʶ��ӵض����ķ���
	//---------------------------------------------------------------------

	// Add a grounding action to be used by the grounding manager
	// ��ӽӵز����Թ��ӵع�����ʹ��
	void UseGroundingAction(string sActionName, CGroundingAction* pgaAGroundingAction);

	// Convert grounding action names to indices and the other way around
	// �ӵ����� �� ��������
	int GroundingActionNameToIndex(string sGroundingActionName);
	string GroundingActionIndexToName(unsigned int iGroundingActionIndex);

	// Access a grounding action via the indexing operator
	// ͨ���������������ʽӵز���
	CGroundingAction* operator[] (string sGroundingActionName);
	CGroundingAction* operator[] (unsigned int iGroundingActionIndex);

	//---------------------------------------------------------------------
	// Methods for requesting grounding needs
	// ����ӵصķ���
	//---------------------------------------------------------------------

	// signal the need for grounding the turn
	// ������ǰturn��Ҫgrouding
	void RequestTurnGrounding(bool bATurnGroundingRequest = true);

	// signal the need for grounding a certain concept
	// ������ҪGrouding��ǰ��concept
	void RequestConceptGrounding(CConcept* pConcept);

	// force the grounding manager to schedule grounding for a concept; returns the action that got scheduled
	// ǿ�ƽӵع�����Ϊһ������Žӵ�; �����ѵ��ȵĲ���
	string ScheduleConceptGrounding(CConcept* pConcept);

	//---------------------------------------------------------------------
	// Methods for manipulating the queue of concept grounding requests
	// ���� ����ӵ�������� �ķ���
	//---------------------------------------------------------------------

	// Lock the concept grounding requests queue
	// ��ס�����������
	void LockConceptGroundingRequestsQueue();

	// Unlock the concept grounding requests queue
	// ����
	void UnlockConceptGroundingRequestsQueue();

	// Set the grounding request state for a concept
	// ���ø���ӵ������״̬
	void SetConceptGroundingRequestStatus(CConcept* pConcept,int iAGroundingRequestStatus);

	// Get the grounding request state for a concept
	// ��ȡ״̬
	int GetConceptGroundingRequestStatus(CConcept* pConcept);

	// Signal that a concept grounding request has completed
	// ����ӵ��������
	void ConceptGroundingRequestCompleted(CConcept* pConcept);

	// Remove a grounding request
	// �Ƴ�����
	void RemoveConceptGroundingRequest(CConcept* pConcept);

	// Purge the grounding requests queue
	// ����ӵ��������
	void PurgeConceptGroundingRequestsQueue();

	//---------------------------------------------------------------------
	// Methods for setting and accessing grounding actions history 
	// information
	// ���úͷ��ʽӵض�����ʷ��Ϣ�ķ���
	//---------------------------------------------------------------------

	// adds a grounding action history item to the current history
	// ���ӵض�����ʷ��Ŀ��ӵ���ǰ��ʷ
	void GAHAddHistoryItem(string sGroundingModelName,string sActionName, int iGroundingActionType);

	// sets the barge-in flag on the last grounding history item
	// �����һ���ӵ���ʷ��Ŀ�����ò����־
	void GAHSetBargeInFlag(int iTurnNum, bool bBargeInFlag);

	// obtains the turn grounding action performed in a certain turn
	// �����һ��turn��ִ�е�turn�ӵض���
	string GAHGetTurnGroundingAction(int iTurnNumber);

	// counts how many times an action was taken in the last N turns
	// ���������NȦ�ڽ���һ�ζ����Ĵ���
	int GAHCountTakenInLastNTurns(bool bAlsoHeard, string sActionName, int iNumTurns = -1);

	// counts how many times an action was taken from a particular grounding model    
	// ������ض��ӵ�ģ�Ͳ�ȡ�����Ĵ���
	int GAHCountTakenByGroundingModelInLastNTurns(bool bAlsoHeard, string sActionName, string sGroundingModelName, int iNumTurns = -1);

	//---------------------------------------------------------------------
	// Methods for registering and creating various grounding model types
	// ע��ʹ������ֽӵ�ģ�����͵ķ���
	//---------------------------------------------------------------------

	// register a grounding model type
	// ע��ӵ�ģ������
	void RegisterGroundingModelType(string sName, FCreateGroundingModel fctCreateGroundingModel);

	// create a grounding model 
	// ����һ���ӵ�ģ��
	CGroundingModel* CreateGroundingModel(string sModelType,string sModelPolicy);

	//---------------------------------------------------------------------
	// Methods for accessing the state of the grounding management layer
	// ��ȡ�ӵع�������״̬
	//---------------------------------------------------------------------

	// Indicates if there's a pending request
	// ��ʾ�Ƿ��д����������
	bool HasPendingRequests();

	// Indicates if there's a pending turn grounding request
	// ָʾ�Ƿ��д�����turn�ӵ�����
	bool HasPendingTurnGroundingRequest();

	// Indicates if there's a pending concept grounding request
	// ָʾ�Ƿ��д��������ӵ�����
	bool HasPendingConceptGroundingRequests();

	// Indicates if there are unprocessed concept grounding requests
	// ָʾ�Ƿ����δ����ĸ���ӵ�����
	bool HasUnprocessedConceptGroundingRequests();

	// Indicates if there are scheduled concept grounding requests
	// ָʾ�Ƿ��е��ȵĸ���ӵ�����
	bool HasScheduledConceptGroundingRequests();

	// Indicates if there are executing concept grounding requests
	// ָʾ�Ƿ�������ִ�и���ӵ�����
	bool HasExecutingConceptGroundingRequests();

	// Check if a certain concept is undergoing grounding
	// ���ĳ�������Ƿ����ڽӵ�
	bool GroundingInProgressOnConcept(CConcept* pConcept);

	// Returns the scheduled action for a concept
	// ���ظ���ļƻ�����
	string GetScheduledGroundingActionOnConcept(CConcept* pConcept);

	//---------------------------------------------------------------------
	// Method for running the grounding process
	// ���нӵع���
	//---------------------------------------------------------------------

	virtual void Run();


private:

	//---------------------------------------------------------------------
	// Auxiliary private methods
	// private ����
	//---------------------------------------------------------------------

	// Return the index of a concept grounding request, or -1 if not found
	// ���ظ���ӵ���������������δ�ҵ����򷵻�-1
	int getConceptGroundingRequestIndex(CConcept* pConcept);

	// Load a policy from its description file
	// �������ļ����ز���
	string loadPolicy(string sFileName);
};

#endif // __GROUNDINGMANAGERAGENT_H__
