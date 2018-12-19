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
// myPOI Dialog definition
// 
// Based on OLYMPUS TUTORIAL 1, by Antoine Raux (antoine@cs.cmu.edu), 2007
//-----------------------------------------------------------------------------
// 
// DialogTask.CPP - This module contains the description of the dialog 
//                         task, (i.e. all the user defined agencies, etc, etc)
// 
//-----------------------------------------------------------------------------


//###################################################################
//# Edited by Peng Li for a POI system
//###################################################################
#include "parameter.h"
#include "DialogTask.h"
#include <string>
//#include "BindingFilters.h"

#define __IN_DIALOGTASK__

// query types ID number (goes into the "QueryType" concept)
#define NQ_QUERY_WHERE			1
#define NQ_QUERY_HOWFAR			2
#define NQ_QUERY_CATEGORY		3
#define NQ_NEW_QUERY			4
#define NQ_CHANGE			5
#define NQ_QUERY_DIRECTIONS             6
//#define NQ_QUERY_MULTIPLE				8 // Added by Ming 6-14,2011
// return codes from the backend
#define RC_OKAY				0
#define RC_INTERNAL_ERROR		1


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
// DIALOG CORE CONFIGURATION
//
//-----------------------------------------------------------------------------

CORE_CONFIGURATION(

	// declare the NLG and the GUI as output devices
	USE_OUTPUT_DEVICES(
			   DEFAULT_OUTPUT_DEVICE("nlg", "nlg.launch_query", 1)
			   )

	USE_LIBRARY_AGENTS(
	       // the terminate agent
	       // LIBRARY_AGENT(CTerminate, Terminate, RegisterTerminateAgent, "")

	       // various help agents
	       LIBRARY_AGENT(CHelp, Help, RegisterHelpAgent, "")
	       LIBRARY_AGENT(CHelpGetTips, HelpGetTips, RegisterHelpGetTipsAgent, "")
	       // LIBRARY_AGENT(CHelpEstablishContext, HelpEstablishContext, 
	       //               RegisterHelpEstablishContextAgent, "")
	       LIBRARY_AGENT(CHelpWhatCanISay, HelpWhatCanISay, RegisterHelpWhatCanISayAgent, "")
	       LIBRARY_AGENT(CHelpSystemCapabilities, HelpSystemCapabilities,
			     RegisterHelpSystemCapabilitiesAgent, "")

	       // repeat handling agent
	       LIBRARY_AGENT(CRepeat, Repeat, RegisterRepeatAgent, "")

	       // suspend handling agency
	       // LIBRARY_AGENT(CSuspend, Suspend, RegisterSuspendAgency, "")

	       // timeout handling agency
	       //LIBRARY_AGENT(CTimeoutTerminate, TimeoutTerminate, 
	       //    RegisterTimeoutTerminateAgency, "")

	       // start-over handling agency
	       // LIBRARY_AGENT(CStartOver, StartOver, RegisterStartOverAgency, "")

	       // quit handling agency
	       // LIBRARY_AGENT(CQuit, Quit, RegisterQuitAgency, "")
	       )


	//USE_BINDING_FILTERS(
    //BINDING_FILTER("query", QueryBindingFilter)
 //)



)

//-----------------------------------------------------------------------------
//
// CONCEPT TYPES DEFINITION
//
//-----------------------------------------------------------------------------

Stat s;

// Result frame giving the final result
DEFINE_FRAME_CONCEPT_TYPE( CResultConcept,
			   ITEMS(
				 INT_ITEM(failed)
				 STRING_ITEM(answer)
				 )
			   )

 //DEFINE_STRUCT_CONCEPT_TYPE( CQueryConcept,
   //ITEMS(
   //  STRING_ITEM(name)
   //  INT_ITEM(type)
   //)
 //)
 
//-----------------------------------------------------------------------------
//
// AGENT SPECIFICATIONS
//
//-----------------------------------------------------------------------------




