#include "Core.h"
#include <ctime>
#include <Windows.h>


static cstring level_names[] = {
	"INFO ",
	"WARN ",
	"ERROR"
};

static Logger* empty_logger = new Logger;
Logger* Logger::global = empty_logger;


//=================================================================================================
cstring Logger::FormatString(Level level, cstring msg)
{
	time_t t = time(0);
	tm tm;
	localtime_s(&tm, &t);
	return Format("%02d:%02d:%02d %s - %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, level_names[level], msg);
}

void Logger::Set(Logger* logger)
{
	assert(logger);
	delete global;
	global = logger;
}


//=================================================================================================
FileLogger::FileLogger(cstring filename)
{
	assert(filename);
	out.open(filename);
}

FileLogger::~FileLogger()
{
	Log(L_INFO, "*** End of log");
}

void FileLogger::Log(Level level, cstring msg)
{
	out << FormatString(level, msg);
}

void FileLogger::Flush()
{
	out.flush();
}


//=================================================================================================
ConsoleLogger::ConsoleLogger()
{
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
}

void ConsoleLogger::Log(Level level, cstring msg)
{
	printf(FormatString(level, msg));
	fflush(stdout);
}


//=================================================================================================
MultiLogger::~MultiLogger()
{
	DeleteElements(loggers);
}

void MultiLogger::Log(Level level, cstring msg)
{
	for(Logger* logger : loggers)
		logger->Log(level, msg);
}

void MultiLogger::Flush()
{
	for(Logger* logger : loggers)
		logger->Flush();
}

void MultiLogger::Add(Logger* logger)
{
	assert(logger);
	loggers.push_back(logger);
}
