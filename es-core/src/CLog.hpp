/*
Output Log File: /userdata/system/configs/emulationstation/es_log.txt
*/

#pragma once
#ifndef ES_CORE_LOG_H
#define ES_CORE_LOG_H

#include <sstream>
#include <exception>
	
#define LOG(level) if(!Log::enabled() || level > Log::getReportingLevel()) ; else Log().get(level)

#define TRYCATCH(m, x) { try { x; } \
catch (const std::exception& e) { LOG(LogError) << m << " Exception " << e.what(); Log::flush(); throw e; } \
catch (...) { LOG(LogError) << m << " Unknown Exception occured"; Log::flush(); throw; } }

enum LogLevel { LogError, LogWarning, LogInfo, LogDebug };

class Log
{
public:
	~Log();
	std::ostringstream& get(LogLevel level = LogInfo);

	static inline LogLevel& getReportingLevel() { return mReportingLevel; }
	static inline bool enabled() { return mFile != NULL; }

	static void init();
	static void flush();
	static void close();
	
private:
	static LogLevel     mReportingLevel;
	static bool         mDirty;
	static FILE*        mFile;

protected:
	std::ostringstream  mStream;
	LogLevel		    mMessageLevel;
};

class StopWatch
{
public:
	StopWatch(const std::string& elapsedMillisecondsMessage, LogLevel level = LogDebug);
	~StopWatch();

private:
	std::string mMessage;
	LogLevel    mLevel;
	int         mStartTicks;
};

#endif // ES_CORE_LOG_H

#include "utils/FileSystemUtil.h"
#include "platform.h"
#include <iostream>
#include <mutex>
#include "Settings.h"
#include <iomanip> 
#include <SDL_timer.h>
#include "Paths.h"

#if WIN32
#include <Windows.h>
#endif

static std::mutex mLogLock;

LogLevel Log::mReportingLevel = (LogLevel) -1;
bool     Log::mDirty          = false;
FILE*    Log::mFile           = NULL;

void Log::init()
{		
	mReportingLevel = (LogLevel)-1;

	close();

	LogLevel lvl = LogInfo;

	if (Settings::getInstance()->getBool("Debug"))
		lvl = LogDebug;
	else
	{
		auto level = Settings::getInstance()->getString("LogLevel");
		if (level == "debug")
			lvl = LogDebug;
		else if (level == "information")
			lvl = LogInfo;
		else if (level == "warning")
			lvl = LogWarning;
		else if (level == "error" || level.empty())
			lvl = LogError;
		else
			lvl = (LogLevel) -1; // Disabled
	}

	auto logPath = Paths::getUserEmulationStationPath() + "/es_log.txt";
	auto bakPath = logPath + ".bak";

	if ((int)lvl < 0) 
	{		
		Utils::FileSystem::removeFile(logPath);
		return;
	}
	
	Utils::FileSystem::removeFile(bakPath);
	Utils::FileSystem::renameFile(logPath, bakPath);

	bool locked = mLogLock.try_lock();

	mFile = fopen(logPath.c_str(), "w");
	mDirty = false;
	mReportingLevel = lvl;

	if (locked)
		mLogLock.unlock();
}

std::ostringstream& Log::get(LogLevel level)
{
	time_t t = time(nullptr);
	mStream << std::put_time(localtime(&t), "%F %T\t");

	switch (level)
	{
	case LogError:
		mStream << "ERROR\t";
		break;
	case LogWarning:
		mStream << "WARNING\t";
		break;
	case LogDebug:
		mStream << "DEBUG\t";
		break;
	default:
		mStream << "INFO\t";
		break;
	}

	mMessageLevel = level;

	return mStream;
}

void Log::flush()
{
	if (!mDirty)
		return;

	if (mLogLock.try_lock())
	{
		if (mFile != nullptr)
			fflush(mFile);

		mDirty = false;
		mLogLock.unlock();
	}
}

void Log::close()
{
	bool locked = mLogLock.try_lock();

	if (mFile != NULL)
	{
		fflush(mFile);
		fclose(mFile);
		mFile = NULL;
	}

	mDirty = false;
	
	if (locked)
		mLogLock.unlock();
}

Log::~Log()
{
	bool locked = mLogLock.try_lock();

	if (mFile != NULL)
	{
		mStream << std::endl;
		fprintf(mFile, "%s", mStream.str().c_str());
		mDirty = true;
	}
	
	// If it's an error, also print to console
	// print all messages if using --debug
	if (mMessageLevel == LogError || mReportingLevel >= LogDebug)
	{
#if WIN32
		OutputDebugStringA(mStream.str().c_str());
#else
		fprintf(stderr, "%s", mStream.str().c_str());
#endif
	}

	if (locked)
		mLogLock.unlock();
}

StopWatch::StopWatch(const std::string& elapsedMillisecondsMessage, LogLevel level)
{ 
	mMessage = elapsedMillisecondsMessage; 
	mLevel = level;
	mStartTicks = SDL_GetTicks();
}

StopWatch::~StopWatch()
{
	int elapsed = SDL_GetTicks() - mStartTicks;
	LOG(mLevel) << mMessage << " " << elapsed << "ms";
}
