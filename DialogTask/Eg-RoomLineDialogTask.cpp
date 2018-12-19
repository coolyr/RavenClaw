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
// ROOMLINEDIALOGTASK.CPP - This module contains the description of the dialog 
//                          task, (i.e. all the user defined agencies, etc)
//                          for the RoomLine system
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
//   [2002-10-16] (dbohus): started working on this - created a skeleton DM
//
//-----------------------------------------------------------------------------

#include "DialogTask.h"
#include "DateTimeBindingFilters.h"

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
	
    // declare the NLG as the single output device
	USE_OUTPUT_DEVICES(DEFAULT_OUTPUT_DEVICE("nlg", "nlg.launch_query", 1))

	// declare the library agents to be used in the tree
    // the terminate agent
    USE_LIBRARY_AGENTS(
	    LIBRARY_AGENT(CTerminate, Terminate, RegisterTerminateAgent, "")

        // various help agents
        LIBRARY_AGENT(CHelp, Help, RegisterHelpAgent, "")
        LIBRARY_AGENT(CHelpGetTips, HelpGetTips, RegisterHelpGetTipsAgent, "")
        LIBRARY_AGENT(CHelpEstablishContext, HelpEstablishContext, 
            RegisterHelpEstablishContextAgent, "")
        LIBRARY_AGENT(CHelpWhatCanISay, HelpWhatCanISay, 
            RegisterHelpWhatCanISayAgent, "")
		LIBRARY_AGENT(CHelpSystemCapabilities, HelpSystemCapabilities,
			RegisterHelpSystemCapabilitiesAgent, "")

        // repeat handling agent
        LIBRARY_AGENT(CRepeat, Repeat, RegisterRepeatAgent, "")

        // suspend handling agency
        LIBRARY_AGENT(CSuspend, Suspend, RegisterSuspendAgency, "")

        // timeout handling agency
        LIBRARY_AGENT(CTimeoutTerminate, TimeoutTerminate, 
            RegisterTimeoutTerminateAgency, "")

        // start-over handling agency
        LIBRARY_AGENT(CStartOver, StartOver, RegisterStartOverAgency, "")

        // quit handling agency
        LIBRARY_AGENT(CQuit, Quit, RegisterQuitAgency, "")
    )

    // declare the grounding model types to be used
    USE_ALL_GROUNDING_MODEL_TYPES

	// declare the grounding actions to be used
	USE_ALL_GROUNDING_ACTIONS("")

    // declare the binding filters
    USE_BINDING_FILTERS(
        // the date-time binding filter
        BINDING_FILTER("datetime", DateTime4_DateTimeBindingFilter)
        // the end-time binding filter
        BINDING_FILTER("endtime", DateTime4_EndTimeBindingFilter)
        // the number binding filter (used for room sizes)
        BINDING_FILTER("number", DateTime4_NumberBindingFilter)
    )
)

//-----------------------------------------------------------------------------
//
// STRUCTURED CONCEPT TYPES SPECIFICATION
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// D: Definition of the CDateTime structured concept class; this class follows
//    the return specification of the DateTime4 Hub Galaxy agent
//-----------------------------------------------------------------------------

// Date concept
DEFINE_STRUCT_CONCEPT_TYPE( CDateConcept,     
    ITEMS(
        INT_ITEM( month )
        INT_ITEM( day )
        INT_ITEM( year )
        BOOL_ITEM( valid_date )
        BOOL_ITEM( past_date )
        INT_ITEM( weekday )
        BOOL_ITEM( ambiguous )
        STRING_ITEM( date_choice ) )    
    
    DEFINE_PRIOR(
    
        // if there is not hypothesis, return 1 as the prior
        if(pHyp == NULL) return 1;
    
        // array definining the number of days until a certain month
        int viDaysUntilMonth[13];
        viDaysUntilMonth[0] = 0;
        viDaysUntilMonth[1] = 0;        
        viDaysUntilMonth[2] = 31;
        viDaysUntilMonth[3] = 59;        
        viDaysUntilMonth[4] = 90;
        viDaysUntilMonth[5] = 120;        
        viDaysUntilMonth[6] = 151;
        viDaysUntilMonth[7] = 181;        
        viDaysUntilMonth[8] = 212;
        viDaysUntilMonth[9] = 242;        
        viDaysUntilMonth[10] = 273;
        viDaysUntilMonth[11] = 303;        
        viDaysUntilMonth[12] = 334;
    
        // check that we have a valid date
		CHyp* pValidDateHyp = pHyp->operator []("valid_date");
		bool bValidDate = pValidDateHyp && (pValidDateHyp->ValueToString()=="true");
		// if the date is not valid 
		if(!bValidDate) return 1;
		
        // compute the month, day and year for this hyp		
		CHyp* pMonthHyp = pHyp->operator []("month");
		CHyp* pDayHyp = pHyp->operator []("day");
		CHyp* pYearHyp = pHyp->operator []("year");
		int iMonth = pMonthHyp?atoi(pMonthHyp->ValueToString().c_str()):-1;
		int iDay = pDayHyp?atoi(pDayHyp->ValueToString().c_str()):-1;
		int iYear = pYearHyp?atoi(pYearHyp->ValueToString().c_str()):-1;
		// compute the date
		int iDate = iDay + viDaysUntilMonth[iMonth] + (iYear-2000)*365;		
		// do the difference to 2005/12/11
		int iDOW = ((iDate - 2170) % 7) + 1;		
		
		// get the month, day and year for the current sessions
        struct tm *now;
        __time64_t long_time;
        _time64( &long_time );
        now = _localtime64( &long_time );        
        int iNowMonth = now->tm_mon + 1;
        int iNowYear = 1900 + now->tm_year;
        int iNowDay = now->tm_mday;
        // compute the date for now
        int iNowDate = iNowDay + viDaysUntilMonth[iNowMonth] + (iNowYear-2000)*365;

        // compute the difference
        int iRelDate = iDate - iNowDate;  
        float fPrior = 0.0;   
        if(iRelDate < 0)
            fPrior = 0;
        else if(iRelDate > 28)
            fPrior = (float)(365.0 / 830.0);
        else if(iRelDate > 16)
            fPrior = (float)(10*365.0 / 830.0);
        else 
            fPrior = (float)(20*365.0 / 830.0);
        if((iDOW == 1) || (iDOW == 7))
            fPrior = (float)(365.0 / 830.0);
        return fPrior;
    )
    
    DEFINE_CONFUSABILITY(

        // if there is not hypothesis, return through the grounding manager
        if(pHyp == NULL) 
            return pGroundingManager->GetConfusabilityForConceptHyp(
                GetSmallName(), "<UNDEFINED>");
    
        // array definining the number of days until a certain month
        int viDaysUntilMonth[13];
        viDaysUntilMonth[0] = 0;
        viDaysUntilMonth[1] = 0;        
        viDaysUntilMonth[2] = 31;
        viDaysUntilMonth[3] = 59;        
        viDaysUntilMonth[4] = 90;
        viDaysUntilMonth[5] = 120;        
        viDaysUntilMonth[6] = 151;
        viDaysUntilMonth[7] = 181;        
        viDaysUntilMonth[8] = 212;
        viDaysUntilMonth[9] = 242;        
        viDaysUntilMonth[10] = 273;
        viDaysUntilMonth[11] = 303;        
        viDaysUntilMonth[12] = 334;

        // check that we have a valid date
		CHyp* pValidDateHyp = pHyp->operator []("valid_date");
		bool bValidDate = pValidDateHyp && (pValidDateHyp->ValueToString()=="true");
		// if the date is not valid 
		if(!bValidDate) 
		    return pGroundingManager->GetConfusabilityForConceptHyp(
		        GetSmallName(), "<UNDEFINED>");
		
        // compute the month, day and year for this hyp		
		CHyp* pMonthHyp = pHyp->operator []("month");
		CHyp* pDayHyp = pHyp->operator []("day");
		CHyp* pYearHyp = pHyp->operator []("year");
		int iMonth = pMonthHyp?atoi(pMonthHyp->ValueToString().c_str()):-1;
		int iDay = pDayHyp?atoi(pDayHyp->ValueToString().c_str()):-1;
		int iYear = pYearHyp?atoi(pYearHyp->ValueToString().c_str()):-1;
		// compute the date
		int iDate = iDay + viDaysUntilMonth[iMonth] + (iYear-2000)*365;		
		// do the difference to 2005/12/11
		int iDOW = ((iDate - 2170) % 7) + 1;		

        if((iMonth == -1) || (iDay == -1) ||  (iYear == -1)) 
            return pGroundingManager->GetConfusabilityForConceptHyp(
                GetSmallName(), "<UNDEFINED>");
        else 
    	    return pGroundingManager->GetConfusabilityForConceptHyp(
		        GetSmallName(), FormatString("%d", iDOW));
	)
)

// Specialized version of int for start_time and end_time
DEFINE_CONCEPT_TYPE(CIntTimeConcept, CIntConcept, 
    DEFINE_PRIOR(
	    // by default, first look at the information from the grounding manager
	    // about priors on a concept
	    string sValue;
	    if(pHyp) {
	        // convert it to an integer
	        int iTime = atoi(pHyp->ValueToString().c_str());
	        int iHour = iTime/100;
	        int iMin = iTime % 100;
	        if(iMin == 0) {
	            sValue = FormatString("%d00", iHour);
	        } else if(iMin <= 20) {
	            sValue = FormatString("%d15", iHour);
	        } else if(iMin <= 40) {
	            sValue = FormatString("%d30", iHour);
	        } else {
	            sValue = FormatString("%d45", iHour);	            
	         }
	    } else {
	        sValue = "<UNDEFINED>";
	    }
	    float fPrior = pGroundingManager->GetPriorForConceptHyp(
	        GetSmallName(), sValue);
	    // if the grounding manager has information about it
	    if(fPrior != -1) {
		    // then return it
		    return fPrior;
	    } 
	    // otherwise return a uniform prior
	   return (float) 1.0 / (iCardinality + 1);
    )
    DEFINE_CONFUSABILITY(
	    // by default, first look at the information from the grounding manager
	    // about priors on a concept
	    string sValue;
	    if(pHyp) {
	        // convert it to an integer
	        int iTime = atoi(pHyp->ValueToString().c_str());
	        int iHour = iTime/100;
	        int iMin = iTime % 100;
	        if(iMin == 0) {
	            sValue = FormatString("%d00", iHour);
	        } else if(iMin <= 20) {
	            sValue = FormatString("%d15", iHour);
	        } else if(iMin <= 40) {
	            sValue = FormatString("%d30", iHour);
	        } else {
	            sValue = FormatString("%d45", iHour);	            
	         }
	    } else {
	        sValue = "<UNDEFINED>";
	    }
	    float fConfusability = 
	        pGroundingManager->GetConfusabilityForConceptHyp(
	            GetSmallName(), sValue);
	    // if the grounding manager has information about it
	    if(fConfusability != -1) {
		    // then return it
		    return fConfusability;
	    } 
	    // otherwise return a uniform prior
	    return (float) 1.0 / (iCardinality + 1);
    )
)

// Specialized version of string for location 
DEFINE_CONCEPT_TYPE(CStringLocationConcept, CStringConcept, 
    DEFINE_PRIOR(
	    // by default, first look at the information from the grounding manager
	    // about priors on a concept
	    string sValue;
	    if(pHyp) {
	        // convert it to an integer
	        string sLocation = ToLowerCase(pHyp->ValueToString());
	        if((sLocation == "wean hall") ||
	           (sLocation == "wean")) {
	            sValue = "weh";
	        } else if((sLocation == "newell simon hall") ||
	                  (sLocation == "newell simon")) {
	            sValue = "nsh";
	        }
	    } else {
	        sValue = "<UNDEFINED>";
	    }
	    float fPrior = pGroundingManager->GetPriorForConceptHyp(
	        GetSmallName(), sValue);
	    // if the grounding manager has information about it
	    if(fPrior != -1) {
		    // then return it
		    return fPrior;
	    } 
	    // otherwise return a uniform prior
	    return (float) 1.0 / (iCardinality + 1);
    )
    DEFINE_CONFUSABILITY(
	    // by default, first look at the information from the grounding manager
	    // about priors on a concept
	    string sValue;
	    if(pHyp) {
	        // convert it to an integer
	        string sLocation = ToLowerCase(pHyp->ValueToString());
	        if((sLocation == "wean hall") ||
	           (sLocation == "wean")) {
	            sValue = "weh";
	        } else if((sLocation == "newell simon hall") ||
	                  (sLocation == "newell simon")) {
	            sValue = "nsh";
	        }
	    } else {
	        sValue = "<UNDEFINED>";
	    }
	    float fConfusability = 
	        pGroundingManager->GetConfusabilityForConceptHyp(
	            GetSmallName(), sValue);
	    // if the grounding manager has information about it
	    if(fConfusability != -1) {
		    // then return it
		    return fConfusability;
	    } 
	    // otherwise return a uniform prior
	    return (float) 1.0 / (iCardinality + 1);
    )
)

