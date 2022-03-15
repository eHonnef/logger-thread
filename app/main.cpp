#include <LoggerThread/LoggerDaemon.cc>
#include <algorithm>
#include <iostream>

/*
 * Main program
 */
int main() {
  CLogger Logger;
  Logger.AddLogFile("teste1");

  Logger.Start();
  Logger.Detach();

  while (true) {
    std::string msg;
    fmt::print("Write something: ");
    std::cin >> msg;
    if (msg == "exit")
      break;
    Logger.WriteInfo(msg);
    fmt::print("Enqueued: {}\n", msg);
  }

  try {
    Logger.Join();
  } catch (const std::exception &e) {
    fmt::print("{}\n", e.what());
  }

  return 0;
}
