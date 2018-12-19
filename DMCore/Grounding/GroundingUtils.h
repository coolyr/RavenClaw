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
// GROUNDINGUTILS.H   - definitions of the CState and CBeliefDistribution 
//                      classes used to model the state and the abstracted
//                      or aggregated state of a grounding model
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
//   [2004-02-24] (dbohus): added CState
//   [2004-02-10] (dbohus): changed so that belief distribution can have 
//                           invalid events
//   [2004-02-09] (dbohus): added epsilon-greedy and soft-max selection
//   [2003-02-12] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#pragma once
#ifndef __GROUNDINGUTILS_H__
#define __GROUNDINGUTILS_H__

#include "../../Utils/Utils.h"

// D: define the invalid event probability (these events are not considered
//    when operating with a probability distribution)
// D：定义无效事件概率（当使用概率分布操作时不考虑这些事件）
#define INVALID_EVENT ((float)-1000000.203040)

//-----------------------------------------------------------------------------
// D: CState - models a set of state variables describing the full state of a
//             grounding model
//D：CState - 模型化一组描述接地模型的完整状态的状态变量
//-----------------------------------------------------------------------------
class CState
{

private:
	//---------------------------------------------------------------------
	// private class members
	//---------------------------------------------------------------------

	// the map holding the values for the state variables		状态 -> 值 hash表
	STRING2STRING s2sStateVars;

public:
	//---------------------------------------------------------------------
	// public class members
	//---------------------------------------------------------------------

	// indexing operator
	string& operator[] (string sIndex);

	// clear the state
	void Clear();

	// adding to the state from another STRING2STRING hash
	void Add(STRING2STRING s2s);

	// string conversion function
	string ToString();
};

//-----------------------------------------------------------------------------
// D: CBeliefDistribution - models a probability distribution over a set of events
// D：CBeliefDistribution - 对一组事件的概率分布建模
//-----------------------------------------------------------------------------
class CBeliefDistribution
{

private:
	//---------------------------------------------------------------------
	// private class members
	//---------------------------------------------------------------------

	// the vector of probabilities for each event (action or state, or anything else
	// 每个事件的概率向量（动作或状态，或任何其他
	vector<float> vfProbability;

	// and the lower and upper bounds for the confidence interval on that 
	// vector
	// 该概率向量的置信区间的下限和上限
	vector<float> vfProbabilityLowBound;
	vector<float> vfProbabilityHiBound;

public:
	//---------------------------------------------------------------------
	// Constructors and destructors
	//---------------------------------------------------------------------
	//
	CBeliefDistribution(int iNumEvents = 0);
	CBeliefDistribution(CBeliefDistribution& rbdABeliefDistribution);
	~CBeliefDistribution();

	// Resize the event space for the distribution 
	//
	void Resize(int iNumEvents);

	//---------------------------------------------------------------------
	// Access 
	//---------------------------------------------------------------------

	// Access to probabilities, via the [] operator
	//
	float& operator[] (unsigned int iIndex);

	// Access to bounds
	// 
	float& LowBound(unsigned int iIndex);
	float& HiBound(unsigned int iIndex);

	// Obtain the number of valid events
	//
	int GetValidEventsNumber();

	//---------------------------------------------------------------------
	// Various functions for transforming the distribution
	//---------------------------------------------------------------------

	// Normalizes the distribution (so that the probabilities sum up to 1)
	//
	void Normalize();

	// Sharpens the distribution (by raising it to the power fPower and 
	// normalizing)
	//
	void Sharpen(float fPower);

	//---------------------------------------------------------------------
	// Various functions for choosing a particular event from the 
	// distribution
	//---------------------------------------------------------------------

	// Choose the event with the highest probability/utility
	//
	int GetModeEvent();

	// Choose the highest upper bound event
	// 
	int GetMaxHiBoundEvent();

	// Choose an event according to an epsilon-greedy policy
	//
	int GetEpsilonGreedyEvent(float fEpsilon);

	// Choose an event according to a soft-max policy
	//
	int GetSoftMaxEvent(float fTemperature);

	// Randomly draw an event from the given probability/utility 
	// distribution
	//
	int GetRandomlyDrawnEvent();

};

#endif // __GROUNDINGUTILS_H__