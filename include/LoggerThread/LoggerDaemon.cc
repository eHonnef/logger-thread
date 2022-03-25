#ifndef LOGGER_DAEMON_H
#define LOGGER_DAEMON_H
#ifdef LOGGER_DAEMON_H
#include <ThreadWrapper/Daemon.cc>
#include <cstring>
#include <filesystem>
#include <fmt/chrono.h>
#include <fstream>
#include <regex>
#include <string_view>
#endif

/*
 * Logger thread class.
 * Instantiate it and call Start;
 * Add a log file with AddLogFile;
 * Set the buffer size in c_nBufferSize;
 * Set the iterations to write to file in c_nMaxIterations;
 * Set the thread rate in c_nThreadRateMs;
 */
class CLogger : public CDaemon<std::string> {
private:
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
  };

private:
  static const int c_nMaxIterations = 50; // Max iterations before we write the buffer to file
  std::vector<CLogFile> m_lstFiles;       // Files managed by this thread
  std::filesystem::path m_PathLogDir;     // Formatted log directory path @see CreateLogDir

  // Enumerator to list the log levels.
  enum ELogLevel { llDebug, llNormal, llImportant, llWarning, llError, llCritical };

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

    // We create the folder
    m_PathLogDir.append(strTodayDirName);
    std::filesystem::create_directories(m_PathLogDir);
  }

  /*
   * It just post the log message in the thread.
   */
  void PostMessage(ELogLevel LogLevel, int nLogIndex, const std::string_view &strMessage) {
    SData Data(0, nLogIndex, FormatMessage(LogLevel, strMessage));
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
    std::string strLogLevel;
    switch (LogLevel) {
    case llDebug:
      strLogLevel = "[DEBUG]";
      break;
    case llImportant:
      strLogLevel = "[IMPORTANT]";
      break;
    case llWarning:
      strLogLevel = "[WARNING]";
      break;
    case llError:
      strLogLevel = "[ERROR]";
      break;
    case llCritical:
      strLogLevel = "[CRITICAL]";
      break;
    case llNormal:
      strLogLevel = "[INFO]";
      break;
    }

    // Formatting each message as below:
    // Date Time.Ms [Level] :: Message\n
    return fmt::format("{} {:<11} :: {}\n", GetTimeNow(), strLogLevel, strMessage);
  }

protected:
  /*
   * Process before the queue.
   * We increase the iterations without writing to file.
   */
  void ProcessPreQueue() override {
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
      m_lstFiles.at(Data.nMessageID).WriteOrBufferize(Data.Data);

    for (auto &elem : m_lstFiles)
      elem.WriteToFile("");
  }

  /*
   * Process the queue.
   */
  void Process(int nMessageID, const SData &Data) override {
    m_lstFiles.at(nMessageID).WriteOrBufferize(Data.Data);
  }

public:
  /*
   * Constructor.
   * @param strLogDir The root directory where the logs will be created.
   */
  CLogger(const std::string_view &strLogDir) : CDaemon() { CreateLogDir(strLogDir); }

  /*
   * Constructor.
   * It'll assume the default directory.
   */
  CLogger() : CDaemon() { CreateLogDir("./Logs/"); }

  /*
   * Add a log file to write.
   * @param strLogName The base log name, for example, "ProgramRun". And it'll add the today's date
   * becoming "ProgramRun_2022-03-14".
   * @return The log file index, save it to ask when writting the message using the Write functions.
   */
  int AddLogFile(const std::string_view &strLogName) {
    auto strFile = fmt::format("{}/{}_{:%Y-%m-%d}.log", m_PathLogDir.c_str(), strLogName,
                               fmt::localtime(std::time(nullptr)));

    m_lstFiles.emplace_back(strFile);
    return m_lstFiles.size() - 1;
  }

  /*
   * Write a simple info to file.
   * @param nLogIndex If no param is provided, we assume 0.
   * @param strMessage The message that will be written to file. No need to format, we will handle
   * it :)
   */
  void WriteInfo(const std::string_view &strMessage) { WriteInfo(0, strMessage); }
  void WriteInfo(int nLogIndex, const std::string_view &strMessage) {
    PostMessage(llNormal, nLogIndex, strMessage);
  }

  /*
   * Write an error info to file.
   * @param nLogIndex If no param is provided, we assume 0.
   * @param strMessage The message that will be written to file. No need to format, we will handle
   * it :)
   */
  void WriteError(const std::string_view &strMessage) { WriteError(0, strMessage); }
  void WriteError(int nLogIndex, const std::string_view &strMessage) {
    PostMessage(llError, nLogIndex, strMessage);
  }

  /*
   * Write an warning info to file.
   * @param nLogIndex If no param is provided, we assume 0.
   * @param strMessage The message that will be written to file. No need to format, we will handle
   * it :)
   */
  void WriteWarning(const std::string_view &strMessage) { WriteWarning(0, strMessage); }
  void WriteWarning(int nLogIndex, const std::string_view &strMessage) {
    PostMessage(llWarning, nLogIndex, strMessage);
  }

  /*
   * Write a debug info to file.
   * It'll only write if the program is built as debug.
   * @param nLogIndex If no param is provided, we assume 0.
   * @param strMessage The message that will be written to file. No need to format, we will handle
   * it :)
   */
  void WriteDebug(const std::string_view &strMessage) { WriteDebug(0, strMessage); }
  void WriteDebug(int nLogIndex, const std::string_view &strMessage) {
    PostMessage(llDebug, nLogIndex, strMessage);
  }

  /*
   * Write a critial info to file.
   * @param nLogIndex If no param is provided, we assume 0.
   * @param strMessage The message that will be written to file. No need to format, we will handle
   * it :)
   */
  void WriteCritical(const std::string_view &strMessage) { WriteCritical(0, strMessage); }
  void WriteCritical(int nLogIndex, const std::string_view &strMessage) {
    PostMessage(llCritical, nLogIndex, strMessage);
  }

  /*
   * Write an important info to file.
   * @param nLogIndex If no param is provided, we assume 0.
   * @param strMessage The message that will be written to file. No need to format, we will handle
   * it :)
   */
  void WriteImportant(const std::string_view &strMessage) { WriteCritical(0, strMessage); }
  void WriteImportant(int nLogIndex, const std::string_view &strMessage) {
    PostMessage(llImportant, nLogIndex, strMessage);
  }
};

#endif // LOGGER_DAEMON_H
