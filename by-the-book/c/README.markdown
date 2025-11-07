# Bytecode Virtual Machine in C

Based off Crafting Interpreters by Bob Nystrom with support for additional features:

- Integers and complex numbers
- Arrays and hashmaps are built-in
- C-like string syntax
- Bitwise arithmetic
- Runtime `type()` function
- Improved printing features

```bash
$ make && ./main -h
```

## Run all tests

```
make commitready
make fuzz memory-check memory-fuzz  # These are slower
```

### Tests and code quality

`-Wall` and `-Wpedantic` warnings are enabled.

`-Werror=switch` ensures that switch statements are exhaustive and reduces runtime errors.

Various types of tests are implemented for each language feature and to find memory leaks:

```
make unit         # run unit tests
make integration  # run the compiler on various Lox files
make fuzz         # generate a large random Lox program with various types of operations
make memory-check # run tests with valgrind to find leaks
make memory-fuzz  # run fuzz tests with valgrind
```

## Sample code

- [Big Integers implemented in lox](tests/eval/bigint.lox)
- [The Fast Fourier Transform algorithm in lox](tests/eval/fft.lox)
