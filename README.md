# isolate

`isolate` is a tool that helps you test functions in a binary individually. Functions that are nested in deep codepaths are hard to test/debug, since triggering the codepath requires a very specific program state. In this case, given a binary, function within that binary, and concrete argument values, `isolate` will automatically launch a debugger at the desired function with memory/registers initialized to provided values.

Currently supported platforms:

* MacOS
* GNU/Linux

## Usage

To build:

Note: for MacOS, you need to self-sign the binary under test (write instructions for how to do that here)
