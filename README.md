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

Note: for MacOS, you need to self-sign the binary under test to enable ptrace-like functionality (TODO: write instructions for how to do that here)

## Usage
To use:

```./isolate --binary /path/to/binary --function-address address```

You will be prompted with concrete values to assign to parameters.
