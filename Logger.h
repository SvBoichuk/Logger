#ifndef _LOGGER_H
#define _LOGGER_H

#include "SomeLibs.h"

const char g_cszDefaultFormat[] = "%Y-%m-%d";

enum LogType { LogType_Info, LogType_Warning, LogType_Error };

class Logger
{
private:
	FILE *m_pFileHandle;
	LogType m_eLogType;
	std::string m_LogFileName;
	mutable struct stat m_LogFileStat;
	bool m_IsFileOpen;
	std::mutex m_mtx;
private:
	std::string LogTypeToString();
	bool IsValidLogFileSize(std::size_t nMaxFileSize) const;
	bool CloseFile();
	bool OpenFile();
	bool CreateNewFile();
	void AddSuffixToName();
	void PrintLabel();
	void PrintFarewell();
public:

	void SetLogType(LogType eType = LogType::LogType_Info) noexcept;
	void Log(LogType ceLogType, const char* szcFormat, ...);
	bool Init() noexcept;
	bool IsOpen() const noexcept;
	std::string GetLastErrorAsString() const noexcept;

	Logger(const std::string & file_name);
	Logger(const Logger &) = delete;
	Logger& operator=(const Logger &) = delete;
	~Logger();
};

std::string getCurrentDateTime(const char* szcFormat = g_cszDefaultFormat);
const std::string generateLogFileName() noexcept;

		/*********************************************************/
/************************************MACROS*******************************/
extern Logger g_Log;

#define LOG_INIT() g_Log.Init();
#define LOG_INFO(xMessage) g_Log.Log((LogType_Info),(xMessage))
#define LOG_WARNING(xMessage) g_Log.Log((LogType_Warning),(xMessage))
#define LOG_ERROR(xMessage) g_Log.Log((LogType_Error),(xMessage))

#endif // !_LOGGER_H