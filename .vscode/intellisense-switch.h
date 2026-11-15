// IntelliSense-only shim. Never compiled into the app; it's listed as "forcedInclude"
// in c_cpp_properties.json.
//
// VS Code's IntelliSense modes all assume a host OS ("linux-gcc-arm64" defines __linux__,
// "gcc-arm64" on a Mac defines __APPLE__). The real Switch compiler (aarch64-none-elf-gcc,
// bare-metal Horizon) defines neither, so headers like SDL2 take the wrong #ifdef branch and
// fail to find <endian.h> / <AvailabilityMacros.h>. Undo the host-OS macros here.
#undef __linux__
#undef __linux
#undef linux
#undef __gnu_linux__
#undef __unix__
#undef __unix
#undef unix
#undef __APPLE__
#undef __MACH__

// Macros the real compiler/build defines (see `aarch64-none-elf-g++ -dM -E` and the Makefile).
#ifndef __ELF__
#define __ELF__ 1
#endif
#ifndef __SWITCH__
#define __SWITCH__ 1
#endif
