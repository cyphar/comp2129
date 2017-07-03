## `tests/` ##

This testing framework is fairly simple. Each subdirectory represents a single
test, and each test can contain multiple stages. The stages are executed in
numerical order and the output is compared to the expected output for that
stage (this allows for testing of `SAVE` and `LOAD` between different instances
of `./atoms`).

In order to run a single test, call `./run.sh <test>` where `<test>` is the
directory of the test. `run.sh` will output a report and return a non-zero
error code in the case of a failure. See `0000_template` for an example of the
most minimal test.
