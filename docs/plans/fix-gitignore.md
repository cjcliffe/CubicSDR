# Plan: Fix .gitignore

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers adding comprehensive patterns to `.gitignore`.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

**Last Updated:** 2026-07-23

## Current State

`.gitignore` only covers `build/`, `cmake_build/`, `dist/`. Missing IDE, OS, editor, build artifact, and package patterns.

## Implementation Plan

Replace the contents of `.gitignore` with comprehensive patterns:

```gitignore
# Build directories
build/
cmake_build/
dist/
Testing/

# CMake generated files
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
Makefile
compile_commands.json
_deps/

# IDE files
.vs/
*.suo
*.user
*.sln.docstates
.idea/
.vscode/
*.code-workspace
.project
.cproject
.settings/
.classpath
*.xcworkspace/
*.xcodeproj/
*.swp
*.swo
*~

# OS files
.DS_Store
Thumbs.db
Desktop.ini

# Compiled artifacts
*.o
*.obj
*.a
*.lib
*.so
*.dll
*.dylib
*.exe
*.out
*.app/
*.dSYM/

# MSVC artifacts
*.pdb
*.ilk
*.exp
*.log

# Package artifacts
*.deb
*.rpm
*.dmg
*.nsi

# Misc
.cache/
```

## Files to Modify

| File | Action |
|------|--------|
| `.gitignore` | Rewrite |
