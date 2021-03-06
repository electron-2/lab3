---
layout: page
title: 'Lab 3: Debugging and Testing'
permalink: /labs/lab3/
released: true
toc: true
---

*Lab written by Pat Hanrahan*

## Goals

During this lab you will:

1. Learn how to debug using `arm-none-eabi-gdb` in simulation mode. 
Simulation mode is also a good way to learn more about how the ARM processor executes instructions.
2. Understand function calls and stack frames.
3. Learn how to use `screen` with the USB-serial breakout board.Also understand how the UART peripheral works on the Raspberry Pi.
4. Implement a few simple C-string operations and use a combination of
unit testing and gdb simulation to debug your work.

## Prelab preparation
To prepare for lab, do the following: 

1. Read our [gdb guide](/guides/gdb) about running gdb in simulation mode.
2. Read the background section below on "Stack Intuition".
3. Pull the latest version of the `cs107e.github.io` repository.
4. Clone the lab repository which can be found at
   `https://github.com/cs107e/lab3`. Make sure you clone it in the same
   directory as your copy of the `cs107e.github.io` repository.

## Background: stack intuition

Functions often need space (e.g., for variables or to store the return
address of their caller before they call another function). There's
nothing special about this space, and we could allocate it as we
would any other memory.  However,
functions calls are frequent, so we want them as fast as possible.
Fortunately, we give
function calls two properties we can exploit for speed:

(1) when functions return, all allocated memory is considered dead

(2) functions return in LIFO order

As a result
of optimizing
for speed, people have converged on using a contiguous array of memory
(called *stack* because of its LIFO usage).  Roughly speaking it
works as follows:

+ At program start we allocate a fixed-sized region to hold the stack and set a
pointer (the stack pointer) to the start of that region.
+ At each function call, all the memory the function needs is
allocated contiguously and all-at-once by simply adjusting
the stack pointer.
+ At each function call return, all the memory the function
allocated is then freed all-at-once by simply returning
the stack pointer to its previous position.

Operating systems generally provide ways to dynamically grow the stack if the call stack grows to be deep, but we will
ignore this here.

Note that the stack on our system operates as a _descending_ stack. This means the stack pointer is initialized to at the highest address and we decrement the stack pointer to make space for a function call and increment the stack pointer when that function call returns. The stack on other systems may work in the opposite direction.

This organization is such a clear winner that compilers have explicit
support for it (and do the pointer increment and decrement) and
architecture manuals provide the rules for how to do so.
(This basic data structure appears in many other contexts when all the
data for a given purpose or type can be freed all at once.)
If you know `malloc()` and `free()`, one way to compare this method
and those functions is that the fastest malloc() you can do is a pointer
increment and the fastest free() a pointer decrement.

## Lab Exercises
Pull up the [checklist](checklist) so you have it open as you go.

### 1. Debugging with gdb

Once 45 minutes of the lab has elapsed, please explore sections 2 and
on! We have included lots of adventures in GDB so you can continue
exploring after lab.

#### 1a) Using `gdb` in simulation mode

The goal of the first exercise 
is to practice using `gdb` in ARM simulation mode.
This is useful as a way of learning what happens
when ARM instructions are executed, and to debug programs.

We will begin by running `gdb` on the example `simple` program. This
is similar to code showed in the lecture on functions.
Change to the `lab3/code/simple` directory and build the program using `make`.

Now run `arm-none-eabi-gdb simple.elf`.  Note that is the ELF 
version `simple.elf` that we use in conjunction with the gdb simulator, 
not the raw `simple.bin` that we have been running on the actual Pi.
Ignore the warning related to the Python gdb module.

