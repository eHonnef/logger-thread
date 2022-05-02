#ifndef LOGGER_DAEMON_H
#define LOGGER_DAEMON_H
#ifdef LOGGER_DAEMON_H
#include <ThreadWrapper/Daemon.cc>
#include <cstring>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fstream>
#include <regex>
#include <string_view>
#endif

namespace NLogger {

// Enumerator to list the log levels.
enum ELogLevel {
  llTrace,
  llDebug,
  llInfo,
  llImportant,
  llWarning,
  llError,
  llCritical,
  llFatal,
  llNone
};

static std::string LogLevelToString(ELogLevel LogLevel) {
  switch (LogLevel) {
  case llTrace:
    return "TRACE";
  case llDebug:
    return "DEBUG";
  case llInfo:
    return "INFO";
  case llImportant:
    return "IMPORTANT";
  case llWarning:
    return "WARNING";
  case llError:
    return "ERROR";
  case llCritical:
    return "CRITICAL";
  case llFatal:
    return "FATAL";
  case llNone:
    return "NONE";
  }
  return "";
}

struct SLogData {
  std::string strMessage = "";
  ELogLevel LogLevel = llNone;

  SLogData(const std::string_view &message, ELogLevel LL) : strMessage(message), LogLevel(LL) {}
  SLogData() = default;
};

/*
 * Logger thread class.
 * Instantiate it and call Start;
 * Add a log file with AddLogFile;
 * Set the buffer size in c_nBufferSize;
 * Set the iterations to write to file in c_nMaxIterations;
 * Set the thread rate in c_nThreadRateMs;
 */
class CLogger : public CDaemon<SLogData> {
private:
  /*
   * Private struct for settings.
   */
  struct SLogSettings {
    // Log detail level, only log the logs that is bigger or equal than the set one.
    ELogLevel LogLevel = llDebug;
    std::string LogFolder = "./Logs/";  // The log folder location
    bool SelfLog = false;               // Log this library info? (obs.: Only in console).
    bool LogInFile = true;              // Log in the file?
    bool ConsoleLog = false;            // Log in the console?
    bool RemoveLogFolderOnInit = false; // Remove the log folder on init?

    fmt::text_style Color(ELogLevel LL) {
      switch (LL) {
      case llTrace:
        return fmt::fg(fmt::color::light_gray);
      case llDebug:
        return fmt::emphasis::bold | fmt::fg(fmt::color::dark_cyan);
      case llInfo:
        return fmt::fg(fmt::color::white);
      case llImportant:
        return fmt::fg(fmt::color::orange);
      case llWarning:
        return fmt::emphasis::bold | fmt::fg(fmt::color::yellow);
      case llError:
        return fmt::fg(fmt::color::red);
      case llCritical:
        return fmt::emphasis::bold | fmt::bg(fmt::color::dark_red) | fmt::fg(fmt::color::white);
      case llFatal:
        return fmt::emphasis::bold | fmt::bg(fmt::color::red) | fmt::fg(fmt::color::white);
      case llNone:
        return fmt::text_style();
      }
      return fmt::text_style();
    }
  }; // end struct

  /*
   * Private class that represents a log file with its own buffer.
   */
  class CLogFile {
  private:
    // Buffer size in bytes, we subtract 1 because strings need to be null terminated
    static const size_t c_nBufferSize = 4096;
    int m_nWOWrite = 0;               // Number of iterations without writting the buffer into file.
    char m_Buffer[c_nBufferSize];     // This file buffer
    size_t m_nBufferUsage;            // Buffer usage
    std::filesystem::path m_PathFile; // This file path

    /*
     * Clears the buffer, writting null character
     */
    void ClearBuffer() {
      std::fill_n(m_Buffer, c_nBufferSize, '\0');
      m_nBufferUsage = 0;
      m_nWOWrite = 0;
    }

  public:
    /*
     * Constructor
     */
    CLogFile(const std::string_view &strFilePath) {
      m_PathFile = std::filesystem::path(strFilePath);
      ClearBuffer();
      // Creating file
      std::ofstream{m_PathFile};
    }

    /*
     * Write to file or bufferize strMessage.
     */
    void WriteOrBufferize(const std::string_view &strMessage) {
      size_t nNewBufferSize = m_nBufferUsage + strMessage.size();

      // Strings should be null terminated (\0), so we reserve the last char
      if (nNewBufferSize < (c_nBufferSize - 1)) {
        // Bufferize
        std::memcpy(&m_Buffer[m_nBufferUsage == 0 ? 0 : m_nBufferUsage], strMessage.data(),
                    strMessage.size());
        m_nBufferUsage += strMessage.size();
      } else
        // Write
        WriteToFile(strMessage);
    }

    /*
     * Unconditionally write to file.
     * @param strMessage Message to be written.
     */
    void WriteToFile(const std::string_view &strMessage) {
      // Write
      std::ofstream File;
      File.open(m_PathFile, std::ios_base::app); // Append

      File << m_Buffer;
      File << strMessage.data();

      // Clear
      ClearBuffer();
    }

