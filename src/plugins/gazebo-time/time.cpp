/**
 * @file time.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/message.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/msg/timestamp.hpp>
#include <labrat/lbot/plugins/gazebo-time/msg/gazebo_time.hpp>
#include <labrat/lbot/plugins/gazebo-time/time.hpp>
#include <gz/msgs.hh>
#include <gz/transport.hh>

inline namespace labrat {
namespace lbot::plugins {

class GazeboTimeConv : public MessageBase<GazeboTime, gz::msgs::Clock> {
public:
  static void convertFrom(const gz::msgs::Clock &source, Storage &destination) {
    destination.real_time = std::make_unique<TimeNative>();
    destination.real_time->seconds = source.real().sec();
    destination.real_time->nanoseconds = source.real().nsec();

    destination.simulation_time = std::make_unique<TimeNative>();
    destination.simulation_time->seconds = source.sim().sec();
    destination.simulation_time->nanoseconds = source.sim().nsec();
  }
};

class TimestampConv : public MessageBase<Timestamp, gz::msgs::Clock> {
public:
  static void convertFrom(const gz::msgs::Clock &source, Storage &destination) {
    destination.value = std::make_unique<foxglove::Time>(source.sim().sec(), source.sim().nsec());
  }
};

class GazeboTimeSourceNode : public UniqueNode {
public:
  GazeboTimeSourceNode();
  ~GazeboTimeSourceNode();

private:
  Sender<TimestampConv>::Ptr sender_time;
  Sender<GazeboTimeConv>::Ptr sender_debug;

  gz::transport::Node node;

  void callback(const gz::msgs::Clock &time) {
    sender_time->put(time);
    sender_debug->put(time);
  } 
};

GazeboTimeSource::GazeboTimeSource() {
  addNode<GazeboTimeSourceNode>("gazebo-time");
}

GazeboTimeSource::~GazeboTimeSource() = default;

GazeboTimeSourceNode::GazeboTimeSourceNode() {
  sender_time = addSender<TimestampConv>("/time");
  sender_debug = addSender<GazeboTimeConv>("/gazebo/time");

  if (!node.Subscribe("/clock", &GazeboTimeSourceNode::callback, this)) {
    throw IoException("Failed to connect to gazebo");
  }
}

GazeboTimeSourceNode::~GazeboTimeSourceNode() = default;

}  // namespace lbot::plugins
}  // namespace labrat
