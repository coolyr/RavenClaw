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
// MEETINGLINEDIALOGTASK.CPP - This module contains the description of the 
//                             dialog task, (i.e. all the user defined 
//                             agencies, etc) for the MeetingLine system
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
//   [2006-03-22] (dbohus): started working on this - created a skeleton DM
//                          starting from the RoomLine one
//
//-----------------------------------------------------------------------------

#include "DialogTask.h"

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
// 核心配置
//
//-----------------------------------------------------------------------------
CORE_CONFIGURATION(
	
    // declare the NLG as the single output device
	//将NLG声明为单个输出设备
	USE_OUTPUT_DEVICES(DEFAULT_OUTPUT_DEVICE("nlg", "nlg.launch_query", 1))

	// declare the library agents to be used in the tree the terminate agent
	// 声明要在树中使用的 库代理  终止代理
    USE_LIBRARY_AGENTS(
		//CDTTManagerAgent::Use(string sDAType, string sDAName, FRegisterAgent fRegisterAgent, string sDAConfiguration)
	    LIBRARY_AGENT(CTerminateAndCloseSession, TerminateAndCloseSession, RegisterTerminateAndCloseSessionAgent, "")

        // various help agents
        LIBRARY_AGENT(CHelp, Help, RegisterHelpAgent, "")
        LIBRARY_AGENT(CHelpGetTips, HelpGetTips, RegisterHelpGetTipsAgent, "")
        LIBRARY_AGENT(CHelpEstablishContext, HelpEstablishContext, RegisterHelpEstablishContextAgent, "")
        LIBRARY_AGENT(CHelpWhatCanISay, HelpWhatCanISay,  RegisterHelpWhatCanISayAgent, "")
		LIBRARY_AGENT(CHelpSystemCapabilities, HelpSystemCapabilities,RegisterHelpSystemCapabilitiesAgent, "")

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

    // declare the grounding model types to be used
	// 声明要使用的接地模型类型
    USE_ALL_GROUNDING_MODEL_TYPES

	// declare the grounding actions to be used
	// 声明要使用的接地操作
	USE_ALL_GROUNDING_ACTIONS("")
)

//-----------------------------------------------------------------------------
//
// STRUCTURED CONCEPT DEFINITIONS
// concept定义
//-----------------------------------------------------------------------------

// User Profile concept type
DEFINE_FRAME_CONCEPT_TYPE( CUserProfileConcept,
    ITEMS(
        STRING_ITEM(user_name)
        STRING_ITEM(name)
        BOOL_ITEM(valid_user) )
)

// array holding participants
//数组保存参与者
DEFINE_ARRAY_CONCEPT_TYPE( CStringConcept, 
	CParticipantsArrayConcept 
)

// structure holding a note (or an action item)
DEFINE_FRAME_CONCEPT_TYPE( CNoteItemConcept,
    ITEMS(
        STRING_ITEM(type)
        STRING_ITEM(id)
        STRING_ITEM(text)
        STRING_ITEM(author)
        STRING_ITEM(timeStamp)
        STRING_ITEM(meetingName)
        STRING_ITEM(startsWhen)
        STRING_ITEM(dueBy)
        STRING_ITEM(whoResponsible)
        STRING_ITEM(comments))
)

// array holding notes
DEFINE_ARRAY_CONCEPT_TYPE( CNoteItemConcept, CNoteItemArrayConcept )

// stucture holding an agenda item
DEFINE_FRAME_CONCEPT_TYPE( CAgendaItemConcept,
    ITEMS(
        STRING_ITEM(id)
        STRING_ITEM(text)
        STRING_ITEM(author)
        STRING_ITEM(timeStamp)
        STRING_ITEM(meetingName)
        CUSTOM_ITEM(notes, CNoteItemArrayConcept))
)

// array holding agenda items
DEFINE_ARRAY_CONCEPT_TYPE(CAgendaItemConcept, CAgendaItemArrayConcept)

// Meeting concept type
DEFINE_FRAME_CONCEPT_TYPE( CMeetingConcept,
    ITEMS(
        STRING_ITEM(meetingName)
        STRING_ITEM(meeting_referent)
        CUSTOM_ITEM(participants, CParticipantsArrayConcept)
        CUSTOM_ITEM(agendas, CAgendaItemArrayConcept)
    )
)

//-----------------------------------------------------------------------------
//
// AGENT SPECIFICATIONS
// agent说明
//-----------------------------------------------------------------------------
 
//-----------------------------------------------------------------------------
// /MeetingLine 
//-----------------------------------------------------------------------------
DEFINE_AGENCY(CMeetingLine,
	IS_MAIN_TOPIC()
	DEFINE_CONCEPTS(
	    CUSTOM_SYSTEM_CONCEPT(user_profile, CUserProfileConcept)
	    CUSTOM_SYSTEM_CONCEPT(meeting, CMeetingConcept))
	DEFINE_SUBAGENTS(
		SUBAGENT(Welcome, CWelcome, "")
		SUBAGENT(Login, CLogin, "")
		SUBAGENT(IdentifyMeeting, CIdentifyMeeting, "")
		SUBAGENT(DiscussMeeting, CDiscussMeeting, "")
		SUBAGENT(Logout, CLogout, ""))
	SUCCEEDS_WHEN(COMPLETED(Logout))
)

//-----------------------------------------------------------------------------
// /MeetingLine/Welcome
//-----------------------------------------------------------------------------
DEFINE_INFORM_AGENT( CWelcome,
    PROMPT(":non-interruptable inform welcome")
)

//-----------------------------------------------------------------------------
// /MeetingLine/Login
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CLogin, 
    DEFINE_CONCEPTS(
        STRING_USER_CONCEPT(user_pin, "none"))
    DEFINE_SUBAGENTS(
        SUBAGENT(RequestPIN, CRequestPIN, "request_default")
        SUBAGENT(CheckPIN, CCheckPIN, "")
        SUBAGENT(GreetUser, CGreetUser, "")
        SUBAGENT(UnknownUser, CUnknownUser, ""))
    SUCCEEDS_WHEN(COMPLETED(GreetUser))
)
    