// Specialized version of int for room sizes
DEFINE_CONCEPT_TYPE(CIntSizeConcept, CIntConcept, 
    DEFINE_PRIOR(
	    // by default, first look at the information from the grounding manager
	    // about priors on a concept
	    if(pHyp) {
	        // convert it to an integer
	        int iValue = atoi(pHyp->ValueToString().c_str());
	        if(((iValue <= 10) && (iValue >= 4)) ||
	            (iValue == 15) || (iValue == 20) ||
	            (iValue == 25) || (iValue == 30) ||
	            (iValue == 35) || (iValue == 40) ||
	            (iValue == 45) || (iValue == 50) ||
	            (iValue == 55) || (iValue == 60) ||
	            (iValue == 65) || (iValue == 70) ||
	            (iValue == 75) || (iValue == 80) ||
	            (iValue == 85) || (iValue == 90) ||
	            (iValue == 95) || (iValue == 100)) {
	            return (float) 6.570302;
	        } else {
	            return (float) 0.657030;
	        }
	    } else {
	        float fPrior = pGroundingManager->GetPriorForConceptHyp(
	            GetSmallName(), "<UNDEFINED>");
	        // if the grounding manager has information about it
	        if(fPrior != -1) {
		        // then return it
		        return fPrior;
	        } 
	        // otherwise return a uniform prior
	        return (float) 1.0 / (iCardinality + 1);
	    }
    )
)

// Time concept
DEFINE_FRAME_CONCEPT_TYPE( CTimeConcept, 
    ITEMS(
        INT_ITEM( time_duration )
        CUSTOM_ITEM( start_time, CIntTimeConcept )
        CUSTOM_ITEM( end_time, CIntTimeConcept )
        STRING_ITEM( timeperiod_spec ))
)

// DateTime concept
DEFINE_FRAME_CONCEPT_TYPE( CDateTimeConcept,
    ITEMS(
	    CUSTOM_ITEM(date, CDateConcept)
	    CUSTOM_ITEM(time, CTimeConcept))
)

// Room query concept
DEFINE_FRAME_CONCEPT_TYPE( CRoomQuery,
    ITEMS(
        CUSTOM_ITEM( date_time, CDateTimeConcept )
        CUSTOM_ITEM( room_location, CStringLocationConcept )
        INT_ITEM( room_number )
        CUSTOM_ITEM( room_size, CIntSizeConcept )
        STRING_ITEM( room_size_spec )
        BOOL_ITEM( room_projector )
        BOOL_ITEM( room_network )
        BOOL_ITEM( room_whiteboard )
        BOOL_ITEM( room_computer ))
)

// Room query refinement concept
DEFINE_FRAME_CONCEPT_TYPE( CRoomQueryRefinement,
    ITEMS(
        CUSTOM_ITEM( location, CStringLocationConcept )
        CUSTOM_ITEM( size, CIntSizeConcept )
        STRING_ITEM( size_spec )
        STRING_ITEM( equip ))
)

// Room result concept
DEFINE_FRAME_CONCEPT_TYPE( CRoomResult, 
    ITEMS(
        CUSTOM_ITEM( date_time, CDateTimeConcept )
        CUSTOM_ITEM( room_location, CStringLocationConcept )
        INT_ITEM( room_number )
        CUSTOM_ITEM( room_size, CIntSizeConcept )
        STRING_ITEM( room_size_spec )
        BOOL_ITEM( room_projector )
        BOOL_ITEM( room_network )
        BOOL_ITEM( room_whiteboard )
        BOOL_ITEM( room_computer ))
)

// Array of results from the backend
DEFINE_ARRAY_CONCEPT_TYPE(CRoomResult, CQueryResults)

//-----------------------------------------------------------------------------
//
// AGENT SPECIFICATIONS
//
//-----------------------------------------------------------------------------
 
//-----------------------------------------------------------------------------
// /RoomLine
//-----------------------------------------------------------------------------
DEFINE_AGENCY(CRoomLine,
	IS_MAIN_TOPIC()
    DEFINE_CONCEPTS(
        BOOL_USER_CONCEPT(satisfied, "expl")
        CUSTOM_SYSTEM_CONCEPT(reserve_room, CRoomResult))
	DEFINE_SUBAGENTS(
        SUBAGENT(ResetDateTime, CResetDateTime, "")
		SUBAGENT(Welcome, CWelcome, "")
		SUBAGENT(Task, CTask, "")		
        SUBAGENT(AnythingElse, CAnythingElse, "")
		SUBAGENT(InformCancelPrevReservation, CInformCancelPrevReservation, "")
		SUBAGENT(Logout, CLogout, ""))
	SUCCEEDS_WHEN(COMPLETED(Logout)))

//-----------------------------------------------------------------------------
// /RoomLine/ResetDateTime
//-----------------------------------------------------------------------------
DEFINE_EXECUTE_AGENT( CResetDateTime,
    CALL("datetime.begin_session :non-blocking"))

//-----------------------------------------------------------------------------
// /RoomLine/Welcome
//-----------------------------------------------------------------------------
DEFINE_INFORM_AGENT( CWelcome,
    PROMPT(":non-interruptable inform welcome"))

//-----------------------------------------------------------------------------
// /RoomLine/Task
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CTask,
	IS_MAIN_TOPIC()
    DEFINE_CONCEPTS(
        CUSTOM_USER_CONCEPT(room_query, CRoomQuery, 
			"none, room_location=expl_impl, room_number=expl_impl, room_size=expl_impl,"\
			"room_size_spec=expl_impl, room_projector=expl_impl, room_network=expl_impl,"\
			"room_whiteboard=expl_impl, room_computer=expl_impl, date_time.date=expl_impl,"\
			"date_time.time.start_time=expl_impl, date_time.time.end_time=expl_impl,"\
			"date_time.time.timeperiod_spec=expl_impl, date_time.time.time_duration=expl_impl")
        CUSTOM_SYSTEM_CONCEPT(results, CQueryResults)        
		STRING_SYSTEM_CONCEPT(suggested_refine)
        BOOL_SYSTEM_CONCEPT(backend_error))
	DEFINE_SUBAGENTS(
		SUBAGENT(GetQuery, CGetQuery, "")
        SUBAGENT(InformPendingQuery, CInformPendingQuery, "")
		SUBAGENT(PerformQuery, CPerformQuery, "")
        SUBAGENT(BackendError, CBackendError, "")
		SUBAGENT(DiscussResults, CDiscussResults, ""))
    SUCCEEDS_WHEN(COMPLETED(GetQuery) && 
                  COMPLETED(InformPendingQuery) &&
                  COMPLETED(PerformQuery) &&
                  COMPLETED(DiscussResults))
)

// /RoomLine/Task/GetQuery
DEFINE_AGENCY( CGetQuery,
	IS_MAIN_TOPIC()
    DEFINE_CONCEPTS(
        BOOL_USER_CONCEPT(need_room, ""))
	DEFINE_SUBAGENTS(
        SUBAGENT(HowMayIHelpYou, CHowMayIHelpYou, "request_default")
        SUBAGENT(HowMayIHelpYouAgain, CHowMayIHelpYouAgain, "request_default")
		SUBAGENT(RequestDateTime, CRequestDateTime, "")
		SUBAGENT(ExpectRoomNumber, CExpectRoomNumber, "")
		SUBAGENT(ExpectRoomLocation, CExpectRoomLocation, "")
		SUBAGENT(ExpectRoomSize, CExpectRoomSize, "")
		SUBAGENT(ExpectRoomSizeSpec, CExpectRoomSizeSpec, "")
		SUBAGENT(ExpectRoomProjector, CExpectRoomProjector, "")
		SUBAGENT(ExpectRoomNetwork, CExpectRoomNetwork, "")
		SUBAGENT(ExpectRoomWhiteboard, CExpectRoomWhiteboard, "")
        SUBAGENT(ExpectRoomComputer, CExpectRoomComputer, "")
        SUBAGENT(ExpectNeedRoom, CExpectNeedRoom, ""))
    SUCCEEDS_WHEN(SUCCEEDED(RequestDateTime))
    PROMPT_ESTABLISHCONTEXT("establish_context get_query <room_query")
    ON_COMPLETION(
        int iEndTime = (int)(C("room_query.date_time.time.end_time"));
        int iStartTime = (int)(C("room_query.date_time.time.start_time"));        
        int iDuration = (iEndTime%100 + 60*(iEndTime/100)) - 
                        (iStartTime%100 + 60*(iStartTime/100));
        C("room_query.date_time.time.time_duration") = 
            100*(iDuration/60) + iDuration%60;)
)

// /RoomLine/Task/GetQuery/HowMayIHelpYou
DEFINE_REQUEST_AGENT( CHowMayIHelpYou, 
    PRECONDITION(UNAVAILABLE(room_query) && !A("..").WasReset())
    NONUNDERSTANDING_THRESHOLD(0)
    PROMPT(":non-interruptable request how_may_i_help_you")
    PROMPT_ESTABLISHCONTEXT("establish_context how_may_i_help_you")
    SUCCEEDS_WHEN(UPDATED(room_query) || UPDATED(need_room))
    MAX_ATTEMPTS(2)
)

// /RoomLine/Task/GetQuery/HowMayIHelpYouAgain
DEFINE_REQUEST_AGENT( CHowMayIHelpYouAgain, 
    PRECONDITION(UNAVAILABLE(room_query) && A("..").WasReset())
    PROMPT("request how_may_i_help_you_again")
    NONUNDERSTANDING_THRESHOLD(0)
    SUCCEEDS_WHEN(UPDATED(room_query) || UPDATED(need_room))
    MAX_ATTEMPTS(2)
)

// /RoomLine/Task/GetQuery/RequestDateTime
DEFINE_AGENCY( CRequestDateTime,	
    DEFINE_SUBAGENTS(
        SUBAGENT(RequestWhen, CRequestWhen, "request_default")
        SUBAGENT(RequestDay, CRequestDay, "request_default")
        SUBAGENT(RequestTime, CRequestTime, "request_default")
        SUBAGENT(RequestSpecificTimes, CRequestSpecificTimes, "request_default")
        SUBAGENT(RequestStartTime, CRequestStartTime, "request_default")
        SUBAGENT(RequestEndTime, CRequestEndTime, "request_default")
        SUBAGENT(DisambiguateDate, CDisambiguateDate, "")
        SUBAGENT(PastDate, CPastDate, "")
        SUBAGENT(WrongTimeOrder, CWrongTimeOrder, "")
        SUBAGENT(ComputeEndTimeFromDuration, CComputeEndTimeFromDuration, "")
        SUBAGENT(WrongDuration, CWrongDuration, ""))
    SUCCEEDS_WHEN(
        AVAILABLE(room_query.date_time.date.valid_date) && 
        AVAILABLE(room_query.date_time.time.start_time) && 
        AVAILABLE(room_query.date_time.time.end_time) && 
        C("room_query.date_time.time.start_time") < C("room_query.date_time.time.end_time"))
)

// /RoomLine/Task/GetQuery/RequestDateTime/RequestWhen
DEFINE_REQUEST_AGENT( CRequestWhen, 
    PRECONDITION(!IS_TRUE(room_query.date_time.date.valid_date) &&
                 UNAVAILABLE(room_query.date_time.time.start_time) &&
                 UNAVAILABLE(room_query.date_time.time.end_time))
	REQUEST_CONCEPT(room_query.date_time; room_query.date_time.date; 
	                room_query.date_time.time.end_time; room_query.date_time.time.start_time; 
	                room_query.date_time.time.timeperiod_spec; 
	                room_query.date_time.time.time_duration)
	PROMPT("request room_when")
    GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.date_time]>:datetime,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[DateTimeSpec.date_time]>:datetime")
    SUCCEEDS_WHEN(IS_TRUE(room_query.date_time.date.valid_date) ||
                  AVAILABLE(room_query.date_time.time.start_time) ||
                  AVAILABLE(room_query.date_time.time.end_time))
    MAX_ATTEMPTS(2)
)

