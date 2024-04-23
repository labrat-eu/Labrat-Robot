#include <labrat/lbot/config.hpp>
#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/plugins/foxglove-ws/server.hpp>
#include <labrat/lbot/utils/signal.hpp>

#include <cstdint>

int main(int argc, char **argv) {
  lbot::Logger logger("main");
  lbot::Manager::Ptr manager = lbot::Manager::get();

  lbot::Config::Ptr config = lbot::Config::get();
  config->load("06-config/config.yaml");

  config->setParameter("/test_param", 1L);
  logger.logInfo() << "Read parameter '/test_param': " << config->getParameter("/test_param").get<int64_t>();

  config->removeParameter("/test_param");
  logger.logInfo() << "Read parameter '/test_param': " << config->getParameterFallback("/test_param", 2L).get<int64_t>();

  config->setParameter("/test_param", 1L);
  config->clear();
  logger.logInfo() << "Read parameter '/test_param': " << config->getParameterFallback("/test_param", 3L).get<int64_t>();

  return 0;
}
