# Pretty Stack Trace library

# unwinding-related gcc options
*-funwind-tables* tells linker to generate .eh_frame section containing CFI. enabled by default if gcc/g++ used for linking
*-fexceptions* Enable exception handling. Generates extra code needed to propagate exceptions.
*-rdynamic* exports the symbols of an executable, allows to print function names in backtrace
*-fno-omit-frame-pointer* instructs the compiler to store the stack frame pointer in a register RBP for x86_64.