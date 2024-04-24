#include <labrat/lbot/manager.hpp>
#include <labrat/lbot/node.hpp>

#include <string>

// Nodes
//
// Any robotics project can be understood to be the sum of many components.
// A component might be a sensor or an actuator.
// There exist also more complex logical components such as motion planning controllers etc.
// A node encapsulates the logic of one such component.
//
// This example showcases how to work with nodes.

// Nodes inherit from lbot::Node.
class ExampleNode : public lbot::Node {
public:
  // A node behaves like any other class. Its constructor can take custom arguments.
  ExampleNode(const std::string &param) {
    // Nodes have their own logger.
    getLogger().logInfo() << "Example node has been started with the parameter " << param << ".";
  }

  ~ExampleNode() {
    getLogger().logInfo() << "Node is shutting down.";
  }
};

// Nodes can also inherit from lbot::UniqueNode.
// This ensures that this node can only be instanciated once.
class OtherNode : public lbot::UniqueNode {
public:
  // Nodes can also specify their preferred name. This name must not match their actial name.
  // But if it does not match, a warning will be printed.
  OtherNode() : lbot::UniqueNode("") {
    // Nodes have their own logger.
    getLogger().logInfo() << "This node can only be instanciated once.";
  }
};

int main(int argc, char **argv) {
  // First we create the central node manager.
  // Nodes should ALWAYS be created through the central node manager!
  lbot::Manager::Ptr manager = lbot::Manager::get();

  // Now we can create an instance of the example node.
  // The first parameter of the addNode() function is the name of the node.
  // Afterwards custom arguments can be provided.
  manager->addNode<ExampleNode>("node_a", "1234");

  // We can create multiple instances of the same node class, but node names must be unique.
  manager->addNode<ExampleNode>("node_b", "ABCD");
  manager->addNode<ExampleNode>("node_c", "EFGH");

  // Nodes can also be removed. However unremoved nodes will be automatically removed on shutdown.
  manager->removeNode("node_b");

  // Add a unique node.
  manager->addNode<OtherNode>("node_d");

  return 0;
}
