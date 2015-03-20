# cliopts - Single source/header option parsing

cliopts is a small option parsing library for both C and C++. It offers an
ANSI C ("C89") implementation for C and a more advanced idiomatic C++ based
header-only implementation for C++.

The API for this library is modified after GLib's
[GOption](https://developer.gnome.org/glib/stable/glib-Commandline-option-parser.html)
interface (which itself is modelled after [popt](http://rpm5.org/files/popt/).
However, I wanted something easily embeddable.

## Building

A `Makefile` is provided for GNU Make. A `CMakeLists.txt` is provided as well
for compilation on any system with CMake. Since the library _only_ depends
on standard C library features, this should work anywhere (I've tested this on
Windows and various unices).

## Using

See the `c-example.c` and `cxx-example.cpp` for examples using the C and C++
interfaces respectively.

The default behavior of the parser is to print a help message and exit the
program upon error. This is suitable for most command line applications,
and can be disabled by passing a custom `cliopts_extra_settings` structure.

### Using in C

Declare and statically initialize an array of `cliopts_entry` structures
containing describing each of your options. Typically you will need to declare
the actual target pointers ahead of the declarations (the example is very
useful for this). The last entry should be an empty structure, e.g.

```c
int a_number = 42; // default value is 42
char *a_string = "Hello"; // default value is "hello"

cliopts_entry entries[] = {
    {'i', "int-argument", CLIOPTS_ARGT_INT, &a_number, "help message for this option"},
    {'s', "string-argument", CLIOPTS_ARG_STRING, &a_string, "help message for this option"},
    {0}
}
```

The `cliopts_parse_options` should be called to process the arguments. If
everything goes well, the various variable pointers will contain the proper
value (if found on the command line, it will contain the new value, or if not
found, will remain unchanced).

```c
int main(int argc, char **argv)
{
    cliopts_parse_options(entries, argc, argv, NULL, NULL);
    printf("Number is now: %d\n", a_number);
    printf("String is now: %s\n", a_string);
    return 0;
}
```

If the program is invoked as `./a.out --int-argument 99` then `a_number` will
be 99, and `a_string` will remain "Hello".

### Using in C++

The C++ API builds upon the C interface, eliminating the need to explicitly
define your container variables before the option is declared. You should
also define the `CLIOPTS_ENABLE_CXX` macro before including the header file
to be able to use the C++ API.

To define an option entry, create an instance of one of the option classes:

```c++
cliopts::IntOption int_option("int-argument");
cliopts::StringOption string_option("string-argument");
```

To modify attributes of the option (for example, to give it a short
single-character name), call the appropriate member functions:

```c++
int_option.abbrev('i').setDefault(42);
string_option.abbrev('s').setDefault("Hello");
```

You can also use method chaining (as in the example file):

```c++
cliopts::IntOption int_option = cliopts::IntOption("int-argument)
    .abbrev('i')
    .setDefault(42);
```

To parse the options, create a parser and add the option objects to it:

```c++
cliopts::Parser parser("my-app-name");
parser.addOption(int_option);
parser.addOption(string_option);
```

To actually parse the options, invoke the `parse` member function.

```c++
parser.parse(argc, argv)
printf("Int option is: %d\n", int_option.result());
printf("String option is: %s\n", string_option.result().c_str());
```

### Memory Usage

Parsed string values are _copied_ to their destination (via `malloc`). In
the future I may add an option to simply store the pointer from `argv`.

# Author & Copyright

Copyright (C) 2012-2015 Mark Nunberg. See `LICENSE` file for licensing.
