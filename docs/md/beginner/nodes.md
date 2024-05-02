@page nodes Nodes

@note
You may also want to take a look at the [code example](@ref example_nodes).

# Basics
Any robotics project can be understood to be the sum of many components. A component might be a sensor or an actuator. There exist also more complex logical components such as motion planning controllers etc. Nodes encapsulate the logic of such components. In general, you should create a node for each external input your system might receive (sensors, external messages, etc.) and output you might generate (actuators, external messages, etc.). In addition you should also create nodes to model the data flow of your internal logic.

@image html nodes.svg

Nodes can communicate with each other via topics and services. This is the only interface they should expose to each other. This ensures that all transactions between nodes can be traced.

In order to create a node, you have to define a class that is derived from [lbot::Node](@ref labrat::lbot::Node).
```cpp
class ExampleNode : public lbot::Node { ... }
```

# Manager
Nodes should never be created by directly calling their constructor. Instead you should use the [central manager](@ref labrat::lbot::Manager). You can access it by calling the following function.
```cpp
lbot::Manager::Ptr manager = lbot::Manager::get();
```
Now you can add nodes to the manager. A call to [addNode()](@ref labrat::lbot::Manager::addNode()) will construct the node and will take care of any cleanup after your program has finished. The first argument of the [addNode()](@ref labrat::lbot::Manager::addNode()) function is the name of the node. Afterwards custom arguments can be provided that are forwarded to the nodes constructor. Generally, you can add multiple nodes of the same type, but you cannot add multiple nodes with the same name.
```cpp
manager->addNode<ExampleNode>("node_a", ...);
```
You can remove nodes from the manager explicitly via the [removeNode()](@ref labrat::lbot::Manager::removeNode()) method. However you do not need to do this at the end of your program, as any remaining nodes will be removed automatically.
```cpp
manager->removeNode("node_a");
```

# Inside the Node
Inside of the node you have access to a variety of functions. Many are related to topics and services. But they also include `getLogger()`, a convenience function to access the node specific logger and `getName()` to access the node's name.
```cpp
getLogger().logInfo() << "This node is called " << getName() << ".";
```

# Unique Nodes
Nodes can also inherit from [lbot::UniqueNode](@ref labrat::lbot::UniqueNode). This ensures that only one instance of the node can be added to the central manager. Writing a unique node might therefore be easier, as you do not have to worry about duplicate topic/service names or shared access to hardware resources.
```cpp
class ExampleNode : public lbot::UniqueNode { ... }
```

You may also specify a preferred name of the node in the constructor of a node. Name violations will then result in a warning to the user.
```cpp
ExampleNode(...) : lbot::UniqueNode("example_node") {}
```
