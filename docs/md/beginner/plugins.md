@page plugins Plugins

@note
You may also want to take a look at the [code example](@ref example_plugins).

# Basics
Plugins are small extensions that encapsulate solutions to common problems. Plugins can be grouped into two categories:
 - Node plugins: plugins that create nodes. They may send/receive topics or provide services. They are usually used to create interfaces to external devices or bridges to other labrat-robot instances.
 - Trace plugins: plugins that read from topics globally. They are usually used to create data logs for debugging purposes.

# Bundled plugins
The following plugins are currently bundled with labrat-robot

| Name          |  Category | Description |
| ---           | ---       | ---         |
| udp-bridge    | Node      | A bridge that allows you to connect two labrat-robot instances via UDP. |
| serial-bridge | Node      | A bridge that allows you to connect two labrat-robot instances via a serial port. |
| mcap          | Trace     | Records topics into a [MCAP](https://mcap.dev/) file. MCAP files can be loaded in [Foxglove](https://foxglove.dev/). |
| foxglove-ws   | Trace     | Opens a [Foxglove](https://foxglove.dev/) WebSocket connection. This allows you to visualize topics within Foxglove while your program is running. |
| mavlink       | Node      | Creates an interface to read/write common [MAVLink](https://mavlink.io/en/) messages. |

## MCAP
The MCAP recorder plugin allows you to trace messages to a `.mcap` file. This file format can be loaded in [Foxglove Studio](https://foxglove.dev/). In order to use the plugin in your code you should create an instance of the [lbot::plugins::McapRecorder](@ref labrat::lbot::plugins::McapRecorder) class within your `main()` function. The first parameter specifies the path of the file you want to record your data to. It is recommended that you use a timestamp within this path to avoid data loss.
```cpp
std::unique_ptr<lbot::plugins::McapRecorder> mcap_plugin = std::make_unique<lbot::plugins::McapRecorder>("path/to/trace.mcap");
```
In order to properly use this plugin you also need to:
1. Install and open [Foxglove Studio](https://foxglove.dev/).
2. Open a local file with the path of your generated `.mcap` file.

## Foxglove WebSocket
The Foxglove server plugin allows you to trace messages via the Foxglove WebSocket protocol. This allows you to analyze data within [Foxglove Studio](https://foxglove.dev/) while your program is running. In order to use the plugin in your code you should create an instance of the [lbot::plugins::FoxgloveServer](@ref labrat::lbot::plugins::FoxgloveServer) class within your `main()` function. The first parameter specifies the name of the server. The second parameter specifies the port the server uses. The default used is `8765`. 
```cpp
std::unique_ptr<lbot::plugins::FoxgloveServer> foxglove_plugin = std::make_unique<lbot::plugins::FoxgloveServer>("Example Server", 8765);
```
In order to properly use this plugin you also need to:
1. Install and open [Foxglove Studio](https://foxglove.dev/).
2. Open a connection via Foxglove WebSocket with the URL `ws://[IP of your target machine]:[port]` (The default when working on the same machine as your program is `ws://localhost:8765`).