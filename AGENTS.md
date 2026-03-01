# AI Agent Guidelines

## Code Editing

When editing files, only modify the code that is directly related to the task. Do NOT touch unrelated code, including:
- Comments
- Whitespace
- Formatting
- Unused code sections

Keep changes minimal and focused on the specific task at hand.

## Build Verification

After making any code edits, always run the build to verify your changes compile correctly:

```batch
.\build.bat
```

This script will:
1. Detect and initialize the Visual Studio MSVC environment
2. Configure the project using Qt's CMake integration
3. Build the project with Ninja

Ensure the build completes without errors before considering the task done.
