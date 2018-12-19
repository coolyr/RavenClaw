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
// LOG.CPP    - implementation of logging support
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
//   [2004-03-15] (dbohus):  fixed bug in logging long strings to the screen
//   [2003-05-13] (dbohus):  changed InitializeLogging function to work with 
//                            the new configuration parameters
//   [2003-04-09] (dbohus, 
//                 antoine): added dialog task logging stream
//   [2002-12-05] (dbohus):  added support for colored logging
//   [2002-10-20] (dbohus):  changed initialize function so that streams are 
//                            added according to specified parameters
//   [2002-05-25] (dbohus):  deemed preliminary stable version 0.5
//   [2002-03-21] (dbohus):  added support for multiple logging streams
//   [2002-01-02] (dbohus):  started working on this
// 
//-----------------------------------------------------------------------------

#include "../DMInterfaces/DMInterface.h"
#include "DMBridge.h"
#include "Log.h"

//-----------------------------------------------------------------------------
// D: Exit on fatal error flag
//-----------------------------------------------------------------------------

bool bExitOnFatalError;

//-----------------------------------------------------------------------------
// Logging streams: definition and functions
//-----------------------------------------------------------------------------

string sAllLoggingStreams = (string)CORETHREAD_STREAM + ";" +
(string)REGISTRY_STREAM + ";" +
(string)DMCORE_STREAM + ";" +
(string)EXPECTATIONAGENDA_STREAM + ";" +
(string)TRAFFICMANAGER_STREAM + ";" +
(string)TRAFFICMANAGERDUMP_STREAM + ";" +
(string)INPUTMANAGER_STREAM + ";" +
(string)OUTPUTMANAGER_STREAM + ";" +
(string)OUTPUTHISTORY_STREAM + ";" +
(string)STATEMANAGER_STREAM + ";" +
(string)DTTMANAGER_STREAM + ";" +
(string)GROUNDINGMANAGER_STREAM + ";" +
(string)GROUNDINGMODELX_STREAM + ";" +
(string)DIALOGTASK_STREAM + ";" +
(string)CONCEPT_STREAM + ";" +
(string)BELIEFUPDATING_STREAM;


// D: Add a logging stream
// ����һ����־��¼��
void AddLoggingStream(string sStreamName, string sColorMap, bool bDisplayed, bool bEnabled)
{
	TLoggingStreamsHash::iterator iPtr;

	if ((iPtr = lshLogStreams.find(sStreamName)) == lshLogStreams.end())
	{
		// if the stream is not already in there
		TLoggingStream lsLogStream;
		lsLogStream.bDisplayed = bDisplayed;
		lsLogStream.bEnabled = bEnabled;
		if (sColorMap.empty())
		{
			// if no color map specified, use white
			// ���û��ָ����ɫ��Ĭ��Ϊ��ɫ
			lsLogStream.iColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		}
		else
		{
			// sColorMap should be RGBI
			lsLogStream.iColor = 0;
			if (sColorMap[0] == '1')
				lsLogStream.iColor |= FOREGROUND_RED;
			if (sColorMap[1] == '1')
				lsLogStream.iColor |= FOREGROUND_GREEN;
			if (sColorMap[2] == '1')
				lsLogStream.iColor |= FOREGROUND_BLUE;
			if (sColorMap[3] == '1')
				lsLogStream.iColor |= FOREGROUND_INTENSITY;
		}

		lshLogStreams.insert(TLoggingStreamsHash::value_type(sStreamName, lsLogStream));
	}
	else
	{
		// if the stream is already in there, just set its properties
		// �����־���Ѿ����ڣ���ֻ�Ǹ�������
		iPtr->second.bDisplayed = bDisplayed;
		iPtr->second.bEnabled = bEnabled;
		int iColor;
		if (!sColorMap.empty())
		{
			// sColorMap should be RGBI
			iColor = 0;
			if (sColorMap[0] == '1')
				iColor |= FOREGROUND_RED;
			if (sColorMap[1] == '1')
				iColor |= FOREGROUND_GREEN;
			if (sColorMap[2] == '1')
				iColor |= FOREGROUND_BLUE;
			if (sColorMap[3] == '1')
				iColor |= FOREGROUND_INTENSITY;
			iPtr->second.iColor = iColor;
		}
	}
}

