# About

[![Build Status](https://api.cirrus-ci.com/github/Feral-Lang/Feral.svg?branch=master)](https://cirrus-ci.com/github/Feral-Lang/Feral)

Feral is a dynamically typed, imperative, interpreted language which revolves (to most extent) around the idea of minimalism.

The primary example being that the language syntax itself does not contain anything related to imports, structure, or enums.
Instead, there are libraries/functions that allow the user to import modules, and create structures as well as enums.

For feral, all imports, structures, enums, and functions are variables. This makes all of them a first class citizen.
One can pass and modify all of those around in functions, etc, just like a normal variable.

Do note that Feral is not an object oriented programming language, but does support one primary construct - the `dot` operator.
```py
variable.inside = 10;
let x = variable.func();
```
This makes the code a bit cleaner and easier to understand. See examples to understand its usage.

There is also a (WIP) book/guide for Feral available here: [https://feral-lang.github.io/Book/](https://feral-lang.github.io/Book/) ([source](https://github.com/Feral-Lang/Book))

# Examples

## Hello World

```py
let io = import('std/io');
io.println('Hello World');
```

## Hello greeting using a function

```py
let io = import('std/io');

let hello_fn = fn(name) {
	io.println('Hello ', name);
};

hello_fn('Electrux'); # prints 'Hello Electrux`
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
let lang = import('std/lang');
let struct_t = lang.struct(); # empty structure type (struct with no fields)
```

## Creating a struct with fields
```py
# fields `a` and `b` of type integers having default values `10`, and `20` respectively
let lang = import('std/lang');

let struct_t = lang.struct(a = 10, b = 20);
```
To create objects of this structure:
```py
# default values for struct fields
let struct_obj1 = struct_t(); # a = 10, b = 20

# overwrite first field's value (a)
let struct_obj2 = struct_t(30); # a = 30, b = 20

# overwrite using assigned argument
let struct_obj3 = struct_t(b = 30); # a = 10, b = 30
```

# Installation

## Prerequisites

To install `Feral`, the following packages are required:
* CMake (build system - for compiling the project)
* LibGMP (with C++ support - libgmpxx)
* LibMPFR (floating point numbers)

**Note**: Feral doesn't yet support Windows.

## Automated Build

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
```
cd Feral && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release # optionally PREFIX_DIR=<dir> can be set before this
make -j<cpu cores on your system> install
```

By default, `PREFIX_DIR=$HOME/.feral`.
Once installation is done, execute the installed `feral` binary (`$PREFIX_DIR/bin/feral`) to use the Feral language.

## Post Installation

After installation is done, you'd probably also like to use the `feral init` command to initialize the `$FERAL_HOME` directory, which currently is `$HOME/.feral`. This directory is where external packages shall be installed.

# Syntax Highlighting Extensions

As of right now, there are Feral language's syntax highlighting extensions available for `Visual Studio Code` and `Vim` editors.
Installation steps can be found on their repositories.

Visual Studio Code: [Feral-Lang/Feral-VSCode](https://github.com/Feral-Lang/Feral-VSCode)

Vim: [Feral-Lang/Feral-Vim](https://github.com/Feral-Lang/Feral-Vim)

# Roadmap

Roadmap for Feral is available [here](https://github.com/Feral-Lang/Feral/blob/master/ROADMAP.md).

# Communication

Join us on Discord: [https://discord.gg/zMAjSXn](https://discord.gg/zMAjSXn)