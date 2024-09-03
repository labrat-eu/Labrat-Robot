# lbot Framework

Minimal robot framework to provide an alternative to ROS.

## Install dependencies
Install the conan package manager alongside other build dependencies.
```
apt install gcc g++ python3 python3-pip python3-regex git cmake
pip3 install --break-system-packages conan
```

You may need to update your `PATH` 
```
source ~/.profile
```

Please make sure that you installed Conan 2.0 or higher.
```shell
conan -v
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