```
$ arm-none-eabi-gdb simple.elf
GNU gdb (GDB) 7.8.1
Python Exception <type 'exceptions.ImportError'> No module named gdb:

warning:
Could not load the Python gdb module from `/Users/ibush/bin/arm-none-eabi/share/gdb/python'.
Limited Python support is available from the _gdb module.
Suggest passing --data-directory=/path/to/gdb/data-directory.
...
(gdb) 
```

Start by putting `gdb` in simulation mode,
and then load the program.

```
(gdb) target sim
Connected to the simulator.
(gdb) load
Loading section .text, size 0x168 vma 0x8000
Start address 0x8000
Transfer rate: 2880 bits in <1 sec.
```

`gdb` allows us to control program execution.
Let's single step through the program. Follow along
with the sequence of commands below.

```
(gdb) break main
Breakpoint 1 at 0x8128: file simple.c, line 31.
(gdb) run
Starting program: .../lab3/code/simple/simple.elf 
Breakpoint 1, main () at simple.c:31
```
Note the last line in the snippet above. 
`gdb` has run the program up to line 31 and stopped,
because we have set a *breakpoint* at the start of `main`.

```
Breakpoint 1, main () at simple.c:31
31  {
(gdb) next
34      int d = diff(x, y);
(gdb) next
35      int f = factorial(7);
(gdb) print d
$1 = 74
```
When we typed the `next` command,
the next line in the program is executed. 
This line calls the function `diff`.
We then use `print` to see the value of `d`
after the call completes. 

It is helpful to note that if you’re paused at a line, 
you’re paused _before_ that line has executed. The call to 
`factorial` is next to come. If you attempt to print `f`
before executing its declaration statement, the debugger
will report that the variable is not (yet) accessible.

Sometimes you want to step into the function being called.
To do this, we use `step` instead of `next`.
Whereas `next` executes the next function call as one unit,
`step` will step into it. Use `run` to restart the program
and then use `step` to when you hit the breakpoint.

```
(gdb) run
The program being debugged has been started already.
Start it from the beginning? (y or n) y

Breakpoint 1, main () at simple.c:31
16	{
(gdb) step
34      int d = diff(x, y);
(gdb) step
diff (a=a@entry=33, b=b@entry=107) at simple.c:27
27      return abs(a - b);
```

We are now stopped at the first line of `diff`.  If you `step` from here, you will enter in the call to the `abs` function.

```
(gdb) step
abs (v=v@entry=-74) at simple.c:5
5   }
```

#### 1b) Using `gdb` to observe the stack

gdb also allows you to drop down to the assembly instructions and keep an eye
on the current values in the registers. Note that although `diff` is a function, not a variable,
gdb allows us to inspect using `diff`.

```
(gdb) print diff
$1 =  {int (int, int)} 0x8108 <diff>
```

The value of `diff` is the address of the beginning of the function, which is
`0x8108`.  Use `delete` to delete any existing breakpoints and set a breakpoint at the `diff` function;

    (gdb) delete
    (gdb) break diff
    Breakpoint 2 at 0x8108: file simple.c, line 26.
    (gdb) run
    Breakpoint 2, diff (a=a@entry=33, b=b@entry=107) at simple.c:26

When a breakpoint is hit, gdb stops the program just before executing the
instruction at that address.

We can print out the ARM instructions by disassembling the function `diff` using the command `disas diff`.
    
```
(gdb) disass diff
Dump of assembler code for function diff:
=> 0x00008108 <+0>:     mov r12, sp
   0x0000810c <+4>:     push    {r11, r12, lr, pc}
   0x00008110 <+8>:     sub r11, r12, #4
   0x00008114 <+12>:    rsb r0, r1, r0
   0x00008118 <+16>:    bl  0x8054 <abs>
   0x0000811c <+20>:    sub sp, r11, #12
   0x00008120 <+24>:    ldm sp, {r11, sp, lr}
   0x00008124 <+28>:    bx  lr
End of assembler dump.
```
Note that the first instruction of `diff` is at the
address `0x8108`, as we expect.

Use the command `info reg` to display all of the current registers.
```
(gdb) info reg
r0             0x21 33
r1             0x6b 107
r2             0x8168 33128
r3             0x8168 33128
r4             0x0  0
r5             0x4a 74
r6             0x0  0
r7             0x0  0
r8             0x0  0
r9             0x0  0
r10            0x0  0
r11            0x7ffffec  134217708
r12            0x7fffff0  134217712
sp             0x7ffffd8  0x7ffffd8
lr             0x8140 33088
pc             0x8108 0x8108 <diff>
cpsr           0x60000013 1610612755
```
What value is in `r0`? Why does `r0` contain that value?

We can access a single
register by using the syntax $regname, e.g. `$r0`.

```
(gdb) print $sp
$9 = (void *) 0x7ffffd8
(gdb) stepi
(gdb) stepi
(gdb) print $sp
$10 = (void *) 0x7ffffc8
```

The `stepi` command executes one assembly language instruction.
Note that the value of `sp` has decreased by 16
after executing the first few instructions in `diff`.

In this case, we're seeing that at the very start of `diff`, `$sp =
0x7ffffd8`, and then it's decreased by 16 after the `push`
instruction that writes the four registers that form the APCS frame. 
(Recall that the stack grows downward; the stack pointer
decreases as more values are pushed.)

`gdb` has a very neat feature that you can always
print out (display) values every time you step.
This is done with the `display` command.

Let's display what's at location in memory pointed to by `sp`:

    (gdb) display /4xw $sp
    1: x/4xw $sp
    0x7ffffc8:      0x07ffffec      0x07ffffd8      0x00008140      0x00008114

This has the effect of displaying 4 words (w) going upward in memory,
beginning at the address `0x7ffffc8`. What is printed is the 4 values 
stored lastmost on the stack.  Those four values were placed there by a
`push` instruction. Examine the disassembly and figure out which four registers
were pushed.

Because we used the `display` command, gdb will reevaluate and print that
same expression after each gdb command. In this way, we can monitor what's on
top of our stack as we step through our program.

Here is [a diagram of the state of memory](images/stack_abs.html)
right before the source line `return `
is executed in the `abs` function. 
The diagram shows the complete address space of 
the `simple` program, including the memory where
the instructions are stored, as well as the contents of the stack.
Reviewing this diagram can be very helpful in learning what
is stored where in your program's address space.

Now use `stepi` to move forward from here and watch what happens to the
stack contents:

    (gdb) stepi
    (gdb) [RETURN]
    (gdb) [RETURN]
    (gdb) [RETURN]

Hitting just [RETURN],
causes `gdb` to repeat the last command (in this case `stepi`).

Watch how the stack changes as you step through the function.
On which instructions does the value of the stack pointer change?

Use `delete` to delete any existing breakpoints. Set a breakpoint on the  `abs` function and re-run the program until you hit this breakpoint.
Use the gdb `backtrace` to show the sequence of function calls leading
to where we are:

```
(gdb) backtrace
#0  abs (v=v@entry=-74) at simple.c:2
#1  0x0000811c in diff (a=a@entry=33, b=b@entry=107) at simple.c:27
#2  0x00008140 in main () at simple.c:34
```

The backtrace shows that the function `abs` has
been called by `diff` from line 27,
which in turn was called by `main` from line 34.
The numbers on the left refer to the *frame*.
The current frame is numbered 0,
and corresponds to the invocation of function `abs`. 
Frames for caller functions
have higher numbers.

    (gdb) info frame
    Stack level 0, frame at 0x7ffffc8:
    pc = 0x8054 in abs (simple.c:2); saved pc = 0x811c
    called by frame at 0x7ffffd8
    source language c.
    Arglist at 0x7ffffc8, args: v=v@entry=-74
    Locals at 0x7ffffc8, Previous frame's sp is 0x7ffffc8
    (gdb) info args
    v = -74
    (gdb) info locals
    result = <optimized out>


`info locals` reports that `result` is optimized out, which is to say
that it did not need to use the stack to store its value.  Where,
then, is the value of `result` being tracked? (Hint: look at the generated
assembly instructions to figure it out!)

We can also inspect the state of our callers.

    (gdb) up
    #1  0x0000811c in diff (a=a@entry=33, b=b@entry=107) at simple.c:28

This moves up to function #1,
which is the function `diff` which called `abs`.

    (gdb) info args
    a = 33
    b = 107
    (gdb) info locals
    No locals.

Now let's go back down the stack frame for `abs`.

    (gdb) down
    #0  abs (v=v@entry=-74) at simple.c:3

Disassemble the code for `abs` and trace its operation instruction
by instruction.

```
(gdb) disass abs
Dump of assembler code for function abs:
=> 0x00008054 <+0>:     mov r12, sp
   0x00008058 <+4>:     push    {r11, r12, lr, pc}
   0x0000805c <+8>:     sub r11, r12, #4
   0x00008060 <+12>:    cmp r0, #0
   0x00008064 <+16>:    rsblt   r0, r0, #0
   0x00008068 <+20>:    sub sp, r11, #12
   0x0000806c <+24>:    ldm sp, {r11, sp, lr}
   0x00008070 <+28>:    bx  lr
End of assembler dump.
```

The first three instructions are the function _prolog_ which set up the
stack frame. Which four registers are pushed to the stack to set up the
APCS frame?  Where/how in the prolog is the frame pointer `fp` anchored?
What location in the stack does the `fp` point to?

The fourth instruction is the body of the `abs` function that does the
comparison operation to determine if v is negative. Where does `abs` read its argument from?  Where does `abs`
 write the return value of the function?

The fifth and sixth instructions are the function epilog. The epilog
is responsible for undoing the stack frame and restoring the
saved values for all caller-owned registers that were overwritten.
The `ldm` instruction ("load multiple") is mostly just a more general-purpose 
variant of `pop`.

The final instruction of `abs` is branch exchange that returns control
to the caller. Who is the caller of `abs`? What is the address of the 
instruction in the caller that will be executed when `abs` returns?

Once you understand the instruction sequence in `abs`, 
examine the disassembly for `diff` and `main`. 
Identify what portions of the prolog and
epilog are common to all three functions and what portions differ.

The `simple` program contains a few other functions that you can observe to see their use of the stack. 

The `factorial` function operates recursively. Set a breakpoint on the base case  `break 10` and run until the breakpoint is hit. Use the `backtrace` command to get the lay of the land. Try moving `up` and `down` and use `info frame` and `info args` to explore the stack frames. Compare what you observe to this [stack diagram](images/stack_factorial.html).

The remaining function of `simple` demonstrates how the stack is used for storage of local variables. Whereas simple variables, e.g. integers, are likely to be stored only in registers, larger data structures, such as arrays and structs, will be placed on the stack. Set a breakpoint on the `make_array` function. Use `info locals` to see the array contents at the start of the function. Are the array elements initialized to any particular value?  Step through the loop a few times and use `info locals` to see how the array is updated. Use this [stack diagram](images/stack_array.html)
 as a road map.

Continue to play around with `gdb`.
It is a great way to learn ARM assembly language,
as well as track down bugs in your program.
It also lets you look at both the C and the assembly language source.

Answer the first question in the [checklist](checklist).

### 2. Serial communication
#### 2a) Loopback test

First, insert the USB serial breakout board into a USB port on your laptop.

Verify that the board appears as a `tty` device
(remember, `tty` stands for teletype)

On a Mac:

    $ ls /dev/tty.SLAB_USBtoUART
    /dev/tty.SLAB_USBtoUART

On Linux:

    $ ls /dev/ttyUSB0
    /dev/ttyUSB0

You have been using the USB-serial breakout board 
to download programs to the Pi. 
To understand what is going on,
let's do a simple *loop back* test.

Remove the RX and TX jumpers between the USB-breakout board and 
from the header on the Pi.

Next, connect TX to RX directly on the USB-breakout board.

In loop back mode,
the signals sent out on the TX pin are wired straight to the RX pin.
This causes characters sent out to be echoed back.

![loop back](images/loopback.jpg)

We will use `screen` to send and receive characters over the tty port.
The command below establishes a connection to the USB-serial breakout 
at the baud rate of 115200.

    Mac:
    $ screen /dev/tty.SLAB_USBtoUART 115200

    Linux:
    $ screen /dev/ttyUSB0 115200

The screen should be cleared and the cursor positioned
in the upper left hand corner.
Type in some characters.  What happens?
What happens if you push return on your keyboard?

To exit screen, type `Ctrl-A` followed by `k`.
You should see the following message.

    Really kill this window? [y/n]

Typing `y` should return you to the shell.

    [screen is terminating]

#### 2b) Echo test

Now, rewire the USB-serial breakout board to the Raspberry Pi 
in the usual way.
Connect TX on the breakout board to RX on the Raspberry Pi.
Also connect RX on the breakout board to TX on the Raspberry Pi.

Change to the directory
`lab3/code/echo`.
`make` the program in that directory and send to the Pi with
the command `rpi-install.py -s echo.bin`.  (Recall that 
invoking `rpi-install.py` with the `-s` flag will automatically
run `screen` after sending the program.) Any characters you now 
type should be echoed back to your terminal.

Unplug the jumper from the RX pin on the USB-serial board. What changes?

Use `Ctrl-A` `k` to exit `screen`.

#### 2c) UART test

Change to the directory
`lab3/code/uart`.
The example in this directory show how to use `uart_putc`
to send characters using the TX pin on the Raspberry Pi. 

    $ cd lab3/code/uart
    $ ls
    Makefile  cstart.c  hello.c   memmap    start.s

Read and understand the source to `hello.c`. Edit `Makefile` to 
change the recipe for the `install` target to
include the `-s` flag when invoking `rpi-install.py`.  Now you can
use `make install` to send the program to the Pi and start `screen`
in one go. 

    % make install
    Found serial port: /dev/cu.SLAB_USBtoUART
    Sending `hello.bin` (1128 bytes): .........
    Successfully sent!
    [screen starting will clear your terminal window here]
    hello, laptop
    hello, laptop
    hello, laptop
    hello, laptop
    hello, laptop
    hello, laptop
    hello, laptop

This will print forever until you reset the Pi. Use `Ctrl-A` `k` to exit `screen`.

#### 2d) printf

Change to the 
`lab3/code/printf` 
directory.
Running the program in this directory will
print "hello, laptop" using `printf`.
Assignment 3 will have you implement your own version of `printf`.

Now that your Pi can communicate with your computer, you can use the
`printf` we provide you in `libpi.a` to debug program state.

For example, you can call `printf("value: %d\n", 10);` 
to print the number 10 or `printf("value: %c\n", 'a');` 
to print the letter a. To learn more about how to use printf, check out
[the documentation for it here](http://www.tutorialspoint.com/c_standard_library/c_function_printf.htm).

Open `hello.c` in your text editor and edit the `main` function to do the following three things.

1. Use `printf` to print out the value in the `FSEL2` register in hex form. 

2. Use `gpio_set_function` to make pins 20 and 21 output pins.

3. Print the value of `FSEL2` again.

Then restart your Pi, and run `make install` again to see your program's output.
Recall that in order to see the value of each bit,
you have to convert the value
that was printed into binary.
Record the answer on the checklist.

### 3. C-strings
Change to the `lab3/code/strings` directory 
and open the `strings.c` file in your editor. This last exercise is writing code to practice with pointers and C-strings and further flexing your unit-testing muscles. 

The two functions we are going to experiment with are `strlen` (to report the length of a string) and `strcpy` (to copy the characters from one string to another). 

We have implemented the `strlen` function for you. First consider how the operation works. How can you determine the number of characters in a string? Is the null terminator included in the count? Now review the given code. The function applies array brackets to its `char *` argument. What does that syntax do in this context?  Is it possible that `strlen` will access an index that is "out of bounds" for the string? Why or why not?

The approach is fairly simple enough that you might feel confident testing `strlen` "by inspection",
but let's use our unit testing strategy to make sure that
the function returns the correct result when actually executing.

The `main` function calls `test_strlen` to run a simple unit test to check that `strlen` returns the correct 
result. Compile and run this program on the Pi and admire the green light
of success. 

Now it's your turn to implement `strcpy`, which copies the characters from one string to another:

    char *strcpy(char *dst, const char *src);

The function `strcpy` is supplied in the 
standard C library (for those spoiled programmers who work on hosted systems),
but you are going to implement it for your Pi. If you aren't sure of the 
function's interface, trying reading its man page:

    $ man strcpy

After you have working draft version of `strcpy`, uncomment the call 
to `test_strcpy` in `main`. 
Build and run on your Pi to see how your implementation fares on the
unit tests. If you get the red flash of doom, dig in to find 
out what's gone wrong and resolve the issue. Don't move on until you
earned the green badge of honor.

Now, let's run this same program under the debugger to get more
practice running gdb in simulation mode. You can make your 
job a little easier by first creating a file named `.gdbinit` in the current directory. The file should contain these four commands:

    target sim
    load
    b pi_abort
    b main

This configuration file is read when you start gdb. The above commands
will set up the simulator and put breakpoints on `main()` and `pi_abort()` 
(`pi_abort` is called when a unit test fails).

Start gdb on the `strings`
program and use step/next/print and so on to trace through a run
of your program executing correctly.

Now edit your code to intentionally 
plant a bug, such as changing strlen to always return 5. Rebuild and run 
under the debugger to learn how a unit test failure will be observed.

When `pi_abort` is called, it will valiantly set and clear the GPIO 
for the red on-Pi LED, but remember there is no Pi here and the simulator
does not emulate any of the peripheral behavior. Instead of fruitlessly 
looking for a sign from a non-existent LED, we will use a breakpoint 
on `pi_abort` (as you did in `.gdbinit`) to be notified of a failed unit test.

At this point, both `strlen` and `strcpy` should work correctly for any
valid call. We are now going to do a little exploration into what happens 
for calls that are not so kosher. 

Review the code in the aptly-named `bogus_strlen_calls` function. 
Get together with your tablemates and analyze the three "bogus" calls.
For each consider __why__ it is invalid.

As a rule, the standard C-string
library functions don't offer much in the way of terms of error-reporting.
The reason for this is not out of spite or laziness-- it's actually
not possible for `strlen` to reliably detect that its argument is
not a valid C-string. Given a bad call, the function is just 
going to blunder through its execution. With that fact in mind, 
for each of the three bogus calls, what will happen when you execute them?

Uncomment the call to `stress_test_strlen` in `main`, rebuild, and run the
program under gdb. Single step through the call to `bogus_strlen_calls`
and trace what value is returned from each of the bad calls. Do you get the result you anticipated?  What are the observed consequences of reading uninitialized or invalid memory?

Now move on to review the code for the `sketchy_strcpy_call` function.  The `stress_test_strcpy` makes three
calls, two of which execute without harm, but that one in the middle is
bad news. Just as with `strlen`, `strcpy` has no means to detect or
report that the arguments it was given are invalid.  And whereas `strlen` 
was able carry on and silently blunder through __reading__ from an 
improper memory location, what is going to happen when `strcpy` starts
 __writing__ to one?  Have you and your partner draw a diagram on paper of
what is happening with the stack memory during this disastrous call.

Comment out the call to `stress_test_strlen`, uncomment the call to 
`stress_test_strcpy`, and rebuild. Run under `gdb`
to trace through the execution. The sketchy call doesn't appear to
fail the unit test as the program won't hit the breakpoint set on
`pi_abort`, but yet this test did fail and in a rather spectacular way.

This code exhibits a classic __buffer overflow__ bug where writing
too much data to a too-small stack buffer overwrites adjacent data
in the stack frame. What is the critical data stored in the stack that has
been destroyed here? At what point in the execution does the
overwritten data result in a bad consequence?

## Checkoff with TA

Make sure you walk through the [checklist questions](checklist)
with a TA before you leave. The TA will ensure you are properly credited
for your work.
