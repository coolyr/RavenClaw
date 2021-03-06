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
// DATETIMEBINDINGFILTERS.CPP - implementation of concept binding filters for
//                      numbers and date-times
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
//   [2003-03-30] (dbohus): added end-time binding filter to fix datetime4
//                           behavior
//   [2003-03-29] (dbohus): added number binding filter for room size
//   [2003-02-16] (dbohus): no longer need the roomnumber binding filter
//   [2003-02-16] (dbohus): changed roombindingfilter to a domain specific 
//                           beast
//   [2002-12-12] (dbohus): hacked in the DateTime4 filter so that it does all
//                           the right things for the RoomLine domain
//   [2002-11-21] (dbohus): wrote filters for numbers and date_time which use
//                           the DateTime4 hub server
//   [2002-11-20] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

#include "DateTimeBindingFilters.h"
#include "../DMInterfaces/DMInterface.h"
#include "../DMCore/Core.h"

// D: Filter for transforming an ascii string representation of a date into 
//    an actual date structure: uses the DateTime4 Hub Galaxy module
string DateTime4_DateTimeBindingFilter(string sSlotName, string sSlotValue) {

	// Get the parse of the last utterance
	CInteractionEvent *lastInputEvent = pInteractionEventManager->GetLastInput();

    //	THIS IS SOMEWHAT OF AN UGLY HACK:
    //   We recreate a Galaxy frame containing the parse from the input,
    //   so that it matches what DateTime expects.

    // Check that the last input event is not null, as it could be null
    // if the only events received in this session from Apollo
    // were GUI events.
	string galParse;
	if (lastInputEvent != NULL && pInteractionEventManager->GetLastEvent()->GetType() != IET_GUI) {
        galParse = lastInputEvent->GetStringProperty("[gal_slotsframe]");

		if (galParse == "") {
            return sSlotValue;   
		}
	} else {
	    return sSlotValue;
	}

	string sParseString = "{c parse :slots " + galParse + "}";

	// call the DateTime.ParseDateTime function and feed it the whole 
    // parse tree 
	TGIGalaxyCall gcGalaxyCall;
	gcGalaxyCall.sModuleFunction = "DateTime.ParseDateTime";
    gcGalaxyCall.bBlockingCall = true;     
	gcGalaxyCall.s2sInputs.insert(STRING2STRING::value_type(":Date_Time_Parse", sParseString));

    // set the outputs
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":month", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":day", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":year", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":valid_date", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":start_time", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":end_time", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":time_duration", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":timeperiod_spec", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":valid_time", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":past_date", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":day_skip", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":weekday", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":past_time", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":need_date", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":date_exists", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":ambiguous", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":date_choice", ""));

    // retrieve the current thread id
    DWORD dwThreadId = GetCurrentThreadId();

    // send the message to the Galaxy Interface Thread
    PostThreadMessage(g_idDMInterfaceThread, WM_GALAXYCALL,
                      (WPARAM)&gcGalaxyCall, dwThreadId);	
	
	// and wait for a reply
	MSG Message;
	GetMessage(&Message, NULL, WM_ACTIONFINISHED, WM_ACTIONFINISHED);

    // construct the results string
    string sResult = "{\n";

	// start the time part
	sResult += "time\t{\n";
	if(gcGalaxyCall.s2sOutputs[":valid_time"] == "true") {
        // hacks for adapting to the way DateTime4 returns default values in some of the
        // fields (i.e start_time, end_time)
        if((gcGalaxyCall.s2sOutputs[":start_time"] != "") &&
           (gcGalaxyCall.s2sOutputs[":start_time"] != "0") &&
		   ((atoi(gcGalaxyCall.s2sOutputs[":start_time"].c_str()) % 100) != 1) &&
		   (atoi(gcGalaxyCall.s2sOutputs[":start_time"].c_str()) > 0) &&
		   (atoi(gcGalaxyCall.s2sOutputs[":start_time"].c_str()) < 2400) &&
		   ((atoi(gcGalaxyCall.s2sOutputs[":start_time"].c_str()) % 100) != 59) &&
		   ((atoi(gcGalaxyCall.s2sOutputs[":start_time"].c_str()) % 100) != 99))
            sResult += "start_time\t" + gcGalaxyCall.s2sOutputs[":start_time"] + "\n";
        if((gcGalaxyCall.s2sOutputs[":end_time"] != "") &&
            (gcGalaxyCall.s2sOutputs[":end_time"] != "0") &&
			((atoi(gcGalaxyCall.s2sOutputs[":end_time"].c_str()) % 100) != 1) &&
 		    (atoi(gcGalaxyCall.s2sOutputs[":end_time"].c_str()) > 0) &&
		    (atoi(gcGalaxyCall.s2sOutputs[":end_time"].c_str()) < 2400) &&
            ((atoi(gcGalaxyCall.s2sOutputs[":end_time"].c_str()) % 100) != 59) &&
			((atoi(gcGalaxyCall.s2sOutputs[":end_time"].c_str()) % 100) != 99) &&
            (gcGalaxyCall.s2sOutputs[":end_time"] != gcGalaxyCall.s2sOutputs[":start_time"]))
            sResult += "end_time\t" + gcGalaxyCall.s2sOutputs[":end_time"] + "\n";
        if(gcGalaxyCall.s2sOutputs[":timeperiod_spec"] != "") {
            string sTimePeriodSpec = Trim(gcGalaxyCall.s2sOutputs[":timeperiod_spec"]);
            if(sTimePeriodSpec == "pm") 
                sTimePeriodSpec = "afternoon";
            else if(sTimePeriodSpec == "am") 
                sTimePeriodSpec = "morning";
			if((sTimePeriodSpec != "anytime") && (sTimePeriodSpec != "now"))
				sResult += "timeperiod_spec\t" + sTimePeriodSpec + "\n";
        }
    }
    if(gcGalaxyCall.s2sOutputs[":time_duration"] != "") 
        sResult += "time_duration\t" + gcGalaxyCall.s2sOutputs[":time_duration"] + "\n";
   	// finish the time part
	sResult += "}\n";

	// now do the date part
	sResult += "date\t{\n";
    if(gcGalaxyCall.s2sOutputs[":valid_date"] == "true") {
        if(gcGalaxyCall.s2sOutputs[":month"] != "") 
            sResult += "month\t" + gcGalaxyCall.s2sOutputs[":month"] + "\n";
        if(gcGalaxyCall.s2sOutputs[":day"] != "") 
            sResult += "day\t" + gcGalaxyCall.s2sOutputs[":day"] + "\n";
        if(gcGalaxyCall.s2sOutputs[":year"] != "") 
            sResult += "year\t" + gcGalaxyCall.s2sOutputs[":year"] + "\n";
        sResult += "valid_date\ttrue\n";
        if(gcGalaxyCall.s2sOutputs[":past_date"] != "") 
            sResult += "past_date\t" + gcGalaxyCall.s2sOutputs[":past_date"] + "\n";
        if(gcGalaxyCall.s2sOutputs[":weekday"] != "") 
            sResult += "weekday\t" + gcGalaxyCall.s2sOutputs[":weekday"] + "\n";
    }
    if(gcGalaxyCall.s2sOutputs[":ambiguous"] != "") 
        sResult += "ambiguous\t" + gcGalaxyCall.s2sOutputs[":ambiguous"] + "\n";
    else if(gcGalaxyCall.s2sOutputs[":valid_date"] == "true")
        sResult += "ambiguous\tfalse\n";
    if(gcGalaxyCall.s2sOutputs[":date_choice"] != "") 
        sResult += "date_choice\t" + gcGalaxyCall.s2sOutputs[":date_choice"] + "\n";
	// finish the date part
	sResult += "}\n";
	
	// close it all
	sResult += "}\n";

    return sResult;
}



