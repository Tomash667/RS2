#pragma once

#include <fstream>

//-----------------------------------------------------------------------------
struct Logger
{
	enum Level
	{
		L_INFO,
		L_WARN,
		L_ERROR
	};

	virtual ~Logger() {}
	virtual void Log(Level level, cstring msg) {}
	virtual void Flush() {}

	static Logger* Get() { return global; }
	static void Set(Logger* logger);

protected:
	cstring FormatString(Level level, cstring msg);

private:
	static Logger* global;
};

//-----------------------------------------------------------------------------
struct FileLogger : Logger
{
	explicit FileLogger(cstring filename);
	~FileLogger();
	void Log(Level level, cstring msg) override;
	void Flush() override;

private:
	std::ofstream out;
	string path;
};

//-----------------------------------------------------------------------------
struct ConsoleLogger : Logger
{
	ConsoleLogger();
	void Log(Level level, cstring msg) override;
};

//-----------------------------------------------------------------------------
struct MultiLogger : Logger
{
	~MultiLogger();
	void Log(Level level, cstring msg) override;
	void Flush() override;
	void Add(Logger* logger);

private:
	vector<Logger*> loggers;
};

//-----------------------------------------------------------------------------
inline void Info(cstring msg)
{
	Logger::Get()->Log(Logger::L_INFO, msg);
}
template<typename... Args>
inline void Info(cstring msg, const Args&... args)
{
	Logger::Get()->Log(Logger::L_INFO, Format(msg, args...));
}

inline void Warn(cstring msg)
{
	Logger::Get()->Log(Logger::L_WARN, msg);
}
template<typename... Args>
inline void Warn(cstring msg, const Args&... args)
{
	Logger::Get()->Log(Logger::L_WARN, Format(msg, args...));
}

inline void Error(cstring msg)
{
	Logger::Get()->Log(Logger::L_ERROR, msg);
}
template<typename... Args>
inline void Error(cstring msg, const Args&... args)
{
	Logger::Get()->Log(Logger::L_ERROR, Format(msg, args...));
}