// /MeetingLine/Login/RequestPIN
DEFINE_REQUEST_AGENT( CRequestPIN, 
    REQUEST_CONCEPT(user_pin)
    // INPUT_LINE_CONFIGURATION("set_dtmf_len=4")
    GRAMMAR_MAPPING("![UserPin]")
)

// /MeetingLine/Login/CheckPIN
DEFINE_EXECUTE_AGENT( CCheckPIN,
    CALL("gal_be.launch_query query=check_user <user_pin >user_profile")
)

// /MeetingLine/Login/GreetUser
DEFINE_INFORM_AGENT( CGreetUser, 
    PRECONDITION(IS_TRUE(user_profile.valid_user))
    PROMPT("inform greet_user <user_profile")
)    

// /MeetingLine/Login/UnknownUser
DEFINE_INFORM_AGENT( CUnknownUser,
    PRECONDITION(IS_FALSE(user_profile.valid_user))
    PROMPT("inform unknown_user")
)

//-----------------------------------------------------------------------------
// /MeetingLine/IdentifyMeeting
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CIdentifyMeeting, 
    DEFINE_CONCEPTS(
    	STRING_USER_CONCEPT(meeting_referent, "expl_impl"))
    DEFINE_SUBAGENTS(
        SUBAGENT(WhichMeeting, CWhichMeeting, "request_default")
        SUBAGENT(ResolveMeeting, CResolveMeeting, ""))
    TRIGGERED_BY(UPDATED(meeting_referent))
    SUCCEEDS_WHEN(AVAILABLE(meeting) && !UPDATED(meeting_referent))
)

// /MeetingLine/IdentifyMeeting/WhichMeeting
DEFINE_REQUEST_AGENT( CWhichMeeting,
    REQUEST_CONCEPT(meeting_referent)
    GRAMMAR_MAPPING(
        "[MeetingReferent.last_meeting]>last_meeting, [MeetingReferent.previous_meeting]>previous_meeting")
)

// /MeetingLine/IdentifyMeeting/ResolveMeeting
DEFINE_EXECUTE_AGENT( CResolveMeeting,
    CALL("gal_be.launch_query query=resolve_meeting <meeting_referent <meeting >meeting")
    ON_COMPLETION(
        REOPEN_CONCEPT(meeting_referent); 
        C("/MeetingLine/DiscussMeeting/query").MergeHistory();
        RESET)
)

//-----------------------------------------------------------------------------
// /MeetingLine/DiscussMeeting
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CDiscussMeeting,
    DEFINE_CONCEPTS(
        STRING_USER_CONCEPT(query, "expl"))
    DEFINE_SUBAGENTS(
        SUBAGENT(GetRequest, CGetRequest, "request_default")
        SUBAGENT(InformWhen, CInformWhen, "")
        SUBAGENT(InformWho, CInformWho, "")
        SUBAGENT(InformAgenda, CInformAgenda, "")
        SUBAGENT(DiscussActionItems, CDiscussActionItems, "")
        SUBAGENT(AnythingElse, CAnythingElse, ""))
    SUCCEEDS_WHEN(C("query") == CString("nothing"))        
)

// /MeetingLine/DiscussMeeting/GetRequest
DEFINE_REQUEST_AGENT( CGetRequest,
    REQUEST_CONCEPT(query)
    PROMPT("request query <meeting")
    GRAMMAR_MAPPING("[QueryWhen]>when, [QueryWho]>who, [QueryAgenda]>agenda, "
                    "[QueryActionItems]>action_items")
)

