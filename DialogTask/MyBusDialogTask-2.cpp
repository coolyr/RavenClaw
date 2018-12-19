//=============================================================================
//
//   Copyright (c) 2000-2007, Carnegie Mellon University.  
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
// 
// OLYMPUS TUTORIAL 2
// 
// by Antoine Raux (antoine@cs.cmu.edu), 2007
//-----------------------------------------------------------------------------
// 
// MYBUSDIALOGTASK.CPP - This module contains the description of the dialog 
//                         task, (i.e. all the user defined agencies, etc, etc)
//                         for the MyBus system
// 
//-----------------------------------------------------------------------------


#include "DialogTask.h"

#define __IN_DIALOGTASK__

//-----------------------------------------------------------------------------
//
// DM GALAXY SERVER CONFIGURATION
//
//-----------------------------------------------------------------------------
#ifdef GALAXY
    DMSERVER_CONFIGURATION("DialogManager", 17000)
#endif

//-----------------------------------------------------------------------------
//
// CONSTANT DEFINITIONS
//
//-----------------------------------------------------------------------------

// query types ID number (goes into the "QueryType" concept)
#define NQ_NEW_REQUEST			1
#define NQ_NEXT_BUS				2
#define NQ_PREVIOUS_BUS			3

// return codes from the backend
#define RC_OKAY					0
#define RC_INTERNAL_ERROR		1
#define RC_NO_BUS_AFTER_THAT	2
#define RC_NO_BUS_BEFORE_THAT	3

//-----------------------------------------------------------------------------
//
// DIALOG CORE CONFIGURATION
//
//-----------------------------------------------------------------------------

CORE_CONFIGURATION(

	// declare the NLG and the GUI as output devices
	USE_OUTPUT_DEVICES(
		DEFAULT_OUTPUT_DEVICE("nlg", "nlg.launch_query", 1)
	)

)

//-----------------------------------------------------------------------------
//
// CONCEPT TYPES DEFINITION
//
//-----------------------------------------------------------------------------

// Result frame giving bus schedules
DEFINE_FRAME_CONCEPT_TYPE( CResultConcept,
    ITEMS(
	    INT_ITEM(failed)
		STRING_ITEM(route)
		INT_ITEM(departure_time)
		INT_ITEM(arrival_time)
	)
)

//-----------------------------------------------------------------------------
//
// AGENT SPECIFICATIONS
//
//-----------------------------------------------------------------------------
 
// /MyBus
DEFINE_AGENCY( CMyBus, 

	IS_MAIN_TOPIC()

	DEFINE_SUBAGENTS(
		SUBAGENT(GiveIntroduction, CGiveIntroduction, "")
        SUBAGENT(PerformTask, CPerformTask, "")
		SUBAGENT(GreetGoodbye, CGreetGoodbye, "")
	)

)

// /MyBus/GiveIntroduction
DEFINE_INFORM_AGENT( CGiveIntroduction,
	PROMPT(	"inform welcome")
)

// /MyBus/PerformTask
DEFINE_AGENCY( CPerformTask,

 	DEFINE_CONCEPTS(
		INT_USER_CONCEPT(query_type, "")
		STRING_USER_CONCEPT(origin_place, "")
		STRING_USER_CONCEPT(destination_place, "")
		
		CUSTOM_SYSTEM_CONCEPT(result, CResultConcept)
		CUSTOM_SYSTEM_CONCEPT(new_result, CResultConcept)
	)
    
	DEFINE_SUBAGENTS(
		SUBAGENT(GetQuerySpecs, CGetQuerySpecs, "")
		SUBAGENT(ProcessQuery, CProcessQuery, "")
		SUBAGENT(GiveResults, CGiveResults, "")
	)

)

// /MyBus/PerformTask/GetQuerySpecs
DEFINE_AGENCY( CGetQuerySpecs,
	
	DEFINE_SUBAGENTS(
		SUBAGENT(RequestQuery, CRequestQuery, "")
		SUBAGENT(RequestOriginPlace, CRequestOriginPlace, "")
		SUBAGENT(RequestDestinationPlace, CRequestDestinationPlace, "")
	)

)

// /MyBus/PerformTask/GetQuerySpecs/RequestQuery
DEFINE_REQUEST_AGENT( CRequestQuery,

	PROMPT("request query")

)

// /MyBus/PerformTask/GetQuerySpecs/RequestOriginPlace
DEFINE_REQUEST_AGENT( CRequestOriginPlace,

	PROMPT("request origin_place")
	REQUEST_CONCEPT(origin_place)
	GRAMMAR_MAPPING("[origin_place], ![Place]")

)

// /MyBus/PerformTask/GetQuerySpecs/RequestDestinationPlace
DEFINE_REQUEST_AGENT( CRequestDestinationPlace,

    PROMPT("request destination_place")
	REQUEST_CONCEPT(destination_place)
	GRAMMAR_MAPPING("[destination_place], ![Place]")

)

// /MyBus/PerformTask/ProcessQuery
DEFINE_AGENCY( CProcessQuery,
		
	DEFINE_SUBAGENTS(
		SUBAGENT(InformFirstProcessing, CInformFirstProcessing, "")
		SUBAGENT(InformSubsequentProcessing, CInformSubsequentProcessing, "")
		SUBAGENT(ExecuteBackendCall, CExecuteBackendCall, "")
	)

	SUCCEEDS_WHEN(
		SUCCEEDED(ExecuteBackendCall)
	)
)