// /RoomLine/Task/GetQuery/RequestDateTime/RequestDay
DEFINE_REQUEST_AGENT( CRequestDay, 
    PRECONDITION(!IS_TRUE(room_query.date_time.date.valid_date))
	REQUEST_CONCEPT(room_query.date_time;room_query.date_time.date)
	PROMPT("request room_day")
    GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.date_time]>:datetime,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[DateTimeSpec.date_time]>:datetime")
    SUCCEEDS_WHEN(IS_TRUE(room_query.date_time.date.valid_date))
)

// /RoomLine/Task/GetQuery/RequestDateTime/RequestTime
DEFINE_REQUEST_AGENT( CRequestTime,	
    PRECONDITION(UNAVAILABLE(room_query.date_time.time.start_time) &&
				 UNAVAILABLE(room_query.date_time.time.end_time) &&
				 UNAVAILABLE(room_query.date_time.time.time_duration))
	REQUEST_CONCEPT(room_query.date_time; room_query.date_time.time.start_time; 
	                room_query.date_time.time.end_time; room_query.date_time.time.timeperiod_spec; 
	                room_query.date_time.time.time_duration)
	PROMPT("request what_time")
    GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.date_time]>:datetime,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[DateTimeSpec.date_time]>:datetime")
    SUCCEEDS_WHEN(
        AVAILABLE(room_query.date_time.time.start_time) ||
        AVAILABLE(room_query.date_time.time.end_time) ||
        AVAILABLE(room_query.date_time.time.timeperiod_spec) ||
		AVAILABLE(room_query.date_time.time.time_duration))
    MAX_ATTEMPTS(2)
)

// /RoomLine/Task/GetQuery/RequestDateTime/RequestSpecificTimes
DEFINE_REQUEST_AGENT( CRequestSpecificTimes,
    PRECONDITION(SAME_AS_TRIGGER)
    TRIGGERED_BY(UPDATED(room_query.date_time.time.timeperiod_spec) &&
                 UNAVAILABLE(room_query.date_time.time.start_time) &&
                 UNAVAILABLE(room_query.date_time.time.end_time))
	REQUEST_CONCEPT(room_query.date_time; room_query.date_time.time.start_time; 
	                room_query.date_time.time.end_time)
	PROMPT("request what_specific_time <room_query.date_time.time.timeperiod_spec")
    GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.date_time]>:datetime,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[DateTimeSpec.date_time]>:datetime")
    SUCCEEDS_WHEN(
        AVAILABLE(room_query.date_time.time.start_time) ||
        AVAILABLE(room_query.date_time.time.end_time))
)

// /RoomLine/Task/GetQuery/RequestDateTime/RequestStartTime
DEFINE_REQUEST_AGENT( CRequestStartTime,
    PRECONDITION(UNAVAILABLE(room_query.date_time.time.start_time))
	REQUEST_CONCEPT(room_query.date_time; room_query.date_time.time.start_time)
	PROMPT("request start_time")
    GRAMMAR_MAPPING("![NeedRoom.date_time]>:datetime,"\
                    "![DateTimeSpec.date_time]>:datetime")
    SUCCEEDS_WHEN(AVAILABLE(room_query.date_time.time.start_time))
)

// /RoomLine/Task/GetQuery/RequestDateTime/RequestEndTime
DEFINE_REQUEST_AGENT( CRequestEndTime,
    PRECONDITION(
        UNAVAILABLE(room_query.date_time.time.end_time) && 
        (UNAVAILABLE(room_query.date_time.time.start_time) ||
         UNAVAILABLE(room_query.date_time.time.time_duration)))
	REQUEST_CONCEPT(room_query.date_time; room_query.date_time.time.end_time)
	PROMPT("request end_time")
    GRAMMAR_MAPPING("![NeedRoom.date_time]>:endtime,"\
                    "![DateTimeSpec.date_time]>:endtime")
    SUCCEEDS_WHEN(
        AVAILABLE(room_query.date_time.time.end_time) && 
        (C("room_query.date_time.time.start_time") <= C("room_query.date_time.time.end_time")))
)

// /RoomLine/Task/GetQuery/RequestDateTime/DisambiguateDate
DEFINE_AGENCY( CDisambiguateDate, 
    PRECONDITION(SAME_AS_TRIGGER)
    TRIGGERED_BY(IS_TRUE(room_query.date_time.date.ambiguous) && 
                 !IS_TRUE(room_query.date_time.date.valid_date))
    DEFINE_CONCEPTS(
        CUSTOM_USER_CONCEPT(disambiguated_date_time, CDateTimeConcept, ""))
    DEFINE_SUBAGENTS(
        SUBAGENT(RequestUnambiguousDate, CRequestUnambiguousDate, "request_default")
        SUBAGENT(NeitherDate, CNeitherDate, "")
        SUBAGENT(FirstDate, CFirstDate, "")
        SUBAGENT(SecondDate, CSecondDate, ""))
    SUCCEEDS_WHEN(IS_TRUE(disambiguated_date_time.date.valid_date)
                  || UNAVAILABLE(room_query.date_time.date))
    RESET_ON_COMPLETION
)

// /RoomLine/Task/GetQuery/RequestDateTime/DisambiguateDate/RequestUnambiguousDate
DEFINE_REQUEST_AGENT( CRequestUnambiguousDate,
    REQUEST_CONCEPT(disambiguated_date_time)
    PROMPT("request disambiguated_date_time date_choice<room_query.date_time.date.date_choice")
    GRAMMAR_MAPPING("![NeedRoom.date_time]>:datetime, ![DateTimeSpec.date_time]>:datetime")
    SUCCEEDS_WHEN(IS_TRUE(disambiguated_date_time.date.valid_date))
    ON_COMPLETION(C("room_query.date_time.date") = C("disambiguated_date_time.date"))
)

// /RoomLine/Task/GetQuery/RequestDateTime/DisambiguateDate/NeitherDate
DEFINE_EXECUTE_AGENT( CNeitherDate, 
    TRIGGERED_BY_COMMANDS("@(..)[Neither]", "")
    EXECUTE(CLEAR(room_query.date_time.date))
)

// /RoomLine/Task/GetQuery/RequestDateTime/DisambiguateDate/FirstDate
DEFINE_EXECUTE_AGENT( CFirstDate, 
    TRIGGERED_BY_COMMANDS("@(..)[FirstOne]", "")
    CALL("gal_be.launch_query query=disambiguate_date "
         "choices<room_query.date_time.date.date_choice choice=1 "
         "date>disambiguated_date_time.date")
)

// /RoomLine/Task/GetQuery/RequestDateTime/DisambiguateDate/SecondDate
DEFINE_EXECUTE_AGENT( CSecondDate, 
    TRIGGERED_BY_COMMANDS("@(..)[SecondOne]", "")
    CALL("gal_be.launch_query query=disambiguate_date "
         "choices<room_query.date_time.date.date_choice choice=2 "
         "date>disambiguated_date_time.date")
)

// /RoomLine/Task/GetQuery/RequestDateTime/PastDate
DEFINE_INFORM_AGENT( CPastDate, 
    PRECONDITION(SAME_AS_TRIGGER)
    TRIGGERED_BY(IS_TRUE(room_query.date_time.date.past_date))
    PROMPT("inform past_date <room_query.date_time.date")
    ON_COMPLETION(
        CLEAR(room_query.date_time.date)
        RESET
    )
)       

// /RoomLine/Task/GetQuery/RequestDateTime/WrongTimeOrder
DEFINE_INFORM_AGENT( CWrongTimeOrder,
	PRECONDITION(SAME_AS_TRIGGER)
    TRIGGERED_BY(
        AVAILABLE(room_query.date_time.time.start_time) && 
        AVAILABLE(room_query.date_time.time.end_time) && 
        (C("room_query.date_time.time.start_time") >= C("room_query.date_time.time.end_time"))
    )
    PROMPT("inform wrong_time_order <room_query.date_time.time")
    ON_COMPLETION(
        if(IS_ACTIVE_TOPIC(../RequestStartTime)) {
            FINISH(../RequestStartTime)
            REOPEN_TOPIC(../RequestStartTime)
        }
        if(IS_ACTIVE_TOPIC(../RequestEndTime)) {
            FINISH(../RequestEndTime)
            REOPEN_TOPIC(../RequestEndTime)
        }
        CLEAR(room_query.date_time.time.start_time)
        CLEAR(room_query.date_time.time.end_time)
		REOPEN)
)

// /RoomLine/Task/GetQuery/RequestDateTime/ComputeEndTimeFromDuration
DEFINE_EXECUTE_AGENT( CComputeEndTimeFromDuration, 
    PRECONDITION(SAME_AS_TRIGGER)
    TRIGGERED_BY(
        AVAILABLE(room_query.date_time.time.start_time) &&
        AVAILABLE(room_query.date_time.time.time_duration) &&
        UNAVAILABLE(room_query.date_time.time.end_time) &&
        ((int)(C("room_query.date_time.time.start_time")) + 
         (int)(C("room_query.date_time.time.time_duration")) < 2400))
    EXECUTE(
        int iStartTime = (int)(C("room_query.date_time.time.start_time"));
        int iDuration = (int)(C("room_query.date_time.time.time_duration"));
        int iEndTime = 60*(iStartTime/100) + iStartTime%100 + 
                       60*(iDuration/100) + iDuration%100;        
        C("room_query.date_time.time.end_time") = 
            100*(iEndTime/60) + iEndTime%60;
    )
    RESET_ON_COMPLETION
)

// /RoomLine/Task/GetQuery/RequestDateTime/WrongDuration
DEFINE_INFORM_AGENT( CWrongDuration, 
    PRECONDITION(SAME_AS_TRIGGER)
    TRIGGERED_BY(
        UPDATED(room_query.date_time.time.start_time) &&
        UPDATED(room_query.date_time.time.time_duration) &&
        UNAVAILABLE(room_query.date_time.time.end_time) &&
        ((int)(C("room_query.date_time.time.start_time")) + 
         (int)(C("room_query.date_time.time.time_duration")) >= 2400))
    PROMPT("inform wrong_duration <room_query.date_time.time")
    ON_COMPLETION(
        CLEAR(room_query.date_time.time.time_duration);
        RESET
    )
)

// /RoomLine/Task/GetQuery/ExpectRoomNumber
DEFINE_EXPECT_AGENT( CExpectRoomNumber,	
	EXPECT_CONCEPT(room_query.room_number)
    GRAMMAR_MAPPING(
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.4625]>4625,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.4623]>4623,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.7220]>7220,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.8220]>8220,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.4632]>4632,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.4513]>4513,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.3509]>3509,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.1505]>1505,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.1507]>1507,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.2507]>2507,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.3001]>3001,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.3305]>3305,"\
        "@(..;/RoomLine/AnythingElse)[NeedRoom.room.room_number.3501]>3501")
)

// /RoomLine/Task/GetQuery/ExpectRoomLocation
DEFINE_EXPECT_AGENT( CExpectRoomLocation,	
	EXPECT_CONCEPT(room_query.room_location)
	GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[RoomLocationSpec.location],"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.location]")
)

// /RoomLine/Task/GetQuery/ExpectRoomSize
DEFINE_EXPECT_AGENT( CExpectRoomSize,	
	EXPECT_CONCEPT(room_query.room_size)
    GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[RoomSizeSpec.room_size.number]>:number,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.room_size.number]>:number")
)

// /RoomLine/Task/GetQuery/ExpectRoomSizeSpec
DEFINE_EXPECT_AGENT( CExpectRoomSizeSpec,	
	EXPECT_CONCEPT(room_query.room_size_spec)
    GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[RoomSizeSpec.room_size_spec.rss_large]>large,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.room_size_spec.rss_large]>large,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[RoomSizeSpec.room_size_spec.rss_small]>small,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.room_size_spec.rss_small]>small")
)

// /RoomLine/Task/GetQuery/ExpectRoomProjector
DEFINE_EXPECT_AGENT( CExpectRoomProjector,	
	EXPECT_CONCEPT(room_query.room_projector)
	GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[ProjectorSpec.projector]>true,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[QueryProjector.projector]>true,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.projector]>true")
)

// /RoomLine/Task/GetQuery/ExpectRoomNetwork
DEFINE_EXPECT_AGENT( CExpectRoomNetwork,	
	EXPECT_CONCEPT(room_query.room_network)
	GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[NetworkingSpec.networking]>true,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[QueryNetworking.networking]>true,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.networking]>true")
)

