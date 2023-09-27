# Strange Forth

This language extension supports a variant of Forth, Strange Forth.

## Features

This is still early days for the language and the extension.  Currently `sorth`, or Strange Forth only runs on MacOS and Linux.  Both on x86-64 and Arm64.

See [the Strange Forth repository](https://github.com/cstrainge/sorth/tree/main) for details on the language itself and to acquire the interpreter for local development.

One mark of the language's growing maturity is that the language server itself is written in Strange Forth, check out [the language server](https://github.com/cstrainge/sorth/tree/main/strange-forth/server) starting with `language_server.f` if you're interested.

## Requirements

MacOS or Linux, (currently only tested with Ubuntu,) on x86-64 and Arm64

### 0.1.6

Start of Windows support.

### 0.1.0 - 0.1.5

Fixed some small problems with the Linux build.

Will generate symbols for structs, variables, and constants now.

Added logo.

Updated the Readme with more information about the status of the project.

Cleaned up some of the logging and made it easier to debug locally.

### 0.0.0

First release.  The extension supports simple word lookup but not too much more yet.
