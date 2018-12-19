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
// 增加一个日志记录流
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
			// 如果没有指定颜色，默认为白色
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
		// 如果日志流已经存在，则只是更改属性
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
//记录日志函数
int Log(string sLoggingStream, const char* lpszFormat, ...)
{

	//######################################################

	/*
	vsnprintf和snprintf是C语言printf家族函数的成员，相关函数列表如下：

	#include <stdio.h>
	int printf(const char *format, ...); //输出到标准输出
	int fprintf(FILE *stream, const char *format, ...); //输出到文件
	int sprintf(char *str, const char *format, ...); //输出到字符串str中
	int snprintf(char *str, size_t size, const char *format, ...); //按size大小输出到字符串str中

	以下函数功能与上面的一一对应相同，只是在函数调用时，把上面的...对应的一个个变量用va_list调用所替代。在函数调用前ap要通过va_start()宏来动态获取。
	#include <stdarg.h>
	int vprintf(const char *format, va_list ap);
	int vfprintf(FILE *stream, const char *format, va_list ap);
	int vsprintf(char *str, const char *format, va_list ap);
	int vsnprintf(char *str, size_t size, const char *format, va_list ap);
	可变参数列表va_list宏说明：
	void va_start(va_list ap, last);
	void va_end(va_list ap);
	va_start与va_end是成对被调用的，
	开始的时候被调用va_start，获得各输出变量地址
	结束的时候被调用va_end，释放相应的资源



	例，通过vsnprintf()实现snprintf()功能：（更多详情可参阅linux man va_start手册）
	#include <stdio.h>
	#include <stdarg.h>
	int my_snprintf(char *s, int size, const char *fmt, ...) //该自定义函数，与系统提供的snprintf()函数相同。
	{
	va_list ap;
	int n=0;
	va_start(ap, fmt); //获得可变参数列表
	n=vsnprintf (s, size, fmt, ap); //写入字符串s
	va_end(ap); //释放资源
	return n; //返回写入的字符个数
	}

	int main() {
	char str[1024];
	my_snprintf( str, sizeof(str), "%d,%d,%d,%d",5,6,7,8);
	printf("%s\n",str);
	return 0;
	}
	*/


	//########################################################

	//#typedef char *  va_list; 外部依赖
	va_list pArgs; //char* 变量
	int returnCode = 0;


	//#查找日志流是否存在 sLoggingStream
	// check if the logging stream exists
	TLoggingStreamsHash::iterator iPtr;
	// if the stream is not defined, return
	// #用find函数来定位数据出现位置，它返回一个迭代器，当数据出现时，它返回数据所在位置的迭代器，
	// #如果map中没有要查找的数据，它返回的迭代器等于end函数返回的迭代器
	if ((iPtr = lshLogStreams.find(sLoggingStream)) == lshLogStreams.end())
	{
		Log(WARNING_STREAM, "Attempt to log on undefined logging stream: " + sLoggingStream);
		return 0;
	}

	// get the current time
	//获取当前时间
	_int64 liTimestamp = GetCurrentAbsoluteTimestamp();

	// put it on the screen, if the stream is to be displayed
	//显示到屏幕上
	va_start(pArgs, lpszFormat);
	if (iPtr->second.bDisplayed)
	{
		// construct the string
		char lpszBuffer1[STRING_MAX];
		char lpszBuffer2[STRING_MAX];

		//函数原型int snprintf(char *str, size_t size, const char *format, ...)
		//将可变个参数(...)按照format格式化成字符串，然后将其复制到str中
		//#c_str()函数返回一个指向正规C字符串的指针, 内容与本string串相同
		_snprintf(lpszBuffer1, STRING_MAX - 1, "[%s@%s (%d)] ", sLoggingStream.c_str(), TimestampToString(liTimestamp).c_str(), GetSessionTimestamp(liTimestamp));
		_vsnprintf(lpszBuffer2, STRING_MAX - 1, lpszFormat, pArgs);
		//#把src所指字符串的前n个字符添加到dest结尾处，覆盖dest结尾处的'/0'，实现字符串连接。
		strncat(lpszBuffer1, lpszBuffer2, STRING_MAX - 2 - strlen(lpszBuffer1));
		strncat(lpszBuffer1, "\n", STRING_MAX - 1 - strlen(lpszBuffer1));
		// display it
		//显示函数
		SetConsoleTextAttribute(hStdOutput, (WORD)lshLogStreams[sLoggingStream].iColor);
		DWORD cWritten;
		//WriteFile函数是用来写数据到文件，ReadFile函数是从文件里读取数据出来。
		//但这两个函数不但可以读取写磁盘的文件，也可以接收和发送网络的数据，还有读写串口、USB、并口等设备的数据。
		WriteFile(hStdOutput, lpszBuffer1, lstrlen(lpszBuffer1), &cWritten, NULL);
	}

	// also send it to the file if logging is initialized
	// and the stream is to enabled for logging
	//如果日志记录已初始化, 并且启用日志记录，也会将其  -记录到文件
	if (fileLog && iPtr->second.bEnabled)
	{

		// then do the printf into the file
		//用法：	int fprintf (FILE* stream, const char*format, [argument])
		//参数：	FILE*stream：文件指针
		//		const char* format：输出格式
		//		[argument]：附加参数列表
		fprintf(fileLog, "[%s@%s (%d)] ", sLoggingStream.c_str(), TimestampToString(liTimestamp).c_str(), GetSessionTimestamp(liTimestamp));
		//功  能: 送格式化输出到一流中
		//用  法 : int vfprintf(FILE *stream, char *format, va_list param);
		//简  介：stream为文件流，当该参数为stderr时，打印到屏幕输出；后两个参数用法与printf相同。
		returnCode = vfprintf(fileLog, lpszFormat, pArgs);
		fprintf(fileLog, "\n");
		fflush(fileLog);
	}

	return(returnCode);
}

