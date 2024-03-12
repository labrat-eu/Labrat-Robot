# Labrat Robot Framework

Minimal robot framework to provide an alternative to ROS.

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
conan build .
```

If you want to make the development version available to other conan projects on your system export your package.
```
conan export-pkg .
```
