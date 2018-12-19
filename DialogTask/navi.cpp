//#ifdef NAVI
#include "DialogTask.h"
//#include "DateTimeBindingFilters.h"

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
CORE_CONFIGURATION(		// ==> DialogTaskOnBeginSession()

        // declare the NLG as the single output device
	USE_OUTPUT_DEVICES(DEFAULT_OUTPUT_DEVICE("nlg", "nlg.launch_query", 1))		// <1>	==> RegisterOutputDevice(Name, ServerCall, Params)
        // declare the library agents to be used in the tree
        // the terminate agent
        USE_LIBRARY_AGENTS(
                LIBRARY_AGENT(CTerminate, Terminate, RegisterTerminateAgent, "")	// <2>	==> pDTTManager->Use(#AgentType, #AgentName, &RegistrationFunction, ConfigurationString)

                // various help agents
                LIBRARY_AGENT(CHelp, Help, RegisterHelpAgent, "")
                LIBRARY_AGENT(CHelpGetTips, HelpGetTips, RegisterHelpGetTipsAgent, "")
                LIBRARY_AGENT(CHelpEstablishContext, HelpEstablishContext, RegisterHelpEstablishContextAgent, "")
                LIBRARY_AGENT(CHelpWhatCanISay, HelpWhatCanISay, RegisterHelpWhatCanISayAgent, "")
				LIBRARY_AGENT(CHelpSystemCapabilities, HelpSystemCapabilities,	RegisterHelpSystemCapabilitiesAgent, "")
                
                // repeat handling agent
                LIBRARY_AGENT(CRepeat, Repeat, RegisterRepeatAgent, "")

                // suspend handling agency
                LIBRARY_AGENT(CSuspend, Suspend, RegisterSuspendAgency, "")

                // timeout handling agency
                LIBRARY_AGENT(CTimeoutTerminate, TimeoutTerminate, RegisterTimeoutTerminateAgency, "")

                // start-over handling agency
                LIBRARY_AGENT(CStartOver, StartOver, RegisterStartOverAgency, "")

                // quit handling agency
                LIBRARY_AGENT(CQuit, Quit, RegisterQuitAgency, "")
        )

		/*
			concept_default			=>		 CGMConcept::GroundingModelFactory

			request_default			=>	 /	 CGMRequestAgent::GroundingModelFactory
			request_handcrafted		=>		 CGMRequestAgent_HandCrafted::GroundingModelFactory
			request_lr				=>		 CGMRequestAgent_LR::GroundingModelFactory
			request_numnonu			=>		 CGMRequestAgent_NumNonu::GroundingModelFactory
			request_experiment		=>		 CGMRequestAgent_Experiment::GroundingModelFactory
		*/
		// void CGroundingManagerAgent::RegisterGroundingModelType(string sName, FCreateGroundingModel fctCreateGroundingModel)
        // declare the grounding model types to be used
        USE_ALL_GROUNDING_MODEL_TYPES	//  <3>	==>  pGroundingManager->RegisterGroundingModelType(#Name, FactoryMethod);

	// declare the grounding actions to be used
	USE_ALL_GROUNDING_ACTIONS("")		//	<4>	==>	 pGroundingManager->UseGroundingAction(#Name, new GAType((string)Configuration)
)

//-----------------------------------------------------------------------------
// /define user query concept
//-----------------------------------------------------------------------------
DEFINE_STRUCT_CONCEPT_TYPE(CUserQuery,
                           ITEMS(
                                   STRING_ITEM(query)
                                   STRING_ITEM(domain)
                                   STRING_ITEM(intent)
                                 )
                           )
//-----------------------------------------------------------------------------
// /define poi search result
//-----------------------------------------------------------------------------
DEFINE_FRAME_CONCEPT_TYPE(CPoiInfo,
                          ITEMS(
                                  STRING_ITEM(poiName)
                                  FLOAT_ITEM(longitude)
                                  FLOAT_ITEM(latitude)
                                  STRING_ITEM(phone)
                                )
                          )

