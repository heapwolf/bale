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
The module imported by the code in `main.cc`.

```cpp
#include <iostream>

//
// GLOBAL SCOPE
//
// Anything outside of export will be global, this will be 
// accessible from ANY file. you should not put anything here 
// unless you REALLY MEAN TO.
//

export {

  //
  // MODULE SCOPE
  //
  // Variables defined here are implicitly private, they are
  // not exported, this applies to varibles imported from other 
  // modules.
  //
  int num = 10;

  //
  // import ./awesome.cc 
  //
  import awesome "./awesome.cc";

  //
  // Variables found after the "public:" label are exported!
  //
  public:

    void doSomething(string s) {
      std::cout << s << awesome.square(num) << endl;
    }

  //
  // You can switch back and forth between public and private
  // by using the "private:" label.
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
bale <input-file> <output-directory>
```

### EXAMPLE
```bash
bale ./main.cc ./out
g++ ./out/main.cc -std=c++1y -o main -include-pch ./out
```

