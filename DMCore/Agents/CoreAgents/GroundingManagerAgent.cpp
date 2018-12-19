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
// GROUNDINGMANAGERAGENT.CPP - implements the grounding management agent. This 
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

#include "GroundingManagerAgent.h"
#include "../../../DMCore/Core.h"

// D: constant describing the string version of GRS types
char vsGRS[7][20] = { "UNPROCESSED", "PENDING", "READY", "SCHEDULED",
"ONSTACK", "EXECUTING", "DONE" };

//-----------------------------------------------------------------------------
// D: Constructors and destructors
//-----------------------------------------------------------------------------

// D: Default constructor - merely inherited so far
CGroundingManagerAgent::CGroundingManagerAgent(string sAName, string sAConfiguration, string sAType) :
CAgent(sAName, sAConfiguration, sAType)
{

	// set the turn grounding signal to false
	//标志，指示我们是否需要Grouding = false
	bTurnGroundingRequest = false;

	// set the lock to false
	// 加锁标志 = false
	bLockedGroundingRequests = false;
}

// D: Virtual destructor 
CGroundingManagerAgent::~CGroundingManagerAgent()
{
	// release all external policies
	// 释放所有外部策略
	ReleaseExternalPolicyInterfaces();
}

//-----------------------------------------------------------------------------
// D: Static function for dynamic agent creation
//-----------------------------------------------------------------------------
CAgent* CGroundingManagerAgent::AgentFactory(string sAName, string sAConfiguration)
{
	return new CGroundingManagerAgent(sAName, sAConfiguration);
}

//-----------------------------------------------------------------------------
//
// GroundingManagerAgent class specific public methods
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// D: Access to configuration information
//-----------------------------------------------------------------------------

// D: Set the configuration string
// 设置配置参数
void CGroundingManagerAgent::SetConfiguration(string sAGroundingManagerConfiguration)
{
	gmcConfig.sGroundingManagerConfiguration =	ToLowerCase(sAGroundingManagerConfiguration);

	Log(GROUNDINGMANAGER_STREAM, "Setting configuration: %s",
		gmcConfig.sGroundingManagerConfiguration.c_str());

	TStringVector vsTokens = PartitionString(gmcConfig.sGroundingManagerConfiguration, ";");

	// set the defaults
	gmcConfig.bGroundConcepts = false;
	gmcConfig.bGroundTurns = false;
	gmcConfig.sBeliefUpdatingModelName = "npu";

	for (unsigned int i = 0; i < vsTokens.size(); i++)
	{
		if (vsTokens[i] == "no_grounding")
		{
			// if no grounding
			gmcConfig.bGroundConcepts = false;
			gmcConfig.bGroundTurns = false;
			gmcConfig.sBeliefUpdatingModelName = "npu";
			break;
		}
		else
		{
			string sSlot = "", sValue = "default";
			SplitOnFirst(vsTokens[i], ":", sSlot, sValue);

			if (sSlot == "concepts")
			{
				// concept grounding model specification
				if (ToLowerCase(sValue) == "default")
					sValue = "concept_default";
				gmcConfig.sConceptGM = sValue;
				gmcConfig.bGroundConcepts = !(sValue == "none");
			}
			else if (sSlot == "turns")
			{
				// turn grounding model specification
				if (ToLowerCase(sValue) == "default")
					sValue = "request_default";
				gmcConfig.sTurnGM = sValue;
				gmcConfig.bGroundTurns = !(sValue == "none");
			}
			else if (sSlot == "beliefupdatingmodel")
			{
				// belief updating model specification
				// 信念更新模型
				SetBeliefUpdatingModelName(sValue);
			}
		}
	}
}

// D: Returns the configuration string
TGroundingManagerConfiguration CGroundingManagerAgent::GetConfiguration()
{
	return gmcConfig;
}

//-----------------------------------------------------------------------------
// D: Operations with the model data
//-----------------------------------------------------------------------------

// D: Load the model specifications
// 加载策略模型
void CGroundingManagerAgent::LoadPoliciesFromString(string sPolicies)
{

	// log
	Log(GROUNDINGMANAGER_STREAM, "Loading grounding models ...");

	// read the file line by line
	vector<string> vsPolicies = PartitionString(sPolicies, ";");
	for (unsigned int i = 0; i < vsPolicies.size(); i++)
	{
		string sModelName;		//model名
		string sModelFileName;	//model的文件名
		string sModelData;		//model数据

		/*
			expl_impl = expl_impl.pol
			expl = expl.pol
			request_default = request_default.pol
			request_lr = request_lr.pol
		*/

		// check for ModelName = ModelFileName pair
		if (SplitOnFirst(vsPolicies[i], "=", sModelName, sModelFileName))
		{

			// extract the model name and filename
			sModelName = ToLowerCase(Trim(sModelName));
			//######################################################
			// 从接地file中加载polices => 
			//	【只是从中读出需要的内容，以string的形式，存储到policy的hash表中, 
			//	 当真实创建Agent的时候，运行Create函数的时候再在policy Hash表中查找到配置的Grouding模型，进行policy字符串的解析，创建相应的对象】
			sModelData = loadPolicy(Trim(sModelFileName));
			//######################################################

			if (sModelData.empty())
			{
				Log(WARNING_STREAM, "Unable to load policy for %s from file %s",
					sModelName.c_str(), sModelFileName.c_str());
			}
			else
			{
				/*
					expl_impl = expl_impl.pol 文件内容
					expl = expl.pol	文件内容
					request_default = request_default.pol 文件内容
					request_lr = request_lr.pol 文件内容
				*/
				// add it to the hash
				// 添加进策略Hash表
				s2sPolicies.insert( STRING2STRING::value_type(sModelName, loadPolicy(sModelFileName)));
			}
		}
	}

	// Log the models loaded
	Log(GROUNDINGMANAGER_STREAM, "%d grounding model(s) loaded.", vsPolicies.size());
}

// A: Load the model specifications
// 加载策略模型
void CGroundingManagerAgent::LoadPoliciesFromFile(string sFileName)
{

	// open the file
	FILE *fid = fopen(sFileName.c_str(), "r");
	if (!fid)
	{
		Warning(FormatString(
			"Grounding policies specification file (%s) could not be open for"\
			" reading.", sFileName.c_str()));
		return;
	}

	// log
	Log(GROUNDINGMANAGER_STREAM, "Loading grounding models ...");

	// read the file line by line
	// 一行一行的读取文件grouding.policy
	char lpszLine[STRING_MAX];
	int iModelsCount = 0;
	while (fgets(lpszLine, STRING_MAX, fid) != NULL)
	{
		string sModelName;
		string sModelFileName;
		string sModelData;
		// check for comments
		// 是否是注释 ?
		if ((lpszLine[0] == '#') ||
			((lpszLine[0] == '/') && lpszLine[1] == '/'))
			continue;

	/*
		expl_impl = expl_impl.pol
		expl = expl.pol
		request_default = request_default.pol
		request_lr = request_lr.pol
	*/
		// check for ModelName = ModelFileName pair
		if (SplitOnFirst((string)lpszLine, "=", sModelName, sModelFileName))
		{

			// extract the model name and filename
			sModelName = ToLowerCase(Trim(sModelName));		//expl_impl
			sModelData = loadPolicy(Trim(sModelFileName));	//expl_impl.pol 文件内容

			if (sModelData.empty())
			{
				Log(WARNING_STREAM, "Unable to load policy for %s from file %s", sModelName.c_str(), sModelFileName.c_str());
			}
			else
			{
				// add it to the hash
				s2sPolicies.insert(STRING2STRING::value_type(sModelName, sModelData));

				// and count it up
				iModelsCount++;
			}
		}
	}
	fclose(fid);

	// Log the models loaded
	Log(GROUNDINGMANAGER_STREAM, "%d grounding model(s) loaded.", iModelsCount);
}

// D: Return the data for a certain grounding model
string CGroundingManagerAgent::GetPolicy(string sModelName)
{
	return s2sPolicies[sModelName];
}

// D: create an external policy interface
CExternalPolicyInterface* CGroundingManagerAgent::CreateExternalPolicyInterface(
	string sAHost)
{
	CExternalPolicyInterface* pEPI;
	TExternalPolicies::iterator iPtr;
	// check if it's a new policy
	if ((iPtr = mapExternalPolicies.find(sAHost)) == mapExternalPolicies.end())
	{
		// create it
		pEPI = new CExternalPolicyInterface(sAHost);
		// add it to the hash
		mapExternalPolicies.insert(TExternalPolicies::value_type(sAHost, pEPI));
		// and return it
		return pEPI;
	}
	else
	{
		// o/w return it from the hash
		return iPtr->second;
	}
}

// D: release the set of external policy interfaces
// D：释放外部策略接口集
void CGroundingManagerAgent::ReleaseExternalPolicyInterfaces()
{
	TExternalPolicies::iterator iPtr;
	for (iPtr = mapExternalPolicies.begin(); iPtr != mapExternalPolicies.end(); iPtr++)
	{
		// call the destructor on each policy
		// 删除所有外部策略指针
		delete iPtr->second;
	}
	// and now erase the hash
	// 清空hash
	mapExternalPolicies.clear();
}

//-----------------------------------------------------------------------------
// Methods for the belief updating model
//-----------------------------------------------------------------------------

