#include <labrat/robot/logger.hpp>

using namespace labrat;

// Hello world
//
// Minimal sample program.

int main(int argc, char **argv) {
  robot::Logger logger("main");
  logger.logInfo() << "Hello world!";

  return 0;
}
