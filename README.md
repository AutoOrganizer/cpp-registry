# cpp-registry

A modern, header-only C++ library for Windows Registry operations with RAII principles and exception safety.

## Features

- **Modern C++17/20 Design**: Clean, intuitive API with RAII resource management
- **Type-Safe**: Strongly-typed wrappers around Windows Registry API
- **Exception Safety**: Comprehensive error handling with meaningful exceptions
- **Unicode Support**: Full support for Unicode registry keys and values
- **Comprehensive Value Types**: Support for all standard registry value types (REG_SZ, REG_DWORD, REG_QWORD, REG_BINARY, REG_MULTI_SZ, etc.)
- **Iterator Support**: Range-based iteration over keys and values
- **Transaction Support**: Optional support for transactional registry operations
- **Cross-Platform Build**: Works with MSVC, MinGW, and Clang on Windows

## Requirements

- C++17 or later
- Windows Vista or later
- CMake 3.15+ (for building examples and tests)

## Installation

### CMake Integration

```cmake
add_subdirectory(cpp-registry)
target_link_libraries(your_target PRIVATE cpp-registry::cpp-registry)
```

Or using FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(
    cpp-registry
    GIT_REPOSITORY https://github.com/Karelian-na/cpp-registry.git
    GIT_TAG main
)
FetchContent_MakeAvailable(cpp-registry)

target_link_libraries(your_target PRIVATE cpp-registry::cpp-registry)
```

### Examples

- Basic key and value operations: [examples/basic_operations.cc](examples/basic_operations.cc)
- Default value ((Default) in regedit): [examples/default_value.cc](examples/default_value.cc)
- Iterate subkeys and values: [examples/iterate_entries.cc](examples/iterate_entries.cc)

On Windows, an empty value name maps to the key default value. See [examples/default_value.cc](examples/default_value.cc).


## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by modern C++ best practices
- Windows Registry API documentation from Microsoft
- Community feedback and contributions

## Support

For issues, questions, or contributions, please visit the [GitHub repository](https://github.com/Karelian-na/cpp-registry).
