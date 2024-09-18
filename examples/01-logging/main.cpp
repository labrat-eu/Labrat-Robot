#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/logger.hpp>

// Logging
//
// Logging information is crucial in order to get basic status information about your program.
// Debugging is also aided by good logging practices.
//
// This example showcases the logging system.

int main(int argc, char **argv)
{
  // First we create a logger object.
  lbot::Logger logger("main");

  // In order to write messages to stdout we use the created logger.
  logger.logInfo() << "Testing... " << 123;

  // We can also register multiple logger.
  lbot::Logger logger_a("log_a");
  lbot::Logger logger_b("log_b");
  logger_a.logInfo() << "This message comes from A.";
  logger_b.logInfo() << "This message comes from B.";

  // There are 5 verbosity levels.
  logger.logCritical() << "This is a critical message.";
  logger.logError() << "This is an error message.";
  logger.logWarning() << "This is a warning message.";
  logger.logInfo() << "This is an info message.";
  logger.logDebug() << "This is a debug message.";

  // We can specify which messages are printed to the console.
  lbot::Logger::setLogLevel(lbot::Logger::Verbosity::debug);
  logger.logDebug() << "Now debug messages are also printed.";

  // Exceptions are also printed through the logging system.
  try {
    throw lbot::Exception("Something went wrong.");
  } catch (lbot::Exception &e) {}

  return 0;
}
