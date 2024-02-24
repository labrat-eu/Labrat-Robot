# Labrat Robot Framework

Minimal robot framework to provide an alternative to ROS.

## Building the library
```
conan create . --build=missing
```

## Running the tests
```
conan test testing labrat-robot/<version>
```

## Building the library for development
```
mkdir build
cd build
conan install .. -of . --build=missing
conan build .. -of .
```

If you want to make the development version available to other conan projects on your system export your package.
```
conan export-pkg --force .. <reference>
```
