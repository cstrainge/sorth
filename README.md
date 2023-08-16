
# Strange Forth

A simple implementation of the language Forth.  This is a interpreted version of Forth.  This
version is aware of the standard ANS Forth however we don't adhere to it very closely.  We've taken
liberties to follow some idioms from newer languages where the author finds it enhances readability.
It's acknowledged that this is an entirely subjective standard.

The second reason why this version deviates from most Forth implementations is that this version
doesn't compile to machine code.  Instead this version compiles and runs as a bytecode interpreter,
and also makes use of exceptions and features available in newer versions of C++ as opposed to
mostly being implemented in assembly.

The knock on effects of these decisions is that this version of Forth may be slower than other
compiled versions.  The intention of this is for this version to act more as a scripting language
rather than as a compiler/executable generator with optional REPL.

Compile with:

```
clang++ -std=c++20 sorth.cpp -o sorth
```

Or use `g++` with the same parameters.

To enter the REPL simply run the `sorth` executable.  Pass a file name to run that file as a script.

Ex: `sorth tests.f`


## A Quick Word About Forth

As mentioned this version loosely follows standard Forth.  One deviation is that strings do not need
a trailing space after the `"` character.  Functions are called words in Forth, data is generally
passed around on the stack.

The language does have constants and variables as well.  Variables are defined by the word
`variable` followed by the name of the new variable.

So, to define a variable named `my_variable` you would write:

```
variable my_variable
```

Reading and writing variables is performed
by the words '@' and '!' respectively.  So, for example to read from a variable, `my_variable` and
place it's value onto the stack one would write:

```
my_variable @
```

This places the index of the variable onto the stack and then calls the word `@` to perform the
actual read.

To write a value, say 1024 to that variable one would write:

```
1024 my_variable !
```

This writes the constant value 1024 onto the stack, then the index of `my_variable` is also pushed.
The word `!` will pop both of those things and write the value to the variable in memory.

Calling words in Forth is done by writing it's name in your script or on the REPL.  The
documentation for the word should describe what the word will take form the stack and what it'll
put back.

Math is handled in a similar way.  Values are poped off of the stack, the math is performed and the
result is pushed back on.  For example to add two numbers and print out the result with a carrage
return:

```
10 24 + .cr
```

This version also supports building strings with the word `+`.

This page is only a quick overview of the language, there are many tutorials on the Forth language
out there and we've just barely scratched the surface here.  One thing to keep in mind, most Forths
use uppercase for it's keywords where lower case is used here.

For example, `VARIABLE my_variable`, instead of, `variable my_variable`.


## Creating New Words

New words are created with the colon keyword, `:` and finished off with the semi-colon, `;`.  For
example:

```
: hello "Hello world!" .cr ;
```

And you execute that word like any other word by typing `hello`.  If you include the keyword
`immediate` after the name, then that word will run during the compile time instead of at run time.


## Built-in Words

Following is the descriptions of the words built into this implementation of the language.


### Control Structures

In Forth control structures are words and can actually be redefined!


#### if else then

The if works by taking a value off of the stack and evaluating and if true, executes the if clause
otherwise if an else clause is given, that is executed instead.

```
perform_test

if
    "The test was successful."
else
    "The test failed!"
then
```


#### begin until and begin while loops

The `begin until` loop iterates until a given condition is evaluated to be true.  The following
example loops 10 times printing out a message:

```
10                      ( Push a 10 onto the stack to act as our loop index. )
begin
    "Looping until:" .  ( Print our message )
    --                  ( Subtract 1 from the value at the top of the stack. )
    dup .cr             ( Duplicate the value preserving it and print out the copy. )

    dup 0<=             ( Duplicate the loop index and make sure it's greater or equal to 0. )
until                   ( Once we hit zero the loop will then exit. )

drop                    ( We're done with the loop index so drop the copy off of the stack. )
```

Will print the following on the console:

```
Looping until: 9
Looping until: 8
Looping until: 7
Looping until: 6
Looping until: 5
Looping until: 4
Looping until: 3
Looping until: 2
Looping until: 1
Looping until: 0
```

The `begin while` loop on the other hand continues the loop for as long as the condition is true.
The next example will loop 10 times like the previous one.

```
10
begin
    -- dup 0>=             ( Keep looping while this statement evaluates to true. )
    while
        "Looping while:" . ( Print our message and... )
        dup .cr            ( the index. )
repeat

drop
```

Will produce the following output:

```
Looping while: 9
Looping while: 8
Looping while: 7
Looping while: 6
Looping while: 5
Looping while: 4
Looping while: 3
Looping while: 2
Looping while: 1
Looping while: 0
```

#### case of

A case statement is introduced with the word `case` and the various clauses by the word `of`.  Each
clause is terminated by a `endof` while the whole thing is terminated by `endcase`.  The case will
pull a value off of the stack, and check for equality against all of the given clauses.  If there
is a match that block is executed.  If there are no matches the default block is executed.

So for example, the word we define here `hello_something` will print a hello message depending on
the number passed in by the stack:

```
: hello_something
    "Hello" .

    case
        1 of "world."        endof
        2 of "solar system." endof
        3 of "universe!"     endof

        "everything else!"
    endcase

    .cr
;
```

When called like the following:

```
1 hello_something
2 hello_something
3 hello_something
1024 hello_something
```

This script will produce the output:

```
Hello world.
Hello solar system.
Hello universe!
Hello everything else!
```


### Interpreter Control

`quit`

Quits the interpreter.  If there is a number at the top of the stack, that will be the exit value
returned by the process.

For example to indicate failure from your script you would call `exit_failure quit`.


`reset`

Used in the REPL.  Call reset to clear the REPL back to it's default state as if it had just started
up.  This word takes no parameters.


