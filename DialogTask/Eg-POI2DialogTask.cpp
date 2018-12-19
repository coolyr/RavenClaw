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
//# Edited by Peng Li & Ming Sun for a POI system
//###################################################################
#include "parameter.h"
#include "DialogTask.h"
#include <string>
#include <stdio.h>
#include <windows.h>
//#include "BindingFilters.h"

#define __IN_DIALOGTASK__

// query types ID number (goes into the "QueryType" concept)
#define NQ_QUERY_WHERE			1
#define NQ_QUERY_HOWFAR			2
#define NQ_QUERY_CATEGORY		3
//#define NQ_NEW_QUERY			4
//#define NQ_CHANGE			5
#define NQ_QUERY_DIRECTIONS             4
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

//#define ALWAYS_IMPLICIT_CONFIRM
#ifdef ALWAYS_IMPLICIT_CONFIRM
#define CONF_STYLE      "expl_impl"
#else
#define CONF_STYLE      "expl"
#endif

//-----------------------------------------------------------------------------
//
// DIALOG CORE CONFIGURATION
// 核心配置
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
	       //LIBRARY_AGENT(CRepeat, Repeat, RegisterRepeatAgent, "")

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

	USE_ALL_GROUNDING_MODEL_TYPES
	USE_ALL_GROUNDING_ACTIONS("")

)




//-----------------------------------------------------------------------------
//
// CONCEPT TYPES DEFINITION
//  concept定义
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
// agent说明
//-----------------------------------------------------------------------------




// /MyPOI
DEFINE_AGENCY( CMyPOI, 

	       IS_MAIN_TOPIC()
			DEFINE_CONCEPTS(
				//INT_USER_CONCEPT(QueryType,"")
				BOOL_USER_CONCEPT(ExperiencedUser,"")
				INT_USER_CONCEPT(QueryType,"")
				INT_USER_CONCEPT(StartOver,"")

				STRING_USER_CONCEPT(OriginPlace,CONF_STYLE)
				STRING_USER_CONCEPT(DestinationPlace,CONF_STYLE)

				// concept from statistics of previous interactions
				
				//STRING_USER_CONCEPT(OriginPlaceTmp,"")
				
				CUSTOM_SYSTEM_CONCEPT(result, CResultConcept)
			)
	       DEFINE_SUBAGENTS(
				SUBAGENT(Adaptation,CAdaptation,"")
				SUBAGENT(GiveIntroduction, CGiveIntroduction, "")
				SUBAGENT(PerformTask,CPerformTask,"")
				SUBAGENT(AllDoneDummy, CAllDoneDummy,"")
				)
		   SUCCEEDS_WHEN(COMPLETED(AllDoneDummy))   
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
							//C("OriginPlaceTmp") = s.get_origin();
							C("OriginPlace") = s.get_origin();
						}
						Sleep(5000);
						
					 )
)
// /MyPOI/GiveIntroduction
DEFINE_INFORM_AGENT(CGiveIntroduction,
					//PRECONDITION(s.get_used() == false)
					PROMPT(":non-listening :non-interruptable inform adapted_welcome <ExperiencedUser")
)