// /MyPOI
DEFINE_AGENCY( CMyPOI, 

	       IS_MAIN_TOPIC()
			DEFINE_CONCEPTS(
				BOOL_USER_CONCEPT(ExperiencedUser,"")
				INT_USER_CONCEPT(QueryType,"")
				INT_USER_CONCEPT(StartOver,"")
				STRING_USER_CONCEPT(OriginPlace,"")
				STRING_USER_CONCEPT(DestinationPlace,"")
				CUSTOM_SYSTEM_CONCEPT(result, CResultConcept)
				BOOL_USER_CONCEPT(OriginConfirm,"")	
				BOOL_USER_CONCEPT(DestinationConfirm,"")
			)
	       DEFINE_SUBAGENTS(
				SUBAGENT(Adaptation,CAdaptation,"")
				SUBAGENT(GiveIntroduction, CGiveIntroduction, "")
				SUBAGENT(PerformTask,CPerformTask,"")
				
				/*SUBAGENT(RequestQueryType, CRequestQueryType, "")
				SUBAGENT(Origin,COrigin,"")
				SUBAGENT(Destination,CDestination,"")
				SUBAGENT(PerformTask, CPerformTask, "")
				SUBAGENT(InformResults,CInformResults, "")
				SUBAGENT(RequestNextQuery,CRequestNextQuery, "")*/
				SUBAGENT(InformGoodbye, CInformGoodbye ,"")
				//SUBAGENT(AllDoneDummy, CAllDoneDummy, "")
				)
	       SUCCEEDS_WHEN(COMPLETED(InformGoodbye))
		   //SUCCEEDS_WHEN(COMPLETED(AllDoneDummy))  
		  
		   
)



// /MyPOI/Adapatation
DEFINE_EXECUTE_AGENT(CAdaptation,

					 EXECUTE(
						s.Stat_from_file("..\\..\\Resources\\Adaptation\\stat.txt");
						// whether the user is experienced
						if(s.get_used() == true)
						{
							C("ExperiencedUser") = true;
						}
						else
						{
							C("ExperiencedUser") = false;
						}
						// find out from where the user always start
						if(s.get_origin() != "not found")
						{
							C("OriginPlace") = s.get_origin();
						}
					 )
)
// /MyPOI/GiveIntroduction
DEFINE_INFORM_AGENT(CGiveIntroduction,
					//PRECONDITION(s.get_used() == false)
					PROMPT(":non-listening :non-interruptable inform adapted_welcome <ExperiencedUser")
)



DEFINE_AGENCY(CPerformTask,    
			DEFINE_SUBAGENTS(
				SUBAGENT(RequestQueryType, CRequestQueryType, "")
				SUBAGENT(Origin,COrigin,"")
				SUBAGENT(Destination,CDestination,"")
				SUBAGENT(PerformQuery, CPerformQuery, "")
				SUBAGENT(InformResults,CInformResults, "")
				SUBAGENT(RequestNextQuery,CRequestNextQuery, "")
				)
			SUCCEEDS_WHEN(
				COMPLETED(RequestNextQuery)
			)
			ON_COMPLETION(
				if ((int)C("StartOver") == 1)
				{
					C("StartOver").Clear();
					C("DestinationPlace").Clear();
					C("QueryType").Clear();
					C("OriginConfirm").Clear();
					C("DestinationConfirm").Clear();
					A("/MyPOI/PerformTask").ReOpenTopic();
				}			
			)	  
)


// /MyPOI/RequestQueryType
DEFINE_REQUEST_AGENT(CRequestQueryType,
				PROMPT(":non-listening :non-interruptable request query_type")
				REQUEST_CONCEPT(QueryType)
				GRAMMAR_MAPPING("![QueryWhere]>1, "
					  "![QueryHowFar]>2, "
					  "![QueryCategory]>3, "
					  "![QueryDirections]>4 "
				)
)

// /MyPOI/Origin
DEFINE_AGENCY(COrigin,
				 //PRECONDITION( ((int)C("QueryType")==2) || ((int)C("QueryType")==3) || ((int)C("QueryType")==4) )
				 //DEFINE_CONCEPTS(
					//BOOL_USER_CONCEPT(OriginConfirm,"")	
				 //)
				 DEFINE_SUBAGENTS(
					SUBAGENT(RequestOrigin, CRequestOrigin, "")
					SUBAGENT(RequestOriginConfirm, CRequestOriginConfirm, "")
				 )
)

