@page custom_plugins Custom Plugins

# Basics
Not only can you use existing plugins in your project, you can also write one yourself as well. There are roughly two use cases where it makes sense to develop a plugin:
1. You want to logically group your nodes together. This can be useful when developing a package that uses multiple nodes but you don't want the user to start them up separately.
2. You want to trace messages from all topics to stream/record/analyze the data globally.

In order to create a plugin, you have to define a class that is derived from [lbot::Plugin](@ref lbot::Plugin).
```cpp
class ExamplePlugin : public lbot::Plugin { ... }
```

# Inside the Plugin
Inside of the plugin you have access to a variety of functions. The most basic ones are [getLogger()](@ref lbot::Plugin::getLogger()), a convenience function to access the plugin specific logger and [getName()](@ref lbot::Plugin::getName()) to access the plugin's name.
```cpp
getLogger().logInfo() << "This plugin is called " << getName() << ".";
```

## Node creation
You can also create the nodes contained by the plugin. This can be done through the [addNode()](@ref lbot::Plugin::addNode()) method. This function behaves similarly to the [Manager::addNode()](@ref lbot::Manager::addNode()) method. Typically the child nodes are created within the constructor of the plugin. Contained nodes are automatically deleted upon destruction of the plugin.
```cpp
ExamplePlugin() {
  addNode<ExampleNode>("node_a", ...);
}
```

## Tracing
In order to trace messages, you need to define certain member functions within your plugin. These will then automatically be called after your plugin has been registered. One is `void topicCallback(const TopicInfo &info)` and it will be called once a new topic has been created. The [TopicInfo](@ref lbot::TopicInfo) struct contains data about the topic such as its name. You may also define `void messageCallback(const MessageInfo &info)` which will be called every time a message is sent. The [MessageInfo](@ref lbot::MessageInfo) struct contains information about the topic of the message as well as a serialized version of the message itself.

# Unique Plugins
Nodes can also inherit from [lbot::UniquePlugin](@ref lbot::UniquePlugin). This ensures that only one instance of the plugin can be added to the central manager. Just like with nodes, writing a unique plugin might therefore be easier, as you do not have to worry about duplicate node names or shared access to hardware resources.
```cpp
class ExamplePlugin : public lbot::UniquePlugin { ... }
```

You may also specify a preferred name of the plugin in the constructor of a plugin. Name violations will then result in a warning to the user.
```cpp
ExamplePlugin(...) : lbot::UniquePlugin("example_plugin") {}
```
