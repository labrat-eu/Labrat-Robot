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
conan install .. --build=missing
conan build ..
```

## Building the examples
```
cd testing
mkdir build
cd build
conan install .. --build=missing
conan build ..
```