// /MyPOI/PerformTask
DEFINE_AGENCY(CPerformTask,

			  DEFINE_SUBAGENTS(
				SUBAGENT(RequestWaitForCommands, CRequestWaitForCommands,"")
				SUBAGENT(HowFar, CHowFar, "")
				SUBAGENT(Direction, CDirection, "")
				SUBAGENT(Where,CWhere,"")
				SUBAGENT(Category,CCategory,"")
				SUBAGENT(PerformQuery, CPerformQuery, "")
			  )			 
			  SUCCEEDS_WHEN(COMPLETED(PerformQuery))
)

	// /MyPOI/PerformTask/RequestWaitForCommands
	DEFINE_REQUEST_AGENT(CRequestWaitForCommands,
						 PROMPT(":non-listening :non-interruptable request query_type")
						 REQUEST_CONCEPT(QueryType)
						 GRAMMAR_MAPPING(
						 "![QueryWhere]>1,"
						 "![QueryHowFar]>2,"
						 "![QueryCategory]>3,"
						 "![QueryDirections]>4"
						 )
	)

	// /MyPOI/PerformTask/HowFar
	DEFINE_AGENCY(CHowFar,
				  PRECONDITION((int)C("QueryType") == NQ_QUERY_HOWFAR)
				  DEFINE_SUBAGENTS(
					SUBAGENT(HowFarOrigin, CHowFarOrigin, "")
					SUBAGENT(HowFarDestination, CHowFarDestination, "")
				  )
	)


	// /MyPOI/PerformTask/HowFar/ExpectHowFarStart
	DEFINE_EXPECT_AGENT(CExpectHowFarStart,
						EXPECT_CONCEPT(HowFarIssued)
						GRAMMAR_MAPPING("@(../../RequestWaitForCommands)[QueryHowFar]>true")
	)
	// /MyPOI/PerformTask/HowFar/HowFarOrigin
	DEFINE_AGENCY(CHowFarOrigin,
				//PRECONDITION(IS_TRUE(HowFarIssued))
				DEFINE_CONCEPTS(
					BOOL_USER_CONCEPT(OriginConfirm,"")
				)
				DEFINE_SUBAGENTS(
					//SUBAGENT(RequestHowFarOriginConfirm, CRequestHowFarOriginConfirm, "")
					SUBAGENT(ExpectHowFarOrigin, CExpectHowFarOrigin, "")
					SUBAGENT(RequestHowFarOrigin, CRequestHowFarOrigin, "")
				)
				SUCCEEDS_WHEN(UPDATED(OriginPlace))
	)

	// /MyPOI/PerformTask/HowFar/RequestHowFarOriginConfirm
	DEFINE_REQUEST_AGENT(CRequestHowFarOriginConfirm,
				PRECONDITION(AVAILABLE(OriginPlaceTmp) && UNAVAILABLE(OriginPlace))
				REQUEST_CONCEPT(OriginConfirm)
				PROMPT(":non-listening :non-interruptable request origin_confirm <OriginPlaceTmp")
				GRAMMAR_MAPPING("![Yes]>true,"
								"![No]>false"	
				)
				ON_COMPLETION(
					if ((bool)C("OriginConfirm") == true)
					{
						C("OriginPlace") = C("OriginPlaceTmp");
					}
				)
	)
	
	// /MyPOI/PerformTask/HowFar/ExpectHowFarOrigin
	DEFINE_EXPECT_AGENT(CExpectHowFarOrigin,
				EXPECT_CONCEPT(OriginPlace)
				GRAMMAR_MAPPING("[QueryHowFar.Origin]")
	)

	// /MyPOI/PerformTask/HowFar/RequestHowFarOrigin
	DEFINE_REQUEST_AGENT(CRequestHowFarOrigin,
				REQUEST_CONCEPT(OriginPlace)
				PROMPT(":non-listening :non-interruptable request origin_place")
				GRAMMAR_MAPPING("![District],"
								"![Location]"
						)
	)

	// /MyPOI/PerformTask/HowFar/HowFarDestination
	DEFINE_AGENCY(CHowFarDestination,
				//PRECONDITION(IS_TRUE(HowFarIssued))
				DEFINE_SUBAGENTS(
					SUBAGENT(ExpectHowFarDestination,CExpectHowFarDestination,"")
					SUBAGENT(RequestHowFarDestination,CRequestHowFarDestination,"")
				)
	)
		
		// /MyPOI/PerformTask/HowFar/HowFarDestination/ExpectHowFarDestination
		DEFINE_EXPECT_AGENT(CExpectHowFarDestination,
						EXPECT_CONCEPT(DestinationPlace)
						GRAMMAR_MAPPING("[QueryHowFar.Destination]")
		)

		// /MyPOI/PerformTask/HowFar/HowFarDestination/RequestHowFarDestination
		DEFINE_REQUEST_AGENT(CRequestHowFarDestination,
						PROMPT(":non-listening :non-interruptable request destination_place")
						REQUEST_CONCEPT(DestinationPlace)
						GRAMMAR_MAPPING(
										"![District],"
										"![Location]"
						)
		)
	
DEFINE_AGENCY(CDirection,
		  PRECONDITION((int)C("QueryType") == NQ_QUERY_DIRECTIONS)
		  DEFINE_SUBAGENTS(
			SUBAGENT(DirectionOrigin, CDirectionOrigin, "")
			SUBAGENT(DirectionDestination, CDirectionDestination, "")
		  )
)

DEFINE_AGENCY(CDirectionOrigin,
		//PRECONDITION(IS_TRUE(HowFarIssued))
		DEFINE_SUBAGENTS(
			SUBAGENT(ExpectDirectionOrigin, CExpectDirectionOrigin, "")
			SUBAGENT(RequestDirectionOrigin, CRequestDirectionOrigin, "")
		)
		SUCCEEDS_WHEN(UPDATED(OriginPlace))
)

DEFINE_EXPECT_AGENT(CExpectDirectionOrigin,
		EXPECT_CONCEPT(OriginPlace)
		GRAMMAR_MAPPING("[QueryDirections.Origin]")			
)

