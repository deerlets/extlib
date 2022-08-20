# Extlib

[![Build status](https://ci.appveyor.com/api/projects/status/xw7be399o260xftd?svg=true)](https://ci.appveyor.com/project/yonzkon/extlib)

Extension for libc

## Supported platforms

- Windows
- MacOS
- Linux
- arm-none-eabi-gcc with newlib

## Build
```
mkdir build && cd build
cmake ..
make && make install
```

## Build Test
```
mkdir build && cd build
cmake .. -DBUILD_DEBUG=on -DBUILD_TESTS=on
make && make test
```
