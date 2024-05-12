@page troubleshooting Troubleshooting

# Compilation with clang/libc++
Please use LLVM version 18 or higher. When using libc++ you need to add the following lines to your conan profile.
```ini
[conf]
tools.build:cxxflags=['-fexperimental-library']
tools.build:exelinkflags=['-lc++', '-lm']
```