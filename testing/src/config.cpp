#include <labrat/lbot/config.hpp>

#include <gtest/gtest.h>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

class ConfigTest : public LbotTest {};

TEST_F(ConfigTest, parse) {
  lbot::Config::Ptr config = lbot::Config::get();

  ASSERT_NO_THROW(config->load("../../data/test_config.yaml"));

  EXPECT_EQ(config->getParameter("/bool").get<bool>(), true);
  EXPECT_EQ(config->getParameter("/int").get<i64>(), 1);
  EXPECT_EQ(config->getParameter("/double").get<double>(), 1.0);
  EXPECT_EQ(config->getParameter("/string").get<std::string>(), std::string("test"));

  const std::vector<lbot::ConfigValue> &sequence = config->getParameter("/sequence").get<std::vector<lbot::ConfigValue>>();
  EXPECT_EQ(sequence[0].get<bool>(), true);
  EXPECT_EQ(sequence[1].get<i64>(), 1);
  EXPECT_EQ(sequence[2].get<double>(), 1.0);
  EXPECT_EQ(sequence[3].get<std::string>(), std::string("test"));

  const std::vector<lbot::ConfigValue> &sequence_child = sequence[4].get<std::vector<lbot::ConfigValue>>();
  EXPECT_EQ(sequence_child[0].get<bool>(), true);

  EXPECT_EQ(config->getParameter("/path/to/value").get<i64>(), 42);
}

}  // namespace lbot::test
}  // namespace labrat
