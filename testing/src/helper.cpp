#include <labrat/lbot/manager.hpp>

#include <helper.hpp>

inline namespace labrat {
namespace lbot::test {

void lbotReset()
{
  if (lbot::Manager::instance.expired()) {
    lbot::Manager::instance.reset();
  }

  lbot::Manager::instance_flag = true;
}

}  // namespace lbot::test
}  // namespace labrat
