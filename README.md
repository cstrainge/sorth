![Logo](./strange-forth/assets/strange-forth-logo.png)

# Strange Forth

## Getting started

A simple implementation of the language Forth.  This is a interpreted version of Forth.  This version is aware of the standard ANS Forth however we don't adhere to it very closely.  We've taken liberties to follow some idioms from newer languages where the author finds it enhances readability.  It's acknowledged that this is an entirely subjective standard.

The second reason why this version deviates from most Forth implementations is that this version doesn't compile to machine code.  Instead this version compiles and runs as a bytecode interpreter, and also makes use of exceptions and features available in newer versions of C++ as opposed to mostly being implemented in assembly.

The knock on effects of these decisions is that this version of Forth may be slower than other compiled versions.  The intention of this is for this version to act more as a scripting language rather than as a compiler/executable generator with optional REPL.

On Windows make sure you have `vcpkg` installed, (it helps if you are running from within a Dev Studio command prompt,) and run:

```
mkdir build ; cd build
cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\vcpkg\scripts\buildsystems\vcpkg.cmake --preset=default .. -G Ninja
```

On Linux and MacOS make sure you have `libffi-dev` installed and then you can run:

```
mkdir build ; cd build
cmake .. -G Ninja
```

You will end up with a runnable binary in the build directory off of the project root.

You can give the binary a quick spin running with the tests script, ex:

```
../dist/sorth ../tests.f
```

For more details with getting started and playing with the language please see the [Wiki](https://github.com/cstrainge/sorth/wiki)


## JIT Support

If LLVM is available on your system the CMake script will attempt to build Strange Forth with JIT support enabled.

Note that this has only been tested against LLVM version 19 and expects that LLVM was compiled with shared libraries enabled.

When compiled with JIT support, it is disabled by default.  So, to enable the JIT engine set the environment variable `SORTH_EXE_MODE=jit`.  When you run the REPL you should see the line in the startup banner:

```
Execution Mode: jit
```

That indicates that all user functions are JIT compiled, including code you enter into the REPL.


## Experimental Implementations

There are three experimental versions of the language being worked on.

 - [C# Version](https://github.com/cstrainge/sorth.net)  There is a version written in C# that jit
   compiles the code using .net's IL code generation facilities.  One future experiment would be to
   build a way to access any arbitrary .net struct or class and it's members.  The syntax for this
   is still being decided on.  This version supports threads, but not the WIP FFI.  I feel the
   approach to extensibility should be specific to this implementation.
 - [Rust Version](https://github.com/cstrainge/rsorth)  The Rust based version is meant to take
   advantage of it's memory safety grantees.  One potential avenue for exploration would be to
   separate out the interpreter code and define a public API.  Then it could be published on
   Crates.io for potential embedding within other programs.  How to properly distribute the standard
   library in this case so that it's useable by client programs is TBD.  Perhaps as a feature that
   includes the code in the binary?
 - [AOT Compiler Version](https://github.com/cstrainge/sorthc/tree/main)  A full ahead of time
   compiler for the language.  It's still catching up in terms of how much of the language it can
   compile, but it's compiling a lot of code and the standard library.  It has basically 2
   run-times, one that lives inside the compiler itself that's more limited than the run-time that
   executes the compiled code.  All immediate words run within the compiler's run-time and all other
   words execute in the compiled run-time.

Neither version is up to full feature parity with the C++ version.  It's also being decided if the
Rust version will eventually replace the C++ version.

I feel this illustrates one of the benefits of the language being byte-code based.  It was quite
easy to create the other versions of the interpreter.  It was also equally easy to introduce a IL
based JIT backend for the C# version.

Due to the nature of Forth, we expose many internal details of the interpreter to user code.  It is
in this way that user code can define major language features, unmodified across implementations.


## Future plans.

There is still a lot to do with the language... Non exhaustively some things that would really help the usefulness of the language...

 - [x] The language has the start of a FFI.  But better interop with other languages is a must.
 - [ ] Continue to build out the io words.  For example, currently we can not create IPC servers with the language.
 - [ ] Take a stability pass on the REPL.  There are known bugs with single/multi-line editing modes.
 - [ ] Also, take a stability pass on the language server.  We've introduced threads to the language so we should use that support within the LSP implementation.
 - [ ] In that light, we need to identify and eliminate bugs.  We need to elevate the test script to a full testing system and integrate it with our CI processes.
 - [ ] Take a pass through the existing words.  Make sure that they make sense with each other.  Fill out APIs and make sure what we have is nicely ergonomic.
 - [ ] Perhaps look at adding a module system?  We have includes, but everything ends up in the global namespace.  But perhaps that's better for a language like Forth?
 - [ ] Look into maybe implementing some kind of packaging/downloading system to allow the easy sharing of code?
