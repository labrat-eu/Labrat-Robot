#include <labrat/lbot/logger.hpp>

#include <gtest/gtest.h>

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  lbot::Logger::setLogLevel(lbot::Logger::Verbosity::debug);

  return RUN_ALL_TESTS();
}
