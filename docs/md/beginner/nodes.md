@page nodes Nodes

@note
You may also want to take a look at the [code example](@ref example_nodes).

# Basics
Any robotics project can be understood to be the sum of many components. A component might be a sensor or an actuator. There exist also more complex logical components such as motion planning controllers etc. Nodes encapsulate the logic of such components. In general, you should create a node for each external input your system might receive (sensors, external messages, etc.) and output you might generate (actuators, external messages, etc.). In addition you should also create nodes to model the data flow of your internal logic.

![nodes.drawio.svg](uploads/6a27157c539af2d64c5087811fcc50b9/nodes.drawio.svg)

Nodes can communicate with each other via topics and services. This is the only interface they should expose to each other. This ensures that all transactions between nodes can be traced.

In order to create a node, you have to define a class that is derived from [lbot::Node](@ref labrat::lbot::Node).
```cpp
class ExampleNode : public lbot::Node { ... }
```
You need to define at least one constructor for your node. The first argument of the constructor must be of the type [const lbot::NodeEnvironment](@ref labrat::lbot::NodeEnvironment) and it must be forwarded to the parent constructor.
```cpp
ExampleNode(const lbot::NodeEnvironment environment, ...) : lbot::Node(environment) {}
```

# Manager
Nodes should never be created by directly calling their constructor. Instead you should use the [central node manager](@ref labrat::lbot::Manager). You can access it by calling the following function.
```cpp
lbot::Manager::Ptr manager = lbot::Manager::get();
```
Now you can add nodes to the manager. A call to [addNode()](@ref labrat::lbot::Manager::addNode()) will construct the node and will take of any cleanup after your program has finished. The first argument of the [addNode()](@ref labrat::lbot::Manager::addNode()) function is the name of the node. Afterwards custom arguments can be provided. You can add multiple nodes of the same type, but you cannot add multiple nodes with the same name.
```cpp
manager->addNode<ExampleNode>("node_a", ...);
```
You can remove nodes from the manager explicitly via the [removeNode()](@ref labrat::lbot::Manager::removeNode()) method. However you do not need to do this at the end of your program, as any remaining nodes will be removed automatically.
```cpp
manager->removeNode("node_a");
```

# Inside the node
Inside of the node you have access to a variety of functions. Many are related to topics and services. But they also include `getLogger()`, a convenience function to access the node specific logger and `getName()` to access the node's name.
```cpp
getLogger().logInfo() << "This node is called " << getName() << ".";
```