// D: Filter for transforming an ascii string representation of a date into 
//    an actual date structure: uses the DateTime4 Hub Galaxt module. This
//    one is very similar to the one above, only that it only looks for times
//    and if the start and end times are the same, then it considers it an
//    end_time. This is basically just a hack to deal with inconsistencies in 
//    DateTime4
string DateTime4_EndTimeBindingFilter(string sSlotName, string sSlotValue) {

	// Get the parse of the last utterance
	CInteractionEvent *lastInputEvent = pInteractionEventManager->GetLastInput();

	//	THIS IS SOMEWHAT OF AN UGLY HACK:
    //   We recreate a Galaxy frame containing the parse from the input,
    //   so that it matches what DateTime expects.


    // Check that the last input event is not null, as it could be null
    // if the only events received in this session from Apollo
    // were GUI events.
	string galParse;
	if (lastInputEvent != NULL && pInteractionEventManager->GetLastEvent()->GetType() != IET_GUI) {
        galParse = lastInputEvent->GetStringProperty("[gal_slotsframe]");

		if (galParse == "") {
            return sSlotValue;   
		}
	} else {
	    return sSlotValue;
	}

	string sParseString = "{c parse :slots " + galParse + "}";

	// call the DateTime.ParseDateTime function and feed it the whole 
    // parse tree 
	TGIGalaxyCall gcGalaxyCall;
	gcGalaxyCall.sModuleFunction = "DateTime.ParseDateTime";
    gcGalaxyCall.bBlockingCall = true;     
	gcGalaxyCall.s2sInputs.insert(STRING2STRING::value_type(":Date_Time_Parse", sParseString));

    // set the outputs
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":month", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":day", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":year", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":valid_date", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":start_time", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":end_time", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":time_duration", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":timeperiod_spec", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":valid_time", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":past_date", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":day_skip", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":weekday", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":past_time", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":need_date", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":date_exists", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":ambiguous", ""));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":date_choice", ""));

    // retrieve the current thread id
    DWORD dwThreadId = GetCurrentThreadId();

    // send the message to the Galaxy Interface Thread
    PostThreadMessage(g_idDMInterfaceThread, WM_GALAXYCALL,
                      (WPARAM)&gcGalaxyCall, dwThreadId);	
	
	// and wait for a reply
	MSG Message;
	GetMessage(&Message, NULL, WM_ACTIONFINISHED, WM_ACTIONFINISHED);

    // construct the results string
    string sResult = "{\n";

	// only the time part
	sResult += "time\t{\n";
    if(gcGalaxyCall.s2sOutputs[":valid_time"] == "true") {
        // hacks for adapting to the way DateTime4 returns default values in some of the
        // fields (i.e start_time, end_time)
        if((gcGalaxyCall.s2sOutputs[":start_time"] != "") &&
            (gcGalaxyCall.s2sOutputs[":start_time"] != "0") && 
		    ((atoi(gcGalaxyCall.s2sOutputs[":start_time"].c_str()) % 100) != 1) &&
		    (atoi(gcGalaxyCall.s2sOutputs[":start_time"].c_str()) > 0) &&
		    (atoi(gcGalaxyCall.s2sOutputs[":start_time"].c_str()) < 2400) &&
			((atoi(gcGalaxyCall.s2sOutputs[":start_time"].c_str()) % 100) != 59) &&
			((atoi(gcGalaxyCall.s2sOutputs[":start_time"].c_str()) % 100) != 99) &&
            (gcGalaxyCall.s2sOutputs[":start_time"] != gcGalaxyCall.s2sOutputs[":end_time"]))
            sResult += "start_time\t" + gcGalaxyCall.s2sOutputs[":start_time"] + "\n";
        if((gcGalaxyCall.s2sOutputs[":end_time"] != "") &&
            (gcGalaxyCall.s2sOutputs[":end_time"] != "0") && 
			((atoi(gcGalaxyCall.s2sOutputs[":end_time"].c_str()) % 100) != 1) && 
		    (atoi(gcGalaxyCall.s2sOutputs[":end_time"].c_str()) > 0) &&
		    (atoi(gcGalaxyCall.s2sOutputs[":end_time"].c_str()) < 2400) &&
 		    ((atoi(gcGalaxyCall.s2sOutputs[":end_time"].c_str()) % 100) != 59) && 
 		    ((atoi(gcGalaxyCall.s2sOutputs[":end_time"].c_str()) % 100) != 99))
            sResult += "end_time\t" + gcGalaxyCall.s2sOutputs[":end_time"] + "\n";
        if(gcGalaxyCall.s2sOutputs[":timeperiod_spec"] != "") {
            string sTimePeriodSpec = Trim(gcGalaxyCall.s2sOutputs[":timeperiod_spec"]);
            if(sTimePeriodSpec == "pm") 
                sTimePeriodSpec = "afternoon";
            else if(sTimePeriodSpec == "am") 
                sTimePeriodSpec = "morning";
			if((sTimePeriodSpec != "anytime") && (sTimePeriodSpec != "now"))
                sResult += "timeperiod_spec\t" + sTimePeriodSpec + "\n";
        }
    }
    if(gcGalaxyCall.s2sOutputs[":time_duration"] != "") 
        sResult += "time_duration\t" + gcGalaxyCall.s2sOutputs[":time_duration"] + "\n";
	// finish the time part
    sResult += "}\n";

	// close it all
	sResult += "}\n";

    return sResult;
}

