# AI Agent Guidelines

## Build Verification

After making any code edits, always run the build to verify your changes compile correctly:

```batch
build.bat
```

This script will:
1. Detect and initialize the Visual Studio MSVC environment
2. Configure the project using Qt's CMake integration
3. Build the project with Ninja

Ensure the build completes without errors before considering the task done.
