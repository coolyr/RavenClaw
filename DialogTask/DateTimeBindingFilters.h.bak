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
// DATETIMEBINDINGFILTERS.H   - declaration of concept binding filters for 
//                      numbers and date-times for the RoomLine system
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

#pragma once
#ifndef __BINDINGFILTERS_H__
#define __BINDINGFILTERS_H__

#include "Utils/Utils.h"

// D: Filter for transforming an ascii string representation of a date into 
//    an actual date structure: uses the DateTime4 Hub Galaxt module
string DateTime4_DateTimeBindingFilter(string sSlotName, string sSlotValue);

// D: Filter for transforming an ascii string representation of a date into 
//    an actual date structure: uses the DateTime4 Hub Galaxt module. This
//    one is very similar to the one above, only that it only looks for times
//    and if the start and end times are the same, then it considers it an
//    end_time. This is basically just a hack to deal with inconsistencies in 
//    DateTime4
string DateTime4_EndTimeBindingFilter(string sSlotName, string sSlotValue);

// D: Filter for transforming an ascii string representation of a number (i.e.
//    five hundred thirty four) to the ascii numerical representation (i.e. 
//    534). Uses the DateTime4 Hub Galaxy server (the ParseInt function)
string DateTime4_NumberBindingFilter(string sSlotName, string sSlotValue);

#endif // __BINDINGFILTERS_H__