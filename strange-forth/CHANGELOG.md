# Change Log

Latest updates to the published version of this package.

### 0.1.18

Many changes an enhancements, including:

* The start of a ffi.
* Moved to CMake, including using VCPKG on Windows for the libffi dependency.
* Many updates to the user docs.
* Many useful helper words have been added.


### 0.1.17

Updated language server for the changes and fixes introduced in v0.1.16.

### 0.1.16

#### Many updates to the language:

* Added binary literals.
* Added terminal words.
* Added the word string.format
* Added the word %.
* Struct words can no be hidden.
* Automatically define more convenience words for accessing struct fields like struct.field!! and struct.field@@
* Added multi-line strings.

* Rewrote the repl.
    * Added an experimental multi-line mode.
    * History saving and restoring is more robust.  It's also .sorth_history.json now.
* Made the printing of arrays and hash tables nicer.
* Made the stack dump nicer.
* Made bytecode output cleaner.

* Started work on an extension system.  (Currently only working in Windows.)
    * Including a C based API for extension from other languages.

* Fixed bugs with json generation.
* Fixed bugs with exception handling.
* Minor fixes and cleanups.
* Made the syntax highlighting more robust.
* Minor build cleanups and optimizations.

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

### 0.1.5

Fixed some issues with the Linux build.

### 0.1.4

Will generate symbols for structs, variables, and constants now.

### 0.1.3

Added logo.

### 0.1.2

Updated the Readme with more information about the status of the project.

### 0.1.0

Cleaned up some of the logging and made it easier to debug locally.

### 0.0.0

First release.  The extension supports simple word lookup but not too much more yet.
