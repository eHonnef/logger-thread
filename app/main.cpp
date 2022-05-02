#include <LoggerThread/LoggerDaemon.cc>
#include <algorithm>
#include <iostream>
#include <random>

void Write(NLogger::CLogger &Logger, int nLevel, int nFile, const std::string &strMessage) {
  switch (nLevel) {
  case 0:
    Logger.WriteDebug(nFile, strMessage);
    break;
  case 1:
    Logger.WriteInfo(nFile, strMessage);
    break;
  case 2:
    Logger.WriteImportant(nFile, strMessage);
    break;
  case 3:
    Logger.WriteWarning(nFile, strMessage);
    break;
  case 4:
    Logger.WriteError(nFile, strMessage);
    break;
  default:
    Logger.WriteCritical(nFile, strMessage);
    break;
  }
}

/*
 * Main program.
 * For this test I set the c_nBufferSize to 150 and c_nMaxIterations to 5.
 */
int main() {
  NLogger::CLogger Logger;

  // Settings
  Logger.Settings().ConsoleLog = true;
  Logger.Settings().LogInFile = true;
  Logger.Settings().RemoveLogFolderOnInit = true;

  // Make the necessary initializations
  Logger.Init();

  // Add files
  Logger.AddLogFile("test0");
  Logger.AddLogFile("test1");
  Logger.AddLogFile("test2");

  // Start the thread
  Logger.Start();

  // Let's write less then a buffer
  Write(Logger, 2, 0, "1 A message");
  Write(Logger, 0, 1, "1 Another message");
  Write(Logger, 2, 2, "1 This is also a message");

  // Now let's write a lot on file 1
  Write(Logger, 0, 1, "2 A debug message");
  Write(Logger, 1, 1, "3 An Info message, this is the common (or base level)");
  Write(Logger, 2, 1, "4 An important info, to highlight a \"common\" info");
  Write(Logger, 3, 1, "5 A warning message, something to be aware that can lead to a problem");
  Write(Logger, 4, 1, "6 An error message, something went wrong that shouldn't, for example, a segmentation fault?");
  Write(Logger, 5, 1, "7 A critial error message, should we do a rollback or call the fire department?");
  Logger.WriteFatal(2, "2 A fatal error message, well, probably the rebellion has begun");

  // Now, before exiting, we should have wrote in the log files
  // because it reach the c_nMaxIterations on ProcessPreQueue.
  fmt::print("Press enter to continue");
  std::cin.ignore();

  // Let's write less then a buffer
  // So when we call stop, these lines should be written.
  Write(Logger, 4, 0, "2 You just pressed enter");
  Write(Logger, 5, 1, "8 Oh!! Hello there :)");
  Write(Logger, 1, 2, "3 Hello world!!!");
  Logger.WriteInfo("Also you can directly format as you would do in {}: Let's do for the number {}", "python", 7);

  // Log that will not be shown or written
  Logger.WriteTrace("This will not be shown");

  // Stoping and exiting logger
  try {
    Logger.Stop();
  } catch (const std::exception &e) {
    fmt::print("{}\n", e.what());
  }

  return 0;
}
