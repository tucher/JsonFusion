# Package Manager Support

This directory contains package manager-specific files organized by package manager.

## Structure

```
packaging/
├── conan/
│   ├── test_package/       # Conan verification test
│   └── example/            # Complete usage example
└── README.md               # This file
```

## Usage

### Conan
See [docs/conan_usage.md](../docs/conan_usage.md) for details.

```bash
conan create . --build=missing --test-folder=packaging/conan/test_package
```

### vcpkg
JsonFusion is available in the [official vcpkg registry](https://github.com/microsoft/vcpkg/tree/master/ports/jsonfusion).
See [docs/vcpkg_usage.md](../docs/vcpkg_usage.md) for details.

### PlatformIO
Uses `library.json` at the repository root.
See [docs/PLATFORMIO.md](../docs/PLATFORMIO.md) for details.
