# Strange Forth

This language extension supports a variant of Forth, Strange Forth.

## Features

This is still early days for the language and the extension.  Currently `sorth`, or Strange Forth  runs on macOS, Linux and Windows.  Both on x86-64 and Arm64.

See [the Strange Forth repository](https://github.com/cstrainge/sorth/tree/main) for details on the language itself and to acquire the interpreter for local development.

One mark of the language's growing maturity is that the language server itself is written in Strange Forth, check out [the language server](https://github.com/cstrainge/sorth/tree/main/strange-forth/server) starting with `language_server.f` if you're interested.

## Wiki

Also, there is a [Wiki!](https://github.com/cstrainge/sorth/wiki) where we go over details of the language and help you get started with the interpreter.

## Requirements

MacOS or Linux, (currently only tested with Ubuntu,) on x86-64 and Arm64

### 0.1.16

Many changes and fixes to the language, see the changelog for more details.

### 0.1.10

Fixed multi-line comments syntax highlighting.

### 0.1.9

Some minor fixes.

### 0.1.8

Test packaging.

### 0.1.7

Windows support up to par.  Also added a wiki to the project to help describe the language.


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
