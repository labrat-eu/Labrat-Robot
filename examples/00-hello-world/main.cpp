#include <labrat/lbot/logger.hpp>

// Hello world
//
// Minimal sample program.

int main(int argc, char **argv) {
  lbot::Logger logger("main");
  logger.logInfo() << "Hello world!";

  return 0;
}