// D: load the belief updating model from a file
void CGroundingManagerAgent::LoadBeliefUpdatingModel(string sAFileName)
{

	// open the file
	FILE *fid = fopen(sAFileName.c_str(), "r");
	if (!fid)
	{
		Warning(FormatString(
			"Belief updating model file (%s) could not be open for"\
			" reading.", sAFileName.c_str()));
		return;
	}

	// log that we are loading the belief updating model
	Log(GROUNDINGMANAGER_STREAM, "Loading belief updating model from %s ...",
		sAFileName.c_str());

	// read the file line by line
	char lpszLine[STRING_MAX];
	while (fgets(lpszLine, STRING_MAX, fid) != NULL)
	{

		// get the line as a string
		string sLine = Trim((string)lpszLine);

		string sModelName;
		string sModelFileName;
		// check for comments
		if ((lpszLine[0] == '#') ||
			((lpszLine[0] == '/') && lpszLine[1] == '/'))
			continue;

		// o/w, check for constants declaration
		if (ToLowerCase(sLine.substr(0, 9)) == "constants")
		{

			// then go on and read the constant parameters 
			while ((fgets(lpszLine, STRING_MAX, fid) != NULL) &&
				ToLowerCase(Trim((string)lpszLine)) != "end")
			{
				// get the line
				sLine = Trim((string)lpszLine);
				string sParam, sCoef;
				SplitOnFirst(sLine, "=", sParam, sCoef);
				// insert the parameters in the hash
				s2fConstantParameters.insert(STRING2FLOAT::value_type(
					ToLowerCase(Trim(sParam)), (float)atof(sCoef.c_str())));
			}
		}

		// o/w, check for lr_model(action)
		if (ToLowerCase(sLine.substr(0, 8)) == "lr_model")
		{

			// then go on and read the parameters for each individual
			// feature in the model
			string sFoo, sAction;
			SplitOnFirst(sLine, "(", sFoo, sAction);
			sAction = TrimRight(sAction, " \n)");

			// check that the action is one of the ones we expect
			if ((sAction != SA_REQUEST) && (sAction != SA_EXPL_CONF) &&
				(sAction != SA_IMPL_CONF) && (sAction != SA_OTHER) &&
				(sAction != SA_UNPLANNED_IMPL_CONF))
			{
				FatalError(FormatString(
					"Error loading belief updating model. Unknown action %s.",
					sAction.c_str()));
			}

			// construct the model for this action
			STRING2FLOATVECTOR s2vfMRModel;

			// now continue through the lines
			while ((fgets(lpszLine, STRING_MAX, fid) != NULL) &&
				ToLowerCase(Trim((string)lpszLine)) != "end")
			{
				// get the line
				sLine = Trim((string)lpszLine);
				string sParam, sCoef;
				SplitOnFirst(sLine, "=", sParam, sCoef);
				TStringVector vsCoefs = PartitionString(sCoef, ";");
				vector<float> vfCoefs;
				for (unsigned int i = 0; i < vsCoefs.size(); i++)
				{
					vfCoefs.push_back((float)atof(vsCoefs[i].c_str()));
				}
				// insert the parameters in the hash
				s2vfMRModel.insert(STRING2FLOATVECTOR::value_type(
					ToLowerCase(Trim(sParam)), vfCoefs));
			}

			// now store the model
			s2s2vfBeliefUpdatingModels.insert(
				STRING2STRING2FLOATVECTOR::value_type(
				sAction, s2vfMRModel));
		}

		// o/w, check for concept values info
		if (ToLowerCase(sLine.substr(0, 14)) == "concept_values")
		{

			// then go on and read the priors and confusability for each 
			// value for that concept
			string sFoo, sFoo2, sConcept, sConceptType;
			SplitOnFirst(sLine, "(", sFoo, sFoo2);
			sFoo2 = TrimRight(sFoo2, " \n)");
			SplitOnFirst(sFoo2, ":", sConcept, sConceptType);

			// construct the list of values for this concept
			STRING2FLOATVECTOR s2vfConceptValues;

			// now continue through the lines
			while ((fgets(lpszLine, STRING_MAX, fid) != NULL) &&
				ToLowerCase(Trim((string)lpszLine)) != "end")
			{
				// get the line
				sLine = Trim((string)lpszLine);
				TStringVector vsItems = PartitionString(sLine, "\t");
				vector<float> vfParams;
				for (unsigned int i = 1; i < vsItems.size(); i++)
				{
					vfParams.push_back((float)atof(vsItems[i].c_str()));
				}
				// insert the parameters in the hash
				s2vfConceptValues.insert(STRING2FLOATVECTOR::value_type(
					ToLowerCase(Trim(vsItems[0])), vfParams));
			}

			// now store the list of values
			s2s2vfConceptValuesInfo.insert(
				STRING2STRING2FLOATVECTOR::value_type(
				sConcept, s2vfConceptValues));

			// and the concept type
			s2sConceptTypeInfo.insert(STRING2STRING::value_type(
				sConcept, sConceptType));
		}

	}
	fclose(fid);

	// Log the models loaded
	Log(GROUNDINGMANAGER_STREAM, "Belief updating grounding model loaded.");

}

// D: Sets the belief updating model
// D：设置置信更新模型
void CGroundingManagerAgent::SetBeliefUpdatingModelName( string sABeliefUpdatingModelName)
{
	//概念更新模型,目前只支持 => ["npu", "calista" ]
	// store the belief updating model
	gmcConfig.sBeliefUpdatingModelName = ToLowerCase(sABeliefUpdatingModelName);

	// if the model type is not NPU
	// 如果概念更新模型不是 NPU
	string sModelName, sModelFile;
	if (SplitOnFirst(gmcConfig.sBeliefUpdatingModelName, "(", sModelName, sModelFile))
	{
		// check that we have a calista model
		if (sModelName == "calista")
		{
			gmcConfig.sBeliefUpdatingModelName = "calista";
			// trim the filename
			sModelFile = TrimRight(sModelFile, ") \t\n");
			// load the belief updating model
			LoadBeliefUpdatingModel(sModelFile);
		}
		else
		{
			FatalError(FormatString(
				"Unknown belief updating model type: %s", sModelName.c_str()));
		}
	}
}

// D: Gets the belief updating model name
// D：获取信任更新模型名称
string CGroundingManagerAgent::GetBeliefUpdatingModelName()
{
	return gmcConfig.sBeliefUpdatingModelName; //概念更新模型["npu", "calista"]
}

// D: Returns an actual belief updating model for an action
STRING2FLOATVECTOR& CGroundingManagerAgent::GetBeliefUpdatingModelForAction(
	string sSystemAction)
{

	// check that the model exists
	STRING2STRING2FLOATVECTOR::iterator iPtr;
	if ((iPtr = s2s2vfBeliefUpdatingModels.find(sSystemAction)) ==
		s2s2vfBeliefUpdatingModels.end())
	{
		// if it doesn't, issue a fatal error
		FatalError(FormatString(
			"Could not find belief updating model for action %s.",
			sSystemAction.c_str()));
	}

	// return 
	return iPtr->second;
}

// D: Return the constant parameter from the configuration
float CGroundingManagerAgent::GetConstantParameter(string sParam)
{
	STRING2FLOAT::iterator iPtr;
	if ((iPtr = s2fConstantParameters.find(sParam)) !=
		s2fConstantParameters.end())
	{
		return iPtr->second;
	}
	else
	{
		FatalError(FormatString("Could not find constant parameter %s",
			sParam.c_str()));
		return 0;
	}
}