// Array of results from the backend
DEFINE_ARRAY_CONCEPT_TYPE(CPoiInfo, CPoiArrayResults)
//-----------------------------------------------------------------------------
// /define contact name search result
//-----------------------------------------------------------------------------
DEFINE_FRAME_CONCEPT_TYPE(CContactInfo,
                          ITEMS(
                                  STRING_ITEM(peopleName)
                                  STRING_ITEM(phone)
                                )
                          )
// Array of results from the backend
DEFINE_ARRAY_CONCEPT_TYPE(CContactInfo, CContactArrayResults)
//-----------------------------------------------------------------------------
// /define sc search result
//-----------------------------------------------------------------------------
DEFINE_FRAME_CONCEPT_TYPE(CScResult,
                          ITEMS(
                                  STRING_ITEM(title)
                                  STRING_ITEM(summary)
                                  STRING_ITEM(body)
                                )
                          )

//-----------------------------------------------------------------------------
// /CMap
//-----------------------------------------------------------------------------
DEFINE_AGENCY(CMap,
              IS_MAIN_TOPIC()
              DEFINE_CONCEPTS(
                      STRING_USER_CONCEPT(userQuery, "expl")
                      STRING_USER_CONCEPT(userDomain, "expl")
                      STRING_USER_CONCEPT(userIntent, "expl")
                              )
              DEFINE_SUBAGENTS(
                      SUBAGENT(Welcome, CWelcome, "")
                      SUBAGENT(AskAnyQuery, CAskAnyQuery, "")
                      SUBAGENT(Sc, CSc, "")
                      SUBAGENT(Navi, CNavi, "")
                      SUBAGENT(Telephone, CTelephone, "")
                      SUBAGENT(Music, CMusic, "")
                               )
              )
//-----------------------------------------------------------------------------
// /Map/Welcome
//-----------------------------------------------------------------------------
DEFINE_INFORM_AGENT(CWelcome,
        PROMPT(":non-interruptable inform welcome")
                    )
//-----------------------------------------------------------------------------
// /Map/AskAnyQuery
//-----------------------------------------------------------------------------
DEFINE_AGENCY(CAskAnyQuery,
              DEFINE_SUBAGENTS(
                      SUBAGENT(AskQuery, CAskQuery, "")
                      SUBAGENT(AskDomain, CAskDomain, "")
                      SUBAGENT(AskIntent, CAskIntent, "")
                               )
              SUCCEEDS_WHEN(UPDATED(userQuery))
              ON_COMPLETION(
                      REOPEN
                            )
              )
// /Map/AskAnyQuery/AskQuery
DEFINE_REQUEST_AGENT(CAskQuery,
                     PRECONDITION(UNAVAILABLE(userQuery))
                     PROMPT(":non-interruptable request say_what_u_want")
                     REQUEST_CONCEPT(userQuery)
                     GRAMMAR_MAPPING("[query]") 
                     SUCCEEDS_WHEN(UPDATED(userQuery))
                     )
// /Map/AskAnyQuery/AskDomain
DEFINE_REQUEST_AGENT(CAskDomain,
                     PRECONDITION(UNAVAILABLE(userDomain))
                     REQUEST_CONCEPT(userDomain)
                     GRAMMAR_MAPPING("[domain]")
                     SUCCEEDS_WHEN(UPDATED(userDomain))
                     )
// /Map/AskAnyQuery/AskIntent
DEFINE_REQUEST_AGENT(CAskIntent,
                     PRECONDITION(UNAVAILABLE(userIntent))
                     REQUEST_CONCEPT(userIntent)
                     GRAMMAR_MAPPING("[intent]")
                     SUCCEEDS_WHEN(UPDATED(userIntent))
                     )