    /*
     * Set m_nWOWrite;
     * @return The number of iterations without writting to file.
     */
    int &IterationsWOWrite() { return m_nWOWrite; }

    /*
     * Get m_nWOWrite;
     * @param nNumber The number of iterations without writting to file.
     */
    const int &IterationsWOWrite() const { return m_nWOWrite; }
  }; // end class

private:
  static const int c_nMaxIterations = 50; // Max iterations before we write the buffer to file
  std::vector<CLogFile> m_lstFiles;       // Files managed by this thread
  std::filesystem::path m_PathLogDir;     // Formatted log directory path @see CreateLogDir
  SLogSettings m_LogSettings;             // Log settings

  template <typename... Args> void SelfLog(fmt::string_view strFormat, const Args &...args) {
    if (m_LogSettings.SelfLog)
      fmt::print("[Logger] {}\n",
                 fmt::vformat(strFormat, fmt::format_args(fmt::make_format_args(args...))));
  }

  /*
   * I assume there will be a base log dir like: /project/Logs.
   */
  void CreateLogDir(const std::string_view &strLogDir) {
    // No param for the base log directory, we assume the local log dir
    m_PathLogDir.assign(strLogDir);

    // Get the current date and time
    std::string strTodayDirName =
        fmt::format("Logs_{:%Y-%m-%d}", fmt::localtime(std::time(nullptr)));

    // Check if there was already a directory
    int nCount = 0;
    if (std::filesystem::exists(m_PathLogDir))
      for (auto const &itDir : std::filesystem::directory_iterator{m_PathLogDir})
        if (itDir.path().filename().string().starts_with(strTodayDirName))
          nCount += 1;

    // We want to split the program run
    if (nCount > 0)
      strTodayDirName += "_" + std::to_string(nCount);

    SelfLog("Created folder: {}", strTodayDirName);

    // We create the folder
    m_PathLogDir.append(strTodayDirName);
    std::filesystem::create_directories(m_PathLogDir);
  }

  /*
   * It just post the log message in the thread.
   */
  template <typename... Args>
  void PostMessage(int nLogIndex, ELogLevel LogLevel, fmt::string_view strFormat,
                   const Args &...args) {
    if (LogLevel < m_LogSettings.LogLevel)
      return; // We don't post the message.

    SData Data(0, nLogIndex,
               SLogData(FormatMessage(LogLevel,
                                      fmt::vformat(strFormat, fmt::format_args(
                                                                  fmt::make_format_args(args...)))),
                        LogLevel));
    SafeAddMessage(Data);
  }

  /*
   * Get the current date and time with milliseconds of precision.
   * https://stackoverflow.com/q/63829143
   * @return Return the formatted string with today's date with milliseconds.
   */
  std::string GetTimeNow() {
    using namespace std::chrono;
    std::chrono::sys_time<nanoseconds> now = (std::chrono::system_clock::now());
    auto round_now = std::chrono::round<milliseconds>(now);
    int64_t ms = (round_now.time_since_epoch() % seconds(1)).count();
    return fmt::format("{:%Y-%m-%d %H:%M:%S}.{:04}", now, ms);
  }

  /*
   * Add a timestamp and format the message accordingly.
   * @return Returns the formatted message with a timestamp and the log level.
   */
  std::string FormatMessage(ELogLevel LogLevel, const std::string_view &strMessage) {
    // Formatting each message as below:
    // Date Time.Ms [Level] :: Message\n
    return fmt::format("{} {:<11} :: {}\n", GetTimeNow(),
                       fmt::format("[{}]", LogLevelToString(LogLevel)), strMessage);
  }

protected:
  /*
   * Process before the queue.
   * We increase the iterations without writing to file.
   */
  void ProcessPreQueue() override {
    if (!m_LogSettings.LogInFile)
      return;

    for (auto &elem : m_lstFiles) {
      elem.IterationsWOWrite() += 1;
      if (elem.IterationsWOWrite() >= c_nMaxIterations)
        elem.WriteToFile("");
    }
  }

  /*
   * Dequeue the remaining messages and Unconditionally write to file.
   */
  void ProcessThreadEpilogue() override {
    SData Data;
    while (TryDequeue(Data))
      Process(Data.nMessageID, Data);

    if (m_LogSettings.LogInFile)
      for (auto &elem : m_lstFiles)
        elem.WriteToFile("");
  }

  /*
   * Process the queue.
   */
  void Process(int nMessageID, const SData &Data) override {
    if (m_LogSettings.LogInFile)
      m_lstFiles.at(nMessageID).WriteOrBufferize(Data.Data.strMessage);

    if (m_LogSettings.ConsoleLog)
      fmt::print(m_LogSettings.Color(Data.Data.LogLevel), "[File: {}]{}", nMessageID,
                 Data.Data.strMessage);
  }

public:
  /*
   * Constructor.
   */
  CLogger() = default;