// D: Precompute the belief updating features for a concept update
void CGroundingManagerAgent::PrecomputeBeliefUpdatingFeatures(
	CConcept* pIConcept, CConcept* pNewConcept, string sSystemAction)
{

	// Log(BELIEFUPDATING_STREAM, "Precompute belief updating features ...");
	// check that pIConcept is there, o/w issue a fatal error
	if (!pIConcept)
		FatalError("Cannot precompute belief updating features on NULL concept.");

	// grab the top hypothesis 
	int iIndexI_TH = pIConcept->GetTopHypIndex();
	CHyp* phI_TH = (iIndexI_TH != -1) ? pIConcept->GetHyp(iIndexI_TH) : NULL;

	// grab the second best hypothesis
	int iIndexI_2H = pIConcept->Get2ndHypIndex();
	CHyp* phI_2H = (iIndexI_2H != -1) ? pIConcept->GetHyp(iIndexI_2H) : NULL;

	// look into history
	int iIndexH_TH = (pIConcept->GetHistorySize() > 0) ?
		pIConcept->operator [](-1).GetTopHypIndex() : -1;
	CHyp* phH_TH = (iIndexH_TH != -1) ?
		pIConcept->operator [](-1).GetHyp(iIndexH_TH) : NULL;

	// now define empty with history 
	bool bEmptyWithHistory = (iIndexI_TH == -1) && (iIndexH_TH != -1);
	// and HOHH (whenever we need to replace the initial hypothesis with the 
	// history hypothesis)
	bool bHOHH = bEmptyWithHistory &&
		((sSystemAction == SA_IMPL_CONF) || (sSystemAction == SA_UNPLANNED_IMPL_CONF));
	s2fBeliefUpdatingFeatures.insert(STRING2FLOAT::value_type("h00hhh", (float)bHOHH));

	// now if we need to replace the initial with the history one, do that
	if (bHOHH)
	{
		phI_TH = phH_TH;
		iIndexI_TH = iIndexH_TH;
	}

	// get the new top hypothesis and its index
	int iIndexNewTop = pNewConcept ? pNewConcept->GetTopHypIndex() : -1;
	CHyp* phNewTop = (iIndexNewTop != -1) ? pNewConcept->GetHyp(iIndexNewTop) : NULL;

	// check if it matches the top or the second best hypothesis
	bool bNEWMatchesI_TH =
		phNewTop && phI_TH && (*phNewTop == *phI_TH);
	bool bNEWMatchesI_2H =
		phNewTop && phI_2H && (*phNewTop == *phI_2H);
	bool bNEWMatchesNEW_1 = phNewTop && !bNEWMatchesI_TH && !bNEWMatchesI_2H;
	// check if it matches the history hypothesis
	bool bNEWMatchesH_TH =
		phNewTop && phH_TH && (*phNewTop == *phH_TH);

	// now compute the index for NEW_1
	int iIndexNEW_1 = bNEWMatchesNEW_1 ? iIndexNewTop : -1;
	CHyp* phNEW_1 = bNEWMatchesNEW_1 ? phNewTop : NULL;

	// precompute the i_th_explicitly_confirmed_already
	s2fBeliefUpdatingFeatures.insert(STRING2FLOAT::value_type(
		"i_th_explicitly_confirmed_already",
		(phI_TH != NULL) &&
		(pIConcept->GetExplicitlyConfirmedHypAsString() == phI_TH->ValueToString())));

	// precompute the ur_selh_new_1_explicitly_disconfirmed_already
	s2fBeliefUpdatingFeatures.insert(STRING2FLOAT::value_type(
		"ur_selh_new_1_explicitly_disconfirmed_already",
		(phNEW_1 != NULL) &&
		(pIConcept->GetExplicitlyDisconfirmedHypAsString() == phNEW_1->ValueToString())));

	// precompute response_new_hyps_in_selh
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("ur_selh_h_th_avail", (float)bNEWMatchesH_TH));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("ur_selh_i_th_avail", (float)bNEWMatchesI_TH));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("ur_selh_i_2h_avail", (float)bNEWMatchesI_2H));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("ur_selh_new_1_avail", (float)bNEWMatchesNEW_1));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("response_new_hyps_in_selh", (float)bNEWMatchesNEW_1));

	// precompute concept_repeat_selh family
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("concept_repeat_selh", (float)(phNewTop != NULL)));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("concept_repeat_selh_i_th", (float)bNEWMatchesI_TH));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("concept_repeat_selh_not_i_th",
		(float)(phNewTop && !bNEWMatchesI_TH)));

	// precompute the concept_id family of features
	STRING2STRING2FLOATVECTOR::iterator iPtr;
	for (iPtr = s2s2vfConceptValuesInfo.begin();
		iPtr != s2s2vfConceptValuesInfo.end(); iPtr++)
	{
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type(
			FormatString("concept_id(%s)",
			ToLowerCase(iPtr->first).c_str()),
			(iPtr->first == pIConcept->GetSmallName())));
	}

	// precompute initial_num_hyps, history_num_hyps and family
	int iInitialNumHyps = pIConcept->GetNumHyps();
	int iHistoryNumHyps = 0;
	if (pIConcept->GetHistorySize() >= 1)
		iHistoryNumHyps = pIConcept->GetHistoryVersion(-1).GetNumHyps();
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("initial_num_hyps", (float)iInitialNumHyps));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("initial_num_hyps_gt_0", iInitialNumHyps > 0));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("initial_num_hyps_gt_1", iInitialNumHyps > 1));

	// precompute initial_value_structure
	if (iInitialNumHyps > 0)
		s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("initial_value_structure", 0));
	else if (iHistoryNumHyps > 0)
		s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("initial_value_structure", 1));
	else
		s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("initial_value_structure", 2));

	// precompute concept_2 and concept_bool
	int iConcept2 = 0;
	if (pIConcept->GetConceptTypeInfo() == "non_bool")
		iConcept2 = 0;
	else if (pIConcept->GetConceptTypeInfo() == "bool")
		iConcept2 = 1;
	else if (pIConcept->GetConceptTypeInfo() == "trigger")
		iConcept2 = 2;
	else
		iConcept2 = -1;
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("concept_2", (float)iConcept2));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("concept_bool", iConcept2 > 0));

	// precompute i_th_confusability, i_th_conf
	if (iIndexI_TH != -1)
	{
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type(
			"i_th_conf", phI_TH->GetConfidence()));
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("i_th_conf_gtm",
			phI_TH->GetConfidence() > GetConstantParameter("i_th_conf_mean")));
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type(
			"i_th_confusability", pIConcept->GetConfusabilityForHyp(phI_TH)));
	}
	else
	{
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("i_th_conf", 0));
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("i_th_conf_gtm", 0));
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("i_th_confusability",
			pIConcept->GetConfusabilityForHyp(NULL)));
	}

	// precompute ur_selh_new_1_confusability, ur_selh_new_1_conf
	if (iIndexNEW_1 != -1)
	{
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("ur_selh_new_1_conf",
			phNEW_1->GetConfidence()));
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("ur_selh_new_1_conf_gt_25",
			phNEW_1->GetConfidence() > 0.25));
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("ur_selh_new_1_conf_gt_75",
			phNEW_1->GetConfidence() > 0.75));
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("ur_selh_new_1_confusability",
			pNewConcept->GetConfusabilityForHyp(phNEW_1)));
	}
	else
	{
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("ur_selh_new_1_conf", 0));
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("ur_selh_new_1_conf_gt_25", 0));
		s2fBeliefUpdatingFeatures.insert(
			STRING2FLOAT::value_type("ur_selh_new_1_conf_gt_75", 0));
		// if we have an other type update, introduce the missing
		// value
		if (sSystemAction == SA_OTHER)
		{
			s2fBeliefUpdatingFeatures.insert(
				STRING2FLOAT::value_type("ur_selh_new_1_confusability",
				GetConstantParameter("missing_ur_selh_new_1_confusability")));
		}
		else
		{
			s2fBeliefUpdatingFeatures.insert(
				STRING2FLOAT::value_type("ur_selh_new_1_confusability",
				pIConcept->GetConfusabilityForHyp(NULL)));
		}
	}

	// precompute i_th_prior and derived versions
	float fI_THPrior = pIConcept->GetPriorForHyp(phI_TH);
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("i_th_prior", fI_THPrior));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("i_th_prior_gt_1", (fI_THPrior >= 0.95)));

	// precompute ur_selh_new_1_prior and derived versions
	float fNEW_1Prior = 1;
	if (pNewConcept)
		fNEW_1Prior = pNewConcept->GetPriorForHyp(phNEW_1);
	// introduce the missing feature value if we are doing an other update
	if ((sSystemAction == SA_OTHER) && (iIndexNEW_1 == -1))
		fNEW_1Prior = GetConstantParameter("missing_ur_selh_new_1_prior");
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("ur_selh_new_1_prior", fNEW_1Prior));
	s2fBeliefUpdatingFeatures.insert(
		STRING2FLOAT::value_type("ur_selh_new_1_prior_gt_1", (fNEW_1Prior >= 0.95)));

	// at the end log all the precomputed belief updating features
	string sPrecomputedFeatures = "";
	STRING2FLOAT::iterator iPtr2;
	for (iPtr2 = s2fBeliefUpdatingFeatures.begin();
		iPtr2 != s2fBeliefUpdatingFeatures.end();
		iPtr2++)
	{
		sPrecomputedFeatures = FormatString("%s  %s = %.4f\n",
			sPrecomputedFeatures.c_str(), iPtr2->first.c_str(),
			iPtr2->second);
	}
	// Log(BELIEFUPDATING_STREAM, "Finished precomputing belief updating features.");
}

// D: Return the value of a belief updating feature
float CGroundingManagerAgent::GetGroundingFeature(string sFeatureName)
{
	STRING2FLOAT::iterator iPtr;

	// check if it's a class / enum feature (something like feature[value])
	string sClassFeature = "";
	string sClassFeatureValue = "";
	if (SplitOnFirst(sFeatureName, "[", sClassFeature, sClassFeatureValue))
	{
		sFeatureName = sClassFeature;
		sClassFeatureValue = TrimRight(sClassFeatureValue, "]");
	}

	float fValue = -1;
	string sValue = "";
	if ((iPtr = s2fBeliefUpdatingFeatures.find(sFeatureName)) !=
		s2fBeliefUpdatingFeatures.end())
	{
		// if we found the feature in the hash, simply return it
		fValue = iPtr->second;
	}
	else if (sFeatureName == "k")
	{
		// o/w if we are looking for the constant feature return 1
		fValue = 1.0;
	}
	else if (sFeatureName == "lex")
	{
		// o/w if we have a lexical feature, look in the input for that
		// word
		string sHypothesis = pInteractionEventManager->GetValueForExpectation(
			FormatString("[hypothesis]", sFeatureName.c_str()));
		TStringVector vsWords = PartitionString(sHypothesis, " ");
		for (unsigned int i = 0; i < vsWords.size(); i++)
		if (ToLowerCase(sClassFeatureValue) == ToLowerCase(vsWords[i]))
			return 1;
		// o/w we have not found it so return 0
		return 0;
	}
	else if (sFeatureName == "lexw1")
	{
		// o/w if we have a lexical feature, look in the input for that
		// word
		string sHypothesis = pInteractionEventManager->GetValueForExpectation(
			FormatString("[hypothesis]", sFeatureName.c_str()));
		TStringVector vsWords = PartitionString(sHypothesis, " ");
		if (vsWords.size() != 1)
			return 0;
		else
			return ToLowerCase(sClassFeatureValue) == ToLowerCase(Trim(vsWords[0]));
	}
	else if (sFeatureName == "last_nonu_action")
	{
		// look up the last non-understanding action
		sValue = GAHGetTurnGroundingAction(pDMCore->GetLastInputTurnNumber() - 1);
	}
	else if (pInteractionEventManager->GetLastInput()->Matches(
		FormatString("[%s]", sFeatureName.c_str())))
	{
		// o/w check for it in the last input (it might be from helios)
		sValue = pInteractionEventManager->GetLastInput()->
			GetValueForExpectation(FormatString("[%s]",
			sFeatureName.c_str()));
	}
	else
	{
		// finally, throw a warning
		Warning(FormatString(
			"Could not find grounding feature %s. Defaulting to 0.",
			sFeatureName.c_str()));
		return 0;
	}

	// now that we have the value check if we are dealing with a enumeration 
	// feature
	if (sClassFeatureValue != "")
	{
		if (sValue == "")
			return ((float)atof(sClassFeatureValue.c_str()) == fValue);
		else
			return (float)(ToLowerCase(sClassFeatureValue) == ToLowerCase(sValue));
	}
	else
	{
		// o/w simply convert to a float
		if (sValue == "")
			return fValue;
		else
			return (float)atof(sValue.c_str());
	}
}