// /Map/Navi
DEFINE_AGENCY(CNavi,
              TRIGGERED_BY("map" == string(C("userDomain")))
              DEFINE_CONCEPTS(
                      CUSTOM_SYSTEM_CONCEPT(destination, CPoiInfo)
                      STRING_USER_CONCEPT(method, "expl") 
                              )
              DEFINE_SUBAGENTS(
                      SUBAGENT(Search, CSearch, "")
                      SUBAGENT(Method, CMethod, "")
                      SUBAGENT(BeginNavi, CBeginNavi, "")
                               )
              SUCCEEDS_WHEN(COMPLETED(BeginNavi) || FAILED(Search))
              ON_CREATION(C("method") = "躲避拥堵|0.8")
              ON_COMPLETION(REOPEN
                      CLEAR(userQuery)
                      CLEAR(userDomain)
                      CLEAR(userIntent)
                      RESTORE_CONCEPT(destination)
                      RESTORE_CONCEPT(method)
                      RESET_AGENT(/Map/AskAnyQuery)
                      pDMCore->ContinueWith(&A(".."), &A("/Map/AskAnyQuery"))
                            )
              )
// /Map/Navi/Search
DEFINE_AGENCY(CSearch,
              PRECONDITION(UPDATED(poi) || UPDATED(poiType))
              DEFINE_CONCEPTS(
                      STRING_USER_CONCEPT(poi, "expl")
                      STRING_USER_CONCEPT(poiType, "expl")
                              )
              DEFINE_SUBAGENTS(
                      SUBAGENT(SearchPoi, CSearchPoi, "")
                      SUBAGENT(SearchPoiType, CSearchPoiType, "")
                               )
              SUCCEEDS_WHEN(COMPLETED(SearchPoi) || COMPLETED(SearchPoiType)) 
              FAILS_WHEN(FAILED(SearchPoi) || FAILED(SearchPoiType))
              )
// /Map/Navi/Search/SearchPoi
DEFINE_AGENCY(CSearchPoi,
              PRECONDITION(UPDATED(poi))
              TRIGGERED_BY(UPDATED(poi))
              DEFINE_CONCEPTS(
                      CUSTOM_SYSTEM_CONCEPT(searchResult, CPoiArrayResults)
                      BOOL_USER_CONCEPT(confirm, "none")
                      INT_USER_CONCEPT(choice, "none")
                              )
              DEFINE_SUBAGENTS(
                      SUBAGENT(Poi, CPoi, "")
                      SUBAGENT(InformProcessing, CInformProcessing, "")
                      SUBAGENT(CallMap, CCallMap, "")
                      SUBAGENT(InformNoResult, CNoResult, "")
                      SUBAGENT(SingleResult, CSingleResult, "")
                      SUBAGENT(InformResult, CInformResult, "")
                      SUBAGENT(Choice, CChoice, "")
                               )
              SUCCEEDS_WHEN(SIZE(searchResult) > 0 && UPDATED(choice) && int(C("choice")) > 0 && int(C("choice")) <= SIZE(searchResult))
              FAILS_WHEN(COMPLETED(InformNoResult))
              )
// /Map/Navi/Search/SearchPoi/Poi
DEFINE_EXPECT_AGENT(CPoi,
                    EXPECT_CONCEPT(poi)
                    GRAMMAR_MAPPING("@(/Map/Navi)[loc], [destination_place]")
                    )
// /Map/Nav/Search/SearchPoi/InformProcessing
DEFINE_INFORM_AGENT(CInformProcessing,
                    PRECONDITION(!AVAILABLE(searchResult))
                    PROMPT("inform looking_up_database_first")
                    )
// /Map/Navi/Search/SearchPoi/CallMap
DEFINE_EXECUTE_AGENT(CCallMap,
                     PRECONDITION(AVAILABLE(poi))
                     /*TRIGGERED_BY(UPDATED(poi))*/
                     CALL("backend.call poi <poi "\
                             "searchResult>searchResult")
                     )
