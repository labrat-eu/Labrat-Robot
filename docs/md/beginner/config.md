@page config Configuration

@note
You may also want to take a look at the [code example](@ref example_config).

# Basics
Any robotics project should be configurable. That means that any runtime information a program might need (file paths, I/O ports, etc.) should not be hardcoded. Instead such configurations should be supplied externally. This allows for easy reconfigurability without having to recompile the entire project. Lbot provides means of storing such configuration data.

# Centralized configuration
The [lbot::Config](@ref lbot::Config) is the central class to manage configuration data. You can obtain a reference to it by calling the [get()](@ref lbot::Config::get()) function.
```cpp
lbot::Config::Ptr config = lbot::Config::get();
```
This class stores configuration parameters. A parameter is a named object that contains one of the following data types:
- nothing
- `bool`
- `lbot::i64`
- `double`
- `std::string`
- a sequence of parameters

In order to store such a parameter you need to call the [Config::setParameter()](@ref lbot::Config::setParameter()) method. The first argument is the name of the parameter. The convention is to use a path-like naming structure. In the second argument you may specify the value. This method will overwrite any data previously written onto the parameter. You should set all parameters before you load any plugins or add any nodes.
```cpp
config->setParameter("/path/to/parameter", 42L);
```
To access a stored parameter you need to then call the [Config::getParameter()](@ref lbot::Config::getParameter()) method. The first argument is the name of the requested parameter. If the requested parameter does not exist, this function will throw an instance of [lbot::ConfigAccessException](@ref lbot::ConfigAccessException). If you instead just want to use another value, you can use the [Config::getParameterFallback()](@ref lbot::Config::getParameterFallback()) method. It allows you to specify a fallback value in case of an unset parameter. In order to access the stored value you also need to call the [ConfigValue::get()](@ref lbot::ConfigValue::get()) method with the correct data type as a template argument. If there is a type mismatch between the requested and stored types a [lbot::ConfigAccessException](@ref lbot::ConfigAccessException) will be raised.
```cpp
val = config->getParameter("/path/to/parameter").get<lbot::i64>(); // might throw
val = config->getParameterFallback("/path/to/parameter", 42L).get<lbot::i64>(); // doesn't throw
```

# Loading a configuration from a file
You usually don't want to specify all of your parameters within your code. Instead you should use a configuration file to store most of your parameters. The [Config::load()](@ref lbot::Config::load()) method allows you to load parameters from a [YAML](https://yaml.org/) file. If any parsing errors accur, a [lbot::ConfigParseException](@ref lbot::ConfigParseException) will be raised.
```cpp
config->load("path/to/config.yaml");
```
This will automatically create the parameters specified in the configuration file with the correct type. Note that nested YAML nodes will be flattened to a single parameter.
```yaml
bool: true    # name: /bool
int: 1        # name: /int
double: 1.0   # name: /double
string: test  # name: /string
sequence:     # name: /sequence
 - true
 - 1
 - 1.0
 - test
path:
  to:
    value: 42 # name: /path/to/value
```
