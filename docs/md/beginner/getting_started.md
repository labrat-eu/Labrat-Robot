@page getting_started Getting Started

# Environment setup
In order to use labrat-robot, you need to run [Linux](https://www.linux.org/) as your operating system. As of now there are no plans of providing cross-platform support. However this might change in the future.

## GCC/Clang
The C++ standard required by labrat-robot is C++20. You should therefore have a fairly new compiler installed on your system. It is recommended that you use either [gcc](https://gcc.gnu.org/)(>=13) or [clang](https://clang.llvm.org/)(>=18).

## Conan
Labrat-robot is tightly integrated with the package manager [Conan](https://conan.io/). In order to use it, you therefore have to install and set up Conan first. 

@note
If you experience problems while installing Conan, please refer to the official [documentation](https://docs.conan.io/2/installation.html).

To install conan, you need to install [Python](https://www.python.org/)(>=3.10) with pip first. You may use a system package manager of your choice to install it. In this guide we will use apt.
```shell
apt install python3 python3-pip
```

Afterwards you can install Conan with pip.
```shell
pip install conan
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
arch=<your CPU-architecture (e.g. x86_64)>
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

## Labrat-robot
To install labrat-robot, clone the latest release from git.
```shell
git clone https://gitlab.com/labrat.eu/lbot
cd robot
git switch -d <latest release tag>
```

Now you can run Conan to automatically build and install all dependencies alongside labrat-robot itself. *This might take some time.*
```shell
conan create . --build=missing
```

# Project setup
You have now installed all dependencies and are ready to set up your own project.

@note
**You are encouraged to copy code from the [code examples](@ref examples) to start off your project.** In fact, you can likely skip most of this section if you just copy the [hello-world example](@ref example_hello_world). But it might still be useful to read it, as it gives you insights on the build system of your project.

The recommended file structure for a minimal labrat-robot project looks like this:
```
project/
|- conanfile.py        # define dependencies
|- CMakeLists.txt      # setup the project
|- src/
|  |- CMakeLists.txt   # define and configure targets
|  |- main.cpp         # main source file
```

## `conanfile.py`
The `conanfile.py` is the main configuration file for Conan. It will automatically set up CMake to work with the labrat-robot among other dependencies you might want to use.

@note
For a detailed guide how to set up a Conan project, please refer to the official [documentation](https://docs.conan.io/2/reference/conanfile.html).

In order to use labrat-robot, you have to set it as a dependency in the `requirements()` method.
@lbot_requirements

You also need to specify CMake as a build dependency in the `build_requirements()` method.
```python
def build_requirements(self):
  self.tool_requires("cmake/3.29.3")
```

## `CMakeLists.txt`
The recommended build system for labrat-robot projects is [CMake](https://cmake.org/). It is configured through `CMakeLists.txt` files.

@note
For a detailed guide how to set up a CMake project, please refer to the official [documentation](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Writing%20CMakeLists%20Files.html).

It is recommended to place one such file into your project root directory. Here you can define your project and include further `CMakeLists.txt` files. In your source directory you should also create another `CMakeLists.txt` file. Here you can define and configure your targets. In order to link with labrat-robot you first need to include the lbot package.
```cmake
find_package(lbot MODULE REQUIRED)
```

Now you can link your target to the labrat-robot core library. This will also correctly set the include path.
```cmake
target_link_libraries(${TARGET_NAME} PRIVATE lbot::core)
```

## Build and run your program
You can now build your program through Conan.
```shell
conan build . --build=missing
```

You may now start your program. It is located inside your build folder.
```shell
cd build/Debug                # You may replace Debug with your build type
./00-hello-world/hello-world  # Replace with the path to your executable
```