// D: Return the value of a belief updating feature, as a string
string CGroundingManagerAgent::GetGroundingFeatureAsString(string sFeatureName)
{
	STRING2FLOAT::iterator iPtr;

	// check if it's a class / enum feature (something like feature[value])
	string sClassFeature = "";
	string sClassFeatureValue = "";
	if (SplitOnFirst(sFeatureName, "[", sClassFeature, sClassFeatureValue))
	{
		sFeatureName = sClassFeature;
		sClassFeatureValue = TrimRight(sClassFeatureValue, "]");
	}

	string sValue = "";
	if ((iPtr = s2fBeliefUpdatingFeatures.find(sFeatureName)) !=
		s2fBeliefUpdatingFeatures.end())
	{
		// if we found the feature in the hash, simply return it
		sValue = FormatString("%.4f", iPtr->second);
	}
	else if (sFeatureName == "k")
	{
		// o/w if we are looking for the constant feature return 1
		sValue = "1.0000";
	}
	else if (sFeatureName == "lex")
	{
		// o/w if we have a lexical feature, look in the input for that
		// word
		string sHypothesis = pInteractionEventManager->GetLastInput()->
			GetValueForExpectation(FormatString("[hypothesis]",
			sFeatureName.c_str()));
		TStringVector vsWords = PartitionString(sHypothesis, " ");
		for (unsigned int i = 0; i < vsWords.size(); i++)
		if (ToLowerCase(sClassFeatureValue) == ToLowerCase(vsWords[i]))
			return "1";
		// o/w we have not found it so return 0
		return "0";
	}
	else if (sFeatureName == "lexw1")
	{
		// o/w if we have a lexical feature, look in the input for that
		// word
		string sHypothesis = pInteractionEventManager->GetLastInput()->
			GetValueForExpectation(FormatString("[hypothesis]",
			sFeatureName.c_str()));
		TStringVector vsWords = PartitionString(sHypothesis, " ");
		if (vsWords.size() != 1)
			return "0";
		else
			return (ToLowerCase(sClassFeatureValue) ==
			ToLowerCase(Trim(vsWords[0]))) ? "1" : "0";
	}
	else if (pInteractionEventManager->GetLastInput()->Matches(
		FormatString("[%s]", sFeatureName.c_str())))
	{
		// o/w check for it in the last input (it might be from helios)
		sValue = pInteractionEventManager->GetLastInput()->GetValueForExpectation(
			FormatString("[%s]", sFeatureName.c_str()));
	}
	else
	{
		// finally, throw a warning
		Warning(FormatString(
			"Could not find grounding feature %s. Defaulting to 0.",
			sFeatureName.c_str()));
		return "0";
	}

	// now that we have the value check if we are dealing with a enumeration 
	// feature
	if (sClassFeatureValue != "")
	{
		return (ToLowerCase(sClassFeatureValue) == ToLowerCase(sValue)) ? "1" : "0";
	}
	else
	{
		// o/w simply convert to a float
		return sValue;
	}
}

// D: Clear the belief updating features
void CGroundingManagerAgent::ClearBeliefUpdatingFeatures()
{
	// clear the hash of precomputed belief updating feature
	s2fBeliefUpdatingFeatures.clear();
	// and return
	return;
}

//-----------------------------------------------------------------------------
// D: Methods for access to concept priors and confusability information
//-----------------------------------------------------------------------------

// D: Return the prior information for a given concept hypothesis
float CGroundingManagerAgent::GetPriorForConceptHyp(
	string sConcept, string sHyp)
{
	// check if the concept exists in the ConceptValuesInfo hash
	STRING2STRING2FLOATVECTOR::iterator iPtr;
	if ((iPtr = s2s2vfConceptValuesInfo.find(sConcept)) !=
		s2s2vfConceptValuesInfo.end())
	{
		STRING2FLOATVECTOR::iterator iPtr2;
		if ((iPtr2 = iPtr->second.find(ToLowerCase(sHyp))) != iPtr->second.end())
			// we have found it, so return the prior
			return (iPtr2->second)[0];
		else
		{
			// o/w we have not found the value, so return 
			// the information for undefined and issue a warning
			iPtr2 = iPtr->second.find("<undefined>");
			Warning(FormatString(
				"Could not find prior info for concept %s, hypothesis %s. "
				"Defaulting to %.4f .",
				sConcept.c_str(), sHyp.c_str(), (iPtr2->second)[0]));
			return (iPtr2->second)[0];
		}

	}
	else
	{
		// o/w the concept is not found so we have no information about 
		// its priors
		Warning(FormatString(
			"Could not find prior info for concept %s, hypothesis %s.",
			sConcept.c_str(), sHyp.c_str()));
		return -1;
	}
}

// D: Return the prior information for a given concept hypothesis
float CGroundingManagerAgent::GetConfusabilityForConceptHyp(
	string sConcept, string sHyp)
{

	// check if the concept exists in the ConceptValuesInfo hash
	STRING2STRING2FLOATVECTOR::iterator iPtr;
	if ((iPtr = s2s2vfConceptValuesInfo.find(sConcept)) !=
		s2s2vfConceptValuesInfo.end())
	{
		STRING2FLOATVECTOR::iterator iPtr2;
		if ((iPtr2 = iPtr->second.find(ToLowerCase(sHyp))) != iPtr->second.end())
			// we have found it, so return the confusability score
			return (iPtr2->second)[1];
		else
		{
			// o/w we have not found the value, so return 
			// the information for undefined and issue a warning
			iPtr2 = iPtr->second.find("<undefined>");
			Warning(FormatString(
				"Could not find confusability info for concept %s, hypothesis %s. "
				"Defaulting to %.4f .",
				sConcept.c_str(), sHyp.c_str(), (iPtr2->second)[1]));
			return (iPtr2->second)[1];
		}

	}
	else
	{
		// o/w the concept is not found so we have no information about 
		// its confusability
		Warning(FormatString(
			"Could not find confusability scores for concept %s.",
			sConcept.c_str()));
		return -1;
	}
}

// D: Return the concept type information 
string CGroundingManagerAgent::GetConceptTypeInfoForConcept(string sConcept)
{
	// check if the concept exists in the ConceptValuesInfo hash
	STRING2STRING::iterator iPtr;
	if ((iPtr = s2sConceptTypeInfo.find(sConcept)) !=
		s2sConceptTypeInfo.end())
	{
		return iPtr->second;
	}
	else
	{
		// o/w the concept is not found so we have no information about 
		// its type
		return "unknown";
	}
}

//-----------------------------------------------------------------------------
// D: Operations for identifying the various grounding actions
//-----------------------------------------------------------------------------

// D: Add a grounding action to be used by the grounding manager
// D：添加接地Action以供接地管理器使用
//      GROUNDING_ACTION(NO_ACTION, NO_ACTION, Configuration)
//		#define NO_ACTION CGANoAction
void CGroundingManagerAgent::UseGroundingAction(string sActionName, CGroundingAction* pgaAGroundingAction)
{
	Log(GROUNDINGMANAGER_STREAM, "Registering grounding action: %s [%s].", sActionName.c_str(), pgaAGroundingAction->GetName().c_str());
	// 将Action添加到列表
	// add the action to the list
	vpgaActions.push_back(pgaAGroundingAction);
	// 将Name添加到列表
	// add the name to the list
	vsActionNames.push_back(sActionName);
	// and register whatever dialog agents it might use
	// 并注册它可能使用的任何对话代理
	pgaAGroundingAction->RegisterDialogAgency();
}

// D: Find the index on an action given its name
int CGroundingManagerAgent::GroundingActionNameToIndex(
	string sGroundingActionName)
{

	// go through the list of actions and identify by name
	for (unsigned int i = 0; i < vsActionNames.size(); i++)
	{
		if (vsActionNames[i] == sGroundingActionName)
			return i;
	}

	// if not found generate an error
	FatalError(FormatString("Grounding action %s is not registered with the "\
		"GroundingManagerAgent.", sGroundingActionName.c_str()));

	return -1;
}

// D: Find the name of an action given it's index
// D：查找给定索引的操作的名称
string CGroundingManagerAgent::GroundingActionIndexToName(unsigned int iGroundingActionIndex)
{
	return vsActionNames[iGroundingActionIndex];
}

