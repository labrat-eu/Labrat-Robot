#include <labrat/lbot/config.hpp>
#include <labrat/lbot/logger.hpp>

#include <cstdint>

// Configuration
//
// The central configuration enables simple reconfiguration of a project without having to recompile the code.
//
// In this example the central configuration class is showcased.

int main(int argc, char **argv) {
  lbot::Config::Ptr config = lbot::Config::get();
  config->load("06-config/config.yaml");

  lbot::Logger logger("main");

  config->setParameter("/test_param", 1);
  logger.logInfo() << "Read parameter '/test_param': " << config->getParameter("/test_param").get<int>();

  config->removeParameter("/test_param");
  logger.logInfo() << "Read parameter '/test_param': " << config->getParameterFallback("/test_param", 2).get<int>();

  config->setParameter("/test_param", 1);
  config->clear();
  logger.logInfo() << "Read parameter '/test_param': " << config->getParameterFallback("/test_param", 3).get<int>();

  return 0;
}
