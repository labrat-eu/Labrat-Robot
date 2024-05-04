# Bridge between gazebo and lbot

## Building the plugin
```
conan install . --build=missing
conan build .
```

## Running gazebo
```
GZ_SIM_SYSTEM_PLUGIN_PATH=$(pwd)/build/Debug/lib/ gz sim testing/data/world.sdf
```
