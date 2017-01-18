#include "Logger.h"

Logger g_Log(generateLogFileName());

const char g_cszAppend[] = "a+";
const int g_ciBytesFileSize = 1024;
const int g_ciDefaultBufLen = 128;

bool Logger::IsValidLogFileSize(std::size_t nMaxFileSize) const
{
	stat(m_LogFileName.c_str(), &m_LogFileStat);
	return (m_LogFileStat.st_size < nMaxFileSize ? true : false);
}

std::string Logger::LogTypeToString()
{
	std::string log_str;
	switch (m_eLogType)
	{
	case LogType::LogType_Info:
		log_str = "INFO:";
		break;
	case LogType::LogType_Warning:
		log_str = "WARNING:";
		break;
	case LogType::LogType_Error:
		log_str = "ERROR:";
		break;
	default:
		log_str = "INFO:";
		break;
	}

	return log_str;
}

bool Logger::CloseFile()
{
	bool isClose = false;
	if (m_IsFileOpen) 
	{		
		if (fclose(m_pFileHandle) == 0) 
		{
			m_IsFileOpen = false;
			isClose = true;
		}
	}

	return isClose;
}

bool Logger::OpenFile()
{
	int errCode = fopen_s(&m_pFileHandle, m_LogFileName.c_str(), g_cszAppend);
	if (m_pFileHandle != NULL)
	{
		m_IsFileOpen = true;
		PrintLabel();
		return true;
	}
	else
	{
		m_IsFileOpen = false;
		return false;
	}
}

bool Logger::CreateNewFile()
{
	if (!CloseFile())
	{
		printf_s("Can't close file! %s", GetLastErrorAsString().c_str());
		return false;
	}
	else {
		AddSuffixToName();
		if (OpenFile())
		{
			return true;
		}
		else 
		{
			printf_s("Can't open file! %s", GetLastErrorAsString().c_str());
			return false;
		}
	}
}

void Logger::AddSuffixToName()
{
	std::size_t fileNameLen = m_LogFileName.size();
	if (isdigit(m_LogFileName[fileNameLen - 1]))
		m_LogFileName[fileNameLen - 1] = (++(m_LogFileName[fileNameLen - 1]));
	else m_LogFileName.insert(m_LogFileName.end(), '1');
}

void Logger::PrintLabel()
{
	fprintf_s(m_pFileHandle, "Application log file\nLog start at: %s\n", getCurrentDateTime().c_str());
}

Logger::Logger(const std::string & csFileName) :
	m_LogFileName(csFileName),
	m_eLogType(LogType::LogType_Info)
{
}

bool Logger::Init() noexcept
{
	if (OpenFile())
	{
		return true;
	}
	else
	{
		printf_s("Can't open file! %s", GetLastErrorAsString().c_str());
		return false;
	}
}

bool Logger::IsOpen() const noexcept
{
	return m_IsFileOpen;
}

void Logger::SetLogType(const LogType ceType) noexcept
{
	m_eLogType = ceType;
}

void Logger::Log(const LogType ceLogType, const char * szcFormat, ...)
{
	if (!m_IsFileOpen) 
	{
		return;
	}

	std::lock_guard<std::mutex> lock_mutex(m_mtx);
	SetLogType(ceLogType);

	//create new file
	if (!IsValidLogFileSize(g_ciBytesFileSize)) {
		CreateNewFile();
	}

	va_list args;
	va_start(args, szcFormat);

	int nLength = _vscprintf(szcFormat, args) + 1;
	char* sMessage = new char[nLength];

	vsprintf_s(sMessage, nLength, szcFormat, args);
	//format string
	std::string text = getCurrentDateTime("%Y-%m-%d.%X") + ":\t"
		+ LogTypeToString() + "\t" + sMessage + "\n";
	delete[] sMessage;

	fprintf_s(m_pFileHandle, text.c_str());

	va_end(args);
}

std::string Logger::GetLastErrorAsString() const noexcept
{
#ifdef WIN32

	DWORD errorMessageId = ::GetLastError();
	if (errorMessageId == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);
	return message;
#else //LINUX
	return std::string(strerror(errno));
#endif // WIN32
}

void Logger::PrintFarewell()
{
	fprintf_s(m_pFileHandle, "LOG END.");
}

Logger::~Logger()
{
	if (m_IsFileOpen)
	{
		PrintFarewell();
		if (!CloseFile())
		{
			printf_s("Can't close file! %s", GetLastErrorAsString().c_str());
		}
	}
}

const std::string generateLogFileName() noexcept
{
	std::string executableFileName;
	executableFileName.resize(g_ciDefaultBufLen);

#ifdef WIN32
	GetModuleFileNameA(NULL, &executableFileName[0],
		static_cast<unsigned int>(executableFileName.size()));
	executableFileName = PathFindFileNameA(&executableFileName[0]);
	PathRemoveExtensionA(&executableFileName[0]);
#else //LINUX
	char path[g_ciDefaultBufLen];
	char id[g_ciDefaultBufLen];
	sprintf(id, "/proc/%d/exe", getpid());
	readlink(id, path, g_ciDefaultBufLen-1);
	path[g_ciDefaultBufLen-1] = '\0';
	executableFileName = path;
#endif //WIN32

	char fileName[g_ciDefaultBufLen] = {'\0'};
	sprintf_s(fileName, sizeof(fileName), "%s_%s.log", getCurrentDateTime().c_str(), executableFileName.c_str());
	return std::string(fileName);
}

std::string getCurrentDateTime(const char* szcFormat)
{
	time_t     now = time(NULL);
	struct tm  tstruct;
	localtime_s(&tstruct, &now);
	char       buf[128] = {'\0'};
	strftime(buf, sizeof(buf), szcFormat, &tstruct);

	return std::string(buf);
}