// /MyPOI/Origin/RequestOrigin
DEFINE_REQUEST_AGENT(CRequestOrigin,
			  PRECONDITION( !AVAILABLE(OriginPlace) || (bool)C("OriginConfirm") == false)
			  PROMPT(":non-listening :non-interruptable request origin_place")
			  REQUEST_CONCEPT(OriginPlace)
			  GRAMMAR_MAPPING("@(../../RequestQueryType)[Origin],"
						  "![Origin],"
						  "![Location],"
						  "![District]"
			  )
)
// /MyPOI/Origin/RequestOriginConfirm
DEFINE_REQUEST_AGENT(CRequestOriginConfirm,
			  PROMPT(":non-listening :non-interruptable request origin_confirm <OriginPlace")
			  REQUEST_CONCEPT(OriginConfirm)
			  GRAMMAR_MAPPING("![Yes]>true,"
							"![No]>false"
								)
			ON_COMPLETION(
				if((bool)C("OriginConfirm") == false)
				{
					A("../../Origin").ReOpenTopic();
					C("OriginPlace").Clear();
					C("OriginConfirm").Clear();
				}
				
			)
)

// Destination

// /MyPOI/Destination
DEFINE_AGENCY(CDestination,
				 //PRECONDITION( ((int)C("QueryType")==1) || ((int)C("QueryType")==2) || ((int)C("QueryType")==4) )
				 //DEFINE_CONCEPTS(
					//BOOL_USER_CONCEPT(DestinationConfirm,"")
					
				 //)
				 DEFINE_SUBAGENTS(
					SUBAGENT(RequestDestination, CRequestDestination, "")
					//SUBAGENT(InformDestination,CInformDestination,"")
					SUBAGENT(RequestDestinationConfirm, CRequestDestinationConfirm, "")
				 )
)

// /MyPOI/Destination/RequestDestination
DEFINE_REQUEST_AGENT(CRequestDestination,
			  //PRECONDITION( !AVAILABLE(DestinationPlace) || (bool)C("DestinationConfirm") == false)
			  //PRECONDITION( !AVAILABLE(DestinationPlace))
			  PROMPT(":non-listening :non-interruptable request destination_place")
			  REQUEST_CONCEPT(DestinationPlace)
			  GRAMMAR_MAPPING("@(../../RequestQueryType)[Destination],"
							"![Location],"
							"![Business],"
							"![Service],"
							"![Restaurant],"
							"![District]"
			  )
)



// /MyPOI/Destination/RequestDestinationConfirm
DEFINE_REQUEST_AGENT(CRequestDestinationConfirm,

			  //PROMPT("request destination_confirm")

			  PROMPT(":non-listening :non-interruptable request destination_confirm <DestinationPlace")
			  
			  REQUEST_CONCEPT(DestinationConfirm)
			  GRAMMAR_MAPPING("![Yes]>true,"
							"![No]>false"
								)
			ON_COMPLETION(
				if((bool)C("DestinationConfirm") == false)
				{
					A("../../Destination").ReOpenTopic();
					C("DestinationPlace").Clear();
					C("DestinationConfirm").Clear();
				}
				
			)
)
//***********************************
//Peng's code

// /MyPOI/PerformQuery
DEFINE_AGENCY( CPerformQuery,
		
	       DEFINE_SUBAGENTS(
				SUBAGENT(InformFirstProcessing, CInformFirstProcessing, "")
				SUBAGENT(ExecuteBackendCall, CExecuteBackendCall, "")
				)
)

	// /MyPOI/PerformTask/ProcessQuery/InformFirstProcessing
	DEFINE_INFORM_AGENT( CInformFirstProcessing,
			 PROMPT(":non-listening :non-interruptable inform looking_up_database_first")
			 )

	// /MyPOI/PerformTask/ProcessQuery/ExecuteBackendCall
	DEFINE_EXECUTE_AGENT( CExecuteBackendCall,
			  EXECUTE(
				  if(!AVAILABLE(QueryType)) {
					C("QueryType") = NQ_QUERY_WHERE;
				  }
				  
				  // call on the galaxy stub agent to execute that particular call
				  pTrafficManager->Call(this, "gal_be.launch_query "
							"<QueryType "
							"<OriginPlace "
							"<DestinationPlace "
							">result");
				  )
	)


// end of Peng's code
//***********************************************

