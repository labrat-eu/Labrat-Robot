@page troubleshooting Troubleshooting

# LBOT_REFLECTION_PATH
When starting your program without using the provided launch script you may encounter the error message _"Message schema of the log message is unknown. Please set the LBOT_REFLECTION_PATH environment variable"_. In order to fix this issue you need to set the LBOT_REFLECTION_PATH environment variable to the `/var/run/lbot/schema`. In case you use conan this path must be prefixed with the install directory of the relevant conan lbot package (e.g. `$CONAN_HOME/p/b/.../var/run/lbot/schema`).
```bash
export LBOT_REFLECTION_PATH=/var/run/lbot/schema
```

# Compilation with clang/libc++
Please use LLVM version 18 or higher. When using libc++ you need to add the following lines to your conan profile.
```ini
[conf]
tools.build:cxxflags=['-fexperimental-library']
tools.build:exelinkflags=['-lc++', '-lm']
```