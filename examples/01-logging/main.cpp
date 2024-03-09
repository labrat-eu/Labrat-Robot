#include <labrat/robot/logger.hpp>
#include <labrat/robot/exception.hpp>

using namespace labrat;

// Logging
//
// Logging information is crucial in order to get basic status information about your program.
// Debugging is also aided by good logging practices.
//
// This example showcases the logging system.

int main(int argc, char **argv) {
  // First we create a logger object.
  robot::Logger logger("main");

  // In order to write messages to stdout we use the created logger.
  logger.logInfo() << "Testing... " << 123;

  // We can also register multiple logger.
  robot::Logger logger_a("log_a");
  robot::Logger logger_b("log_b");
  logger_a.logInfo() << "This message comes from A.";
  logger_b.logInfo() << "This message comes from B.";

  // There are 5 verbosity levels.
  logger.logCritical() << "This is a critical message.";
  logger.logError() << "This is an error message.";
  logger.logWarning() << "This is a warning message.";
  logger.logInfo() << "This is an info message.";
  logger.logDebug() << "This is a debug message.";

  // We can specify which messages are printed to the console.
  robot::Logger::setLogLevel(robot::Logger::Verbosity::debug);
  logger.logDebug() << "Now debug messages are also printed.";

  // Exceptions are also printed through the logging system.
  try {
    throw robot::Exception("Something went wrong.");
  } catch (robot::Exception &e) {}

  return 0;
}