// D: Indexing operator on the action name - returns the action
CGroundingAction* CGroundingManagerAgent::operator[] (
	string sGroundingActionName)
{

	return operator [] (GroundingActionNameToIndex(sGroundingActionName));
}

// D: Indexing operator - returns the action corresponding to an index
// D：索引操作符 - 返回与索引对应的action
CGroundingAction* CGroundingManagerAgent::operator [] (unsigned int	iActionIndex)
{
	if (iActionIndex >= vpgaActions.size())
	{
		FatalError(FormatString("Grounding action index %d out of bounds.", iActionIndex));
	}
	return vpgaActions[iActionIndex];
}

//-----------------------------------------------------------------------------
// D: Methods for requesting grounding needs
//-----------------------------------------------------------------------------

// D: signal the need for grounding the turn
// D：信号需要接地转
//	void RequestTurnGrounding(bool bATurnGroundingRequest = true); 默认为True
void CGroundingManagerAgent::RequestTurnGrounding(bool bATurnGroundingRequest)
{
	bTurnGroundingRequest = bATurnGroundingRequest && gmcConfig.bGroundTurns; //是否允许Turn Grouding
}

// D: signal the need for grounding a certain concept
// 信号：需要接地一个concept
void CGroundingManagerAgent::RequestConceptGrounding(CConcept* pConcept)
{

	// first check that the current configuration allows to ground concepts
	//首先检查当前配置是否允许接地concept
	if (!gmcConfig.bGroundConcepts) return;

	// if the queue is locked, issue a fatal error
	if (bLockedGroundingRequests)
		FatalError(FormatString(
		"Cannot add concept grounding request for %s: concept "
		"grounding queue is locked.",
		pConcept->GetAgentQualifiedName().c_str()));

	// check if it's in the list
	int iIndex = getConceptGroundingRequestIndex(pConcept);
	if (iIndex != -1)
	{
		// for all other grounding request status, just remove the request 
		// from the queue
		vcgrConceptGroundingRequests.erase(
			vcgrConceptGroundingRequests.begin() + iIndex);
	}

	// add the concept at the end
	// 在末尾添加concept
	TConceptGroundingRequest cgr;
	cgr.pConcept = pConcept;
	cgr.iGroundingRequestStatus = GRS_UNPROCESSED;
	cgr.iSuggestedActionIndex = -1;
	cgr.iTurnNumber = pDMCore->GetLastInputTurnNumber();
	vcgrConceptGroundingRequests.push_back(cgr);
}

// D: force the grounding manager to schedule grounding for a certain concept
// D：强制接地管理器为某个概念安排接地
string CGroundingManagerAgent::ScheduleConceptGrounding(CConcept* pConcept)
{

	// log that we are scheduling concept grounding
	Log(GROUNDINGMANAGER_STREAM, "Scheduling concept grounding for concept %s",
		pConcept->GetName().c_str());

	// first, call a simple request concept grounding
	// 首先，调用一个请求concept接地操作 [GRS_UNPROCESSED]
	RequestConceptGrounding(pConcept);

	// then get the index from the list
	int iIndex = getConceptGroundingRequestIndex(pConcept);

	// now move to schedule this, changing the type from UNPROCESSED

	// start by locking the grounding requests queue
	// 对接地请求队列 加锁
	LockConceptGroundingRequestsQueue();

	// first, set the state to pending
	//首先，将状态设置为待处理 pending
	vcgrConceptGroundingRequests[iIndex].iGroundingRequestStatus = GRS_PENDING;
	// get the grounding model
	CGroundingModel* pGroundingModel = pConcept->GetGroundingModel();

	// compute the state
	pGroundingModel->ComputeState();
	// compute the suggested action
	vcgrConceptGroundingRequests[iIndex].iSuggestedActionIndex = pGroundingModel->ComputeSuggestedActionIndex();
	// get the action that was suggested
	string sActionName = GroundingActionIndexToName(vcgrConceptGroundingRequests[iIndex].iSuggestedActionIndex);

	// now log the model
	vcgrConceptGroundingRequests[iIndex].pConcept->
		GetGroundingModel()->LogStateAction();

	// if the action is an explicit confirm, check that we don't already
	// have on scheduled (if we do, we need to take it off)
	// 如果动作是显式确认，检查我们是否已经计划（如果我们这样做，我们需要关闭它）
	if (sActionName == "EXPL_CONF")
	{
		/*
						 ACCEPT     EXPL_CONF    IMPL_CONF
						 INACTIVE           10            -            -
						 CONFIDENT           8           -5            0
						 UNCONFIDENT       -19           10            5
						 GROUNDED           10            -            -

						 */
		for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)
		{
			TConceptGroundingRequest cgr = vcgrConceptGroundingRequests[i];
			if ((cgr.iGroundingRequestStatus == GRS_SCHEDULED) &&
				(GroundingActionIndexToName(cgr.iSuggestedActionIndex) == "EXPL_CONF"))
			{
				// then we need to set this back to PENDING
				vcgrConceptGroundingRequests[i].iGroundingRequestStatus = GRS_PENDING;
			}
			// put a check that we don't already have something that's on stack or executing
			//放一个检查，我们还没有在堆栈或执行的东西
			if ((cgr.iGroundingRequestStatus == GRS_EXECUTING) &&
				(GroundingActionIndexToName(cgr.iSuggestedActionIndex) == "EXPL_CONF"))
			{
				// then issue a fatal error
				FatalError(FormatString(
					"Could not schedule %s for grounding (EXPL_CONF), since %s"
					" is already undergoing grounding", pConcept->GetName().c_str(),
					cgr.pConcept->GetName().c_str()));
			}
		}
	}

	// set it to scheduled
	vcgrConceptGroundingRequests[iIndex].iGroundingRequestStatus = GRS_SCHEDULED;

	// unlock the grounding requests queue
	UnlockConceptGroundingRequestsQueue();

	// return the action name
	return sActionName;
}

//-----------------------------------------------------------------------------
// D: Methods for manipulating the queue of concept grounding requests
//-----------------------------------------------------------------------------

// D: Lock the grounding requests queue
void CGroundingManagerAgent::LockConceptGroundingRequestsQueue()
{
	bLockedGroundingRequests = true;
}

// D: Unlock the grounding requests queue
void CGroundingManagerAgent::UnlockConceptGroundingRequestsQueue()
{
	bLockedGroundingRequests = false;
}

// D: Set the grounding request state
// D：设置接地请求状态
void CGroundingManagerAgent::SetConceptGroundingRequestStatus(CConcept* pConcept, int iAGroundingRequestStatus)
{
	// get the index of that concept grounding request
	int iIndex = getConceptGroundingRequestIndex(pConcept);
	// now check that it exists
	if (iIndex == -1)
		FatalError(FormatString(
		"Could not set grounding request status on concept %s.",
		pConcept->GetAgentQualifiedName().c_str()));
	/*
	// D: type describing a concept grounding request
	// D：描述概念接受请求的类型
	#define GRS_UNPROCESSED 0		// the unprocessed status for a grounding request	未处理态
	#define GRS_PENDING     1		// the pending status for a grounding request		未下最终论断
	#define GRS_READY       2       // the ready status for a grounding request			准备态
	#define GRS_SCHEDULED   3		// the scheduled status for a grounding request		调度态
	#define GRS_EXECUTING   5		// the executing status for a grounding request		执行态
	#define GRS_DONE        6   	// the completed status for a grounding request		完成态
	*/
	// if it exists, set the status
	// 设置当前接地请求的接地状态
	vcgrConceptGroundingRequests[iIndex].iGroundingRequestStatus = iAGroundingRequestStatus;
	// and log this 
	Log(GROUNDINGMANAGER_STREAM,
		"Setting concept grounding request for %s to [%s]",
		pConcept->GetAgentQualifiedName().c_str(),
		vsGRS[iAGroundingRequestStatus]);
}

// Get the grounding request state for a concept
// 返回concept接地操作的state
int CGroundingManagerAgent::GetConceptGroundingRequestStatus(CConcept* pConcept)
{
	// get the index of that concept grounding request
	int iIndex = getConceptGroundingRequestIndex(pConcept);
	// now check that it exists
	if (iIndex == -1)
	{
		Log(WARNING_STREAM, "Could not get grounding request status on concept %s (not in the queue).", pConcept->GetAgentQualifiedName().c_str());
		return -1;
	}
	else
	{
		// if it exists, get the status
		return vcgrConceptGroundingRequests[iIndex].iGroundingRequestStatus;
	}
}

// D: Signal that a concept grounding request has completed
// D：表示概念接受请求已完成的信号
void CGroundingManagerAgent::ConceptGroundingRequestCompleted(CConcept* pConcept)
{
	// get the index of that concept grounding request
	int iIndex = getConceptGroundingRequestIndex(pConcept);
	// if it exists in the queue, and it was currently executing
	if ((iIndex != -1) && (vcgrConceptGroundingRequests[iIndex].iGroundingRequestStatus == GRS_EXECUTING))
	{

		// if the queue is locked, just mark it as done
		if (bLockedGroundingRequests)
			vcgrConceptGroundingRequests[iIndex].iGroundingRequestStatus = GRS_DONE;
		// o/w remove it altogether
		else
			vcgrConceptGroundingRequests.erase(vcgrConceptGroundingRequests.begin() + iIndex);
	}
}

