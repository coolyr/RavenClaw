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
// GMCONCEPT.CPP - implementation of the CGMConcept grounding model class for
//                 concept grouding
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
//   [2004-02-24] (dbohus): addeded support for full state and collapsed state
//   [2004-02-09] (dbohus): changed model data to "policy"
//   [2003-04-02] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#include "GMConcept.h"
#include "../../../DMCore/Concepts/Concept.h"
#include "../../../DMCore/Log.h"

//-----------------------------------------------------------------------------
// D: Constructors, destructors
//-----------------------------------------------------------------------------

// D: default constructor, just calls upon the CGroundingModel constructor
CGMConcept::CGMConcept(string sAModelPolicy, string sAName) :
CGroundingModel(sAModelPolicy, sAName)
{
	pConcept = NULL;
	// this model has 4 states
	bdBeliefState.Resize(4);
}

// D: constructor from reference
CGMConcept::CGMConcept(CGMConcept& rAGMConcept)
{
	sModelPolicy = rAGMConcept.sModelPolicy;
	sName = rAGMConcept.sName;
	pPolicy = rAGMConcept.pPolicy;
	viActionMappings = rAGMConcept.viActionMappings;
	sExplorationMode = rAGMConcept.sExplorationMode;
	fExplorationParameter = rAGMConcept.fExplorationParameter;
	pConcept = rAGMConcept.pConcept;
	stFullState = rAGMConcept.stFullState;
	bdBeliefState = rAGMConcept.bdBeliefState;
}

// D: destructor - does nothing so far
CGMConcept::~CGMConcept()
{
}

//-----------------------------------------------------------------------------
// D: Grounding model factory method
//-----------------------------------------------------------------------------

CGroundingModel* CGMConcept::GroundingModelFactory(string sModelPolicy)
{
	return new CGMConcept(sModelPolicy);
}

//-----------------------------------------------------------------------------
// D: Member access methods
//-----------------------------------------------------------------------------

// D: return the type of the grounding model (as string)
string CGMConcept::GetType()
{
	return "concept_default";
}

// D: Set the name: block this method (the name is automatically set to the
//    agent-qualified path of the concept it grounds)
void CGMConcept::SetName(string sAName)
{
	// issue an error
	Error("Cannot perform SetName on a concept grounding model.");
}

// D: Get the name of the model
string CGMConcept::GetName()
{
	if (pConcept != NULL)
	{
		return pConcept->GetAgentQualifiedName();
	}
	else
	{
		return "UNKNOWN";
	}
}

// D: Set the concept handled
void CGMConcept::SetConcept(CConcept* pAConcept)
{
	pConcept = pAConcept;
}

// D: Get the concept handled
CConcept* CGMConcept::GetConcept()
{
	return pConcept;
}

//-----------------------------------------------------------------------------
// D: Grounding model specific methods
//-----------------------------------------------------------------------------

// D: Cloning the model
CGroundingModel* CGMConcept::Clone()
{
	return new CGMConcept(*this);
}

// D: Overwritten method for loading the model policy
bool CGMConcept::LoadPolicy()
{
	// first call the inherited LoadPolicy
	if (!CGroundingModel::LoadPolicy())
	{
		return false;
	}
	else if (!bExternalPolicy)
	{
		// then check that the model has the presumed state-space
		if (pPolicy.size() != 4)
		{
			FatalError(FormatString("Error in CGMConcept::LoadPolicy(). "\
				"Invalid state-space size for policy %s (4 states expected, "\
				"%d found).", sModelPolicy.c_str(), pPolicy.size()));
			return false;
		}
		else if ((pPolicy[0].sStateName != SS_INACTIVE) ||
			(pPolicy[1].sStateName != SS_CONFIDENT) ||
			(pPolicy[2].sStateName != SS_UNCONFIDENT) ||
			(pPolicy[3].sStateName != SS_GROUNDED))
		{
			FatalError("Error in CGMConcept::LoadPolicy(). Invalid "\
				"state-space.");
			return false;
		}
	}
	return true;
}

// D: Runs the action (also transmitting the concept the action is ran on
//    as a pointer)
// D：运行动作（把需要的concept传入进去）
void CGMConcept::RunAction(int iActionIndex)
{
	// obtains a pointer to the action from the grounding manager
	// and runs it tranmitting the concept as a parameter
	// 从接管管理器获取一个指向action的指针
	// 把需要的concept传入，当做参数
	pGroundingManager->operator [](iActionIndex)->
		Run((void *)pConcept);
}

// D: Log the state and the suggested action of the model
// D：记录模型的状态和建议的操作
void CGMConcept::LogStateAction()
{
	// dumps the current concept value (hyps)
	Log(GROUNDINGMODELX_STREAM, "Concept %s dumped below:\n%s",
		pConcept->GetAgentQualifiedName().c_str(),
		TrimRight(pConcept->HypSetToString(), "\n").c_str());
	// then call the inherited method
	CGroundingModel::LogStateAction();
}

// D: Compute the full state for this model
// D：计算此模型的完整状态
void CGMConcept::computeFullState()
{
	//		clear the full state
	// <1>	清除完整状态
	stFullState.Clear();
	//		set the updated state variable
	// <2>	设置更新的状态变量
	stFullState["updated"] = BoolToString(pConcept->IsUpdated());
	//		set the grounded state variable
	// <3>	设置接地状态变量
	stFullState["grounded"] = BoolToString(pConcept->IsGrounded());
	//		state the confidence state variable
	// <4>	状态置信状态变量
	stFullState["top_confidence"] = FloatToString(pConcept->GetTopHypConfidence());
}

// D: Compute the belief state for this model
// D：计算此模型的置信状态
void CGMConcept::computeBeliefState()
{

				/*

												ACCEPT     EXPL_CONF    IMPL_CONF
							  INACTIVE           10            -            -
							  CONFIDENT           8           -5            0
							  UNCONFIDENT       -19           10            5
							  GROUNDED           10            -            -
				*/

	// there are 4 states in this grounding model:
	// INACTIVE, CONFIDENT, UNCONFIDENT, GROUNDED

	//		the state is inactive if the concept was not updated (no current value since the last reopen)
	// <1>	如果概念未更新，则状态为非活动状态（自上次reopen()以来没有当前值）
	bdBeliefState[SI_INACTIVE] = (stFullState["updated"] == "false");  //[0, 1]

	//		the state is grounded if the concept is grounded already
	// <2>	如果概念已经接地，则状态是接地的
	bdBeliefState[SI_GROUNDED] = !bdBeliefState[SI_INACTIVE] && (stFullState["grounded"] == "true"); //[0, 1]

	//		the state is CONFIDENT to the extent to which we are confident
	// <3>	如果[概念被更新 & 没有被接地]   =>  设置为concept中的confidence
	bdBeliefState[SI_CONFIDENT] =
		(bdBeliefState[SI_INACTIVE] || bdBeliefState[SI_GROUNDED]) ?
		(float)0 : (float)atof(stFullState["top_confidence"].c_str()); //[confidence]

	// <4>	the state is UNCONFIDENT to the extent to which we are not confident
	bdBeliefState[SI_UNCONFIDENT] =
		(bdBeliefState[SI_INACTIVE] || bdBeliefState[SI_GROUNDED]) ?
		(float)0 : (1 - bdBeliefState[SI_CONFIDENT]);    //[1 - confidence]


	//		and invalidate the suggested action (not computed yet for this state)
	// <5>	并使所建议的动作无效（尚未针对该状态计算）
	iSuggestedActionIndex = -1;
}