// /Map/Navi/Search/SearchPoi/InformNoResult
DEFINE_INFORM_AGENT(CNoResult,
                    PRECONDITION(SIZE(searchResult) == 0)
                    TRIGGERED_BY(UPDATED(searchResult) && SIZE(searchResult) == 0)
                    PROMPT(":non-interruptable inform no_result_poi <poi")
                    )
// /Map/Navi/Search/SearchPoi/singleResult
DEFINE_REQUEST_AGENT(CSingleResult,
                     PRECONDITION(1 == SIZE(searchResult))
                     REQUEST_CONCEPT(confirm)
                     PROMPT("request one_result < searchResult.-1, go to there?")
                     GRAMMAR_MAPPING("![OK]>true, ![DENY]>false")
                     ON_COMPLETION(
                             if (IS_TRUE(confirm)) {
                                 C("/Map/Navi/destination") = C("searchResult.1");
                             }
                                   )
                     )
// /Map/Navi/Search/searchPoi/InformResult
DEFINE_INFORM_AGENT(CInformResult,
                    PRECONDITION(UPDATED(searchResult) && SIZE(searchResult) > 1)
                    PROMPT("inform map_search_result <poi <searchResult")
                    )
// /Map/Navi/Search/SearchPoi/Choice
DEFINE_REQUEST_AGENT(CChoice,
                     PRECONDITION(SIZE(searchResult) > 1)
                     REQUEST_CONCEPT(choice)
                     PROMPT("request mutil_result_choice")
                     GRAMMAR_MAPPING("![第一个]>1, ![第二个]>2, ![第三个]>3")
                     ON_COMPLETION(
                             C("/Map/Navi/destination") = C("searchResult")[int(C("choice")) -  1];
                                   )
                     )
// /Map/Navi/Search/SearchPoiType
DEFINE_AGENCY(CSearchPoiType,
              PRECONDITION(UPDATED(poiType))
              TRIGGERED_BY(UPDATED(poiType))
              DEFINE_CONCEPTS(
                      CUSTOM_SYSTEM_CONCEPT(searchResult, CPoiArrayResults)
                      BOOL_USER_CONCEPT(confirm, "none")
                      INT_USER_CONCEPT(choice, "expl")
                              )
              DEFINE_SUBAGENTS(
                      SUBAGENT(PoiType, CPoiType, "")
                      SUBAGENT(InformProcessing2, CInformProcessing, "")
                      SUBAGENT(CallMap2, CCallMap2, "")
                      SUBAGENT(InformNoResult2, CNoResult2, "")
                      SUBAGENT(SingleResult2, CSingleResult, "")
                      SUBAGENT(InformResult2, CInformResult2, "")
                      SUBAGENT(Choice2, CChoice2, "")
                               )
              SUCCEEDS_WHEN(SIZE(searchResult) > 0 && UPDATED(choice))
              FAILS_WHEN(COMPLETED(InformNoResult2))
              )
// /Map/Navi/Search/SearchPoiType/PoiType
DEFINE_EXPECT_AGENT(CPoiType,
                    EXPECT_CONCEPT(poiType)
                    GRAMMAR_MAPPING("[poi_type]")
                    )
// /Map/Navi/Search/SearchPoiType/CallMap2
DEFINE_EXECUTE_AGENT(CCallMap2,
                     PRECONDITION(AVAILABLE(poiType))
                     /*TRIGGERED_BY(UPDATED(poiType))*/
                     CALL("backend.call poi_type <poiType "\
                             "searchResult>searchResult")
                     )
// /Map/Navi/Search/SearchPoiType/InformNoResult2
DEFINE_INFORM_AGENT(CNoResult2,
                    PRECONDITION(SIZE(searchResult) == 0)
                    TRIGGERED_BY(UPDATED(searchResult) && SIZE(searchResult) == 0)
                    PROMPT(":non-interruptable inform no_result_poitype <poiType")
                    )
