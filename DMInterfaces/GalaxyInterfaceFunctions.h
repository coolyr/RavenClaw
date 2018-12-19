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
// GALAXYINTERFACEFUNCTIONS.H - declares the Galaxy server functionality of 
//                              the Dialog Manager
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
//   [2003-03-17] (dbohus): changed so that the server name and port are 
//                           specified from the dialog task
//   [2002-05-25] (dbohus): deemed preliminary stable version 0.5
//   [2002-01-04] (dbohus): started working on this
// 
//-----------------------------------------------------------------------------

// D: Check that the compiler configuration is correct (these files should 
//    be compiled only when GALAXY is defined)
#ifndef GALAXY
#error GalaxyInterfaceFunctions.cpp should be compiled only in the galaxy version
#endif


// D: external declarations for the server name and port number (these
//    can now be defined in the DialogTask file by the DMSERVER_NAME()
//    and DMSERVER_PORT() macros
extern char* lpszDMServerName;
extern unsigned short iDMServerPort;

static char *ServerName = lpszDMServerName;
GAL_SERVER_PORT(iDMServerPort)

// declarations for the services (functions) provided by the Dialog Manager
// as a galaxy server.
GAL_SERVER_OP(begin_session)
GAL_SERVER_OP(end_session)
GAL_SERVER_OP(start_inactivity_timeout)
GAL_SERVER_OP(cancel_inactivity_timeout)
GAL_SERVER_OP(reinitialize)
GAL_SERVER_OP(handle_event)