// /RoomLine/Task/GetQuery/ExpectRoomWhiteboard
DEFINE_EXPECT_AGENT( CExpectRoomWhiteboard,	
	EXPECT_CONCEPT(room_query.room_whiteboard)
	GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[WhiteboardSpec.whiteboard]>true,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[QueryWhiteboard.whiteboard]>true,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.whiteboard]>true")
)

// /RoomLine/Task/GetQuery/ExpectRoomComputer
DEFINE_EXPECT_AGENT( CExpectRoomComputer,	
	EXPECT_CONCEPT(room_query.room_computer)
	GRAMMAR_MAPPING("@(/RoomLine/Task;/RoomLine/AnythingElse)[ComputerSpec.computer]>true,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[QueryComputer.computer]>true,"\
                    "@(/RoomLine/Task;/RoomLine/AnythingElse)[NeedRoom.computer]>true")
)

// /RoomLine/Task/GetQuery/ExpectNeedRoom
DEFINE_EXPECT_AGENT( CExpectNeedRoom,	
	EXPECT_CONCEPT(need_room)
	GRAMMAR_MAPPING("@(/RoomLine/Task/GetQuery;/RoomLine/AnythingElse)[NeedRoom]>true,"\
                    "@(/RoomLine/Task/GetQuery;/RoomLine/AnythingElse)[ListRooms]>true")
)

// /RoomLine/Task/InformPendingQuery
DEFINE_INFORM_AGENT( CInformPendingQuery,
	PRECONDITION(COMPLETED(../GetQuery))
    PROMPT("inform pending_query")
)

// /RoomLine/Task/PerformQuery
DEFINE_EXECUTE_AGENT( CPerformQuery,
	PRECONDITION(COMPLETED(../GetQuery))
	CALL("gal_be.launch_query query=room_query <room_query "\
         ">results >suggested_refine error>backend_error")
    ON_COMPLETION(
        if(UNAVAILABLE(room_query.date_time.time.timeperiod_spec)) {
            int iStartTime = (int)C("room_query.date_time.time.start_time");
            int iEndTime = (int)C("room_query.date_time.time.end_time");
            if(iStartTime >= 1200)
                C("room_query.date_time.time.timeperiod_spec") = "afternoon";
            if(iEndTime <= 1200) 
                C("room_query.date_time.time.timeperiod_spec") = "morning";
        }
        REOPEN_CONCEPT(room_query)
        SET_COMPLETED(../GetQuery)
        RESET_AGENT(../DiscussResults))
)

// /RoomLine/Task/BackendError
DEFINE_EXECUTE_AGENT( CBackendError, 
    PRECONDITION(IS_TRUE(backend_error))
    virtual TDialogExecuteReturnCode Execute() {
        pOutputManager->Output(this, "inform backend_error", fsFree);
        return dercFinishDialog;
    }
)

//-----------------------------------------------------------------------------
// /RoomLine/Task/DiscussResults
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CDiscussResults,
    DEFINE_CONCEPTS(
        BOOL_SYSTEM_CONCEPT(no_reservation))
    DEFINE_SUBAGENTS(
        SUBAGENT(PresentResults, CPresentResults, "")
        SUBAGENT(MakeReservation, CMakeReservation, "")
        SUBAGENT(GotoQuery, CGotoQuery, ""))
    SUCCEEDS_WHEN(IS_TRUE(no_reservation) ||
                  (UPDATED(reserve_room) && COMPLETED(MakeReservation)))
)

//-----------------------------------------------------------------------------
// /RoomLine/Task/DiscussResults/PresentResults
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CPresentResults,
	DEFINE_SUBAGENTS(
        SUBAGENT(NoResults, CNoResults, "")
		SUBAGENT(SingleResult, CSingleResult, "")
		SUBAGENT(MultipleResults, CMultipleResults, ""))		
	SUCCEEDS_WHEN(UPDATED(reserve_room) || IS_TRUE(no_reservation))
)

//-----------------------------------------------------------------------------
// /RoomLine/Task/DiscussResults/PresentResults/NoResults
//-----------------------------------------------------------------------------
DEFINE_INFORM_AGENT( CNoResults,
    PRECONDITION(SIZE(results) == 0)
    PROMPT("inform no_results <room_query.-1")
    ON_COMPLETION(C("no_reservation") = true;)
)

//-----------------------------------------------------------------------------
// /RoomLine/Task/DiscussResults/PresentResults/SingleResult
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CSingleResult,
    IS_MAIN_TOPIC()
    PRECONDITION(SIZE(results) == 1)
    DEFINE_CONCEPTS(
        BOOL_USER_CONCEPT(want_reservation_single, "expl"))
	DEFINE_SUBAGENTS(
		SUBAGENT(InformSingleResult, CInformSingleResult, "")
        SUBAGENT(RequestWantReservation, CRequestWantReservation, "request_default")
		SUBAGENT(QuerySingleRoomDetails, CQuerySingleRoomDetails, "")
		SUBAGENT(QueryOtherRooms, CQueryOtherRooms, "")
        SUBAGENT(RecordReservation, CRecordReservation, "")
        SUBAGENT(InformNoReservation, CInformNoReservation, ""))
    SUCCEEDS_WHEN(COMPLETED(RecordReservation) ||
                  COMPLETED(InformNoReservation))
    PROMPT_ESTABLISHCONTEXT("{establish_context single_result <results <room_query.-1}"\
                            "{inform room_details room_details<results.0}")
)

// /RoomLine/Task/DiscussResults/PresentResults/SingleResult/InformSingleResult
DEFINE_INFORM_AGENT( CInformSingleResult,
    PROMPT("{inform single_result <results <room_query.-1}"\
           "{inform room_details room_details<results.0}")
)

// /RoomLine/Task/DiscussResults/PresentResults/SingleResult/RequestWantReservation
DEFINE_REQUEST_AGENT( CRequestWantReservation,
    REQUEST_CONCEPT(want_reservation_single)
    PROMPT("request want_reservation_single <results.0")
    GRAMMAR_MAPPING("![Yes]>true, ![No]>false")
)

// /RoomLine/Task/DiscussResults/PresentResults/SingleResult/QuerySingleRoomDetails
DEFINE_INFORM_AGENT( CQuerySingleRoomDetails,
	PRECONDITION(FALSE)
	TRIGGERED_BY_COMMANDS("[QueryRoomDetails], [QueryProjector.projector], "\
		"[QueryWhiteboard.whiteboard], [QueryNetworking.networking], "\
		"[QueryLocation.location], [QueryRoomSize], [Repeat]", "expl")
	PROMPT("inform room_details room_details<results.0 include_name=1")
	RESET_ON_COMPLETION
)

// /RoomLine/Task/DiscussResults/PresentResults/SingleResult/QueryOtherRooms
DEFINE_INFORM_AGENT( CQueryOtherRooms, 
	PRECONDITION(FALSE)
	TRIGGERED_BY_COMMANDS("[QueryOtherRooms], [ListRooms], "\
                          "[rss_larger], [rss_smaller]", "")
    PROMPT("inform single_result_no_other <results <room_query.-1")
	RESET_ON_COMPLETION
)

// /RoomLine/Task/DiscussResults/PresentResults/SingleResult/RecordReservation
DEFINE_EXECUTE_AGENT( CRecordReservation,
    PRECONDITION(IS_TRUE(want_reservation_single))
    EXECUTE(C("reserve_room") = C("results.0");)
)

// /RoomLine/Task/DiscussResults/PresentResults/SingleResult/InformNoReservation
DEFINE_INFORM_AGENT( CInformNoReservation,
    PRECONDITION(IS_FALSE(want_reservation_single))
    PROMPT("{inform sorry}{inform single_result_i_had <room_query}")
    ON_COMPLETION(C("no_reservation") = true;)
)

//-----------------------------------------------------------------------------
// /RoomLine/Task/DiscussResults/PresentResults/MultipleResults
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CMultipleResults,	
    IS_MAIN_TOPIC()
    PRECONDITION(SIZE(results) > 1)
    DEFINE_CONCEPTS(
        BOOL_USER_CONCEPT(want_reservation_any, "expl_impl")
        INT_SYSTEM_CONCEPT(wanted_room_index))
	DEFINE_SUBAGENTS(
		SUBAGENT(InformMultipleResults, CInformMultipleResults, "")
        SUBAGENT(IdentifyRoom, CIdentifyRoom, "")
        SUBAGENT(RecordReservationMultiple, CRecordReservationMultiple, "")
        SUBAGENT(InformNoReservationMultiple, CInformNoReservationMultiple, "")
        SUBAGENT(InformNoReservationRefined, CInformNoReservationRefined, ""))
    SUCCEEDS_WHEN(
        COMPLETED(RecordReservationMultiple) ||
        COMPLETED(InformNoReservationMultiple))
    RESET_ON_COMPLETION
)

// /RoomLine/Task/DiscussResults/PresentResults/MultipleResults/InformMultipleResults
DEFINE_INFORM_AGENT( CInformMultipleResults,
	PROMPT("inform multiple_results <results <room_query.-1 <suggested_refine")
)

//-----------------------------------------------------------------------------
// /RoomLine/Task/DiscussResults/PresentResults/MultipleResults/IdentifyRoom
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CIdentifyRoom,
    DEFINE_CONCEPTS(
        INT_USER_CONCEPT(wanted_room_number, "expl_impl")
        BOOL_USER_CONCEPT(want_reservation_single, "expl"))
    DEFINE_SUBAGENTS(
        SUBAGENT(AskForSingleRoom, CAskForSingleRoom, "request_default")
        SUBAGENT(GiveSingleRoomDetails, CGiveSingleRoomDetails, "")
		SUBAGENT(AutoRefine, CAutoRefine, "")
        SUBAGENT(SuggestRoom, CSuggestRoom, "")
        SUBAGENT(AskForRoom, CAskForRoom, "")
        SUBAGENT(RefineQuery, CRefineQuery, "")
        SUBAGENT(IdentifyRoomByNumber, CIdentifyRoomByNumber, "")
        SUBAGENT(RoomNotInList, CRoomNotInList, "")
        SUBAGENT(ChooseFirstRoom, CChooseFirstRoom, "")
        SUBAGENT(ChooseSecondRoom, CChooseSecondRoom, "")
        SUBAGENT(ChooseAnyRoom, CChooseAnyRoom, "")
        SUBAGENT(GiveListOfRooms, CGiveListOfRooms, ""))
    SUCCEEDS_WHEN(
        IS_FALSE(want_reservation_any) ||            // the user does not want
        (AVAILABLE(wanted_room_index) &&              // or we know 
        (C("wanted_room_index") != CInt(-1))))       // a valid room number    
    PROMPT_ESTABLISHCONTEXT("{establish_context identify_room <room_query <results}")
)

//-----------------------------------------------------------------------------
// .../IdentifyRoom/AskForSingleRoom
//-----------------------------------------------------------------------------
DEFINE_REQUEST_AGENT( CAskForSingleRoom,
    PRECONDITION(SIZE(results) == 1)
    REQUEST_CONCEPT(want_reservation_single)
    PROMPT("request want_reservation_single <results.0")
    GRAMMAR_MAPPING("![Yes]>true, ![No]>false")
    SUCCEEDS_WHEN(AVAILABLE(want_reservation_single) || 
                  AVAILABLE(wanted_room_number))
    ON_COMPLETION(
        if(IS_TRUE(want_reservation_single))
            C("wanted_room_index") = 0;
        if(IS_FALSE(want_reservation_single)) 
            C("want_reservation_any") = false;
    )
)

// .../IdentifyRoom/GiveSingleRoomDetails
DEFINE_INFORM_AGENT( CGiveSingleRoomDetails,
	PRECONDITION(FALSE)
    EXPECT_WHEN(SIZE(results) == 1)
	TRIGGERED_BY_COMMANDS("[QueryRoomDetails], [QueryProjector.projector], "\
		"[QueryWhiteboard.whiteboard], [QueryNetworking.networking], "\
		"[QueryLocation.location], [QueryRoomSize], [Repeat]", "expl")
	PROMPT("inform room_details room_details<results.0 include_name=1")
	RESET_ON_COMPLETION
)

