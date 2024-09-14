# isolate

`isolate` is a tool that helps you test functions in a binary individually. Functions that are nested in deep codepaths can be hard to test/debug, since triggering the codepath requires a very specific program state. In this case, given a binary, function within that binary, and concrete argument values, `isolate` will automatically launch a debugger at the desired function with memory/registers initialized to provided values.

Currently supported platforms:

* MacOS
* GNU/Linux

## Installation
To build:

1. `mkdir build`

2. `cd build`

3. `cmake ..`

4. `make`

### MacOS specifics
MacOS requires you to run a code-signed binary (with specific entitlements) as root in order to access functions like `task_for_pid()`. Therefore, you need to self-sign the binary to enable ptrace-like functionality. To do this, generate a cert using Keychain Access (name it `isolate`):

1. Open Keychain Access > Certificate Assistant > Create a Certificate. Name it `isolate`.

2. Click "Let me override defaults".

3. Use defaults, except in the Extended Key Usage Extension menu, enable "Code Signing".

4. Perform a normal build with CMake.

NOTE: you must be root for this to work.

## Usage
To use:

```./isolate --binary /path/to/binary --function-address address```

You will be prompted with concrete values to assign to parameters.