// /MyBus/PerformTask/ProcessQuery/InformFirstProcessing
DEFINE_INFORM_AGENT( CInformFirstProcessing,
	PRECONDITION(!AVAILABLE(result))
	PROMPT("inform looking_up_database_first")
)

// /MyBus/PerformTask/ProcessQuery/InformSubsequentProcessing
DEFINE_INFORM_AGENT( CInformSubsequentProcessing,
	PRECONDITION(AVAILABLE(result))
	PROMPT("inform looking_up_database_subsequent")
)

// /MyBus/PerformTask/ProcessQuery/ExecuteBackendCall
DEFINE_EXECUTE_AGENT( CExecuteBackendCall,
	EXECUTE(
		if (!AVAILABLE(query_type)) {
			C("query_type") = NQ_NEXT_BUS;		
		}

	    // call on the galaxy stub agent to execute that particular call
	    pTrafficManager->Call(this, "gal_be.launch_query <query_type "
			                        "<origin_place <destination_place "
									"<result >new_result");

		C("result") = C("new_result");
	)
)

// /MyBus/PerformTask/GiveResults
DEFINE_AGENCY( CGiveResults,
	
	DEFINE_CONCEPTS(
		INT_USER_CONCEPT( next_query, "")
		BOOL_USER_CONCEPT( goodbye, "")
	)

	DEFINE_SUBAGENTS(
		SUBAGENT(InformSuccess, CInformSuccess, "")
		SUBAGENT(InformError, CInformError, "")
		SUBAGENT(RequestNextQuery, CRequestNextQuery, "")
		SUBAGENT(ExpectGoodbye, CExpectGoodbye, "")
		SUBAGENT(InformStartingNewQuery, CInformStartingNewQuery, "")
	)

	SUCCEEDS_WHEN(
		((int)C("next_query") == NQ_NEXT_BUS) ||
		((int)C("next_query") == NQ_PREVIOUS_BUS) ||
		SUCCEEDED(InformStartingNewQuery) ||
		IS_TRUE(goodbye)
	)

	ON_COMPLETION(
		if (((int)C("next_query") == NQ_NEXT_BUS) ||
			((int)C("next_query") == NQ_PREVIOUS_BUS)) {
			A("../ProcessQuery").ReOpenTopic();
			A("../GiveResults").ReOpenTopic();
			C("query_type") = (int)C("next_query");
			C("next_query").Clear();
		} else if ((int)C("next_query") == NQ_NEW_REQUEST) {
			A("/MyBus/PerformTask").Reset();
		}
	)
)

// /MyBus/PerformTask/GiveResults/InformSuccess
DEFINE_INFORM_AGENT( CInformSuccess,
	PRECONDITION((int)C("result.failed") == RC_OKAY)
	PROMPT( "inform result <query_type <origin_place <destination_place <result")
)

// /MyBus/PerformTask/GiveResults/InformError
DEFINE_INFORM_AGENT( CInformError,
	PRECONDITION((int)C("result.failed") != RC_OKAY)
	PROMPT( "inform error <query_type <origin_place <destination_place <result")
)

// /MyBus/PerformTask/GiveResults/RequestNextQuery
DEFINE_REQUEST_AGENT( CRequestNextQuery,
	REQUEST_CONCEPT(next_query)
	
	PROMPT("request next_query")

	GRAMMAR_MAPPING("![StartOver]>1, "
					"![NextBus]>2, "
					"![PreviousBus]>3")
)

// /MyBus/PerformTask/GiveResults/ExpectGoodbye
DEFINE_EXPECT_AGENT( CExpectGoodbye,
	EXPECT_CONCEPT( goodbye)
	GRAMMAR_MAPPING("@(../RequestNextQuery)[Quit]>true")
)

// /MyBus/PerformTask/GiveResult/InformStartingNewQuery
DEFINE_INFORM_AGENT( CInformStartingNewQuery,
	PRECONDITION((int)C("next_query") == NQ_NEW_REQUEST)
	PROMPT("inform starting_new_query")
)

// /MyBus/GreetGoodbye
DEFINE_INFORM_AGENT( CGreetGoodbye,
	PROMPT(":non-listening inform goodbye")
)

//-----------------------------------------------------------------------------
//
// AGENT DECLARATIONS
//
//-----------------------------------------------------------------------------
DECLARE_AGENTS(
	DECLARE_AGENT(CMyBus)
		DECLARE_AGENT(CGiveIntroduction)
		DECLARE_AGENT(CPerformTask)
			DECLARE_AGENT(CGetQuerySpecs)
				DECLARE_AGENT(CRequestQuery)
				DECLARE_AGENT(CRequestOriginPlace)
				DECLARE_AGENT(CRequestDestinationPlace)
			DECLARE_AGENT(CProcessQuery)
				DECLARE_AGENT(CInformFirstProcessing)
				DECLARE_AGENT(CInformSubsequentProcessing)
				DECLARE_AGENT(CExecuteBackendCall)
			DECLARE_AGENT(CGiveResults)
				DECLARE_AGENT(CInformSuccess)
				DECLARE_AGENT(CInformError)
				DECLARE_AGENT(CRequestNextQuery)
				DECLARE_AGENT(CExpectGoodbye)
				DECLARE_AGENT(CInformStartingNewQuery)
		DECLARE_AGENT(CGreetGoodbye)
)

//-----------------------------------------------------------------------------
// DIALOG TASK ROOT DECLARATION
//-----------------------------------------------------------------------------
DECLARE_DIALOG_TASK_ROOT(MyBus, CMyBus, "")