// /MeetingLine/DiscussMeeting/InformWhen
DEFINE_INFORM_AGENT( CInformWhen,
    PRECONDITION((C("query") == CString("when")) || 
                 (C("query") == CString("when_and_who")))
    PROMPT("inform when <meeting")
    ON_COMPLETION(REOPEN_CONCEPT(query); RESET)
)

// /MeetingLine/DiscussMeeting/InformWho
DEFINE_INFORM_AGENT( CInformWho,
    PRECONDITION((C("query") == CString("who")) || 
                 (C("query") == CString("who_and_agenda")) ||
                 (C("query") == CString("when_and_who")))
    PROMPT("inform who <meeting <user_profile")
    ON_COMPLETION(REOPEN_CONCEPT(query); RESET)
)

// /MeetingLine/DiscussMeeting/InformAgenda
DEFINE_INFORM_AGENT( CInformAgenda,
    PRECONDITION((C("query") == CString("agenda")) ||
                 (C("query") == CString("who_and_agenda")))
    PROMPT("inform agenda <meeting")
    ON_COMPLETION(REOPEN_CONCEPT(query); RESET)
)

// /MeetingLine/DiscussMeeting/DiscussActionItems
DEFINE_AGENCY( CDiscussActionItems,
    PRECONDITION(C("query") == CString("action_items"))
    DEFINE_CONCEPTS(
        STRING_USER_CONCEPT(action_items_for_user, "expl"))
    DEFINE_SUBAGENTS(
        SUBAGENT(ExpectActionItemsForUser, CExpectActionItemsForUser, "")
        SUBAGENT(InformActionItems, CInformActionItems, ""))
    SUCCEEDS_WHEN(COMPLETED(InformActionItems))
    ON_COMPLETION(REOPEN_CONCEPT(query); RESET)
)

// /MeetingLine/DiscussMeeting/DiscussActionItems/ExpectActionItemsForUser
DEFINE_EXPECT_AGENT( CExpectActionItemsForUser, 
    EXPECT_CONCEPT(action_items_for_user)
    GRAMMAR_MAPPING("[QueryActionItems.user_name]")
)

// /MeetingLine/DiscussMeeting/DiscussActionItems/InformActionItems
DEFINE_INFORM_AGENT( CInformActionItems,
    PROMPT("inform action_items <meeting <action_items_for_user <user_profile")
)

// //MeetingLine/DiscussMeeting/AnythingElse
DEFINE_REQUEST_AGENT( CAnythingElse,
    PRECONDITION(!UPDATED(query))
    REQUEST_CONCEPT(query)
    TIMEOUT_PERIOD(10)
    PROMPT(":non-repeatable request anything_else")
    GRAMMAR_MAPPING("[QueryWhen]>when, [QueryWho]>who, [QueryAgenda]>agenda, "
                    "[QueryActionItems]>action_items, "
                    "![Quit]>nothing, ![No]>nothing")
    ON_COMPLETION(RESET)
)               

//-----------------------------------------------------------------------------
// /MeetingLine/Logout
//-----------------------------------------------------------------------------
DEFINE_INFORM_AGENT( CLogout, 
    PROMPT("inform logout")
)
    

//-----------------------------------------------------------------------------
//
// AGENT DECLARATIONS
//
//-----------------------------------------------------------------------------
DECLARE_AGENTS(
	DECLARE_AGENT(CMeetingLine)
        DECLARE_AGENT(CWelcome)
		DECLARE_AGENT(CLogin)
            DECLARE_AGENT(CRequestPIN)
            DECLARE_AGENT(CCheckPIN)
            DECLARE_AGENT(CGreetUser)
            DECLARE_AGENT(CUnknownUser)
		DECLARE_AGENT(CIdentifyMeeting)
		    DECLARE_AGENT(CWhichMeeting)
		    DECLARE_AGENT(CResolveMeeting)
        DECLARE_AGENT(CDiscussMeeting)
            DECLARE_AGENT(CGetRequest)
            DECLARE_AGENT(CInformWhen)
            DECLARE_AGENT(CInformWho)
            // what was discussed
            DECLARE_AGENT(CInformAgenda)
            // what was decided
            DECLARE_AGENT(CDiscussActionItems)
                DECLARE_AGENT(CExpectActionItemsForUser)
                DECLARE_AGENT(CInformActionItems)
            DECLARE_AGENT(CAnythingElse)
		DECLARE_AGENT(CLogout)
)

//-----------------------------------------------------------------------------
// DIALOG TASK ROOT DECLARATION
//-----------------------------------------------------------------------------
DECLARE_DIALOG_TASK_ROOT(MeetingLine, CMeetingLine, "")