DEFINE_AGENCY( CInformResults,
			  DEFINE_SUBAGENTS(
				SUBAGENT(InformSuccess,CInformSuccess,"")
				SUBAGENT(InformError, CInformError,"")
			  )
			  SUCCEEDS_WHEN(SUCCEEDED(InformSuccess) || SUCCEEDED(InformError))
)

DEFINE_INFORM_AGENT( CInformSuccess,
		     PRECONDITION((int)C("result.failed") == RC_OKAY)
		     PROMPT( ":non-listening :non-interruptable inform result <QueryType <OriginPlace <DestinationPlace <result")
)

DEFINE_INFORM_AGENT( CInformError,
		     PRECONDITION((int)C("result.failed") != RC_OKAY)
		     PROMPT( ":non-listening :non-interruptable inform error <QueryType <OriginPlace <DestinationPlace <result")
)

// 
DEFINE_REQUEST_AGENT(CRequestNextQuery,
			PROMPT(":non-listening :non-interruptable request next_query")
			REQUEST_CONCEPT(StartOver)
			GRAMMAR_MAPPING("![StartOver]>1,"
							"![Quit]>0"
			)
)


DEFINE_INFORM_AGENT(CInformGoodbye,
			PRECONDITION(AVAILABLE(StartOver)&& (int)C("StartOver") == 0)
			PROMPT(":non-listening :non-interruptable inform goodbye")
)

//********************************
// All Dummy Done
//DEFINE_EXECUTE_AGENT(CAllDoneDummy,
//			PRECONDITION(AVAILABLE(StartOver))
//			EXECUTE(
//				if((int)C("StartOver") == 1)
//				{
//					C("QueryType").Clear();
//					C("DestinationPlace").Clear();
//					C("StartOver").Clear();
//					A("/MyPOI").ReOpenTopic();
//				}
//			)
//)




//DEFINE_EXECUTE_AGENT(CStartOver,
//			PRECONDITION(AVAILABLE(StartOver) && (int)C("StartOver") == 1)
//			EXECUTE(
//			  
//			  C("QueryType").Clear();
//			  C("DestinationPlace").Clear();
//			  C("StartOver").Clear();
//			  A("/MyPOI").Reset();
//			)
//)



DECLARE_AGENTS(
	DECLARE_AGENT(CMyPOI)
		DECLARE_AGENT(CAdaptation)
		DECLARE_AGENT(CGiveIntroduction)
		DECLARE_AGENT(CPerformTask)
			DECLARE_AGENT(CRequestQueryType)
			DECLARE_AGENT(COrigin)
				DECLARE_AGENT(CRequestOrigin)
				DECLARE_AGENT(CRequestOriginConfirm)
			DECLARE_AGENT(CDestination)
				DECLARE_AGENT(CRequestDestination)
				//DECLARE_AGENT(CInformDestination)
				DECLARE_AGENT(CRequestDestinationConfirm)
			DECLARE_AGENT(CPerformQuery)
				DECLARE_AGENT(CInformFirstProcessing)
				DECLARE_AGENT(CExecuteBackendCall)
			DECLARE_AGENT(CInformResults)
				DECLARE_AGENT(CInformSuccess)
				DECLARE_AGENT(CInformError)
			DECLARE_AGENT(CRequestNextQuery)
		DECLARE_AGENT(CInformGoodbye)
		//DECLARE_AGENT(CAllDoneDummy)
				
				//
				//DECLARE_AGENT(CStartOver)
		
		/*
		DECLARE_AGENT(CPerformTask)
			DECLARE_AGENT(CGetQuerySpecs)
				DECLARE_AGENT(CRequestOriginPlace)
					DECLARE_AGENT(CRequestOrigPlace)
					DECLARE_AGENT(CInformOriginPlace)
					DECLARE_AGENT(CRequestConformation)
					//DECLARE_AGENT(CExpectOriginPlace)
				DECLARE_AGENT(CRequestQuery)
					DECLARE_AGENT(CRequestQueryType)
					DECLARE_AGENT(CExpectDestinationPlace)
					DECLARE_AGENT(CRequestDestinationPlace)
			DECLARE_AGENT(CProcessQuery)
				DECLARE_AGENT(CInformFirstProcessing)
				DECLARE_AGENT(CExecuteBackendCall)
			DECLARE_AGENT(CGiveResults)
				DECLARE_AGENT(CInformSuccess)
				DECLARE_AGENT(CInformError)
				DECLARE_AGENT(CRequestNextQuery)
				DECLARE_AGENT(CExpectGoodbye)
				DECLARE_AGENT(CInformStartingNewQuery)
				DECLARE_AGENT(CInformChangeQuery)
		DECLARE_AGENT(CGreetGoodbye)
		*/
)


