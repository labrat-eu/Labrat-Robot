#include <labrat/robot/logger.hpp>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  labrat::robot::Logger::setLogLevel(labrat::robot::Logger::Verbosity::debug);

  return RUN_ALL_TESTS();
}
