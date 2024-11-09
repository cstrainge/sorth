![Logo](./strange-forth/assets/strange-forth-logo.png)

# Strange Forth

## Getting started

A simple implementation of the language Forth.  This is a interpreted version of Forth.  This version is aware of the standard ANS Forth however we don't adhere to it very closely.  We've taken liberties to follow some idioms from newer languages where the author finds it enhances readability.  It's acknowledged that this is an entirely subjective standard.

The second reason why this version deviates from most Forth implementations is that this version doesn't compile to machine code.  Instead this version compiles and runs as a bytecode interpreter, and also makes use of exceptions and features available in newer versions of C++ as opposed to mostly being implemented in assembly.

The knock on effects of these decisions is that this version of Forth may be slower than other compiled versions.  The intention of this is for this version to act more as a scripting language rather than as a compiler/executable generator with optional REPL.

On Windows make sure you have `vcpkg` installed, (it helps if you are running from within a Dev Studio command prompt,) and run:

```
mkdir build ; cd build
cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\vcpkg\scripts\buildsystems\vcpkg.cmake --preset=cibuild .. -G Ninja
```

On Linux and MacOS make sure you have `libffi-dev` installed and then you can run:

```
mkdir build ; cd build
cmake .. -G Ninja
```

You will end up with a runnable binary in the build directory off of the project root.

You can give the binary a quick spin running with the tests script, ex:

```
./sorth ../tests.f
```

For more details with getting started and playing with the language please see the [Wiki](https://github.com/cstrainge/sorth/wiki)


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