// /Map/Navi/Search/searchPoiType/InformResult2
DEFINE_INFORM_AGENT(CInformResult2,
                    PRECONDITION(UPDATED(searchResult) && SIZE(searchResult) > 1)
                    PROMPT("inform map_search_result_poitype <poiType <searchResult")
                    )

// /Map/Navi/Search/SearchPoiType/Choice2
DEFINE_REQUEST_AGENT(CChoice2,
                     PRECONDITION(SIZE(searchResult) > 0)
                     PROMPT("request mutil_result_choice")
                     REQUEST_CONCEPT(choice)
                     GRAMMAR_MAPPING("![第一个]>1, ![第二个]>2, ![第三个]>3")
                     ON_COMPLETION(
                             C("/Map/Navi/destination") = C("searchResult")[int(C("choice")) -  1];
                                   )
                     )
// /Map/Navi/Method
DEFINE_EXPECT_AGENT(CMethod,
                    EXPECT_CONCEPT(method)
                    /*GRAMMAR_MAPPING("@(/Map/Navi)[route_sort], [route_sort]")*/
                    GRAMMAR_MAPPING("[route_sort]")
                    )
// /Map/Navi/BeginNavi
DEFINE_INFORM_AGENT(CBeginNavi,
                    PRECONDITION(AVAILABLE(destination))
                    PROMPT("inform begin_navi <destination <method")
)
/*
DEFINE_EXECUTE_AGENT(CBeginNavi,
	PRECONDITION(AVAILABLE(destination))
	CALL("backend.call destination=<destination")
                     )
*/
//-----------------------------------------------------------------------------
// Telephone
//-----------------------------------------------------------------------------
DEFINE_AGENCY(CTelephone,
              TRIGGERED_BY("telephone" == string(C("userDomain")))
              DEFINE_CONCEPTS(
                      STRING_USER_CONCEPT(phoneNum, "")
                              )
              DEFINE_SUBAGENTS(
                      SUBAGENT(AskNum, CAskNum, "")
                      SUBAGENT(AskName, CAskName, "")
                      SUBAGENT(Call, CCall, "")
                               )
              SUCCEEDS_WHEN(COMPLETED(Call))
              FAILS_WHEN(FAILED(AskName) || FAILED(AskNum) || FAILED(Call))
              ON_COMPLETION(RESET
                      CLEAR(userQuery)
                      CLEAR(userDomain)
                      CLEAR(userIntent)
                      RESET_AGENT(/Map/AskAnyQuery)
                      pDMCore->ContinueWith(&A(".."), &A("/Map/AskAnyQuery"))
                            )
              )

// /Telephone/Call
DEFINE_INFORM_AGENT(CCall,
                        PRECONDITION(AVAILABLE(phoneNum))
                        PROMPT("inform call_num <phoneNum")
)
    /*
DEFINE_EXECUTE_AGENT(CCall,
                     PRECONDITION(AVAILABLE(phoneNum))
                     CALL("backend.call query=call <phoneNum")
                     )
    */
// /Telephone/AskNum
DEFINE_REQUEST_AGENT(CAskNum,
                     TRIGGERED_BY("CALL_NUMBER" == string(C("userIntent")))
                     PRECONDITION(UNAVAILABLE(phoneNum))
                     REQUEST_CONCEPT(phoneNum)
                     GRAMMAR_MAPPING("@(/Map/Telephone)[number], [number]")
                     PROMPT("request say_the_num")
                     SUCCEEDS_WHEN(AVAILABLE(phoneNum))
                     )
