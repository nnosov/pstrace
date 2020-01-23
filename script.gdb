# http://sourceware.org/gdb/wiki/FAQ: to disable the
# "---Type <return> to continue, or q <return> to quit---"
# in batch mode:
set width 0
set height 0
set verbose off
# enable breakpoint pending on future shared library load in batch
set breakpoint pending on
dashboard -enabled off
handle SIGSEGV nostop noprint

b _start
commands 1
  p $rsp
  info frame
  info registers
  continue
end

# at __libc_start_main point - print RSP
b __libc_start_main
commands 2
  p $rsp
  info frame
  info registers
  continue
end

# at entry point - print RSP
b main
commands 3
  p $rsp
  info frame
  info registers
  continue
end

# at Fun1 point - print RSP
b Fun1
commands 4
  p $rsp
  info frame
  info registers
  continue
end

# at Fun2 point - print RSP
b Fun2
commands 5
  p $rsp
  info frame
  info registers
  printf "\n"
  continue
end

# show arguments for program
#show args
#printf "Note, however: in batch mode, arguments will be ignored!\n"

# note: even if arguments are shown;
# must specify cmdline arg for "run"
# when running in batch mode! (then they are ignored)
# below, we specify command line argument "2":
run      # run

#start # alternative to run: runs to main, and stops
#continue