//-----------------------------------------------------------------------------
// .../IdentifyRoom/AutoRefine
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CAutoRefine,
	PRECONDITION((SIZE(results) > 2) && 
                 AVAILABLE(suggested_refine) && 
                 (C("suggested_refine") != CString("none")))
    DEFINE_CONCEPTS(
		BOOL_USER_CONCEPT(auto_refine, ""))
	DEFINE_SUBAGENTS(
		SUBAGENT(RefineSize, CRefineSize, "request_default")
		SUBAGENT(RefineProjector, CRefineProjector, "request_default")
		SUBAGENT(RefineLocation, CRefineLocation, "request_default"))
	SUCCEEDS_WHEN(COMPLETED(RefineSize) ||
				  COMPLETED(RefineProjector) ||
				  COMPLETED(RefineLocation))
    ON_COMPLETION(CLEAR(suggested_refine))
)

// .../IdentifyRoom/AutoRefine/RefineSize
DEFINE_REQUEST_AGENT( CRefineSize, 
	PRECONDITION(C("suggested_refine") == CString("size"))
	REQUEST_CONCEPT(auto_refine; room_query.room_size; room_query.room_size_spec; 
	                ../../RefineQuery/query_refinement.size; 
	                ../../RefineQuery/query_refinement.size_spec )
	PROMPT("request auto_refine_size")
    MAX_ATTEMPTS(2)
    GRAMMAR_MAPPING("![Indifferent]>false")
)

// .../IdentifyRoom/AutoRefine/RefineProjector
DEFINE_REQUEST_AGENT( CRefineProjector, 
	PRECONDITION((string)C("suggested_refine") == "projector")
	REQUEST_CONCEPT(auto_refine; ../../RefineQuery/query_refinement.equip)
	MAX_ATTEMPTS(2)
	PROMPT("request auto_refine_projector")
	GRAMMAR_MAPPING("![No]>false, ![Indifferent]>false")
)

// .../IdentifyRoom/AutoRefine/RefineLocation
DEFINE_REQUEST_AGENT( CRefineLocation, 
	PRECONDITION((string)C("suggested_refine") == "location")
	REQUEST_CONCEPT(auto_refine)
	PROMPT("request auto_refine_location")
    MAX_ATTEMPTS(2)
	GRAMMAR_MAPPING("![Indifferent]>false")
)

//-----------------------------------------------------------------------------
// .../IdentifyRoom/SuggestRoom
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CSuggestRoom,               
    PRECONDITION(SIZE(results) > 2)
    DEFINE_CONCEPTS(
        INT_SYSTEM_CONCEPT(suggest_index)
        BOOL_USER_CONCEPT(want_reservation_this, "expl")
        BOOL_USER_CONCEPT(end_of_rooms_still_want_one, "expl"))
    DEFINE_SUBAGENTS(
        SUBAGENT(InformARoom, CInformARoom, "")
        SUBAGENT(RequestWantThisRoom, CRequestWantThisRoom, "request_default")
        SUBAGENT(RepeatARoom, CRepeatARoom, "")
        SUBAGENT(TryNextOne, CTryNextOne, "")
        SUBAGENT(EndOfRoomList, CEndOfRoomList, "request_default"))
    SUCCEEDS_WHEN(
        IS_TRUE(want_reservation_this) ||
        AVAILABLE(end_of_rooms_still_want_one))
    ON_INITIALIZATION(C("suggest_index") = 0)
    ON_COMPLETION(
        if(IS_TRUE(want_reservation_this)) {
            C("wanted_room_index") = C("suggest_index");
        } else if(AVAILABLE(end_of_rooms_still_want_one)) {
            C("want_reservation_any") = C("end_of_rooms_still_want_one");
            if(IS_TRUE(want_reservation_any)) 
                RESET_AGENT(../AskForRoom);
        }
    )
    PROMPT_ESTABLISHCONTEXT("establish_context suggest_room <results")
)

// .../IdentifyRoom/SuggestRoom/CInformARoom
DEFINE_INFORM_AGENT( CInformARoom,
    PROMPT("inform room_suggest_details <results <suggest_index")
)

// .../IdentifyRoom/SuggestRoom/RequestWantThisRoom
DEFINE_REQUEST_AGENT( CRequestWantThisRoom,
    REQUEST_CONCEPT(want_reservation_this)
    PROMPT("request want_reservation_this <results <suggest_index")
    GRAMMAR_MAPPING("![yes]>true, ![no]>false, ![QueryOtherRooms]>false")
)

// .../IdentifyRoom/SuggestRoom/CRepeatARoom
DEFINE_INFORM_AGENT( CRepeatARoom,
    PRECONDITION(FALSE)
    TRIGGERED_BY_COMMANDS("@(..)[QueryRoomDetails], @(..)[QueryRoomSize], "\
                          "@(..)[Repeat]", "expl")
    PROMPT("inform room_suggest_details <results <suggest_index repeat=1")
    RESET_ON_COMPLETION
)

// .../IdentifyRoom/SuggestRoom/TryNextOne
DEFINE_EXECUTE_AGENT( CTryNextOne,
    PRECONDITION(IS_FALSE(want_reservation_this) && 
                 ((int)C("suggest_index") < SIZE(results) - 1))
    ON_COMPLETION(
        C("suggest_index") = (int)C("suggest_index") + 1;
        CLEAR(want_reservation_this)
        RESET_AGENT(../InformARoom)
        RESET_AGENT(../RequestWantThisRoom)
        RESET
    )
)

// .../IdentifyRoom/SuggestRoom/EndOfRoomList
DEFINE_REQUEST_AGENT( CEndOfRoomList,
    PRECONDITION(IS_FALSE(want_reservation_this) && 
                 ((int)C("suggest_index") == SIZE(results) - 1))
    REQUEST_CONCEPT(end_of_rooms_still_want_one)
    PROMPT("request end_of_rooms_still_want_one <results")
    GRAMMAR_MAPPING("![yes]>true, ![no]>false")
)


//-----------------------------------------------------------------------------
// .../IdentifyRoom/AskForRoom
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CAskForRoom,
    DEFINE_SUBAGENTS(
        SUBAGENT(RequestWantReservationMultiple, CRequestWantReservationMultiple, "request_default")
        SUBAGENT(RequestWhichRoom, CRequestWhichRoom, "request_default")
        SUBAGENT(QueryRoomProperties, CQueryRoomProperties, "")
        SUBAGENT(RepeatListOfRooms, CRepeatListOfRooms, ""))
)

// .../IdentifyRoom/AskForRoom/CRequestWantReservationMultiple
DEFINE_REQUEST_AGENT( CRequestWantReservationMultiple,
    REQUEST_CONCEPT(want_reservation_any)
    GRAMMAR_MAPPING("![Yes]>true, ![No]>false, "\
                    "[RejectAllRooms]>false, ![Neither]>false")
    SUCCEEDS_WHEN(AVAILABLE(want_reservation_any) || 
                  AVAILABLE(wanted_room_number))
)

// .../IdentifyRoom/AskForRoom/RequestWhichRoom
DEFINE_REQUEST_AGENT( CRequestWhichRoom,
    PRECONDITION( 
        IS_TRUE(want_reservation_any) && UNAVAILABLE(wanted_room_number))
    REQUEST_CONCEPT(wanted_room_number)
    PROMPT("request wanted_room_number <results")
    GRAMMAR_MAPPING(
        "[NeedRoom.4625]>4625,"\
        "[NeedRoom.4623]>4623,"\
        "[NeedRoom.7220]>7220,"\
        "[NeedRoom.8220]>8220,"\
        "[NeedRoom.4632]>4632,"\
        "[NeedRoom.4513]>4513,"\
        "[NeedRoom.3509]>3509,"\
        "[NeedRoom.1505]>1505,"\
        "[NeedRoom.1507]>1507,"\
        "[NeedRoom.2507]>2507,"\
        "[NeedRoom.3001]>3001,"\
        "[NeedRoom.3305]>3305,"\
        "[NeedRoom.3501]>3501")
)

// .../IdentifyRoom/AskForRoom/QueryRoomProperties
DEFINE_AGENCY( CQueryRoomProperties, 
    PRECONDITION(SAME_AS_TRIGGER)    
    TRIGGERED_BY(UPDATED(queried_room_number))
    DEFINE_CONCEPTS(
        INT_USER_CONCEPT(queried_room_number, "expl_impl")
        INT_SYSTEM_CONCEPT(queried_room_index)
        BOOL_USER_CONCEPT(want_queried_room, "expl"))
    DEFINE_SUBAGENTS(
        SUBAGENT(ExpectQueryRoom, CExpectQueryRoom, "")
        SUBAGENT(IdentifyQueriedRoomIndex, CIdentifyQueriedRoomIndex, "")
        SUBAGENT(InformQueriedRoom, CInformQueriedRoom, "")
        SUBAGENT(RequestWantQueriedRoom, CRequestWantQueriedRoom, "request_default"))
    SUCCEEDS_WHEN(COMPLETED(RequestWantQueriedRoom))
    RESET_ON_COMPLETION
)

// .../IdentifyRoom/AskForRoom/QueryRoomProperties/ExpectQueryRoom
DEFINE_EXPECT_AGENT( CExpectQueryRoom,
    EXPECT_CONCEPT(queried_room_number)
    GRAMMAR_MAPPING(
        "@(../..)[QueryRoomDetails.room.room_number.4625]>4625,"\
        "@(../..)[QueryRoomDetails.room.room_number.4623]>4623,"\
        "@(../..)[QueryRoomDetails.room.room_number.7220]>7220,"\
        "@(../..)[QueryRoomDetails.room.room_number.8220]>8220,"\
        "@(../..)[QueryRoomDetails.room.room_number.4632]>4632,"\
        "@(../..)[QueryRoomDetails.room.room_number.4513]>4513,"\
        "@(../..)[QueryRoomDetails.room.room_number.3509]>3509,"\
        "@(../..)[QueryRoomDetails.room.room_number.1505]>1505,"\
        "@(../..)[QueryRoomDetails.room.room_number.1507]>1507,"\
        "@(../..)[QueryRoomDetails.room.room_number.2507]>2507,"\
        "@(../..)[QueryRoomDetails.room.room_number.3001]>3001,"\
        "@(../..)[QueryRoomDetails.room.room_number.3305]>3305,"\
        "@(../..)[QueryRoomDetails.room.room_number.3501]>3501,"\
        "@(../..)[QueryRoomSize.4625]>4625,"\
        "@(../..)[QueryRoomSize.4623]>4623,"\
        "@(../..)[QueryRoomSize.7220]>7220,"\
        "@(../..)[QueryRoomSize.8220]>8220,"\
        "@(../..)[QueryRoomSize.4632]>4632,"\
        "@(../..)[QueryRoomSize.4513]>4513,"\
        "@(../..)[QueryRoomSize.3509]>3509,"\
        "@(../..)[QueryRoomSize.1505]>1505,"\
        "@(../..)[QueryRoomSize.1507]>1507,"\
        "@(../..)[QueryRoomSize.2507]>2507,"\
        "@(../..)[QueryRoomSize.3001]>3001,"\
        "@(../..)[QueryRoomSize.3305]>3305,"\
        "@(../..)[QueryRoomSize.3501]>3501")
)

// .../IdentifyRoom/AskForRoom/QueryRoomProperties/IdentifyQueriedRoomIndex
DEFINE_EXECUTE_AGENT( CIdentifyQueriedRoomIndex,
    EXECUTE(
        // go through the array and identify the index
        for(int i = 0; i < SIZE(results); i++) {
            if(C("results.%d.room_number", i) == C("queried_room_number")) {
                C("queried_room_index") = i;
                return;
            }
        }
        // o/w set wanted room index so that stuff will get triggered, 
        // and trigger the RoomNotInList
        C("wanted_room_number") = C("queried_room_number");
        C("wanted_room_index") = -1;
        pDMCore->SignalFocusClaimsPhase();
    )
)

// .../IdentifyRoom/AskForRoom/QueryRoomProperties/InformQueriedRoom
DEFINE_INFORM_AGENT( CInformQueriedRoom, 
    PROMPT("inform room_suggest_details <results suggest_index<queried_room_index repeat=1")
)

// .../IdentifyRoom/AskForRoom/QueryRoomProperties/RequestWantQueriedRoom
DEFINE_REQUEST_AGENT( CRequestWantQueriedRoom,
    REQUEST_CONCEPT(want_queried_room)    
    PROMPT("request want_reservation_this <results suggest_index<queried_room_index")
    GRAMMAR_MAPPING("![Yes]>true, ![No]>false, ![OtherRooms]>false")
    ON_COMPLETION(
        if(IS_TRUE(want_queried_room)) 
            C("wanted_room_index") = C("queried_room_index");
        pDMCore->SignalFocusClaimsPhase();
    )
)