// D: another variant of the log function, just marshalls info
//    towards the basic one
// 函数重载 Log()
int Log(string sLoggingStream, string sMessage)
{
	return Log(sLoggingStream, sMessage.c_str());
}

//-----------------------------------------------------------------------------
// Logging initialization and closing functions
//-----------------------------------------------------------------------------
// D: logging initialization
// 日志初始化
void InitializeLogging(string sALogFolder, string sALogPrefix, string sALogFilename, string sLoggedStreamsList, string sDisplayedStreamsList, string sAExitOnFatalError)
{

	// Set the exit on fatal error flag                           
	bExitOnFatalError = (sAExitOnFatalError == "true");

	// Add the basic logging streams
	AddLoggingStream(WARNING_STREAM, "1001");
	AddLoggingStream(ERROR_STREAM, "1001");
	AddLoggingStream(FATALERROR_STREAM, "1001");

	// Add all the default streams, but don't enable them yet
	// 加载所有默认的log流 - disable
	vector<string> vsStreams = PartitionString(sAllLoggingStreams, ";");
	for (unsigned int i = 0; i < vsStreams.size(); i++)
	{
		string sName, sColorMap;
		SplitOnFirst(Trim(vsStreams[i]), ":", sName, sColorMap);
		AddLoggingStream(sName, sColorMap, false, false);
	}

	// Enable the streams from the description
	// enable通过参数配置的log流
	vector<string> vsEnabledStreams = PartitionString(sLoggedStreamsList, ";");
	for (unsigned int i = 0; i < vsEnabledStreams.size(); i++)
	{
		string sName, sColorMap;
		SplitOnFirst(Trim(vsEnabledStreams[i]), ":", sName, sColorMap);
		AddLoggingStream(sName, sColorMap, false, true);
	}

	// And display the ones that are in the displayed list
	// enable通过参数配置的display流
	vector<string> vsDisplayedStreams =	PartitionString(sDisplayedStreamsList, ";");
	for (unsigned int i = 0; i < vsDisplayedStreams.size(); i++)
	{
		string sName, sColorMap;
		SplitOnFirst(Trim(vsDisplayedStreams[i]), ":", sName, sColorMap);
		AddLoggingStream(sName, sColorMap, true, true);
	}


	// try to open the log file
	// 打开日志文件
	sLogFolder = sALogFolder + "\\";
	sLogFilename = sALogPrefix + sALogFilename;
	string sLogPath = sLogFolder + sLogFilename;
	fileLog = fopen(sLogPath.c_str(), "w");

	// if it's not possible in the location indicated, try in the current folder
	// 如果配置的log位置无法创建文件，尝试当前位置创建log
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