// D: Delete a logging stream
void DeleteLoggingStream(string sStreamName)
{
	TLoggingStreamsHash::iterator iPtr;

	// if the stream is not defined, return
	if ((iPtr = lshLogStreams.find(sStreamName)) == lshLogStreams.end())
		return;

	// otherwise delete it, then return
	lshLogStreams.erase(iPtr);
}

// D: Enable a logging stream
void EnableLoggingStream(string sStreamName)
{
	TLoggingStreamsHash::iterator iPtr;

	// if the stream is not defined, return
	if ((iPtr = lshLogStreams.find(sStreamName)) == lshLogStreams.end())
		return;

	// otherwise set it to enabled, and return
	iPtr->second.bEnabled = true;
}

// D: Disable a logging stream
void DisableLoggingStream(string sStreamName)
{
	TLoggingStreamsHash::iterator iPtr;

	// if the stream is not defined, return
	if ((iPtr = lshLogStreams.find(sStreamName)) == lshLogStreams.end())
		return;

	// otherwise set it to disabled, and return
	iPtr->second.bEnabled = false;
}

// D: Enable all logging streams
void EnableAllLoggingStreams()
{
	TLoggingStreamsHash::iterator iPtr;

	// go through all the streams and enable them all
	for (iPtr = lshLogStreams.begin(); iPtr != lshLogStreams.end(); iPtr++)
		iPtr->second.bEnabled = true;
}

// D: Disable all logging streams
void DisableAllLoggingStreams()
{
	TLoggingStreamsHash::iterator iPtr;

	// go through all the streams and enable them all
	for (iPtr = lshLogStreams.begin(); iPtr != lshLogStreams.end(); iPtr++)
		iPtr->second.bEnabled = false;
}


//-----------------------------------------------------------------------------
// Logging defines, levels, function etc
//-----------------------------------------------------------------------------