// .../IdentifyRoom/AskForRoom/RepeatListOfRooms
DEFINE_INFORM_AGENT( CRepeatListOfRooms,
    PRECONDITION(FALSE)
    TRIGGERED_BY_COMMANDS("@(..)[Repeat], @(..)[ListRooms]", "expl")
    PROMPT("inform multiple_results_room_list <results <room_query.-1")
    RESET_ON_COMPLETION
)

//-----------------------------------------------------------------------------
// .../IdentifyRoom/RefineQuery
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CRefineQuery,
    PRECONDITION(FALSE)
    TRIGGERED_BY(UPDATED(query_refinement))
    DEFINE_CONCEPTS(
        CUSTOM_USER_CONCEPT(query_refinement, CRoomQueryRefinement, 
			"none, location=expl_impl, size=expl_impl, size_spec=expl_impl, equip=expl_impl"))
    DEFINE_SUBAGENTS(
        SUBAGENT(ExpectEquipQuery, CExpectEquipQuery, "")
        SUBAGENT(ExpectEquipQuery2, CExpectEquipQuery2, "")
        SUBAGENT(ExpectSizeQuery, CExpectSizeQuery, "")
        SUBAGENT(ExpectSizeSpecQuery, CExpectSizeSpecQuery, "")
        SUBAGENT(ExpectLocationQuery, CExpectLocationQuery, "")
        SUBAGENT(ReOpenResults, CReOpenResults, "")
        SUBAGENT(PerformRefinedQuery, CPerformRefinedQuery, "")
        SUBAGENT(NoRefinedResults, CNoRefinedResults, "")
        SUBAGENT(DifferentRefinedResults, CDifferentRefinedResults, "") 
        SUBAGENT(SameRefinedResults, CSameRefinedResults, ""))
    SUCCEEDS_WHEN(
        COMPLETED(NoRefinedResults) ||
        COMPLETED(DifferentRefinedResults) ||
        COMPLETED(SameRefinedResults) ||
        COMPLETED(..))
    ON_COMPLETION(
        if(COMPLETED(DifferentRefinedResults) && !COMPLETED(..))
            pDMCore->RestartTopic(pdaParent);
        else 
            RESET
    )
)

// .../IdentifyRoom/RefineQuery/ExpectEquipQuery
DEFINE_EXPECT_AGENT( CExpectEquipQuery,
    EXPECT_CONCEPT(query_refinement.equip)
    GRAMMAR_MAPPING("[ProjectorSpec.projector]>projector,"\
                    "@(../../AutoRefine/RefineProjector)[Yes]>projector,"\
                    "[NeedRoom.projector]>projector,"\
                    "[NetworkingSpec.networking]>network,"\
                    "[NeedRoom.networking]>network,"\
                    "[ComputerSpec.computer]>computer,"\
                    "[NeedRoom.computer]>computer,"\
                    "[WhiteboardSpec.whiteboard]>whiteboard,"\
                    "[NeedRoom.whiteboard]>whiteboard")
)

// .../IdentifyRoom/RefineQuery/ExpectEquipQuery2
DEFINE_EXPECT_AGENT( CExpectEquipQuery2,
    EXPECT_CONCEPT(query_refinement.equip)
    EXPECT_WHEN(SIZE(results) > 1)
    GRAMMAR_MAPPING("[QueryProjector.projector]>projector,"\
                    "[QueryNetworking.networking]>network,"\
                    "[QueryComputer.computer]>computer,"\
                    "[QueryWhiteboard.whiteboard]>whiteboard")
)

// .../IdentifyRoom/RefineQuery/ExpectSizeQuery
DEFINE_EXPECT_AGENT( CExpectSizeQuery,
    EXPECT_CONCEPT(query_refinement.size)
    GRAMMAR_MAPPING("[RoomSizeSpec.room_size.number]>:number,"\
                    "[NeedRoom.room_size.number]>:number")
)

// .../IdentifyRoom/RefineQuery/ExpectSizeSpecQuery
DEFINE_EXPECT_AGENT( CExpectSizeSpecQuery,
    EXPECT_CONCEPT(query_refinement.size_spec)
    EXPECT_WHEN(SIZE(results) > 1)
    GRAMMAR_MAPPING("[RoomSizeSpec.room_size_spec.rss_largest]>largest,"\
                    "[QueryRoomSizeSpec.rss_largest]>largest,"\
                    "[NeedRoom.room_size_spec.rss_largest]>largest,"\
                    "@(../../SuggestRoom/RequestWantThisRoom)[RoomSizeSpec.room_size_spec.rss_larger]>larger,"\
                    "@(../../SuggestRoom/RequestWantThisRoom)[QueryRoomSizeSpec.rss_larger]>larger,"\
                    "@(../../SuggestRoom/RequestWantThisRoom)[NeedRoom.room_size_spec.rss_larger]>larger,"\
                    "[RoomSizeSpec.room_size_spec.rss_smallest]>smallest,"\
                    "[QueryRoomSizeSpec.rss_smallest]>smallest,"\
                    "[NeedRoom.room_size_spec.rss_smallest]>smallest,"\
                    "@(../../SuggestRoom/RequestWantThisRoom)[RoomSizeSpec.room_size_spec.rss_smaller]>smaller,"\
                    "@(../../SuggestRoom/RequestWantThisRoom)[QueryRoomSizeSpec.rss_smaller]>smaller,"\
                    "@(../../SuggestRoom/RequestWantThisRoom)[NeedRoom.room_size_spec.rss_smaller]>smaller,"\
                    "[RoomSizeSpec.room_size_spec.rss_large]>large,"\
                    "[QueryRoomSizeSpec.room_size_spec.rss_large]>large,"\
                    "[NeedRoom.room_size_spec.rss_large]>large,"\
                    "[RoomSizeSpec.room_size_spec.rss_small]>small,"\
                    "[QueryRoomSizeSpec.room_size_spec.rss_small]>small,"\
                    "[NeedRoom.room_size_spec.rss_small]>small")
)

// .../IdentifyRoom/RefineQuery/ExpectLocationQuery
DEFINE_EXPECT_AGENT( CExpectLocationQuery,
    EXPECT_CONCEPT(query_refinement.location)
    GRAMMAR_MAPPING("[QueryLocation.location],"\
                    "[NeedRoom.location],"\
                    "[RoomLocationSpec.location]")
)

// .../IdentifyRoom/RefineQuery/ReOpenResults
DEFINE_EXECUTE_AGENT( CReOpenResults, 
    EXECUTE(REOPEN_CONCEPT(results))
)

// .../IdentifyRoom/RefineQuery/PerformRefineQuery
DEFINE_EXECUTE_AGENT( CPerformRefinedQuery,
    CALL("gal_be.launch_query query=refine_results <results.-1 "\
         "<../../SuggestRoom/suggest_index <query_refinement >results")
)

// .../IdentifyRoom/RefineQuery/NoRefinedResults
DEFINE_AGENCY( CNoRefinedResults,
    PRECONDITION(SIZE(results) == 0)
    DEFINE_SUBAGENTS(
        SUBAGENT(InformNoRefinedResults, CInformNoRefinedResults, "")
        SUBAGENT(AskStillWantAny, CAskStillWantAny, "request_default"))
)

// .../IdentifyRoom/RefineQuery/NoRefinedResults/InformNoRefinedResults
DEFINE_INFORM_AGENT( CInformNoRefinedResults,
    PROMPT("inform refined_no_results old_results<results.-1 "\
            "<../../../SuggestRoom/suggest_index <query_refinement")
    ON_COMPLETION(
        RESTORE_CONCEPT(results);
        if(SIZE(results) <= 1) SET_COMPLETED(..);
        if((C("query_refinement.size_spec") == CString("larger")) ||
           (C("query_refinement.size_spec") == CString("smaller")))
            SET_COMPLETED(..);
        CLEAR(want_reservation_any)
    )
)

// .../IdentifyRoom/RefineQuery/NoRefinedResults/AskStillWantAny
DEFINE_REQUEST_AGENT( CAskStillWantAny,
    REQUEST_CONCEPT(want_reservation_any)
    PROMPT("request still_want_reservation_any")
    GRAMMAR_MAPPING("![Yes]>true, ![No]>false, ![RejectAllRooms]>false")
)


// .../IdentifyRoom/RefineQuery/DifferentRefinedResults
DEFINE_INFORM_AGENT( CDifferentRefinedResults,
    PRECONDITION((SIZE(results) > 0) &&
                 (SIZE(results) != SIZE(results.-1)))
    PROMPT("inform refined_different_results <query_refinement "\
            "<../../SuggestRoom/suggest_index <results old_results<results.-1")
)

// .../IdentifyRoom/RefineQuery/SameRefinedResults
DEFINE_INFORM_AGENT( CSameRefinedResults,
    PRECONDITION(SIZE(results) == SIZE(results.-1))
    PROMPT("inform refined_same_results <query_refinement "\
        "<../../SuggestRoom/suggest_index <results old_results<results.-1")
    ON_COMPLETION(RESTORE_CONCEPT(results))
)

//-----------------------------------------------------------------------------
// .../IdentifyRoom/IdentifyRoomByNumber
//-----------------------------------------------------------------------------
DEFINE_EXECUTE_AGENT( CIdentifyRoomByNumber,
    PRECONDITION(SAME_AS_TRIGGER)
    TRIGGERED_BY(UPDATED(wanted_room_number) && 
                 UNAVAILABLE(wanted_room_index))
    EXECUTE(
        // go through the array and identify the index
        for(int i = 0; i < SIZE(results); i++) {
            if(C("results.%d.room_number", i) == C("wanted_room_number")) {
                C("wanted_room_index") = i;
                return;
            }
        }
        C("wanted_room_index") = -1;
        pDMCore->SignalFocusClaimsPhase();
    )
    RESET_ON_COMPLETION
)

//-----------------------------------------------------------------------------
// .../IdentifyRoom/RoomNotInList
//-----------------------------------------------------------------------------
DEFINE_INFORM_AGENT( CRoomNotInList,    
    PRECONDITION(SAME_AS_TRIGGER)
    TRIGGERED_BY(C("wanted_room_index") == CInt(-1))
    PROMPT("{inform room_not_in_list <wanted_room_number}"\
           "{inform multiple_results_room_list <results <room_query.-1}")
    ON_COMPLETION(
        RESET
        CLEAR(wanted_room_index)
        CLEAR(wanted_room_number)
        RESET_AGENT(../AskForRoom)
        SET_COMPLETED(../SuggestRoom)
        SET_COMPLETED(../AutoRefine)
        SET_COMPLETED(../RefineQuery)
        pDMCore->ContinueWith(&A(".."), &A("../AskForRoom")))
)

//-----------------------------------------------------------------------------
// .../IdentifyRoom/ChooseFirstRoom
//-----------------------------------------------------------------------------
DEFINE_EXECUTE_AGENT( CChooseFirstRoom, 
    PRECONDITION(FALSE)
    TRIGGERED_BY_COMMANDS("@(..)[ChooseRoom.first_room], @(..)[FirstOne]", "expl_impl")
    EXECUTE(C("wanted_room_index") = 0)
    RESET_ON_COMPLETION
)
    
//-----------------------------------------------------------------------------
// .../IdentifyRoom/ChooseSecondRoom
//-----------------------------------------------------------------------------
DEFINE_EXECUTE_AGENT( CChooseSecondRoom, 
    PRECONDITION(FALSE)
    EXPECT_WHEN(SIZE(results) >= 2)
    TRIGGERED_BY_COMMANDS("@(..)[ChooseRoom.second_room], @(..)[SecondOne]", "expl_impl")
    EXECUTE(C("wanted_room_index") = 1)
    RESET_ON_COMPLETION
)

//-----------------------------------------------------------------------------
// .../IdentifyRoom/ChooseAnyRoom
//-----------------------------------------------------------------------------
DEFINE_EXECUTE_AGENT( CChooseAnyRoom, 
    PRECONDITION(FALSE)
    TRIGGERED_BY_COMMANDS("@(../AskForRoom;../AutoRefine;../RefineQuery)"\
                            "[ChooseRoom.any_room], "\
                          "@(../AskForRoom)[Indifferent]", "expl_impl")
    EXECUTE(
        RESET_AGENT(../SuggestRoom)
        SET_COMPLETED(../AutoRefine)
        SET_COMPLETED(../AskForRoom)
        SET_COMPLETED(../RefineQuery)
        pDMCore->ContinueWith(&A(".."), &A("../SuggestRoom")))
    RESET_ON_COMPLETION
)

