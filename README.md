# ParGen

Have you ever stood on the ocean shore, with the sand between your toes and the sea breeze against your face, and wondered to yourself: "What if the word 'template' was even more overloaded?" If so, then look no further.

This is a project which uses [libclang](https://github.com/llvm/llvm-project) to parse c++ files and feed that information into custom templating system. The intention is to use templates to generate c++ boilerplate, but in theory it can be used to generate any kind of file.

## Motivation

C++ templates are simultaniously richely featured and also missing really basic functionality. The fact that it is **STILL** _in the year of our lorde two thousand and twenty four_ outragously difficult to do something as simple as a `for_each_member` loop on an arbitrary struct/class without copypasta and/or boilerplate is frustrating to say the least.

This makes metaprogramming generally difficult and requires a ton of boilerplate for simple things like json serialization. This project will hopefully reduce that.

## How to build

Clone the repo, create project files using [premake](https://premake.github.io/), get the libclang binaries (see the note below), and then build the project with your editor of choice.

## Example

Running the following command:

```
ParGen.exe --files ../Test/Test.vcxproj -t ../Test/test.cppt
```

After running premake will use this template:

```
Showing all type info...
{{foreach class in ClassDecl}}
	Spelling: {{ class.spelling }}
	Kind: {{ class.kind }}
	{{foreach field in FieldDecl}}
		Spelling: {{ class.spelling }}::{{ field.spelling }}
		Kind: {{ field.kind }}
	{{/foreach}}
{{/foreach}}
Done showing info
```

To generate the following output:

```
Showing all type info...

        Spelling: SimpleClass
        Kind: ClassDecl

                Spelling: SimpleClass::field1
                Kind: FieldDecl

                Spelling: SimpleClass::field2
                Kind: FieldDecl


        Spelling: ConstructorClass
        Kind: ClassDecl

                Spelling: ConstructorClass::field1
                Kind: FieldDecl

                Spelling: ConstructorClass::field2
                Kind: FieldDecl


        Spelling: BaseClass
        Kind: ClassDecl


        Spelling: DerivedClass
        Kind: ClassDecl


<a few hundred lines more of type info...>
Done showing info
```

## NOTE ABOUT LIBCLANG
This project must statically and dynamically link with libclang. On Windows, this means putting libclang.lib in ./Clang/Lib/\<build configuration\> and libclang.dll in the same directory as the built exes. I couldn't get github to accept the binaries (they're very large) and honestly you probably shouldn't want me to ship with binaries, so just build them from the [llvm project](https://clang.llvm.org/get_started.html).