// D: the log function (works like a printf)
//��¼��־����
int Log(string sLoggingStream, const char* lpszFormat, ...)
{

	//######################################################

	/*
	vsnprintf��snprintf��C����printf���庯���ĳ�Ա����غ����б����£�

	#include <stdio.h>
	int printf(const char *format, ...); //�������׼���
	int fprintf(FILE *stream, const char *format, ...); //������ļ�
	int sprintf(char *str, const char *format, ...); //������ַ���str��
	int snprintf(char *str, size_t size, const char *format, ...); //��size��С������ַ���str��

	���º��������������һһ��Ӧ��ͬ��ֻ���ں�������ʱ���������...��Ӧ��һ����������va_list������������ں�������ǰapҪͨ��va_start()������̬��ȡ��
	#include <stdarg.h>
	int vprintf(const char *format, va_list ap);
	int vfprintf(FILE *stream, const char *format, va_list ap);
	int vsprintf(char *str, const char *format, va_list ap);
	int vsnprintf(char *str, size_t size, const char *format, va_list ap);
	�ɱ�����б�va_list��˵����
	void va_start(va_list ap, last);
	void va_end(va_list ap);
	va_start��va_end�ǳɶԱ����õģ�
	��ʼ��ʱ�򱻵���va_start����ø����������ַ
	������ʱ�򱻵���va_end���ͷ���Ӧ����Դ



	����ͨ��vsnprintf()ʵ��snprintf()���ܣ�����������ɲ���linux man va_start�ֲᣩ
	#include <stdio.h>
	#include <stdarg.h>
	int my_snprintf(char *s, int size, const char *fmt, ...) //���Զ��庯������ϵͳ�ṩ��snprintf()������ͬ��
	{
	va_list ap;
	int n=0;
	va_start(ap, fmt); //��ÿɱ�����б�
	n=vsnprintf (s, size, fmt, ap); //д���ַ���s
	va_end(ap); //�ͷ���Դ
	return n; //����д����ַ�����
	}

	int main() {
	char str[1024];
	my_snprintf( str, sizeof(str), "%d,%d,%d,%d",5,6,7,8);
	printf("%s\n",str);
	return 0;
	}
	*/


	//########################################################

	//#typedef char *  va_list; �ⲿ����
	va_list pArgs; //char* ����
	int returnCode = 0;


	//#������־���Ƿ���� sLoggingStream
	// check if the logging stream exists
	TLoggingStreamsHash::iterator iPtr;
	// if the stream is not defined, return
	// #��find��������λ���ݳ���λ�ã�������һ���������������ݳ���ʱ����������������λ�õĵ�������
	// #���map��û��Ҫ���ҵ����ݣ������صĵ���������end�������صĵ�����
	if ((iPtr = lshLogStreams.find(sLoggingStream)) == lshLogStreams.end())
	{
		Log(WARNING_STREAM, "Attempt to log on undefined logging stream: " + sLoggingStream);
		return 0;
	}

	// get the current time
	//��ȡ��ǰʱ��
	_int64 liTimestamp = GetCurrentAbsoluteTimestamp();

	// put it on the screen, if the stream is to be displayed
	//��ʾ����Ļ��
	va_start(pArgs, lpszFormat);
	if (iPtr->second.bDisplayed)
	{
		// construct the string
		char lpszBuffer1[STRING_MAX];
		char lpszBuffer2[STRING_MAX];

		//����ԭ��int snprintf(char *str, size_t size, const char *format, ...)
		//���ɱ������(...)����format��ʽ�����ַ�����Ȼ���临�Ƶ�str��
		//#c_str()��������һ��ָ������C�ַ�����ָ��, �����뱾string����ͬ
		_snprintf(lpszBuffer1, STRING_MAX - 1, "[%s@%s (%d)] ", sLoggingStream.c_str(), TimestampToString(liTimestamp).c_str(), GetSessionTimestamp(liTimestamp));
		_vsnprintf(lpszBuffer2, STRING_MAX - 1, lpszFormat, pArgs);
		//#��src��ָ�ַ�����ǰn���ַ���ӵ�dest��β��������dest��β����'/0'��ʵ���ַ������ӡ�
		strncat(lpszBuffer1, lpszBuffer2, STRING_MAX - 2 - strlen(lpszBuffer1));
		strncat(lpszBuffer1, "\n", STRING_MAX - 1 - strlen(lpszBuffer1));
		// display it
		//��ʾ����
		SetConsoleTextAttribute(hStdOutput, (WORD)lshLogStreams[sLoggingStream].iColor);
		DWORD cWritten;
		//WriteFile����������д���ݵ��ļ���ReadFile�����Ǵ��ļ����ȡ���ݳ�����
		//�������������������Զ�ȡд���̵��ļ���Ҳ���Խ��պͷ�����������ݣ����ж�д���ڡ�USB�����ڵ��豸�����ݡ�
		WriteFile(hStdOutput, lpszBuffer1, lstrlen(lpszBuffer1), &cWritten, NULL);
	}

	// also send it to the file if logging is initialized
	// and the stream is to enabled for logging
	//�����־��¼�ѳ�ʼ��, ����������־��¼��Ҳ�Ὣ��  -��¼���ļ�
	if (fileLog && iPtr->second.bEnabled)
	{

		// then do the printf into the file
		//�÷���	int fprintf (FILE* stream, const char*format, [argument])
		//������	FILE*stream���ļ�ָ��
		//		const char* format�������ʽ
		//		[argument]�����Ӳ����б�
		fprintf(fileLog, "[%s@%s (%d)] ", sLoggingStream.c_str(), TimestampToString(liTimestamp).c_str(), GetSessionTimestamp(liTimestamp));
		//��  ��: �͸�ʽ�������һ����
		//��  �� : int vfprintf(FILE *stream, char *format, va_list param);
		//��  �飺streamΪ�ļ��������ò���Ϊstderrʱ����ӡ����Ļ����������������÷���printf��ͬ��
		returnCode = vfprintf(fileLog, lpszFormat, pArgs);
		fprintf(fileLog, "\n");
		fflush(fileLog);
	}

	return(returnCode);
}

