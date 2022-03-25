#include <LoggerThread/LoggerDaemon.cc>
#include <algorithm>
#include <iostream>
#include <random>

void Write(CLogger &Logger, int nLevel, int nFile, const std::string &strMessage) {
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
 * For this test I set the c_nBufferSize to 60 and c_nMaxIterations to 5.
 */
int main() {
  CLogger Logger;
  Logger.AddLogFile("test0");
  Logger.AddLogFile("test1");
  Logger.AddLogFile("test2");

  Logger.Start();

  // Let's write less then a buffer
  Write(Logger, 2, 0, "1 aaaa");
  Write(Logger, 0, 1, "1 bbbb");
  Write(Logger, 2, 2, "1 cccc");

  // Now let's write a lot on file 1
  Write(Logger, 3, 1, "2 xxxxxxx");
  Write(Logger, 3, 1, "3 yyyyy");
  Write(Logger, 3, 1, "4 zzzzz");
  Write(Logger, 3, 1, "5 zyzyzyzy");
  Write(Logger, 3, 1, "6 xyxyxyxyx");
  Write(Logger, 3, 1, "7 pypypypypy");

  // Now, before exiting, we should have wrote in the log files
  // because it reach the c_nMaxIterations on ProcessPreQueue.
  fmt::print("Press enter to continue");
  std::cin.ignore();

  // Let's write less then a buffer
  // So when we call stop, these lines should be written.
  Write(Logger, 4, 0, "2 ddd");
  Write(Logger, 5, 1, "8 eee");
  Write(Logger, 1, 2, "2 fff");

  // Stoping and exiting logger
  try {
    Logger.Stop();
  } catch (const std::exception &e) {
    fmt::print("{}\n", e.what());
  }

  return 0;
}