//-----------------------------------------------------------------------------
// .../IdentifyRoom/GiveListOfRooms
//-----------------------------------------------------------------------------
DEFINE_INFORM_AGENT( CGiveListOfRooms,
    PRECONDITION(FALSE)
    TRIGGERED_BY_COMMANDS("@(../SuggestRoom;../AutoRefine;../RefineQuery)"\
                            "[ListRooms]", "expl")
    PROMPT("inform multiple_results_room_list <results <room_query.-1")
    ON_COMPLETION(
        RESET
        RESET_AGENT(../AskForRoom)
        SET_COMPLETED(../SuggestRoom)
        SET_COMPLETED(../AutoRefine)
        SET_COMPLETED(../RefineQuery)
        pDMCore->ContinueWith(&A(".."), &A("../AskForRoom")))
)


//-----------------------------------------------------------------------------
// .../MultipleResults/RecordReservationMultiple
//-----------------------------------------------------------------------------
DEFINE_EXECUTE_AGENT( CRecordReservationMultiple,
    PRECONDITION(AVAILABLE(wanted_room_index))
    EXECUTE(C("reserve_room") = C("results.%d", (int)C("wanted_room_index")))
)

//-----------------------------------------------------------------------------
// .../MultipleResults/InformNoReservationMultiple
//-----------------------------------------------------------------------------
DEFINE_INFORM_AGENT( CInformNoReservationMultiple,
    PRECONDITION(
        IS_FALSE(want_reservation_any) && 
        (C("results").GetHistorySize() == 0))
    PROMPT("inform sorry")
    ON_COMPLETION(C("no_reservation") = true)
)
    
//-----------------------------------------------------------------------------
// .../MultipleResults/InformNoReservationRefined
//-----------------------------------------------------------------------------
DEFINE_INFORM_AGENT( CInformNoReservationRefined,
    PRECONDITION(
        IS_FALSE(want_reservation_any) && 
        (C("results").GetHistorySize() > 0))
    PROMPT("inform back_to_previous_results")
    ON_COMPLETION(
        // back up to the previous results
        RESTORE_CONCEPT(results)
        // and reset the multiple-results, restarting the discussion
        RESET_AGENT(..)
    )
)

//-----------------------------------------------------------------------------
// /RoomLine/Task/DiscussResults/MakeReservation
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CMakeReservation,
    IS_MAIN_TOPIC()
    PRECONDITION(UPDATED(reserve_room))
	DEFINE_CONCEPTS(
		STRING_USER_CONCEPT(user_name, "expl")
		STRING_SYSTEM_CONCEPT(greet_user_name)
		BOOL_SYSTEM_CONCEPT(valid_user)
		BOOL_SYSTEM_CONCEPT(reservation_success))
    DEFINE_SUBAGENTS(
        SUBAGENT(InformMakeReservation, CInformMakeReservation, "")
    	SUBAGENT(GetUser, CGetUser, "")
		SUBAGENT(GreetUser, CGreetUser, "")
		SUBAGENT(InformNoReservationForGuest, CInformNoReservationForGuest, "")
        SUBAGENT(PerformReservation, CPerformReservation, "")
        SUBAGENT(InformReservationSuccess, CInformReservationSuccess, "")
		SUBAGENT(InformReservationError, CInformReservationError, "")
		SUBAGENT(CancelReservation, CCancelReservation, ""))
    SUCCEEDS_WHEN(COMPLETED(InformNoReservationForGuest) || 
				  COMPLETED(InformReservationSuccess) || 
				  COMPLETED(InformReservationError) ||
				  COMPLETED(CancelReservation))
    PROMPT_ESTABLISHCONTEXT("establish_context make_reservation <reserve_room")
)

// /RoomLine/Task/DiscussResults/MakeReservation/InformMakeReservation
DEFINE_INFORM_AGENT( CInformMakeReservation,
    PROMPT("inform make_reservation <reserve_room")
)

// /RoomLine/Task/DiscussResults/MakeReservation/GetUser
DEFINE_AGENCY( CGetUser, 
	DEFINE_SUBAGENTS(
		SUBAGENT(AskUserName, CAskUserName, "")
		SUBAGENT(CheckUserId, CCheckUserId, "")
		SUBAGENT(InvalidUserId, CInvalidUserId, ""))
	SUCCEEDS_WHEN((C("user_name") == CString("guest")) || IS_TRUE(valid_user))
)

// /RoomLine/Task/DiscussResults/MakeReservation/GetUser/AskUserName
DEFINE_REQUEST_AGENT( CAskUserName, 
	REQUEST_CONCEPT(user_name)
	GRAMMAR_MAPPING("[Identification.user_name], "\
		"[Identification.guest_user]>guest, "\
		"[Identification.user_id]")
)

// /RoomLine/Task/DiscussResults/MakeReservation/GetUser/CheckUserId
DEFINE_EXECUTE_AGENT( CCheckUserId,
    CALL("gal_be.launch_query query=check_user_id <user_name >valid_user >greet_user_name")
)

// /RoomLine/Task/DiscussResults/MakeReservation/GetUser/InvalidUserId
DEFINE_INFORM_AGENT( CInvalidUserId,
    PRECONDITION(IS_FALSE(valid_user))
	PROMPT("inform invalid_id")
	ON_COMPLETION(
		CLEAR(user_name); CLEAR(valid_user); CLEAR(greet_user_name);
		RESET_AGENT(..))
)

// /RoomLine/Task/DiscussResults/MakeReservation/GreetUser
DEFINE_INFORM_AGENT( CGreetUser, 
	PRECONDITION(IS_TRUE(valid_user))
	PROMPT("inform greet_user <greet_user_name")
)

// /RoomLine/Task/DiscussResults/MakeReservation/InformNoReservationForGuest
DEFINE_INFORM_AGENT( CInformNoReservationForGuest, 
	PRECONDITION(C("user_name") == CString("guest"))
	PROMPT("inform no_reservation_for_guest")
)

// /RoomLine/Task/DiscussResults/MakeReservation/PerformReservation
DEFINE_EXECUTE_AGENT( CPerformReservation,
    CALL("gal_be.launch_query query=reserve_room <reserve_room <user_name success>reservation_success")
)

// /RoomLine/Task/DiscussResults/MakeReservation/InformReservationSuccess
DEFINE_AGENCY( CInformReservationSuccess,
	PRECONDITION(IS_TRUE(reservation_success))
	DEFINE_CONCEPTS(
		BOOL_USER_CONCEPT(want_summary, "expl"))
	DEFINE_SUBAGENTS(
		SUBAGENT(InformReservationMade, CInformReservationMade, "")
		SUBAGENT(AskWantSummary, CAskWantSummary, "request_default")
		SUBAGENT(GiveSummary, CGiveSummary, ""))
	SUCCEEDS_WHEN(IS_FALSE(want_summary) || COMPLETED(GiveSummary))
    PROMPT_ESTABLISHCONTEXT("establish_context inform_reservation <reserve_room")
)

// /RoomLine/Task/DiscussResults/MakeReservation/InformReservationError
DEFINE_INFORM_AGENT( CInformReservationError,
	PRECONDITION(IS_FALSE(reservation_success))
    PROMPT("inform reservation_error")
)

// /RoomLine/Task/DiscussResults/MakeReservation/InformReservation/InformReservationMade
DEFINE_INFORM_AGENT( CInformReservationMade,
    PROMPT("inform reservation <reserve_room")
)

// /RoomLine/Task/DiscussResults/MakeReservation/InformReservation/AskWantSummary
DEFINE_REQUEST_AGENT( CAskWantSummary, 
	REQUEST_CONCEPT(want_summary)
	GRAMMAR_MAPPING("![Yes]>true, ![No]>false")
)

// /RoomLine/Task/DiscussResults/MakeReservation/InformReservation/GiveSummary
DEFINE_INFORM_AGENT( CGiveSummary, 
	PRECONDITION(IS_TRUE(want_summary))
	PROMPT("{inform give_summary <reserve_room}"\
		   "{inform room_details room_details<reserve_room}")
)

// /RoomLine/Task/DiscussResults/MakeReservation/CancelReservation
DEFINE_AGENCY( CCancelReservation, 
	TRIGGERED_BY_COMMANDS("[No], [CancelReservation]", "expl")
	DEFINE_SUBAGENTS(
		SUBAGENT(ExecuteCancelReservation, CExecuteCancelReservation, "")
		SUBAGENT(InformCancelReservation, CInformCancelReservation, ""))
	ON_COMPLETION(C("no_reservation") = true; CLEAR(reserve_room))
)
		
// /RoomLine/Task/DiscussResults/MakeReservation/CancelReservation/ExecuteCancelReservation
DEFINE_EXECUTE_AGENT( CExecuteCancelReservation,
	CALL("gal_be.launch_query query=cancel_room <reserve_room")
)

// /RoomLine/Task/DiscussResults/MakeReservation/CancelReservation/InformCancelReservation
DEFINE_INFORM_AGENT( CInformCancelReservation,
    PROMPT("inform cancel_reservation <reserve_room")
)

// /RoomLine/Task/DiscussResults/GotoQuery
DEFINE_EXECUTE_AGENT( CGotoQuery,
    TRIGGERED_BY(
        IS_ACTIVE_TOPIC(..) && 
        (UPDATED(room_query) || INVALIDATED(room_query.-1)))
    EXECUTE(
        // if the user specified a wanted room number, don't do anything
        if(AVAILABLE(../PresentResults/MultipleResults/IdentifyRoom/wanted_room_number)) return;

        // o/w set all this stuff to completed
        SET_COMPLETED(../PresentResults/MultipleResults/IdentifyRoom/RefineQuery)
        SET_COMPLETED(../PresentResults/MultipleResults/IdentifyRoom/ChooseFirstRoom)
        SET_COMPLETED(../PresentResults/MultipleResults/IdentifyRoom/ChooseSecondRoom)
        SET_COMPLETED(../PresentResults/MultipleResults/IdentifyRoom/ChooseAnyRoom)
        SET_COMPLETED(../PresentResults/MultipleResults/IdentifyRoom/GiveListOfRooms)
        SET_COMPLETED(../PresentResults/MultipleResults/IdentifyRoom/SuggestRoom/RepeatARoom)
        SET_COMPLETED(../PresentResults/MultipleResults/IdentifyRoom/AskForRoom/QueryRoomProperties)
        SET_COMPLETED(../PresentResults/MultipleResults/IdentifyRoom/AskForRoom/RepeatListOfRooms)
        SET_COMPLETED(../PresentResults/MultipleResults/IdentifyRoom/IdentifyRoomByNumber)

        SET_COMPLETED(..)
        REOPEN_AGENT(../../GetQuery)
        REOPEN_AGENT(../../InformPendingQuery)
        REOPEN_AGENT(../../PerformQuery)


		// if reserve_room was updated, clear it
		if(UPDATED(reserve_room)) CLEAR(reserve_room);

		bool bClearEndTime =
            (UPDATED(room_query.date_time.time.start_time) ||
			 UPDATED(room_query.date_time.time.timeperiod_spec) ||
             UPDATED(room_query.date_time.time.time_duration)) && 
			 NOT_UPDATED(room_query.date_time.time.end_time);

        bool bClearStartTime =
            NOT_UPDATED(room_query.date_time.time.start_time) && 
            (UPDATED(room_query.date_time.time.end_time) ||
			 UPDATED(room_query.date_time.time.timeperiod_spec));

		bool bClearDuration = 
			(NOT_UPDATED(room_query.date_time.time.time_duration) && 
			 UPDATED(room_query.date_time.time.timeperiod_spec)) || 
			 INVALIDATED(room_query.date_time.time.start_time) ||
			 INVALIDATED(room_query.date_time.time.end_time);

        C("room_query").MergeHistory();

        if(bClearEndTime) CLEAR(room_query.date_time.time.end_time);
        if(bClearStartTime) CLEAR(room_query.date_time.time.start_time);
		if(bClearDuration) CLEAR(room_query.date_time.time.time_duration);
    )
)

