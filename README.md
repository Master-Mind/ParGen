# ParGen

This is a project which uses [libclang](https://github.com/llvm/llvm-project) to parse c++ files and feed that information into [Ijna](https://github.com/pantor/inja). The intention is to use templates to generate c++ boilerplate, but in theory it can be used to generate any kind of file.

## Motivation

C++ templates are simultaniously richely featured and also missing really basic functionality. The fact that it is **STILL** _in the year of our lorde two thousand and twenty four_ outragously difficult to do something as simple as a `for_each_member` loop on an arbitrary struct/class without copypasta and/or boilerplate is frustrating to say the least.

This make metaprogramming generally difficult and requires a ton of boilerplate for simple things like json serialization. This project will hopefully reduce that.

## How to build

Clone the repo, create project files using [premake](https://premake.github.io/), get the libclang binaries (see the note below), and then build the project with your editor of choice.

## NOTE ABOUT LIBCLANG
This project must statically and dynamically link with libclang. On Windows, this means putting libclang.lib in ./Clang/Lib/\<build configuration\> and libclang.dll in the same directory as the built exes. I couldn't get github to accept the binaries (they're very large) so just build them from the [llvm project](https://clang.llvm.org/get_started.html).
