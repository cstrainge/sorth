# Strange Forth

This language extension supports a variant of Forth, Strange Forth.

## Features

This is still early days for the language and the extension.  Currently `sorth`, or Strange Forth  runs on macOS, Linux and Windows.  Both on x86-64 and Arm64.

See [the Strange Forth repository](https://github.com/cstrainge/sorth/tree/main) for details on the language itself and to acquire the interpreter for local development.

One mark of the language's growing maturity is that the language server itself is written in Strange Forth, check out [the language server](https://github.com/cstrainge/sorth/tree/main/strange-forth/server) starting with `language_server.f` if you're interested.

There is an experimental runtime written in C# where the bytecode is JITed.  You can find it
[at the C# repo](https://github.com/cstrainge/sorth.net)

## Wiki

Also, there is a [Wiki!](https://github.com/cstrainge/sorth/wiki) where we go over details of the language and help you get started with the interpreter.

## Requirements

macOS, Linux, (currently only tested with Ubuntu,) on x86-64 and Arm64, and Windows on x86-64.