// D: Filter for transforming an ascii string representation of a number (i.e.
//    five hundred thirty four) to the ascii numerical representation (i.e. 
//    534). Uses the DateTime4 Hub Galaxy server (the ParseInt function)
string DateTime4_NumberBindingFilter(string sSlotName, string sSlotValue) {

    // send the string to the DateTime.ParseInt function 
	TGIGalaxyCall gcGalaxyCall;
	gcGalaxyCall.sModuleFunction = "DateTime.ParseInt";
    gcGalaxyCall.bBlockingCall = true;     
	gcGalaxyCall.s2sInputs.insert(STRING2STRING::value_type(":number_string", 
		sSlotValue));
    gcGalaxyCall.s2sOutputs.insert(STRING2STRING::value_type(":number_int", 
        ""));
    // retrieve the current thread id
    DWORD dwThreadId = GetCurrentThreadId();

    // send the message to the Galaxy Interface Thread
    PostThreadMessage(g_idDMInterfaceThread, WM_GALAXYCALL,
                      (WPARAM)&gcGalaxyCall, dwThreadId);	
	
	// and wait for a reply
	MSG Message;
	GetMessage(&Message, NULL, WM_ACTIONFINISHED, WM_ACTIONFINISHED);

    return gcGalaxyCall.s2sOutputs[":number_int"];
}