# lbot Framework

Minimal robot framework to provide an alternative to ROS.

## Install dependencies
Download the conan package from the [downloads page](https://conan.io/downloads) and install it.

Please make sure that you installed Conan 2.0 or higher.
```shell
conan -v
```

Now install other build tools. You may use a system package manager of your choice to install it.
```shell
apt update
apt install gcc g++ python3 git cmake
```

Before you can use Conan, you need to create a default build profile. You can create one automatically.
```shell
conan profile detect
```
You might want to edit the file `~/.conan2/profiles/default` to set the correct compiler and C++ standard among other settings. The following settings were found to cause little problems on a variety of systems.
```ini
[settings]
arch=<your CPU-architecture (e.g. x86_64, armv8)>
build_type=<Debug|Release>
compiler=<your compiler (e.g. gcc)>
compiler.cppstd=20
compiler.libcxx=libstdc++11
compiler.version=<your compiler's major version number (e.g. 13)>
os=Linux

[buildenv]
CC=<your C compiler with version (e.g. gcc-13)>
CXX=<your C++ compiler with version (e.g. g++-13)>
```

## Building the library
```
conan create . --build=missing
```

## Running the tests
```
conan test testing lbot/<version>
```

## Building the library for development
```
conan install . --build=missing
conan source .
conan build .
```

If you want to make the development version available to other conan projects on your system export your package.
```
conan export-pkg .
```

## Automatically format the code
You may automatically format the code by running the following command.
```
shopt -s globstar
clang-format -i --style=file:build/Debug/.clang-format src/*.cpp src/*.hpp src/**/*.cpp src/**/*.hpp
```
