# SYNOPSIS
A binary executable that compiles modules of C++ code.


# MOTIVATION
C++ does not have a module system. Libs and a linker do not constitute a
module system. The ideas in this project are a subset of the ideas from
[`this`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n2073.pdf), 
[`this`](http://isocpp.org/files/papers/n4214.pdf) and clang's 
[`experimental module implementation`](http://clang.llvm.org/docs/Modules.html).


# STATUS
Experimental. A work in progress.


# EXAMPLE MODULE USAGE
A modue is a discrete unit of code, encapsulated in a file, which exposes at 
least one public value or function. 


## ./MAIN.CC
This is the entry point for your program.

```cpp
// import local file (./helloworld.cc) as hello
import hello "./helloworld.cc";

int main() {
  hello.doSomething("something");
}
```


## ./HELLOWORLD.CC
A module that can be imported by `main.cc`.
```cpp
#include <iostream>

//
// GLOBAL
//
// anything outside of export will be global,
// this will be accessible from ANY file. you
// should not put anything here unless you REALLY MEAN TO.
//

export {

  //
  // implicitly private -- local variables
  // variables defined here are not exported
  // (including varibles imported from other modules)
  //
  int num = 10;

  //
  // import awesome/index.cc 
  //

  import awesome "./awesome.cc";

  //
  // explicitly public -- exported variables
  //
  public:

    void doSomething(string s) {
      std::cout << s << awesome.square(num) << endl;
    }

  //
  // explicitly private -- switch back into local mode after export
  //
  private:
    int x = 0;
}
```

## ./AWESOME.CC
This file happens to be included by `helloworld.cc`.

```cpp
export {
  public:
    int square(int i) {
      return i*i;
    }
}
```

## USAGE
It's important to tell `gcc`, `clang` (or whatever) where your 
precompiled headers can be found...

```bash
bale ./main.cc ./out/main.cc
g++ ./out/main.cc -std=c++1y -o main -include-pch ./out
```