DEFINE_REQUEST_AGENT(CRequestDirectionOrigin,
	REQUEST_CONCEPT(OriginPlace)
				PROMPT(":non-listening :non-interruptable request origin_place")
				GRAMMAR_MAPPING("![District],"
								"![Location]")						 
)

DEFINE_AGENCY(CDirectionDestination,
	DEFINE_SUBAGENTS(
					SUBAGENT(ExpectDirectionDestination,CExpectDirectionDestination,"")
					SUBAGENT(RequestDirectionDestination,CRequestDirectionDestination,"")
				)
)

DEFINE_EXPECT_AGENT(CExpectDirectionDestination,
	EXPECT_CONCEPT(DestinationPlace)
	GRAMMAR_MAPPING("[QueryDirections.Destination]")
)

DEFINE_REQUEST_AGENT(CRequestDirectionDestination,
	PROMPT(":non-listening :non-interruptable request destination_place")
	REQUEST_CONCEPT(DestinationPlace)
	GRAMMAR_MAPPING("![District],"
					"![Location]")
)


// Where

DEFINE_AGENCY(CWhere,
		  PRECONDITION((int)C("QueryType") == NQ_QUERY_WHERE)
		  DEFINE_SUBAGENTS(
			SUBAGENT(WhereDestination, CWhereDestination, "")
		  )
)

DEFINE_AGENCY(CWhereDestination,
	DEFINE_SUBAGENTS(
					SUBAGENT(ExpectWhereDestination,CExpectWhereDestination,"")
					SUBAGENT(RequestWhereDestination,CRequestWhereDestination,"")
				)
)

DEFINE_EXPECT_AGENT(CExpectWhereDestination,
	EXPECT_CONCEPT(DestinationPlace)
	GRAMMAR_MAPPING("[QueryWhere.Destination]")
)

DEFINE_REQUEST_AGENT(CRequestWhereDestination,
	PROMPT(":non-listening :non-interruptable request destination_place")
	REQUEST_CONCEPT(DestinationPlace)
	GRAMMAR_MAPPING("![District],"
					"![Location]")
)

// Category
DEFINE_AGENCY(CCategory,
		  PRECONDITION((int)C("QueryType") == NQ_QUERY_CATEGORY)
		  DEFINE_SUBAGENTS(
			//SUBAGENT(ExpectHowFarStart, CExpectHowFarStart,"")
			SUBAGENT(CategoryOrigin, CCategoryOrigin, "")
			SUBAGENT(CategoryDestination, CCategoryDestination, "")
			//SUBAGENT(CategoryQuery, CCategoryQuery, "")
		  )
		  //SUCCEEDS_WHEN(COMPLETED(CategoryOrigin))
)

DEFINE_AGENCY(CCategoryOrigin,
		//PRECONDITION(IS_TRUE(HowFarIssued))
		DEFINE_SUBAGENTS(
			SUBAGENT(ExpectCategoryOrigin, CExpectCategoryOrigin, "")
			SUBAGENT(RequestCategoryOrigin, CRequestCategoryOrigin, "")
		)
		SUCCEEDS_WHEN(UPDATED(OriginPlace))
)

DEFINE_EXPECT_AGENT(CExpectCategoryOrigin,
		EXPECT_CONCEPT(OriginPlace)
		GRAMMAR_MAPPING("[QueryCategory.Origin]")			
)

DEFINE_REQUEST_AGENT(CRequestCategoryOrigin,
				REQUEST_CONCEPT(OriginPlace)
				PROMPT(":non-listening :non-interruptable request origin_place")
				GRAMMAR_MAPPING("![District],"
								"![Location]")						 
)

DEFINE_AGENCY(CCategoryDestination,
	DEFINE_SUBAGENTS(
					SUBAGENT(ExpectCategoryDestination,CExpectCategoryDestination,"")
					SUBAGENT(RequestCategoryDestination,CRequestCategoryDestination,"")
				)
)

DEFINE_EXPECT_AGENT(CExpectCategoryDestination,
	EXPECT_CONCEPT(DestinationPlace)
	GRAMMAR_MAPPING("[QueryCategory.Destination]")
)

DEFINE_REQUEST_AGENT(CRequestCategoryDestination,
	PROMPT(":non-listening :non-interruptable request destination_place")
	REQUEST_CONCEPT(DestinationPlace)
	GRAMMAR_MAPPING("![Service]")
)


