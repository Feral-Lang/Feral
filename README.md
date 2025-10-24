# About

[![Build Status](https://api.cirrus-ci.com/github/Feral-Lang/Feral.svg?branch=master)](https://cirrus-ci.com/github/Feral-Lang/Feral)

Feral is a dynamically typed, imperative, interpreted language which revolves (to most extent) around the idea of minimalism.

The primary example being that the language syntax itself does not contain anything related to imports, structure, or enums.
Instead, there are libraries/functions that allow the user to import modules, and create structures as well as enums.

For feral, all imports, structures, enums, and functions are variables. This makes all of them a first class citizen.
One can pass and modify all of those around in functions, etc, just like a normal variable.

Do note that Feral is not an object oriented programming language, but does support structs and "associated" (member) functions for them.
```py
let Struct = struct(member = 5);
let instance = Struct(); # default instantiation
instance.member = 10;
```
This makes the code a bit cleaner and more pleasant to use. See examples to understand its usage.

There is also a (WIP) book/guide for Feral available here: [https://feral-lang.github.io/Book/](https://feral-lang.github.io/Book/) ([source](https://github.com/Feral-Lang/Book)).

# Examples

## Hello World

```py
let io = import('std/io');
io.println('Hello World');
```

## Hello greeting using a function

```py
let io = import('std/io');

let helloFn = fn(name) {
    io.println('Hello ', name);
};

helloFn('Electrux'); # prints 'Hello Electrux`
```

## Simple factorial of 5 using a function
```py
let io = import('std/io');

let facto = fn(num) {
    let fact = 1;
    for i in range(num, 1, -1) {
        fact *= i;
    }
    return fact;
};

io.println('factorial of 5 is: ', facto(5));
```

## Creating an empty struct
```py
let structTy = struct(); # empty structure type (struct with no fields)
```

## Creating a struct with fields
```py
# fields `a` and `b` of type integers having default values `10`, and `20` respectively
let structTy = struct(a = 10, b = 20);
```
To create objects of this structure:
```py
# default values for struct fields
let structObj1 = structTy(); # a = 10, b = 20

# overwrite first field's value (a)
let structObj2 = structTy(30); # a = 30, b = 20

# overwrite using assigned argument
let structObj3 = structTy(b = 30); # a = 10, b = 30
```

# Installation

## Prerequisites

To install `Feral`, the following packages are required:
* CMake (build system - for compiling the project)

## Automated Build (Unix-like OS)

You can automatically build Feral and its standard library by downloading and running `build.sh`.
It requires [Git](https://git-scm.com/) and the packages listed under [Prerequisites](#prerequisites).

```sh
# Download the script (example using wget:)
wget https://raw.githubusercontent.com/Feral-Lang/Feral/master/build.sh
# Run it!
sh build.sh
```

## Manual Build

Once the prerequisites have been met, clone this repository:
```
git clone https://github.com/Feral-Lang/Feral.git
```

Inside the repository, create a directory (say `build`), `cd` in it and run the commands for building and installing Feral:
```sh
cd Feral && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release # optionally PREFIX_DIR=<dir> can be set before this
cmake --build . --config Release --parallel=8 --target install
```

On Windows, the first cmake command must have this argument as well: ` -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=true`. Therefore, the command will be:
```sh
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=true # optionally PREFIX_DIR=<dir> can be set before this
```

By default, `PREFIX_DIR=$HOME/.feral`.
Once installation is done, execute the installed `feral` binary (`$PREFIX_DIR/bin/feral`) to use the Feral compiler/interpreter.

## Post Installation

After the installation is done, you'd probably also like to setup the package manager to install packages for the language.
To do that, just use `feral pkgbootstrap`.

# Syntax Highlighting Extensions

As of now, there are Feral language's syntax highlighting extensions available for `Visual Studio Code` and `Vim` editors.
Installation steps can be found on their repositories.

Visual Studio Code: [Feral-Lang/Feral-VSCode](https://github.com/Feral-Lang/Feral-VSCode)

Vim: [Feral-Lang/Feral-Vim](https://github.com/Feral-Lang/Feral-Vim)

# For Developers

The `.clang-format` style file is present in the repository: [https://github.com/Electrux/cpp-format](https://github.com/Electrux/cpp-format)