// /Telephone/AskName
DEFINE_AGENCY(CAskName,
              PRECONDITION(UNAVAILABLE(phoneNum))
              TRIGGERED_BY(string(C("userIntent")) == "CALL_NAME")
              DEFINE_CONCEPTS(
                      STRING_USER_CONCEPT(peopleName, "expl")
                      CUSTOM_SYSTEM_CONCEPT(nameResult, CContactArrayResults)
                      INT_USER_CONCEPT(choice, "none")
                             )
              DEFINE_SUBAGENTS(
                      SUBAGENT(Name, CName, "")
                      SUBAGENT(SearchName, CSearchName, "")
                      SUBAGENT(NoName, CNoName, "")
                      SUBAGENT(FineOne, CFindOne, "")
                      SUBAGENT(InformMutilPer, CInformMutilPerson, "")
                      SUBAGENT(Choice3, CChoice3, "")
                              )
              SUCCEEDS_WHEN(AVAILABLE(phoneNum))
              FAILS_WHEN(COMPLETED(NoName))
              )
// /Telephone/AskName/Name
DEFINE_REQUEST_AGENT(CName,
                     PRECONDITION(UNAVAILABLE(peopleName))
                     REQUEST_CONCEPT(peopleName)
                     GRAMMAR_MAPPING("@(/Map/Telephone)[person], [name], [per]")
                     PROMPT("request tell_me_name")
                     SUCCEEDS_WHEN(AVAILABLE(peopleName))
                     )

// /Telephone/AskName/SearchName
DEFINE_EXECUTE_AGENT(CSearchName,
                     PRECONDITION(AVAILABLE(peopleName))
                     CALL("backend.call contact <peopleName nameResult>nameResult")
                     )
// /Telephone/AskName/NoName
DEFINE_INFORM_AGENT(CNoName,
                    PRECONDITION(SIZE(nameResult) == 0)
                    TRIGGERED_BY(UPDATED(nameResult) && SIZE(nameResult) == 0)
                    PROMPT("inform no_person <peopleName")
                    )

// /Telephone/AskName/FineOne
DEFINE_INFORM_AGENT(CFindOne,
                    PRECONDITION(SIZE(nameResult) == 1)
                    TRIGGERED_BY(UPDATED(nameResult) && SIZE(nameResult) == 1)
                    PROMPT("inform call_phone < peopleName")
                    ON_COMPLETION(C("phoneNum") = C("nameResult.-1"))
                    )
// /Telephone/AskName/
    DEFINE_INFORM_AGENT(CInformMutilPerson,
                        PRECONDITION(SIZE(nameResult) > 1)
                        PROMPT("inform mutil_person <nameResult")
)

// /Telephone/AskName/Choice3
DEFINE_REQUEST_AGENT(CChoice3,
                     PRECONDITION(SIZE(nameResult) > 1)
                     REQUEST_CONCEPT(choice)
                     GRAMMAR_MAPPING("![第一个]>1, ![第二个]>2")
                     PROMPT("request mutil_result_choice")
                     SUCCEEDS_WHEN(UPDATED(choice) && int(C("choice")) <= SIZE(nameResult) && int(C("choice")) >= 1)
                     ON_COMPLETION(C("phoneNum") = C("nameResult")[int(C("choice")) - 1]["phone"])
                     )

//-----------------------------------------------------------------------------
// /Map/Sc
//-----------------------------------------------------------------------------
DEFINE_AGENCY(CSc,
              TRIGGERED_BY("sc" == string(C("userDomain")))
              PRECONDITION(AVAILABLE(userQuery))
              DEFINE_CONCEPTS(
                      CUSTOM_SYSTEM_CONCEPT(scResult, CScResult)
                              )
              DEFINE_SUBAGENTS(
                      SUBAGENT(ScSearch, CScSearch, "")
                      SUBAGENT(ShowResult, CShowResult, "")
                               )
              ON_COMPLETION(
                      RESET
                      CLEAR(userQuery)
                      CLEAR(userDomain)
                      CLEAR(userIntent)
                      RESET_AGENT(/Map/AskAnyQuery)
                      pDMCore->ContinueWith(&A(".."), &A("/Map/AskAnyQuery"))
                            )
              )
