
# Strange Forth

## Geting started

A simple implementation of the language Forth.  This is a interpreted version of Forth.  This version is aware of the standard ANS Forth however we don't adhere to it very closely.  We've taken liberties to follow some idioms from newer languages where the author finds it enhances readability.  It's acknowledged that this is an entirely subjective standard.

The second reason why this version deviates from most Forth implementations is that this version doesn't compile to machine code.  Instead this version compiles and runs as a bytecode interpreter, and also makes use of exceptions and features available in newer versions of C++ as opposed to mostly being implemented in assembly.

The knock on effects of these decisions is that this version of Forth may be slower than other compiled versions.  The intention of this is for this version to act more as a scripting language rather than as a compiler/executable generator with optional REPL.

Compile with:

```
make default
```

You will end up with a runnable binary in the build directory off of the project root.

You can give the binary a quick spin running with the tests script, ex:

```
.\build\sorth tests.f
```

For more details with getting started and playing with the language please see the [Wiki](https://github.com/cstrainge/sorth/wiki)


## Future plans.

There is still a lot to do with the language... Non exhaustively some things that would really help the usefulness of the language...

 - [ ] The language needs a module system and a C interface where we can load external code.  Currently the only way to extend the language is through scripts and by editing the interpreter itself.  So interop with ohter languages is a must.
 - [ ] Continue to build out the io words.  For example, currently we can not create IPC servers with the language.
 - [ ] Refactor the fancy repl.  It works as a first cut byt there are way too many corner cases it doesn't handle.
 - [ ] In that light, we need to identify and eliminate bugs.  We need to elevate the test script to a full testing system and integrate it with our CI processes.