// D: another variant of the log function, just marshalls info
//    towards the basic one
// �������� Log()
int Log(string sLoggingStream, string sMessage)
{
	return Log(sLoggingStream, sMessage.c_str());
}

//-----------------------------------------------------------------------------
// Logging initialization and closing functions
//-----------------------------------------------------------------------------
// D: logging initialization
// ��־��ʼ��
void InitializeLogging(string sALogFolder, string sALogPrefix, string sALogFilename, string sLoggedStreamsList, string sDisplayedStreamsList, string sAExitOnFatalError)
{

	// Set the exit on fatal error flag                           
	bExitOnFatalError = (sAExitOnFatalError == "true");

	// Add the basic logging streams
	AddLoggingStream(WARNING_STREAM, "1001");
	AddLoggingStream(ERROR_STREAM, "1001");
	AddLoggingStream(FATALERROR_STREAM, "1001");

	// Add all the default streams, but don't enable them yet
	// ��������Ĭ�ϵ�log�� - disable
	vector<string> vsStreams = PartitionString(sAllLoggingStreams, ";");
	for (unsigned int i = 0; i < vsStreams.size(); i++)
	{
		string sName, sColorMap;
		SplitOnFirst(Trim(vsStreams[i]), ":", sName, sColorMap);
		AddLoggingStream(sName, sColorMap, false, false);
	}

	// Enable the streams from the description
	// enableͨ���������õ�log��
	vector<string> vsEnabledStreams = PartitionString(sLoggedStreamsList, ";");
	for (unsigned int i = 0; i < vsEnabledStreams.size(); i++)
	{
		string sName, sColorMap;
		SplitOnFirst(Trim(vsEnabledStreams[i]), ":", sName, sColorMap);
		AddLoggingStream(sName, sColorMap, false, true);
	}

	// And display the ones that are in the displayed list
	// enableͨ���������õ�display��
	vector<string> vsDisplayedStreams =	PartitionString(sDisplayedStreamsList, ";");
	for (unsigned int i = 0; i < vsDisplayedStreams.size(); i++)
	{
		string sName, sColorMap;
		SplitOnFirst(Trim(vsDisplayedStreams[i]), ":", sName, sColorMap);
		AddLoggingStream(sName, sColorMap, true, true);
	}


	// try to open the log file
	// ����־�ļ�
	sLogFolder = sALogFolder + "\\";
	sLogFilename = sALogPrefix + sALogFilename;
	string sLogPath = sLogFolder + sLogFilename;
	fileLog = fopen(sLogPath.c_str(), "w");

	// if it's not possible in the location indicated, try in the current folder
	// ������õ�logλ���޷������ļ������Ե�ǰλ�ô���log
	if (!fileLog)
	{
		Warning(FormatString("ERROR CREATING LOGFILE %s (%s)\n", sLogPath.c_str(), strerror(errno)));
		fileLog = fopen(("./" + sLogFilename).c_str(), "w");
	}
}

// D: close logging
void TerminateLogging()
{
	if (fileLog)
		fclose(fileLog);
}


//-----------------------------------------------------------------------------
// Error handling functions 
//   these functions are basically stubs for the Log function
//-----------------------------------------------------------------------------

// D: Warning
void __Warning(const char* lpszWarning, char* lpszFile, int iLine)
{
	Log(WARNING_STREAM, "%s <file %s, line %d>.", lpszWarning, lpszFile, iLine);
}

// D: NonFatal error. 
void __Error(const char* lpszError, char* lpszFile, int iLine)
{
	Log(ERROR_STREAM, "%s <file %s, line %d>.", lpszError, lpszFile, iLine);
}

// D: Fatal error. Quit after dealing with it
#pragma warning (disable:4127)
void __FatalError(const char* lpszError, char* lpszFile, int iLine)
{
	Log(FATALERROR_STREAM, "%s <file %s, line %d>.", lpszError, lpszFile, iLine);
	if (!bExitOnFatalError)
	{
		while (1);
	}
	exit(1);
}
#pragma warning (default:4127)