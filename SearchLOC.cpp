#include "DialogTask/DialogTask.h"


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
	// /define loc search result
	//-----------------------------------------------------------------------------
	DEFINE_FRAME_CONCEPT_TYPE(ClocInfo,
	ITEMS(
	STRING_ITEM(locName)
	FLOAT_ITEM(longitude)
	FLOAT_ITEM(latitude)
	STRING_ITEM(phone)
	)
	)

	// Array of results from the backend
	DEFINE_ARRAY_CONCEPT_TYPE(CLocInfo, ClocArrayResults)
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


//---------------------------------------------------------------------------------

// /Map/Navi/Search/SearchLOC
DEFINE_AGENCY(CSearchLOC,
	PRECONDITION(UPDATED(loc))
	TRIGGERED_BY(UPDATED(loc))
	DEFINE_CONCEPTS(
		CUSTOM_SYSTEM_CONCEPT(searchResult, CArrayResults)
		BOOL_USER_CONCEPT(confirm, "none")
		INT_USER_CONCEPT(choice, "none")
	)
	DEFINE_SUBAGENTS(
		SUBAGENT(Loc, CLoc, "")
		SUBAGENT(InformProcess, CInformProcess, "")
		SUBAGENT(CallMap, CCallMap, "")
		SUBAGENT(NoResult, CNoResult, "")
		SUBAGENT(SingleResult, CSingleResult, "")
		SUBAGENT(ShowResult, CShowResult, "")
		SUBAGENT(Choice, CChoice, "")
	)
	SUCCEEDS_WHEN(SIZE(searchResult) > 0 && UPDATED(choice) && 
		int(C("choice")) > 0 && int(C("choice")) <= SIZE(searchResult))
	FAILS_WHEN(COMPLETED(InformNoResult))
)
// /Map/Navi/Search/SearchLOC/Loc
DEFINE_EXPECT_AGENT(CLoc,
	EXPECT_CONCEPT(loc)
	GRAMMAR_MAPPING("@(/Map/Navi)[loc], [destination_place]")
)
// /Map/Nav/Search/SearchLOC/InformProcess
DEFINE_INFORM_AGENT(CInformProcess,
	PRECONDITION(!AVAILABLE(searchResult))
	PROMPT("inform looking_up_database_first")
)
// /Map/Navi/Search/SearchLOC/CallMap
DEFINE_EXECUTE_AGENT(CCallMap,
	PRECONDITION(AVAILABLE(loc))
	CALL("backend.call location<loc result>searchResult")
)



	//####################################################################

// /Map/Navi/Search/SearchLOC/NoResult
DEFINE_INFORM_AGENT(CNoResult,
	PRECONDITION(SIZE(searchResult) == 0)
	TRIGGERED_BY(UPDATED(searchResult) && SIZE(searchResult) == 0)
	PROMPT(":non-interruptable inform no_result_loc <loc")
)
// /Map/Navi/Search/SearchLoc/singleResult
DEFINE_REQUEST_AGENT(CSingleResult,
	PRECONDITION(1 == SIZE(searchResult))
	REQUEST_CONCEPT(confirm)
	PROMPT("request one_result < searchResult.1, go to there?")
	GRAMMAR_MAPPING("![OK]>true, ![DENY]>false")
	ON_COMPLETION(
		if (IS_TRUE(confirm))
		{
			C("/Map/Navi/destination") = C("searchResult.1");
		}
	)
)
// /Map/Navi/Search/searchLoc/InformResult
DEFINE_INFORM_AGENT(CInformResult,
	PRECONDITION(UPDATED(searchResult) && SIZE(searchResult) > 1)
	PROMPT("inform map_search_result <loc <searchResult")
)
// /Map/Navi/Search/SearchLoc/Choice
DEFINE_REQUEST_AGENT(CChoice,
	PRECONDITION(SIZE(searchResult) > 1)
	REQUEST_CONCEPT(choice)
	PROMPT("request mutil_result_choice")
	GRAMMAR_MAPPING("![第一个]>1, ![第二个]>2, ![第三个]>3")
	ON_COMPLETION(
		C("/Map/Navi/destination") = C("searchResult")[int(C("choice"))-1];
	)
)


	//################################################################
	
	DEFINE_AGENCY(Clogin,
		DEFINE_CONCEPTS(
			BOOL_USER_CONCEPT(registered, "default")
			STRING_USER_CONCEPT(user_name)
		)
		DEFINE_SUBAGENTS(
			SUBAGENT(Welcome, CWelcome, "")
			SUBAGENT(AskRegistered, CAskRegistered, "default")
			SUBAGENT(AskName, CAskName, "default")
			SUBAGENT(GreetUser, CGreetUser, "default")
		)
		SUCCEEDS_WHEN(COMPLETED(GreetUser))
	)
	DEFINE_INFORM_AGENT(CWelcome,
		PROMPT(":no-interruptible inform welcome")
	)
	DEFINE_REQUEST_AGENT(CAskRegistered,
		REQUEST_CONCEPT(registered)
		GRAMMAR_MAPPING("[Yes]>true, [No]>false")
	)
	DEFINE_REQUEST_AGENT(CAskName,
		PRECONDITION(IS_TRUE(registered))
		REQUEST_CONCEPT(user_name)
		GRAMMAR_MAPPING("[UserName]")
	)
	DEFINE_INFORM_AGENT(GreetUser,
		PROMPT("inform greet_user <registered <user_")
	)


