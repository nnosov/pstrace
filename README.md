# Pretty Stack Trace library

## Table of Contents
* [Dependencies](#dependencies)
* [Usage](#usage)
* [Commands to work with debug information](#commands-to-work-with-debug-information)
* [Unwinding-related gcc options](#unwinding-related-gcc-options)
* [Useful links](#useful-links)

- The 1st goal of the project is to enable C/C++ (and in future other languages) developers to get informative  stack trace in case of program crash, for example inside of segmentation fault handler. The key feature is to log not only function address/name/line but additionally (if DWARF `.debug_info` section is present) function's parameters and variables and their **values**.

- The 2nd goal is to enable C/C++ (and in future other languages) developers to provide an API to make their own debug facilities on base of PSTrace library.
- The 3rd goal is to enable C/C++ (and in future other languages) developers to provide an API to profile their programs internally instead of external tools.

Currently development is focused on `x86_64` architecture and for Linux OS, but further may be expanded to other architectures and OS if it will be demanded and some people will like to contribute to the project to support this architecture and OS.

As a first sub-stage of 1st goal is to handle only C/C++ base types such as pointer, boolean, integer etc **(done)**.

As a second sub-stage of 1st goal is to handle dereferenced pointers to data types with pointer validation **(done)**.

As a third sub-stage of 1st goal is to handle composite types such as a C structures, arrays, C++ classes, etc **(in progress)**.

## Dependencies

Currently library mostly depends on `libunwind` library which used to unwind function's stack frames from stack in signal handler, `libdw` library which gives access to DWARF information of executed process if it present and `libiberty` which provides demangling of function names for C++ and other languages.

## Usage

To build, install these packages:

1. **sudo apt-get install libdw-dev libunwind-dev libiberty-dev**

2. **git clone https://github.com/nnosov/pstrace** 

3. **cd pstrace**

4. **make**

As an example how to use library, see tests/main.c

## Commands to work with debug information

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

## Useful links

## Assembler tutorials

[Calling conventions](https://en.wikibooks.org/wiki/X86_Disassembly/Calling_Conventions)

[Functions and stack frames](https://en.wikibooks.org/wiki/X86_Disassembly/Functions_and_Stack_Frames)

[x86 Assembli guide](http://www.cs.virginia.edu/~evans/cs216/guides/x86.html)

[GNU extended Asm](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html)

## DWARF links

[DWARF 5 Standard (DWARF comittee)](http://www.dwarfstd.org/doc/DWARF5.pdf)

[Introduction to the DWARF Debugging Format (DWARF comittee)](http://www.dwarfstd.org/doc/Debugging%20using%20DWARF-2012.pdf)

[Exception handling (DWARF wiki)](http://wiki.dwarfstd.org/index.php?title=Exception_Handling)

[Expression Operator For Constants (DWARF Wiki)](http://wiki.dwarfstd.org/index.php?title=Expression_Operator_For_Constants)

[Very  good explanation what's CFA on x86_64](https://www.corsix.org/content/cfa-rsp-x86-64)

[Very informative discussion on dwarf mailing lists about DW_OP_regX vs. DW_OP_bregX 0 ](http://lists.dwarfstd.org/pipermail/dwarf-discuss-dwarfstd.org/2009-February/002056.html)

[Very informative discussion on dwarf mailing list about Semantics of DW_OP_(b)reg](http://lists.dwarfstd.org/pipermail/dwarf-discuss-dwarfstd.org/2017-March/004335.html)

[DW_AT_call_site_XXX (which currently is DW_AT_GNU_call_site_XXX) explanation](http://www.dwarfstd.org/ShowIssue.php?issue=100909.2&type=open)

[Exploring the DWARF debug format information (IBM)](https://developer.ibm.com/articles/au-dwarf-debug-format/)

[DWARF: function return value types and parameter types (simple tutorial)](https://simonkagstrom.livejournal.com/51001.html)

[Exploiting the Hard-Working DWARF (slides)](https://www.usenix.org/legacy/events/woot11/tech/slides/oakley.pdf)

[Exploiting the Hard-Working DWARF (article)](https://www.cs.dartmouth.edu/~trdata/reports/TR2011-688.pdf)

[Local variable location from DWARF info in ARM](https://stackoverflow.com/questions/47359841/local-variable-location-from-dwarf-info-in-arm)

[Reliable and Fast DWARF-Based Stack Unwinding](https://hal.inria.fr/hal-02297690/document)

[Improving debug info for optimized away parameters (GCC summit 2012, GNU extensions)](https://gcc.gnu.org/wiki/summit2010?action=AttachFile&do=get&target=jelinek.pdf)

[VARIABLE TRACKING AT ASSIGNMENTS (Redhat, with link to GCC Wiki)](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/developer_guide/ch-debug-vta)

[Good .eh_frame explanation](https://www.airs.com/blog/archives/460)

[Good itroduction to Debug information](https://engineering.backtrace.io/posts/dwarf/)

[libdwarf example](https://gist.github.com/jsoffer/9e63f2e58ebd90e81b24)

## Stack trace and other

[System V ABI](https://www.uclibc.org/docs/psABI-x86_64.pdf)

[Programmatic access to the call stack in C++](https://eli.thegreenplace.net/2015/programmatic-access-to-the-call-stack-in-c/)

[Where the top of the stack is on x86]()

[Stack frame layout on x86-64](https://eli.thegreenplace.net/2011/09/06/stack-frame-layout-on-x86-64)

[Retrieving function arguments while unwinding the stack](https://nikhilism.com/post/2019/retrieving-function-arguments-while-unwinding-the-stack/)

[Deep Wizardry: Stack Unwinding (more-less helpful)](http://blog.reverberate.org/2013/05/deep-wizardry-stack-unwinding.html)

[Writing a Linux Debugger Part 8: Stack unwinding (other related articles also interesting)](https://blog.tartanllama.xyz/writing-a-linux-debugger-unwinding/)



## Miscellaneous

[Testing pointers for validity (C/C++) (using system calls)](https://stackoverflow.com/questions/551069/testing-pointers-for-validity-c-c)

[Another way for testing pointer to validity (via procfs)](https://mischasan.wordpress.com/2011/04/11/interjection-why-no-linux-isbadreadptr/)

[Implementing a Debugger: The Fundamentals](https://engineering.backtrace.io/posts/2016-08-11-debugger-internals/)