// D: remove a grounding request
// D：删除concept接地请求
void CGroundingManagerAgent::RemoveConceptGroundingRequest(CConcept* pConcept)
{

	// get the index
	int iIndex = getConceptGroundingRequestIndex(pConcept);

	// if it's in the list
	if (iIndex != -1)
	{
		// if the queue is locked, just mark it as done
		if (bLockedGroundingRequests)
			vcgrConceptGroundingRequests[iIndex].iGroundingRequestStatus = GRS_DONE;
		else 
			vcgrConceptGroundingRequests.erase(vcgrConceptGroundingRequests.begin() + iIndex);
	}
}

// D: Purge the list of grounding requests
// D：清除接地请求的列表
void CGroundingManagerAgent::PurgeConceptGroundingRequestsQueue()
{
	/*
	// D: type describing a concept grounding request
	// D：描述概念接受请求的类型
		#define GRS_UNPROCESSED 0		// the unprocessed status for a grounding request	未处理态
		#define GRS_PENDING     1		// the pending status for a grounding request		未下最终论断
		#define GRS_READY       2       // the ready status for a grounding request			准备态
		#define GRS_SCHEDULED   3		// the scheduled status for a grounding request		调度态
		#define GRS_EXECUTING   5		// the executing status for a grounding request		执行态
		#define GRS_DONE        6   	// the completed status for a grounding request		完成态
	*/
	bool bSomethingPurged = true;
	while (bSomethingPurged)
	{
		bSomethingPurged = false;
		for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)
		if (vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_DONE)
		{
			vcgrConceptGroundingRequests.erase(vcgrConceptGroundingRequests.begin() + i);
			bSomethingPurged = true;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// D: Methods for setting and accessing grounding actions history information
//-----------------------------------------------------------------------------

// D: adds a grounding action history item to the current history
//将操作添加到历史记录
void CGroundingManagerAgent::GAHAddHistoryItem(string sGroundingModelName,
	string sActionName, int iGroundingActionType)
{

	// construct the grounding action history item
	TGroundingActionHistoryItem gahi;
	gahi.iGroundingActionType = iGroundingActionType;
	gahi.sActionName = sActionName;
	gahi.sGroundingModelName = sGroundingModelName;
	gahi.bBargeIn = false;

	// add it to the last input
	int iTurnNumber = pDMCore->GetLastInputTurnNumber();
	vgahiGroundingActionsHistory.resize(iTurnNumber + 1);
	vgahiGroundingActionsHistory[iTurnNumber].push_back(gahi);
}

// D: sets the barge-in flag on the specified groudning action history item
void CGroundingManagerAgent::GAHSetBargeInFlag(int iTurnNum, bool bBargeInFlag)
{
	if ((iTurnNum >= 0) && (iTurnNum < (int)vgahiGroundingActionsHistory.size()))
	{
		for (unsigned int i = 0;
			i < vgahiGroundingActionsHistory[iTurnNum].size(); i++)
		{
			vgahiGroundingActionsHistory[iTurnNum].operator[](i).bBargeIn =
				bBargeInFlag;
		}
	}
	else
	{
		// o/w we need to extend the grounding actions history
		TGroundingActionHistoryItem gahi;
		gahi.iGroundingActionType = GAT_NONE;
		gahi.sActionName = "";
		gahi.sGroundingModelName = "";
		gahi.bBargeIn = bBargeInFlag;
		vgahiGroundingActionsHistory.resize(iTurnNum + 1);
		vgahiGroundingActionsHistory[iTurnNum].push_back(gahi);
	}
}

// D: obtains the turn grounding action performed in a certain turn
string CGroundingManagerAgent::GAHGetTurnGroundingAction(int iTurnNumber)
{
	string sActionName = "NO_ACTION";
	if ((iTurnNumber >= 1) &&
		(iTurnNumber < (int)vgahiGroundingActionsHistory.size()))
	{
		TGroundingActionHistoryItems gahis =
			vgahiGroundingActionsHistory[iTurnNumber];
		// go through these and find
		for (unsigned int i = 0; i < gahis.size(); i++)
		{
			if (gahis[i].iGroundingActionType == GAT_TURN)
			{
				sActionName = gahis[i].sActionName;
				break;
			}
		}
	}
	// finally, return
	return sActionName;
}

// D: counts how many times an action was taken in the last N turns
int CGroundingManagerAgent::GAHCountTakenInLastNTurns(bool bAlsoHeard,
	string sActionName, int iNumTurns)
{

	// if the number of turns is not specified, set it to all 
	if (iNumTurns == -1)
		iNumTurns = vgahiGroundingActionsHistory.size();

	// store the counter
	int iCounter = 0;

	// go through the last N actions
	int iStartTurn = vgahiGroundingActionsHistory.size() - iNumTurns;
	if (iStartTurn < 1) { iStartTurn = 1; }
	for (unsigned int t = iStartTurn;
		t < vgahiGroundingActionsHistory.size(); t++)
	{
		TGroundingActionHistoryItems gahis =
			vgahiGroundingActionsHistory[t];
		// go through these and find
		for (unsigned int i = 0; i < gahis.size(); i++)
		{
			if ((gahis[i].iGroundingActionType == GAT_TURN) &&
				((gahis[i].sActionName == sActionName) || (sActionName == "*")) &&
				(!(gahis[i].bBargeIn) || !bAlsoHeard))
			{
				iCounter++;
				break;
			}
		}
	}
	// finally, return the counter
	return iCounter;
}

// D: counts how many times an action was taken from a particular grounding
//    model    
int CGroundingManagerAgent::GAHCountTakenByGroundingModelInLastNTurns(
	bool bAlsoHeard, string sActionName, string sGroundingModelName,
	int iNumTurns)
{

	// if the number of turns is not specified, set it to all 
	if (iNumTurns == -1)
		iNumTurns = vgahiGroundingActionsHistory.size();

	// store the counter
	int iCounter = 0;

	// go through the last N actions
	int iStartTurn = vgahiGroundingActionsHistory.size() - iNumTurns;
	if (iStartTurn < 1) { iStartTurn = 1; }

	for (unsigned int t = iStartTurn;
		t < vgahiGroundingActionsHistory.size(); t++)
	{
		TGroundingActionHistoryItems gahis =
			vgahiGroundingActionsHistory[t];
		// go through these and find
		for (unsigned int i = 0; i < gahis.size(); i++)
		{
			if ((gahis[i].iGroundingActionType == GAT_TURN) &&
				((gahis[i].sActionName == sActionName) || (sActionName == "*")) &&
				(gahis[i].sGroundingModelName == sGroundingModelName) &&
				(!(gahis[i].bBargeIn) || !bAlsoHeard))
			{
				iCounter++;
				break;
			}
		}
	}
	// finally, return the counter
	return iCounter;
}

//-----------------------------------------------------------------------------
// D: Method for registering and creating various grounding model types
//-----------------------------------------------------------------------------

// D: register a grounding model type
// D：注册接地模型类型
void CGroundingManagerAgent::RegisterGroundingModelType(string sName, FCreateGroundingModel fctCreateGroundingModel)
{
	/*
		concept_default			= >		 CGMConcept::GroundingModelFactory
		request_default			= >		 CGMRequestAgent::GroundingModelFactory
		request_handcrafted		= >		 CGMRequestAgent_HandCrafted::GroundingModelFactory
		request_lr				= >		 CGMRequestAgent_LR::GroundingModelFactory
		request_numnonu			= >		 CGMRequestAgent_NumNonu::GroundingModelFactory
		request_experiment		= >		 CGMRequestAgent_Experiment::GroundingModelFactory
	*/
	// insert it in the registry
	// 将其插入到注册表中
	gmthGroundingModelTypeRegistry.insert(TGroundingModelsTypeHash::value_type(sName, fctCreateGroundingModel));
}

// D: create a grounding model for a given type
// D：为给定type创建一个接地模型
/*
sModelType = concept_default [GMConcept]
sModelPolicy = ‘expl’, ‘expl_impl’, ..
*/
CGroundingModel* CGroundingManagerAgent::CreateGroundingModel(string sModelType, string sModelPolicy)
{
	// find the factory method for that model type
	// 找到该模型类型的工厂方法
	TGroundingModelsTypeHash::iterator iPtr;
	// sModelType = 'GMConcept' , 'GMReqeustAgent', 'GMRequestAgent_LR', 'GMRequestAgent_Experment', ...猜的？
	// sModelType = 'concept_default', 'request_default', 'request_lr', 'request_experiment', ...
	if ((iPtr = gmthGroundingModelTypeRegistry.find(sModelType)) !=	gmthGroundingModelTypeRegistry.end())
	{
		// we found the factory, so use it
		// sModelPolicy = 策略名  ‘expl’, ‘expl_impl’, [ ‘request_default’, ‘request_lr’...]
		return (*(iPtr->second))(sModelPolicy);
	}
	else
	{
		// o/w issue a fatal error
		FatalError(FormatString("Could not find grounding model type %s.", sModelType.c_str()));
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// D: Methods for accessing the state of the grounding management layer
//-----------------------------------------------------------------------------

// D: returns true if the grounding engine has been signaled
// D：如果接地引擎已接收到信号，则返回true
bool CGroundingManagerAgent::HasPendingRequests()
{
	return HasPendingTurnGroundingRequest() ||
		HasPendingConceptGroundingRequests();
}

// D: returns true if there is a pending turn grounding request
// 如果有pending turn grouding请求返回true
bool CGroundingManagerAgent::HasPendingTurnGroundingRequest()
{
	return bTurnGroundingRequest;
}

// D: returns true if there are pending (UNPROCESSED or PENDING) concept 
//    grounding requests
// D：如果存在未决（未处理 或 未下最终论断）概念接受请求，则返回true
/*
// D: type describing a concept grounding request
// D：描述概念接受请求的类型
	#define GRS_UNPROCESSED 0		// the unprocessed status for a grounding request	未处理态
	#define GRS_PENDING     1		// the pending status for a grounding request		未下最终论断
	#define GRS_READY       2       // the ready status for a grounding request			准备态
	#define GRS_SCHEDULED   3		// the scheduled status for a grounding request		调度态
	#define GRS_EXECUTING   5		// the executing status for a grounding request		执行态
	#define GRS_DONE        6   	// the completed status for a grounding request		完成态
*/
bool CGroundingManagerAgent::HasPendingConceptGroundingRequests()
{
	//  go through the list    
	for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)
	{
		if ((vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_UNPROCESSED) //未处理
			||
			(vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_PENDING))	//未下最终论断
			return true;
	}
	return false;
}

// D: determines if there are unprocessed concept grounding requests
// D：确定是否存在未处理的概念接收请求
bool CGroundingManagerAgent::HasUnprocessedConceptGroundingRequests()
{
	//  go through the list    
	for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)
	{
		if (vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_UNPROCESSED)
			return true;
	}
	return false;
}


// D: determines if there are scheduled requests
// D：确定是否有 调度请求
bool CGroundingManagerAgent::HasScheduledConceptGroundingRequests()
{
	//  go through the list
	for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)
	{
		if (vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_SCHEDULED) //当前概念接地请求是否处于 “调度态”
			return true;
	}
	return false;
}

// D: determines if there are executing requests
bool CGroundingManagerAgent::HasExecutingConceptGroundingRequests()
{
	//  go through the list
	for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)
	{
		if (vcgrConceptGroundingRequests[i].iGroundingRequestStatus ==
			GRS_EXECUTING)
			return true;
	}
	return false;
}