// /Map/Sc/ScSearch
DEFINE_EXECUTE_AGENT(CScSearch,
                     PRECONDITION(AVAILABLE(userQuery))
                     CALL("backend.call sc <userQuery scResult>scResult")
                     )
// /Map/Sc/ShowResult
DEFINE_INFORM_AGENT(CShowResult,
                    PRECONDITION(AVAILABLE(scResult))
                    TRIGGERED_BY(UPDATED(scResult))
                    PROMPT("inform show_sc_result <userQuery <scResult")
                    )

//-----------------------------------------------------------------------------
// /Map/Music
//-----------------------------------------------------------------------------
DEFINE_AGENCY(CMusic,
              TRIGGERED_BY("music" == string(C("userDomain")))
              PRECONDITION(AVAILABLE(userQuery))
              DEFINE_CONCEPTS(
                              )
              DEFINE_SUBAGENTS(
                      SUBAGENT(PlayMusic, CPlayMusic, "")
                               )
              ON_COMPLETION(
                      RESET
                      CLEAR(userQuery)
                      CLEAR(userDomain)
                      CLEAR(userIntent)
                      RESET_AGENT(/Map/AskAnyQuery)
                      pDMCore->ContinueWith(&A(".."), &A("/Map/AskAnyQuery"))
                            )
              )
// /Map/Music/PlayMusic    
DEFINE_INFORM_AGENT(CPlayMusic,
                    PRECONDITION("music" == string(C("userDomain")))
                    PROMPT("inform play_music")
                    )
    

//-----------------------------------------------------------------------------
//
// AGENT DECLARATIONS
//
//-----------------------------------------------------------------------------
DECLARE_AGENTS(
	DECLARE_AGENT(CMap)
            DECLARE_AGENT(CWelcome)
            DECLARE_AGENT(CAskAnyQuery)
                DECLARE_AGENT(CAskQuery)
                DECLARE_AGENT(CAskDomain)
                DECLARE_AGENT(CAskIntent)
            DECLARE_AGENT(CNavi)
                DECLARE_AGENT(CSearch)
                    DECLARE_AGENT(CSearchPoi)
                        DECLARE_AGENT(CPoi)
                        DECLARE_AGENT(CInformProcessing)
                        DECLARE_AGENT(CCallMap)
                        DECLARE_AGENT(CNoResult)
                        DECLARE_AGENT(CSingleResult)
                        DECLARE_AGENT(CInformResult)
                        DECLARE_AGENT(CChoice)
                    DECLARE_AGENT(CSearchPoiType)
                        DECLARE_AGENT(CPoiType)
                        DECLARE_AGENT(CCallMap2)
                        DECLARE_AGENT(CSingleResult)
                        DECLARE_AGENT(CNoResult2)
                        DECLARE_AGENT(CInformResult2)
                        DECLARE_AGENT(CChoice2)
                DECLARE_AGENT(CMethod)
                DECLARE_AGENT(CBeginNavi)
            DECLARE_AGENT(CTelephone)
                DECLARE_AGENT(CAskName)
                    DECLARE_AGENT(CName)
                    DECLARE_AGENT(CSearchName)
                    DECLARE_AGENT(CNoName)
                    DECLARE_AGENT(CFindOne)
                    DECLARE_AGENT(CInformMutilPerson)
                    DECLARE_AGENT(CChoice3)
                DECLARE_AGENT(CAskNum)
                DECLARE_AGENT(CCall)
            DECLARE_AGENT(CSc)
                DECLARE_AGENT(CScSearch)
                DECLARE_AGENT(CShowResult)
            DECLARE_AGENT(CMusic)
                DECLARE_AGENT(CPlayMusic)
               )
//-----------------------------------------------------------------------------
// DIALOG TASK ROOT DECLARATION
//-----------------------------------------------------------------------------
DECLARE_DIALOG_TASK_ROOT(Map, CMap, "")
//#else
//#endif //NAVI