// /MyPOI/PerformTask/PerformQuery
	DEFINE_AGENCY(CPerformQuery,
		DEFINE_SUBAGENTS(
			SUBAGENT(ExecuteBackendCall,CExecuteBackendCall,"")
			SUBAGENT(InformResult, CInformResult, "")
		)
	)
		
		DEFINE_EXECUTE_AGENT( CExecuteBackendCall,
			  EXECUTE(
				  if(!AVAILABLE(QueryType)) {
					C("QueryType") = NQ_QUERY_HOWFAR;
				  }
				  
				  if ((int)C("QueryType") == NQ_QUERY_WHERE)
				  // call on the galaxy stub agent to execute that particular call
				  pTrafficManager->Call(this, "gal_be.launch_query "
							"<QueryType "
							"<DestinationPlace "
							">result");
				  else
						pTrafficManager->Call(this, "gal_be.launch_query "
							"<QueryType "
							"<OriginPlace "
							"<DestinationPlace "
							">result");
				  )
				
		)

		DEFINE_INFORM_AGENT(CInformResult,
			PRECONDITION((int)C("result.failed") == RC_OKAY)
		    PROMPT( ":non-listening :non-interruptable inform result <QueryType <OriginPlace <DestinationPlace <result")
			ON_COMPLETION(
				C("StartOver") = 1;
			)
		)


	DEFINE_EXECUTE_AGENT(CAllDoneDummy,
		EXECUTE()
		ON_COMPLETION(
			
				C("QueryType").Clear();
				C("OriginPlace").Clear();
				C("DestinationPlace").Clear();
				C("StartOver").Clear();
				A("/MyPOI").Reset();
				A("/MyPOI").ReOpenTopic();
		)
	)

	DEFINE_EXECUTE_AGENT(CGoodbye,
		PRECONDITION(AVAILABLE(QueryType))
		EXECUTE()
	)
DECLARE_AGENTS(
	DECLARE_AGENT(CMyPOI)
		DECLARE_AGENT(CAdaptation)
		DECLARE_AGENT(CGiveIntroduction)
		DECLARE_AGENT(CPerformTask)
			DECLARE_AGENT(CRequestWaitForCommands)
			DECLARE_AGENT(CHowFar)
				//DECLARE_AGENT(CExpectHowFarStart)
				DECLARE_AGENT(CHowFarOrigin)
					//DECLARE_AGENT(CRequestHowFarOriginConfirm)
					DECLARE_AGENT(CExpectHowFarOrigin)
					DECLARE_AGENT(CRequestHowFarOrigin)
				DECLARE_AGENT(CHowFarDestination)
					DECLARE_AGENT(CExpectHowFarDestination)
					DECLARE_AGENT(CRequestHowFarDestination)
			DECLARE_AGENT(CDirection)
				DECLARE_AGENT(CDirectionOrigin)
					//DECLARE_AGENT(CRequestHowFarOriginConfirm)
					DECLARE_AGENT(CExpectDirectionOrigin)
					DECLARE_AGENT(CRequestDirectionOrigin)
				DECLARE_AGENT(CDirectionDestination)
					DECLARE_AGENT(CExpectDirectionDestination)
					DECLARE_AGENT(CRequestDirectionDestination)
			DECLARE_AGENT(CWhere)
				//DECLARE_AGENT(CWhereOrigin)
					//DECLARE_AGENT(CRequestHowFarOriginConfirm)
					//DECLARE_AGENT(CExpectWhereOrigin)
					//DECLARE_AGENT(CRequestWhereOrigin)
				DECLARE_AGENT(CWhereDestination)
					DECLARE_AGENT(CExpectWhereDestination)
					DECLARE_AGENT(CRequestWhereDestination)
			DECLARE_AGENT(CCategory)
				DECLARE_AGENT(CCategoryOrigin)
					//DECLARE_AGENT(CRequestHowFarOriginConfirm)
					DECLARE_AGENT(CExpectCategoryOrigin)
					DECLARE_AGENT(CRequestCategoryOrigin)
				DECLARE_AGENT(CCategoryDestination)
					DECLARE_AGENT(CExpectCategoryDestination)
					DECLARE_AGENT(CRequestCategoryDestination)
			DECLARE_AGENT(CPerformQuery)
				DECLARE_AGENT(CExecuteBackendCall)
				DECLARE_AGENT(CInformResult)
		DECLARE_AGENT(CAllDoneDummy)
		//DECLARE_AGENT(CGoodbye)
)


//-----------------------------------------------------------------------------
// DIALOG TASK ROOT DECLARATION
//-----------------------------------------------------------------------------
DECLARE_DIALOG_TASK_ROOT(MyPOI, CMyPOI, "")