// D: determines if a concept is undergoing grounding 
// D：确定概念是否正在接地
bool CGroundingManagerAgent::GroundingInProgressOnConcept(CConcept* pConcept)
{
	return (getConceptGroundingRequestIndex(pConcept) != -1);
}

// D: check if there is a scheduled action for a concept
string CGroundingManagerAgent::GetScheduledGroundingActionOnConcept(CConcept* pConcept)
{
	// find the request
	int iIndex = getConceptGroundingRequestIndex(pConcept);
	// check that it exists
	if (iIndex == -1) return "";
	// check that it has a scheduled status
	if (vcgrConceptGroundingRequests[iIndex].iGroundingRequestStatus != GRS_SCHEDULED)
		return "";
	// return the action
	return GroundingActionIndexToName(
		vcgrConceptGroundingRequests[iIndex].iSuggestedActionIndex);
}



//-----------------------------------------------------------------------------
// D: Run Method
//-----------------------------------------------------------------------------

// D: Run the grounding management process 
// D：运行接地管理过程
#pragma warning (disable:4706)
void CGroundingManagerAgent::Run()
{

	// and log it
	Log(GROUNDINGMANAGER_STREAM, "Running grounding process ...");

	//		if we are grounding a user turn turn Grouding
	// <1>	Grouding类型：turn Grouding
	if (bTurnGroundingRequest)// Grouding类型：turn Grouding
	{
		  
		// log the grounding of the last user turn
		Log(GROUNDINGMANAGER_STREAM, "Grounding last user turn ...");

		//		get the focused agent from the previous user input
		// <1>	从上一个用户输入获取焦点agent
		CDialogAgent* pdaFocusedAgent = (CDialogAgent *)
			AgentsRegistry[pStateManager->GetLastState().sFocusedAgentName];//从StateManager获取最后state -> 从state获取Focus Agent name

		//		and check that it still exists okay
		// <2>	检测焦点agent是否存在
		if (!pdaFocusedAgent)
		{
			// if the last focused agent is not to be found anymore signal a fatal error
			FatalError(
				FormatString("Could not locate the last focused agent: %s.",
				pStateManager->GetLastState().sFocusedAgentName.c_str()));
		}

		//		first, check whether or not the last turn was a non-understanding
		// <3>	首先，检查最后一轮Turn是否是不理解  non-understanding【Turn Grouding】
		if (pDMCore->LastTurnNonUnderstanding())
		{
			// if the last turn was a non-understanding
			Log(GROUNDINGMANAGER_STREAM, "Last turn: non-understanding.");

			// log which agent we're running
			Log(GROUNDINGMANAGER_STREAM,
				"Running grounding model for agent: %s.",
				pdaFocusedAgent->GetName().c_str());

			//		check that the focused agent indeed has a grounding model
			// <4>	检查所[焦点agent]是否具有接地模型
			CGroundingModel *pgmGroundingModel;
			if (pgmGroundingModel = pdaFocusedAgent->GetGroundingModel())//如果有GroudingModel
			{
				//################################计算状态 和 动作############################################
				//		take the focused agent on the stack, and compute it's state,
				// <5>	将焦点agent放在堆栈上，并计算它的状态，
				pgmGroundingModel->ComputeState();
				//		compute the suggested action index
				// <6>	计算建议的Action索引[全局index]
				int iActionIndex = pgmGroundingModel->ComputeSuggestedActionIndex();
				//################################计算状态 和 动作############################################

				//		and log the state and the suggested action
				// <7>	并记录状态和建议的操作
				pgmGroundingModel->LogStateAction();
				Log(GROUNDINGMANAGER_STREAM,
					"Executing last focused agent grounding action:\n%s <- %s",
					GroundingActionIndexToName(iActionIndex).c_str(),
					pdaFocusedAgent->GetName().c_str());
				//		add the action to history
				// <8>	将操作添加到历史记录
				GAHAddHistoryItem(pgmGroundingModel->GetName(), GroundingActionIndexToName(iActionIndex), GAT_TURN);

				//		now run the action
				//<9>	现在运行动作
				//########################################执行 接地 Action#######################################################
				pgmGroundingModel->RunAction(iActionIndex);
				//########################################执行 接地 Action#######################################################

				// indicate that the grounding process is completed
				Log(GROUNDINGMANAGER_STREAM,
					"Grounding last user turn completed.");

				//		set the signal back to false
				// <10>	将信号bTurnGroundingRequest设置为false
				bTurnGroundingRequest = false;

				// now we're not going to continue with grounding the 
				// concepts, so simply return but record the completion 
				// of the grounding process
				//现在我们不会继续接地的概念，所以暗示返回，但记录完成的接地过程
				Log(GROUNDINGMANAGER_STREAM, "Grounding process completed.");
				return;

			}//if (pgmGroundingModel = pdaFocusedAgent->GetGroundingModel())//如果有GroudingModel
			else
			{
				// if the focused agent does not have a grounding model
				// 如果焦点的代理没有接地模型
				Log(GROUNDINGMANAGER_STREAM,
					"Last focused agent %s does not have a grounding "
					"model. No grounding action performed.",
					pdaFocusedAgent->GetName().c_str());
			}

		}//if (pDMCore->LastTurnNonUnderstanding()) 首先，检查最后一轮是否是不理解  non-understanding【Turn Grouding】
		else
		{
			// o/w/ notify that the turn was grounded 
			Log(GROUNDINGMANAGER_STREAM, "Last turn: successful bindings.");
		}

		// indicate that the grounding process is completed
		Log(GROUNDINGMANAGER_STREAM,
			"Grounding last user turn completed.");

		//		set the signal back to false
		//<11>	将bTurnGroundingRequest设置为false
		bTurnGroundingRequest = false;
	}// Grouding类型：turn Grouding

	//		first purge the queue of completed agents
	// <11>	首先清除已完成代理的队列
	PurgeConceptGroundingRequestsQueue();

	//		now do the concept-level grounding, if there's anything to  ground
	// <12>	现在做concept级的接地，如果有什么地面
	//		如果焦点Agent是DTS, 并且concept接地队列非空
	if (pDMCore->GetAgentInFocus()->IsDTSAgent() && !vcgrConceptGroundingRequests.empty())
	{

		// log that we are grounding concepts
		Log(GROUNDINGMANAGER_STREAM, "Grounding concepts ...");

		//		start by locking the grounding requests queue
		// <13>	通过锁定接地请求队列  - 开始
		LockConceptGroundingRequestsQueue();

		// go through all the concepts in the list, and compute their
		// state and suggested action (also construct the string dump in parallel)
		// <14>	遍历列表中的所有概念，并计算它们的状态和建议的操作（也并行构造字符串转储）
		string sCGRDump;
		for (int i = vcgrConceptGroundingRequests.size() - 1; i >= 0; i--)
		{
			/*
			typedef struct
			{
				CConcept* pConcept;				//概念
				int iGroundingRequestStatus;	//接地请求状态
				int iSuggestedActionIndex;		//建议动作索引
				int iTurnNumber;				//turn id
			} TConceptGroundingRequest;

				// D：描述概念接受请求的类型
				#define GRS_UNPROCESSED 0		// the unprocessed status for a grounding request	未处理态
				#define GRS_PENDING     1		// the pending status for a grounding request		未下最终论断
				#define GRS_READY       2       // the ready status for a grounding request			准备态
				#define GRS_SCHEDULED   3		// the scheduled status for a grounding request		调度态
				#define GRS_EXECUTING   5		// the executing status for a grounding request		执行态
				#define GRS_DONE        6   	// the completed status for a grounding request		完成态
			*/
			CConcept* pConcept = vcgrConceptGroundingRequests[i].pConcept;
			// <15>	如果接地状态为：GRS_UNPROCESSED 或者 GRS_PENDING
			if ((vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_UNPROCESSED) ||
				(vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_PENDING))
			{
				// set the state to pending
				vcgrConceptGroundingRequests[i].iGroundingRequestStatus = GRS_PENDING;
				CGroundingModel* pGroundingModel = pConcept->GetGroundingModel();
				// compute the state
				// 计算状态
				pGroundingModel->ComputeState();
				// compute the suggested action
				// 计算动作
				vcgrConceptGroundingRequests[i].iSuggestedActionIndex = pGroundingModel->ComputeSuggestedActionIndex();
				// and construct the dump string
				//并构造转储字符串
				sCGRDump += FormatString("[PENDING]   %s (turn=%d)\n",
					pConcept->GetAgentQualifiedName().c_str(),
					vcgrConceptGroundingRequests[i].iTurnNumber);
			}
			else if (vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_SCHEDULED)
			{
				// just construct the dump string
				// 只构造转储字符串
				sCGRDump += FormatString("[SCHEDULED] %s (turn=%d)\n",
					pConcept->GetAgentQualifiedName().c_str(),
					vcgrConceptGroundingRequests[i].iTurnNumber);
			}
			else if (vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_EXECUTING)
			{
				// just construct the dump string
				// 并构造转储字符串
				sCGRDump += FormatString("[EXECUTING] %s (turn=%d)\n",
					pConcept->GetAgentQualifiedName().c_str(),
					vcgrConceptGroundingRequests[i].iTurnNumber);
			}
			else
			{
				FatalError(FormatString(
					"Invalid concept grounding request in queue (state: %d).",
					vcgrConceptGroundingRequests[i].iGroundingRequestStatus));
			}
		}

		// now log the concept grounding requests dump
		// 现在记录概念接受请求转储
		Log(GROUNDINGMANAGER_STREAM,
			"Concept grounding requests dumped below:\n%s",
			sCGRDump.c_str());

		// now log the models
		// 记录model
		for (int i = vcgrConceptGroundingRequests.size() - 1; i >= 0; i--)
		{
			if (vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_PENDING)
			{
				vcgrConceptGroundingRequests[i].pConcept->GetGroundingModel()->LogStateAction();
			}
		}

		// now execute the actions, making sure that no more than 1 EC
		// and 2 ICs are running at the same time
		// 现在执行接地Action，确保不超过1个EC和2个IC同时运行
		// start by counting how many we already have scheduled
		int iScheduledExplicitConfirms = 0;
		int iScheduledImplicitConfirms = 0;
		for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)
		{
			/*
			typedef struct
			{
				CConcept* pConcept;				//概念
				int iGroundingRequestStatus;	//接地请求状态
				int iSuggestedActionIndex;		//建议动作索引
				int iTurnNumber;				//turn id
			} TConceptGroundingRequest;
			*/
			TConceptGroundingRequest cgr = vcgrConceptGroundingRequests[i];
			if ((cgr.iGroundingRequestStatus == GRS_SCHEDULED) ||
				(cgr.iGroundingRequestStatus == GRS_EXECUTING))
			{
				/*
								 ACCEPT     EXPL_CONF    IMPL_CONF
								 INACTIVE           10            -            -
								 CONFIDENT           8           -5            0
								 UNCONFIDENT       -19           10            5
								 GROUNDED           10            -            -
								 */
				// get the action name
				// 获取接地Action名称
				string sActionName = GroundingActionIndexToName(cgr.iSuggestedActionIndex);
				if (sActionName == "EXPL_CONF")
					iScheduledExplicitConfirms++;
				else if (sActionName == "IMPL_CONF")
					iScheduledImplicitConfirms++;
			}
		}//for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)

		// now go through them and set them to scheduled
		//现在通过它们并将它们设置为调度态
		for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)
		{
			// look at the pending ones
			if (vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_PENDING)
			{
				string sActionName = GroundingActionIndexToName(vcgrConceptGroundingRequests[i].iSuggestedActionIndex);
				if ((sActionName == "EXPL_CONF") && (iScheduledExplicitConfirms == 0))
				{
					// the explicit confirm case
					vcgrConceptGroundingRequests[i].iGroundingRequestStatus = GRS_SCHEDULED;
					iScheduledExplicitConfirms++;
				}
				else if ((sActionName == "IMPL_CONF")/* && (iScheduledImplicitConfirms < 2)*/)
				{
					// the implicit confirm case
					vcgrConceptGroundingRequests[i].iGroundingRequestStatus = GRS_SCHEDULED;
					iScheduledImplicitConfirms++;
				}
				else if (sActionName == "ACCEPT")
				{
					// the accept case
					vcgrConceptGroundingRequests[i].iGroundingRequestStatus = GRS_SCHEDULED;
				}
			}
		}//for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)

		// now, go through the scheduled actions, and execute all the accepts
		// and one other action (giving preference to implicit confirms over
		// explicit confirms)      
		// 现在，遍历scheduled actions, 并执行所有的accepts和一个其他动作（隐式确认优先，显式确认）
		int iActionsTaken = 0;
		for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)
		{
			// look at the ready ones
			if (vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_SCHEDULED)
			{
				string sActionName = GroundingActionIndexToName(vcgrConceptGroundingRequests[i].iSuggestedActionIndex);
				CConcept* pConcept = vcgrConceptGroundingRequests[i].pConcept;
				if ((sActionName == "ACCEPT") ||
					((iActionsTaken == 0) && (sActionName == "IMPL_CONF")) ||
					((iActionsTaken == 0) && (iScheduledImplicitConfirms == 0) &&
					(sActionName == "EXPL_CONF")))
				{
					// log the action we're about to take
					// 记录我们要采取的行动
					Log(GROUNDINGMANAGER_STREAM,
						"Executing concept grounding action:\n%s <- %s",
						sActionName.c_str(),
						pConcept->GetAgentQualifiedName().c_str());

					// seal the concept
					//密封concept
					pConcept->Seal();

					// increment the actions taken if we just did an explicit or implicit confirm
					// 增加所做的action计数，如果我们只是做了一个显式或隐式确认
					if ((sActionName == "IMPL_CONF") || (sActionName == "EXPL_CONF"))
						iActionsTaken++;
					// add the action to history
					//将action添加到历史记录
					GAHAddHistoryItem(pConcept->GetGroundingModel()->GetName(),
						GroundingActionIndexToName(
						vcgrConceptGroundingRequests[i].iSuggestedActionIndex),
						GAT_CONCEPT);
					// and finally, execute the action
					//最后，执行接地Action
					pConcept->GetGroundingModel()->RunAction(vcgrConceptGroundingRequests[i].iSuggestedActionIndex);
				}
			}//if (vcgrConceptGroundingRequests[i].iGroundingRequestStatus == GRS_SCHEDULED)
		}//for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)

		// unlock the grounding requests queue
		//解锁接地请求队列
		UnlockConceptGroundingRequestsQueue();

		// and purge it
		// D: Purge the list of grounding requests
		////首先清除已完成代理的队列
		PurgeConceptGroundingRequestsQueue();

		// log that we are done grounding concepts
		Log(GROUNDINGMANAGER_STREAM, "Grounding concepts completed.");
	}//现在做concept级的接地，如果有什么地面

	// log that the whole grounding process has completed
	Log(GROUNDINGMANAGER_STREAM, "Grounding process completed.");
}// D：运行接地管理过程


