# Pretty Stack Trace library

The goal of the project is to enable C/C++ developers to get informative  stack trace in case of program crash (inside of signal handler). Key feature is to log not only function address/name/line etc but (if DWARF .debug_info section is present) values of function's parameters and variables.

Currently development focused on x86_64 architecture and for Linux OS, but further may be expanded to other architectures and OS if it will be demanded and some people will like to contribute to the project to support this architecture and OS.

As a first stage the goal is to dump only C/C++ base types such as pointer, boolean, integer etc.

Second stage dump composite types such as C structures, C++ classes, arrays etc.

Third stage to dump dereferenced pointers to data types with pointer validation.

## Dependencies

Currently library mostly depends on libunwind library which used to unwind function's stack frames from stack in signal handler, and libdw library which gives access to DWARF information of executed process if it present.

## commands to work with debug information

produce very simple debug info without sources
`echo 'void foo () {}' | gcc -g -O99 -o - -S -xc - -dA | grep frame_base`

dump .debug_info section

`readelf --debug-dump=info ./build/trace`

`objdump --dwarf=info  ./build/trace`

dump symbol tables of executable

`readelf -s ./build/trace`

dump file segment headers

`readelf -l ./build/trace`

dump .eh_frame section

`readelf --debug-dump=frames simple`

dump list of sections

`readelf -S ./build/trace`

dump lie section

`objdump --dwarf=decodedline `

disassemble executable

`objdump --d  ./build/trace`

## unwinding-related gcc options
*-funwind-tables* tells linker to generate .eh_frame section containing CFI. enabled by default if gcc/g++ used for linking
*-fexceptions* Enable exception handling. Generates extra code needed to propagate exceptions.
*-rdynamic* exports the symbols of an executable, allows to print function names in backtrace
*-fno-omit-frame-pointer* instructs the compiler to store the stack frame pointer in a register RBP for x86_64.