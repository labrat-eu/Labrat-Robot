@page plugins Plugins

@note
You may also want to take a look at the [code example](@ref example_plugins).

# Basics
Plugins are small extensions that encapsulate solutions to common problems. Plugins can be roughly grouped into two categories:
- Node plugins: plugins that create nodes. They may send/receive topics or provide services. They are usually used to create interfaces to external devices or bridges to other labrat-robot instances.
- Trace plugins: plugins that read from topics globally. They are usually used to create data logs for debugging purposes.

# Manager
Plugins behave similarly to [nodes](@ref lbot::Node). Like nodes, they should never be created by directly calling their constructor. Instead you should use the [central manager](@ref lbot::Manager). A call to [addPlugin()](@ref lbot::Manager::addPlugin()) will construct the plugin and will take care of any cleanup after your program has finished. The first argument of the [addPlugin()](@ref lbot::Manager::addPlugin()) function is the name of the plugin. Afterwards custom arguments can be provided that are forwarded to the plugins constructor. Generally, you can add multiple plugins of the same type, but you cannot add multiple plugins with the same name.
```cpp
manager->addPlugin<ExamplePlugin>("plugin_a", ...);
```
You can remove plugins from the manager explicitly via the [removePlugin()](@ref lbot::Manager::removePlugin()) method. However you do not need to do this at the end of your program, as any remaining plugins will be removed automatically.
```cpp
manager->removePlugin("plugin_a");
```

# Bundled plugins
The following plugins are currently bundled with labrat-robot

| Name          |  Category | Description |
| ---           | ---       | ---         |
| udp-bridge    | Node      | A bridge that allows you to connect two labrat-robot instances via UDP. |
| serial-bridge | Node      | A bridge that allows you to connect two labrat-robot instances via a serial port. |
| mcap          | Trace     | Records topics into a [MCAP](https://mcap.dev/) file. MCAP files can be loaded in [Foxglove](https://foxglove.dev/). |
| foxglove-ws   | Trace     | Opens a [Foxglove](https://foxglove.dev/) WebSocket connection. This allows you to visualize topics within Foxglove while your program is running. |
| gazebo-time   | Node      | Synchronizes the custom clock with a [gazebo](https://gazebosim.org/home) simulation. |
| mavlink       | Node      | Creates an interface to read/write common [MAVLink](https://mavlink.io/en/) messages. |

## MCAP
The MCAP recorder plugin allows you to trace messages to a `.mcap` file. This file format can be loaded in [Foxglove Studio](https://foxglove.dev/). In order to use the plugin in your code you should add an instance of the [lbot::plugins::McapRecorder](@ref lbot::plugins::McapRecorder) to the manager within your `main()` function. You may set the `/lbot/plugins/mcap/tracefile` parameter to the path of the file you want to record your data to. It is recommended that you use a timestamp within this path to avoid data loss.
```cpp
config->setParameter("/lbot/plugins/mcap/tracefile", "test.mcap");
manager->addPlugin<lbot::plugins::McapRecorder>("mcap");
```
In order to properly use this plugin you also need to:
1. Install and open [Foxglove Studio](https://foxglove.dev/).
2. Open a local file with the path of your generated `.mcap` file.

## Foxglove WebSocket
The Foxglove server plugin allows you to trace messages via the Foxglove WebSocket protocol. This allows you to analyze data within [Foxglove Studio](https://foxglove.dev/) while your program is running. In order to use the plugin in your code you should add an instance of the [lbot::plugins::FoxgloveServer](@ref lbot::plugins::FoxgloveServer) class to the manager within your `main()` function. You may specify the name of the server via the `/lbot/plugins/foxglove-ws/name` parameter. The port of the server can be configured with the `/lbot/plugins/foxglove-ws/port` parameter. The default used is `8765`. 
```cpp
config->setParameter("/lbot/plugins/foxglove-ws/name", "Example Server");
config->setParameter("/lbot/plugins/foxglove-ws/port", 8765);
manager->addPlugin<lbot::plugins::FoxgloveServer>("foxglove-ws");
```
In order to properly use this plugin you also need to:
1. Install and open [Foxglove Studio](https://foxglove.dev/).
2. Open a connection via Foxglove WebSocket with the URL `ws://[IP of your target machine]:[port]` (The default when working on the same machine as your program is `ws://localhost:8765`).