`include`

Include another file your script or load it's contents into the running REPL.

```
"my_file.f" include
```

Will load and execute the contents of `my_file.f` at that point in the script.


word
`

### Math and Logical Operations

`+` `-` `*` `/`

These words will all take 2 values off of the stack and will add, subtract, multiply, and divide
respectively.

`++` `--`

Will increment and decrement the value at the top of the stack.


`+!` `-!`

Will increment and decrement the value within a variable without needing to read the value first.

```
my_variable +!
```


`and` `or` `xor` `not`

Perform standard bitwise operations with the top two values on the stack.

`<<` `>>`

Shift the top value over by the specified number of bits, for example, `0x1 8 << .hex cr`.  Will
print the hex value `100` on the console.


`=` `<>` `>=` `<=` `>` `<`

Standard equality operations.  The top two values are poped off of the stack and compared.  A
boolean result is then pushed back onto the stack.


`0>` `0=` `0<` `0>=` `0<=`

Are like their standard counterparts except that it is only the top stack value and it's compared
with zero.


### Stack operations

`dup`

Duplicate the top value of the stack pushing both copies back.


`drop`

Drop the top value of the stack, discarding it.


`swap`

Pop the top two items from the stack and push them back in reverse order.


`over`

Pop the top two items from the stack and sandwich the second between the first.

```
2
1
```

Becomes

```
1
2
1
```


# Debug and Printing Support

`.`
Print the value at the top of the stack followed by a space.


`cr`

Print a carriage return to the stack.


`.cr`

Print the value at the top of stack and also insert a new line into the output.


`.hex`

Print the top number as a hexadecimal number.


`?`

Print the value stored in a variable without having to directly read it.

```
my_variable ?
```


`.sp`

Print a given number of spaces to the console.  `10 .sp` will print 10 spaces.


`.hcr`

Print the top value as a hex value and also print a line feed.


`.s`

Print all of the current values in the stack without changing it.


`.w`

Print all of the currently defined words.


<boolean> `show_bytecode`

Show bytecode as it is generated by the interpreter.  The default is false.

For example the code `10 20 + .cr` will produce something like the following:

```
0 push_constant_value 10
1 push_constant_value 20
2 execute             26
3 execute             71
```


<boolean> `show_run_code`

Will show the interpreter's bytecode as its being executed.


`show_word` <word_name>

Will print the bytecode for a given word.  (If it's not a word defined in C++).  Normally only word
indicies are stored in the byte code.  But this word will attempt to resolve and print the names
for all execute instructions.  For example `show_word if` will produce something like:

```
Word: if
     0 def_variable        if_fail_label
     1 execute             unique_str
     2 execute             if_fail_label
     3 write_variable      0
     4 execute             code.new_block
     5 execute             if_fail_label
     6 read_variable       0
     7 execute             op.jump_if_zero
     8 push_constant_value else
     9 push_constant_value then
    10 push_constant_value 2
    11 execute             code.compile_until_words
    12 push_constant_value then
    13 execute             =
    14 jump_if_zero        5
    15 execute             if_fail_label
    16 read_variable       0
    17 execute             op.jump_target
    18 jump                19
    19 jump_target         0
    20 def_variable        then_label
    21 execute             unique_str
    22 execute             then_label
    23 write_variable      0
    24 execute             then_label
    25 read_variable       0
    26 execute             op.jump
    27 execute             if_fail_label
    28 read_variable       0
    29 execute             op.jump_target
    30 push_constant_value then
    31 push_constant_value 1
    32 execute             code.compile_until_words
    33 execute             drop
    34 execute             then_label
    35 read_variable       0
    36 execute             op.jump_target
    37 jump_target         0
    38 execute             code.resolve_jumps
    39 execute             code.merge_stack_block
```


### Code Generation

The interpreter supports manually generating bytecode from words marked as immediate.  The bytecode
generation process will be documented more fully in the future.  For examples on how to use these
words see `std.f` as some of the core keywords are defined there.  See the function `execute_code`
in the file `sorth.cpp` to see how these instructions are actually implemented.


`unique_str`

Will generate a new string and push it onto the stack each time it's called.  This is useful
epically for jump generation.


#### op.* Words

All of the `op.*` words will generate exactly one instruction in the bytecode construction stack.
These words almost all take a parameter from the stack to encode into the instruction.

variable name `op.def_variable` - Define a new variable and word with the given name.

constant name `op.def_constant` - Define a new constant with the given name.

`op.read_variable` - Pop the index of a variable and write it's value to the top of stack.

`op.write_variable` - Pop the index of a variable and the new value from the stack and write it to
that index.

word index `op.execute` - Execute the instruction at the given index.  Use the word backtick, ``` to
find the index of a word.  For example to generate a call to `dup` call:

```
` dup op.execute
```

value to push `op.push_constant_value` - Push a value onto the stack.

name of jump target `op.jump` - Always jump to the target.

name of jump target `op.jump_if_zero` - Jump if the top of stack is zero or false.

name of jump target `op.jump_if_not_zero` - Jump if the top of stack is not zero.

name of jump target `op.jump_target` - Acts as the target of another jump.  Multiple jumps can target
the same jump target.


#### code.* Words

`code.new_block` - Create a new code generation block at the top of the generation stack.

`code.merge_stack_block` - Merge the code generation block at the top of the stack into the one
below it.

`code.resolve_jumps` - Resolve all named jumps and jump targets in the block at the top of the stack
to their numerical offsets.

names, count `code.compile_until_words` - Generate code for the tokens in the current input token
stream until one of the given words is matched.

boolean `code.insert_at_front` - When true, new instructions are inserted at the beginning of the
current code block.  False they are inserted at the end.  Default is false.