/***
// /MyPOI/GiveIntroduction
DEFINE_INFORM_AGENT( CGiveIntroduction,
	PROMPT(	"inform welcome")
)

// /MyPOI/PerformTask
DEFINE_AGENCY( CPerformTask,

 	DEFINE_CONCEPTS(
		INT_USER_CONCEPT(query_type, "")
		STRING_USER_CONCEPT(origin_place, "")
		STRING_USER_CONCEPT(destination_place, "")
		
		CUSTOM_SYSTEM_CONCEPT(result, CResultConcept)
		//CUSTOM_SYSTEM_CONCEPT(query, CQueryConcept)
	)
    
	DEFINE_SUBAGENTS(
		SUBAGENT(GetQuerySpecs, CGetQuerySpecs, "")
		SUBAGENT(ProcessQuery, CProcessQuery, "")
		SUBAGENT(GiveResults, CGiveResults, "")
	)

)

// /MyPOI/PerformTask/GetQuerySpecs
DEFINE_AGENCY( CGetQuerySpecs,
	
	DEFINE_SUBAGENTS(
		SUBAGENT(RequestOriginPlace, CRequestOriginPlace, "")
		SUBAGENT(RequestQuery, CRequestQuery, "")
		//SUBAGENT(InformOriginPlace, CInformOriginPlace, "")
		//SUBAGENT(ExpectOriginPlace, CExpectOriginPlace, "")
		// SUBAGENT(RequestQueryType, CRequestQueryType, "")
		// SUBAGENT(ExpectDestinationPlace, CExpectDestinationPlace, "")
		// SUBAGENT(RequestDestinationPlace, CRequestDestinationPlace, "")
		
	)
	
	//SUCCEEDS_WHEN(
	//	AVAILABLE(origin_place) &&
	//	AVAILABLE(query_type) &&
	//	AVAILABLE(destination_place) &&
	//	IS_TRUE(origin_place_conformation)
	//)

)

// /MyPOI/PerformTask/GetQuerySpecs/RequestOriginPlace
DEFINE_AGENCY( CRequestOriginPlace,
	
	DEFINE_CONCEPTS(
		BOOL_USER_CONCEPT(origin_place_conformation, "")
	)
	
	DEFINE_SUBAGENTS(
		SUBAGENT(RequestOrigPlace, CRequestOrigPlace, "")
		SUBAGENT(InformOriginPlace, CInformOriginPlace, "")
		SUBAGENT(RequestConformation, CRequestConformation, "")
		//SUBAGENT(ExpectOriginPlace, CExpectOriginPlace, "")
	)
	
	
	ON_COMPLETION(
		if(IS_FALSE(origin_place_conformation))
		{
		  A("/MyPOI/PerformTask/GetQuerySpecs/RequestOriginPlace").Reset();
		  //A("/MyPOI/PerformTask/GetQuerySpecs/RequestOriginPlace/RequestOrigPlace").ReOpenTopic();
		  C("origin_place").Clear();
		}
	)
)

// /MyPOI/PerformTask/GetQuerySpecs/RequestOriginPlace/RequestOrigPlace		
DEFINE_REQUEST_AGENT( CRequestOrigPlace,

	//PRECONDITION(!AVAILABLE(origin_place) || IS_FALSE(origin_place_conformation))
	// PRECONDITION(IS_FALSE(origin_place_conformation))
	
	PROMPT("request origin_place")
	REQUEST_CONCEPT(origin_place)
	GRAMMAR_MAPPING("![Place.Origin]")
	
	// ON_COMPLETION(
		// C("origin_place_conformation") = "true";
	// )
	
)

// /MyPOI/PerformTask/GetQuerySpecs/RequestOriginPlace/InformOriginPlace
DEFINE_INFORM_AGENT( CInformOriginPlace,
		     PROMPT("inform place <origin_place")
)

// /MyPOI/PerformTask/GetQuerySpecs/RequestOriginPlace/RequestConformation		
DEFINE_REQUEST_AGENT( CRequestConformation,

	//PRECONDITION(!AVAILABLE(origin_place) || IS_FALSE(origin_place_conformation))
	//PRECONDITION(IS_FALSE(origin_place_conformation))
	
		      PROMPT("request conformation")
		      REQUEST_CONCEPT(origin_place_conformation)
		      GRAMMAR_MAPPING("![Yes]>true ,"
				      "![No]>false" )
	
)

// /MyPOI/PerformTask/GetQuerySpecs/ExpectOriginPlace
// DEFINE_EXPECT_AGENT( CExpectOriginPlace,
	// EXPECT_CONCEPT(origin_place_conformation)
	// GRAMMAR_MAPPING("@(../InformOriginPlace)[No]>false")
	// GRAMMAR_MAPPING("@(../RequestNothing)[No]>false")
// )

// /MyPOI/PerformTask/GetQuerySpecs/RequestQuery
DEFINE_AGENCY( CRequestQuery,
	
	       DEFINE_SUBAGENTS(
				SUBAGENT(RequestQueryType, CRequestQueryType, "")
				SUBAGENT(ExpectDestinationPlace, CExpectDestinationPlace, "")
				SUBAGENT(RequestDestinationPlace, CRequestDestinationPlace, "")	
	)
)

    // /MyPOI/PerformTask/GetQuerySpecs/RequestQuery/RequestQueryType
    DEFINE_REQUEST_AGENT( CRequestQueryType,

			  //PRECONDITION(!AVAILABLE(query_type))
	
			  PROMPT("request query_type")
			  REQUEST_CONCEPT(query_type)
			  GRAMMAR_MAPPING("![QueryWhere]>1, "
					  "![QueryHowFar]>2, "
					  "![QueryCategory]>3, "
					  "![QueryDirections]>6 "
					  //"![QueryMultiple]>8" // added by Ming 6-14,2011
					  )
			  )

    // /MyPOI/PerformTask/GetQuerySpecs/RequestQuery/ExpectDestinationPlace
    DEFINE_EXPECT_AGENT( CExpectDestinationPlace,
			 EXPECT_CONCEPT(destination_place)
			 GRAMMAR_MAPPING("@(../RequestQueryType)[Place]")
			 )
    
    // /MyPOI/PerformTask/GetQuerySpecs/RequestQuery/RequestDestinationPlace
    DEFINE_REQUEST_AGENT( CRequestDestinationPlace,
			  
			  PRECONDITION(!AVAILABLE(destination_place))
			  
			  PROMPT("request destination_place")
			  
			  REQUEST_CONCEPT(destination_place);
			  GRAMMAR_MAPPING("![Place]");
			  
			  
			  )

// /MyPOI/PerformTask/ProcessQuery
DEFINE_AGENCY( CProcessQuery,
		
	       DEFINE_SUBAGENTS(
				SUBAGENT(InformFirstProcessing, CInformFirstProcessing, "")
				SUBAGENT(ExecuteBackendCall, CExecuteBackendCall, "")
				)
)

    // /MyPOI/PerformTask/ProcessQuery/InformFirstProcessing
    DEFINE_INFORM_AGENT( CInformFirstProcessing,
			 PROMPT("inform looking_up_database_first")
			 )
    
    // /MyPOI/PerformTask/ProcessQuery/ExecuteBackendCall
    DEFINE_EXECUTE_AGENT( CExecuteBackendCall,
			  EXECUTE(
				  if(!AVAILABLE(query_type)) {
				    C("query_type") = NQ_QUERY_WHERE;
				  }
				  
				  // call on the galaxy stub agent to execute that particular call
				  pTrafficManager->Call(this, "gal_be.launch_query "
							"<query_type "
							"<origin_place "
							"<destination_place "
							">result");
				  )
			  )

// /MyPOI/PerformTask/GiveResults
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
			 SUBAGENT(InformChangeQuery, CInformChangeQuery, "")
			 )

	SUCCEEDS_WHEN(
		      SUCCEEDED(InformStartingNewQuery) ||
		      SUCCEEDED(InformChangeQuery) ||
		      (((int)C("result.failed") != RC_OKAY) && 
		       SUCCEEDED(InformError)) ||
		      IS_TRUE(goodbye)
	)

	ON_COMPLETION(
		      if (((int)C("next_query") == NQ_NEW_QUERY) ||
			  ((int)C("result.failed") != RC_OKAY)) {
			A("/MyPOI/PerformTask").Reset();
		      }
		      else if ((int)C("next_query") == NQ_CHANGE) 
			{
			  A("/MyPOI/PerformTask").ReOpenTopic();
			  C("query_type").Clear();
			  C("destination_place").Clear();
			  C("next_query").Clear();
			  C("goodbye").Clear();
			  
			}
		      )
)

// /MyPOI/PerformTask/GiveResults/InformSuccess
DEFINE_INFORM_AGENT( CInformSuccess,
		     PRECONDITION((int)C("result.failed") == RC_OKAY)
		     PROMPT( "inform result <query_type <origin_place <destination_place <result")
)

// /MyPOI/PerformTask/GiveResults/InformError
DEFINE_INFORM_AGENT( CInformError,
		     PRECONDITION((int)C("result.failed") != RC_OKAY)
		     PROMPT( "inform error <query_type <origin_place <destination_place <result")
)


// /MyPOI/PerformTask/GiveResults/RequestNextQuery
DEFINE_REQUEST_AGENT( CRequestNextQuery,
		      REQUEST_CONCEPT(next_query)
		      
		      PROMPT("request next_query")
		      
		      GRAMMAR_MAPPING("![StartOver]>4, "
				      "![Change]>5")
		      )

// /MyPOI/PerformTask/GiveResults/ExpectGoodbye
DEFINE_EXPECT_AGENT( CExpectGoodbye,
		     EXPECT_CONCEPT(goodbye)
		     GRAMMAR_MAPPING("@(../RequestNextQuery)[Quit]>true")
		     )

// /MyPOI/PerformTask/GiveResult/InformStartingNewQuery
DEFINE_INFORM_AGENT( CInformStartingNewQuery,
		     PRECONDITION((int)C("next_query") == NQ_NEW_QUERY)
		     PROMPT("inform starting_new_query")
)

// /MyPOI/PerformTask/GiveResult/InformChangeQuery
DEFINE_INFORM_AGENT( CInformChangeQuery,
		     PRECONDITION((int)C("next_query") == NQ_CHANGE)
			 PROMPT("inform change_query")
			 )

// /MyPOI/GreetGoodbye
DEFINE_INFORM_AGENT( CGreetGoodbye,
		     PROMPT(":non-listening inform goodbye")
)

//-----------------------------------------------------------------------------
//
// AGENT DECLARATIONS
//
//-----------------------------------------------------------------------------
DECLARE_AGENTS(
	DECLARE_AGENT(CMyPOI)
		DECLARE_AGENT(CGiveIntroduction)
		DECLARE_AGENT(CPerformTask)
			DECLARE_AGENT(CGetQuerySpecs)
				DECLARE_AGENT(CRequestOriginPlace)
					DECLARE_AGENT(CRequestOrigPlace)
					DECLARE_AGENT(CInformOriginPlace)
					DECLARE_AGENT(CRequestConformation)
					//DECLARE_AGENT(CExpectOriginPlace)
				DECLARE_AGENT(CRequestQuery)
					DECLARE_AGENT(CRequestQueryType)
					DECLARE_AGENT(CExpectDestinationPlace)
					DECLARE_AGENT(CRequestDestinationPlace)
			DECLARE_AGENT(CProcessQuery)
				DECLARE_AGENT(CInformFirstProcessing)
				DECLARE_AGENT(CExecuteBackendCall)
			DECLARE_AGENT(CGiveResults)
				DECLARE_AGENT(CInformSuccess)
				DECLARE_AGENT(CInformError)
				DECLARE_AGENT(CRequestNextQuery)
				DECLARE_AGENT(CExpectGoodbye)
				DECLARE_AGENT(CInformStartingNewQuery)
				DECLARE_AGENT(CInformChangeQuery)
		DECLARE_AGENT(CGreetGoodbye)
)
***/
//-----------------------------------------------------------------------------
// DIALOG TASK ROOT DECLARATION
//-----------------------------------------------------------------------------
DECLARE_DIALOG_TASK_ROOT(MyPOI, CMyPOI, "")