//-----------------------------------------------------------------------------
// /RoomLine/AnythingElse
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CAnythingElse, 
    IS_MAIN_TOPIC()
    DEFINE_CONCEPTS(
        BOOL_USER_CONCEPT(anything_else, "expl"))
    DEFINE_SUBAGENTS(
        SUBAGENT(ResetTask, CResetTask, "")
        SUBAGENT(AskAnythingElse, CAskAnythingElse, "request_default")
        SUBAGENT(NothingElse, CNothingElse, ""))
    SUCCEEDS_WHEN(COMPLETED(NothingElse) || IS_TRUE(anything_else) ||
                  UPDATED(/RoomLine/Task/room_query))
    ON_COMPLETION(		
        if(IS_TRUE(anything_else) || UPDATED(/RoomLine/Task/room_query)) Reset();
    )
)

// /RoomLine/AnythingElse/ResetTask
DEFINE_EXECUTE_AGENT( CResetTask, 
    EXECUTE(
        if(!IS_TRUE(/RoomLine/Task/DiscussResults/no_reservation)) {
    		RESET_AGENT(/RoomLine/Task)
	    	if(UPDATED(reserve_room)) REOPEN_CONCEPT(reserve_room)
	    }
	    C("/RoomLine/Task/room_query").ClearExplicitlyConfirmedHyp();
	    C("/RoomLine/Task/room_query").ClearExplicitlyDisconfirmedHyp();
	)    
)


// /RoomLine/AnythingElse/AskAnythingElse
DEFINE_REQUEST_AGENT( CAskAnythingElse, 
    REQUEST_CONCEPT(anything_else)
	PROMPT(":non-repeatable request anything_else")
    GRAMMAR_MAPPING("[yes]>true, [no]>false, [Quit]>false")
    SUCCEEDS_WHEN(AVAILABLE(anything_else) || 
                  UPDATED(/RoomLine/Task/room_query))
    NONUNDERSTANDING_THRESHOLD(0)
    ON_COMPLETION(            

        // finally, if the user indirectly answered the question, then done
        if(UPDATED(/RoomLine/Task/room_query)) {
            REOPEN_AGENT(/RoomLine/Task/GetQuery);
            REOPEN_AGENT(/RoomLine/Task/InformPendingQuery);
            REOPEN_AGENT(/RoomLine/Task/PerformQuery);
            bool bClearEndTime =
                (UPDATED(/RoomLine/Task/room_query.date_time.time.start_time) ||
                UPDATED(/RoomLine/Task/room_query.date_time.time.time_duration)) && 
                NOT_UPDATED(/RoomLine/Task/room_query.date_time.time.end_time);

            bool bClearStartTime =
                NOT_UPDATED(/RoomLine/Task/room_query.date_time.time.start_time) && 
                UPDATED(/RoomLine/Task/room_query.date_time.time.end_time);
	        // if the date was updated but to the same value, then clear the 
	        // start and end_times
	        if(UPDATED(/RoomLine/Task/room_query.date_time.date) && 
	            NOT_UPDATED(/RoomLine/Task/room_query.date_time.time.start_time) && 
	            NOT_UPDATED(/RoomLine/Task/room_query.date_time.time.end_time) && 
	            (C("/RoomLine/Task/room_query.date_time.date") == C("/RoomLine/Task/room_query.date_time.date.-1"))) {
	            bClearStartTime = true;
	            bClearEndTime = true;
	            CLEAR(/RoomLine/Task/room_query.date_time.time.timeperiod_spec);
	        }

            C("/RoomLine/Task/room_query").MergeHistory();

            if(bClearEndTime) CLEAR(/RoomLine/Task/room_query.date_time.time.end_time);
            if(bClearStartTime) CLEAR(/RoomLine/Task/room_query.date_time.time.start_time);

        } else {
            // o/w reset the agent, in case it was just reopened before
            RESET_AGENT(/RoomLine/Task);
            // also reset the date_time reference date
            pTrafficManager->Call(this, "datetime.begin_session :non-blocking");
        }
    )
)

// /RoomLine/AnythingElse/NothingElse
DEFINE_EXECUTE_AGENT( CNothingElse,
    PRECONDITION(IS_FALSE(anything_else))
    EXECUTE(SET_COMPLETED(/RoomLine/Task))
)

//-----------------------------------------------------------------------------
// /RoomLine/InformCancelPrevReservation
//-----------------------------------------------------------------------------
DEFINE_INFORM_AGENT( CInformCancelPrevReservation,
	PRECONDITION(SAME_AS_TRIGGER)
    TRIGGERED_BY_COMMANDS("@[CancelReservation]", "expl")
	EXPECT_WHEN(AVAILABLE(reserve_room) && !UPDATED(reserve_room))
    PROMPT("inform cancel_reservation <@reserve_room")
	ON_COMPLETION(
		CLEAR(reserve_room);
		RESET)
)

//-----------------------------------------------------------------------------
// /RoomLine/Logout
//-----------------------------------------------------------------------------
DEFINE_AGENCY( CLogout,
    IS_MAIN_TOPIC()
    DEFINE_SUBAGENTS(
        SUBAGENT(RequestSatisfied, CRequestSatisfied, "request_default") 
        SUBAGENT(InformLogout, CInformLogout, "")
    )
    PROMPT_ESTABLISHCONTEXT("establish_context logout")
)

// /RoomLine/Logout/RequestSatisfied
DEFINE_REQUEST_AGENT( CRequestSatisfied, 
    REQUEST_CONCEPT(satisfied)
    GRAMMAR_MAPPING("![Yes]>true, ![No]>false")
)

// /RoomLine/Logout/InformLogout
DEFINE_INFORM_AGENT( CInformLogout, 
	PROMPT("inform logout")
)

//-----------------------------------------------------------------------------
//
// AGENT DECLARATIONS
//
//-----------------------------------------------------------------------------
DECLARE_AGENTS(
	DECLARE_AGENT(CRoomLine)
        DECLARE_AGENT(CResetDateTime)
		DECLARE_AGENT(CWelcome)
		DECLARE_AGENT(CTask)
			DECLARE_AGENT(CGetQuery)
                DECLARE_AGENT(CHowMayIHelpYou)
                DECLARE_AGENT(CHowMayIHelpYouAgain)
				DECLARE_AGENT(CRequestDateTime)
       				DECLARE_AGENT(CRequestWhen)
                    DECLARE_AGENT(CRequestDay)
                    DECLARE_AGENT(CRequestTime)
                    DECLARE_AGENT(CRequestSpecificTimes)
                    DECLARE_AGENT(CRequestStartTime)
                    DECLARE_AGENT(CRequestEndTime)
                    DECLARE_AGENT(CDisambiguateDate)
                        DECLARE_AGENT(CRequestUnambiguousDate)
                        DECLARE_AGENT(CNeitherDate)
                        DECLARE_AGENT(CFirstDate)
                        DECLARE_AGENT(CSecondDate)
                    DECLARE_AGENT(CPastDate)
                    DECLARE_AGENT(CWrongTimeOrder)
                    DECLARE_AGENT(CComputeEndTimeFromDuration)
                    DECLARE_AGENT(CWrongDuration)
				DECLARE_AGENT(CExpectRoomLocation)
				DECLARE_AGENT(CExpectRoomNumber)			
				DECLARE_AGENT(CExpectRoomSize)
				DECLARE_AGENT(CExpectRoomSizeSpec)
				DECLARE_AGENT(CExpectRoomProjector)
				DECLARE_AGENT(CExpectRoomNetwork)
				DECLARE_AGENT(CExpectRoomWhiteboard)
                DECLARE_AGENT(CExpectRoomComputer)
                DECLARE_AGENT(CExpectNeedRoom)
            DECLARE_AGENT(CInformPendingQuery)
            DECLARE_AGENT(CPerformQuery)
            DECLARE_AGENT(CBackendError)
			DECLARE_AGENT(CDiscussResults)
                DECLARE_AGENT(CPresentResults)
                    DECLARE_AGENT(CNoResults)
				    DECLARE_AGENT(CSingleResult)
					    DECLARE_AGENT(CInformSingleResult)	
                        DECLARE_AGENT(CRequestWantReservation)
						DECLARE_AGENT(CQuerySingleRoomDetails)
						DECLARE_AGENT(CQueryOtherRooms)
                        DECLARE_AGENT(CRecordReservation)
                        DECLARE_AGENT(CInformNoReservation)
				    DECLARE_AGENT(CMultipleResults)
					    DECLARE_AGENT(CInformMultipleResults)
                        DECLARE_AGENT(CIdentifyRoom)
                            DECLARE_AGENT(CAskForSingleRoom)
                            DECLARE_AGENT(CGiveSingleRoomDetails)
							DECLARE_AGENT(CAutoRefine)
								DECLARE_AGENT(CRefineSize)
								DECLARE_AGENT(CRefineProjector)
								DECLARE_AGENT(CRefineLocation)
							DECLARE_AGENT(CSuggestRoom)
                                DECLARE_AGENT(CInformARoom)
                                DECLARE_AGENT(CRequestWantThisRoom)
                                DECLARE_AGENT(CRepeatARoom)
                                DECLARE_AGENT(CTryNextOne)
                                DECLARE_AGENT(CEndOfRoomList)                                
                            DECLARE_AGENT(CAskForRoom)
                                DECLARE_AGENT(CRequestWantReservationMultiple)
                                DECLARE_AGENT(CRequestWhichRoom)
                                DECLARE_AGENT(CQueryRoomProperties)
                                    DECLARE_AGENT(CExpectQueryRoom)
                                    DECLARE_AGENT(CIdentifyQueriedRoomIndex)
                                    DECLARE_AGENT(CInformQueriedRoom)   
                                    DECLARE_AGENT(CRequestWantQueriedRoom)
                                DECLARE_AGENT(CRepeatListOfRooms)                                
                            DECLARE_AGENT(CRefineQuery)
                                DECLARE_AGENT(CExpectEquipQuery)
                                DECLARE_AGENT(CExpectEquipQuery2)
                                DECLARE_AGENT(CExpectSizeQuery)
                                DECLARE_AGENT(CExpectSizeSpecQuery)
                                DECLARE_AGENT(CExpectLocationQuery)
                                DECLARE_AGENT(CReOpenResults)
                                DECLARE_AGENT(CPerformRefinedQuery)
                                DECLARE_AGENT(CNoRefinedResults)
                                    DECLARE_AGENT(CInformNoRefinedResults)
                                    DECLARE_AGENT(CAskStillWantAny)
                                DECLARE_AGENT(CDifferentRefinedResults)
                                DECLARE_AGENT(CSameRefinedResults)
                            DECLARE_AGENT(CIdentifyRoomByNumber)
                            DECLARE_AGENT(CRoomNotInList)
                            DECLARE_AGENT(CChooseFirstRoom)
                            DECLARE_AGENT(CChooseSecondRoom)
                            DECLARE_AGENT(CChooseAnyRoom)
                            DECLARE_AGENT(CGiveListOfRooms)
                        DECLARE_AGENT(CRecordReservationMultiple)
                        DECLARE_AGENT(CInformNoReservationMultiple)
                        DECLARE_AGENT(CInformNoReservationRefined)
                DECLARE_AGENT(CMakeReservation)
                    DECLARE_AGENT(CInformMakeReservation)
					DECLARE_AGENT(CGetUser)
						DECLARE_AGENT(CAskUserName)
						DECLARE_AGENT(CCheckUserId)
						DECLARE_AGENT(CInvalidUserId)
					DECLARE_AGENT(CGreetUser)
					DECLARE_AGENT(CInformNoReservationForGuest)
                    DECLARE_AGENT(CPerformReservation)
                    DECLARE_AGENT(CInformReservationSuccess)
						DECLARE_AGENT(CInformReservationMade)
						DECLARE_AGENT(CAskWantSummary)
						DECLARE_AGENT(CGiveSummary)
					DECLARE_AGENT(CInformReservationError)
					DECLARE_AGENT(CCancelReservation)
						DECLARE_AGENT(CExecuteCancelReservation)
						DECLARE_AGENT(CInformCancelReservation)
                DECLARE_AGENT(CGotoQuery)
        DECLARE_AGENT(CAnythingElse)
            DECLARE_AGENT(CResetTask)
            DECLARE_AGENT(CAskAnythingElse)
            DECLARE_AGENT(CNothingElse)
		DECLARE_AGENT(CInformCancelPrevReservation)
		DECLARE_AGENT(CLogout)
            DECLARE_AGENT(CRequestSatisfied)
            DECLARE_AGENT(CInformLogout)
)

//-----------------------------------------------------------------------------
// DIALOG TASK ROOT DECLARATION
//-----------------------------------------------------------------------------
DECLARE_DIALOG_TASK_ROOT(RoomLine, CRoomLine, "")