  /*
   * Return the settings object.
   * I'll recommend to change the settings before calling Start.
   */
  SLogSettings &Settings() { return m_LogSettings; }

  /*
   * Call this before start and before AddLogFile.
   */
  void Init() {
    SelfLog("Settings: LogFolder={}; LogLevel={}; SelfLog={}; LogInFile={}; ConsoleLog={}; "
            "RemoveLogFolderOnInit={}",
            m_LogSettings.LogFolder, LogLevelToString(m_LogSettings.LogLevel),
            m_LogSettings.SelfLog, m_LogSettings.LogInFile, m_LogSettings.ConsoleLog,
            m_LogSettings.RemoveLogFolderOnInit);

    if (m_LogSettings.RemoveLogFolderOnInit) {
      SelfLog("Removing folder: {}", m_LogSettings.LogFolder);
      std::filesystem::remove_all(m_LogSettings.LogFolder);
    }

    if (m_LogSettings.LogInFile)
      CreateLogDir(m_LogSettings.LogFolder);
  }

  /*
   * Add a log file to write.
   * @param strLogName The base log name, for example, "ProgramRun". And it'll add the today's date
   * becoming "ProgramRun_2022-03-14".
   * @return The log file index, save it to ask when writting the message using the Write functions.
   */
  int AddLogFile(const std::string_view &strLogName) {
    auto strFile = fmt::format("{}/{}_{:%Y-%m-%d}.log", m_PathLogDir.c_str(), strLogName,
                               fmt::localtime(std::time(nullptr)));

    SelfLog("File added: {}", strFile);
    m_lstFiles.emplace_back(strFile);
    return m_lstFiles.size() - 1;
  }

  /*
   * Write trace info to file.
   */
  template <typename... Args> void WriteTrace(fmt::string_view strFormat, const Args &...args) {
    PostMessage(0, llTrace, strFormat, args...);
  }
  template <typename... Args>
  void WriteTrace(int nLogIndex, fmt::string_view strFormat, const Args &...args) {
    PostMessage(nLogIndex, llTrace, strFormat, args...);
  }

  /*
   * Write a debug info to file.
   */
  template <typename... Args> void WriteDebug(fmt::string_view strFormat, const Args &...args) {
    PostMessage(0, llDebug, strFormat, args...);
  }
  template <typename... Args>
  void WriteDebug(int nLogIndex, fmt::string_view strFormat, const Args &...args) {
    PostMessage(nLogIndex, llDebug, strFormat, args...);
  }

  /*
   * Write a simple info to file.
   */
  template <typename... Args> void WriteInfo(fmt::string_view strFormat, const Args &...args) {
    PostMessage(0, llInfo, strFormat, args...);
  }
  template <typename... Args>
  void WriteInfo(int nLogIndex, fmt::string_view strFormat, const Args &...args) {
    PostMessage(nLogIndex, llInfo, strFormat, args...);
  }

  /*
   * Write an important info to file.
   */
  template <typename... Args> void WriteImportant(fmt::string_view strFormat, const Args &...args) {
    PostMessage(0, llImportant, strFormat, args...);
  }
  template <typename... Args>
  void WriteImportant(int nLogIndex, fmt::string_view strFormat, const Args &...args) {
    PostMessage(nLogIndex, llImportant, strFormat, args...);
  }

  /*
   * Write an warning info to file.
   */
  template <typename... Args> void WriteWarning(fmt::string_view strFormat, const Args &...args) {
    PostMessage(0, llWarning, strFormat, args...);
  }
  template <typename... Args>
  void WriteWarning(int nLogIndex, fmt::string_view strFormat, const Args &...args) {
    PostMessage(nLogIndex, llWarning, strFormat, args...);
  }

  /*
   * Write an error info to file.
   */
  template <typename... Args> void WriteError(fmt::string_view strFormat, const Args &...args) {
    PostMessage(0, llError, strFormat, args...);
  }
  template <typename... Args>
  void WriteError(int nLogIndex, fmt::string_view strFormat, const Args &...args) {
    PostMessage(nLogIndex, llError, strFormat, args...);
  }

  /*
   * Write a critial info to file.
   */
  template <typename... Args> void WriteCritical(fmt::string_view strFormat, const Args &...args) {
    PostMessage(0, llCritical, strFormat, args...);
  }
  template <typename... Args>
  void WriteCritical(int nLogIndex, fmt::string_view strFormat, const Args &...args) {
    PostMessage(nLogIndex, llCritical, strFormat, args...);
  }

  /*
   * Write a fatal info to file.
   */
  template <typename... Args> void WriteFatal(fmt::string_view strFormat, const Args &...args) {
    PostMessage(0, llFatal, strFormat, args...);
  }
  template <typename... Args>
  void WriteFatal(int nLogIndex, fmt::string_view strFormat, const Args &...args) {
    PostMessage(nLogIndex, llFatal, strFormat, args...);
  }
};

} // end namespace NLogger

#endif // LOGGER_DAEMON_H
