@page examples Code Examples

You can use the following code examples to help start off your project.

- @subpage example_hello_world
- @subpage example_logging
- @subpage example_nodes
- @subpage example_concurrency
- @subpage example_topics
- @subpage example_services
- @subpage example_config
- @subpage example_plugins
- @subpage example_conversions
- @subpage example_moves

# Building
Navigate to the `examples/` directory.
```
cd examples
```

You first need to build the lbot library.
```
conan create .. --build=missing
```

Now you can build the examples.
```
conan build .
```

You may now run an example by using the provided launch script.
Choose the relevant script based on your build type.
```
./build/Debug/launch_hello-world.sh
./build/Release/launch_hello-world.sh
```

@page example_hello_world Hello World
# `main.cpp`
@include examples/00-hello-world/main.cpp
# `CMakeLists.txt`
@include examples/00-hello-world/CMakeLists.txt

@page example_logging Logging Example
# `main.cpp`
@include examples/01-logging/main.cpp
# `CMakeLists.txt`
@include examples/01-logging/CMakeLists.txt

@page example_nodes Nodes Example
# `main.cpp`
@include examples/02-nodes/main.cpp
# `CMakeLists.txt`
@include examples/02-nodes/CMakeLists.txt

@page example_concurrency Concurrency Example
# `main.cpp`
@include examples/03-concurrency/main.cpp
# `CMakeLists.txt`
@include examples/03-concurrency/CMakeLists.txt

@page example_topics Topics Example
# `main.cpp`
@include examples/04-topics/main.cpp
# `msg/numbers.fbs`
@include examples/04-topics/msg/numbers.fbs
# `CMakeLists.txt`
@include examples/04-topics/CMakeLists.txt

@page example_services Services Example
# `main.cpp`
@include examples/05-services/main.cpp
# `msg/request.fbs`
@include examples/05-services/msg/request.fbs
# `msg/response.fbs`
@include examples/05-services/msg/response.fbs
# `CMakeLists.txt`
@include examples/05-services/CMakeLists.txt

@page example_config Configuration Example
# `main.cpp`
@include examples/06-config/main.cpp
# `config.yaml`
@include examples/06-config/config.yaml
# `CMakeLists.txt`
@include examples/06-config/CMakeLists.txt

@page example_plugins Plugins Example
# `main.cpp`
@include examples/07-plugins/main.cpp
# `msg/vector.fbs`
@include examples/07-plugins/msg/vector.fbs
# `CMakeLists.txt`
@include examples/07-plugins/CMakeLists.txt

@page example_conversions Conversions Example
# `main.cpp`
@include examples/08-conversions/main.cpp
# `msg/number.fbs`
@include examples/08-conversions/msg/number.fbs
# `CMakeLists.txt`
@include examples/08-conversions/CMakeLists.txt

@page example_moves Move Operations Example
# `main.cpp`
@include examples/09-moves/main.cpp
# `msg/array.fbs`
@include examples/09-moves/msg/array.fbs
# `CMakeLists.txt`
@include examples/09-moves/CMakeLists.txt
