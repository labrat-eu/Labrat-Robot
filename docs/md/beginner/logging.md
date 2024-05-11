@page logging Logging

@note
You may also want to take a look at the [code example](@ref example_logging).

# Basics
Labrat-robot comes with its own logging system. In order to use it you need to create a [lbot::Logger](@ref lbot::Logger) object. You are required to specify a name for the logger. The name should be unique, as it allows you to track where your logging messages come from.
```cpp
lbot::Logger logger("main");
```

Now you can use your logger to create log messages. The logger behaves similarly to `std::cout` or `std::cerr`, as you can stream many types into it.
```cpp
logger.logInfo() << "Testing... " << 123;
```
This might produce the following output on your console when executed:
```
[INFO] (main): Testing... 123
```

# Verbosity
There are 5 distinct verbosity levels. They can be accessed by calling the respective member function of [lbot::Logger](@ref lbot::Logger) as follows.
```cpp
logger.logCritical() << "This is a critical message.";
logger.logError() << "This is an error message.";
logger.logWarning() << "This is a warning message.";
logger.logInfo() << "This is an info message.";
logger.logDebug() << "This is a debug message.";
```

| Verbosity level | Intended use |
| ---             | ---          |
| `critical`      | Critical error messages that (might) cause the program to exit prematurely. |
| `error`         | Error messages. |
| `warning`       | Warnings. |
| `info`          | Information that is intended to be printed during normal operation. |
| `debug`         | Information that could help you debug any problems but might be distracting if printed during normal operation. |

# Output control
By default only log messages of the verbosity `info` and higher are printed to the console. However this behavior can be customized by setting the minimum verbosity through [setLogLevel()](@ref lbot::Logger::setLogLevel()).

By default colored console output is enabled. You can disable color output by calling [disableColor()](@ref lbot::Logger::disableColor()).

By default the time of a log message is printed to the console. You can disable time printing by calling [disableTime()](@ref lbot::Logger::disableTime()).

By default the code location of a log message is not printed to the console. You can enable the printing of code locations by calling [enableLocation()](@ref lbot::Logger::enableLocation()).

# Notes
Log messages are also written to the `/log` topic.

All lbot exceptions (derived from [lbot::Exception](@ref lbot::Exception)) will log their message when thrown. The verbosity used for this feature is `debug`.