#pragma warning (default:4706)

//-----------------------------------------------------------------------------
// D: Private auxiliary methods
//-----------------------------------------------------------------------------

// D: Return the index of a concept grounding request, or -1 if not found
// D：返回概念接收请求的索引，如果找不到，则返回-1
int CGroundingManagerAgent::getConceptGroundingRequestIndex(CConcept* pConcept)
{
	//  go through the list    
	for (int i = 0; i < (int)vcgrConceptGroundingRequests.size(); i++)
	{
		if (vcgrConceptGroundingRequests[i].pConcept == pConcept)
			return i;
	}
	// o/w return -1
	return -1;
}

// A: Load a policy from its description file
// 从文件中加载policy
string CGroundingManagerAgent::loadPolicy(string sFileName)
{
	// try to open the file
	FILE *fidModel = fopen(sFileName.c_str(), "r");
	if (!fidModel)
	{
		Log(GROUNDINGMANAGER_STREAM, "Could not read grounding model "\
			"from %s .", sFileName.c_str());
		return "";
	}

	/*
		# The exploration_mode parameter can take on the values: 
		#  greedy, epsilon-greedy, soft-max, stochastic

		EXPLORATION_MODE=greedy
		EXPLORATION_PARAMETER=0.2

						 ACCEPT     EXPL_CONF    IMPL_CONF
		INACTIVE           10            -            -
		CONFIDENT           8           -5            0
		UNCONFIDENT       -19           10            5
		GROUNDED           10            -            -
	*/
	// o/w we're fine, so read the model data from the file
	char lpszLine2[STRING_MAX];
	string sModelData;
	while (fgets(lpszLine2, STRING_MAX, fidModel) != NULL)
	{
		sModelData += lpszLine2;
	}
	// close the file
	fclose(fidModel);

	// and now log that we successfully loaded it
	Log(GROUNDINGMANAGER_STREAM, "Loaded grounding model from %s .",
		sFileName.c_str());

	return sModelData;
}

