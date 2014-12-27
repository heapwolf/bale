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


## MAIN.CC
```cpp
import hello "./helloworld.cc";

int main() {
  hello.doSomething("something");
}
```


## HELLOWORLD.CC
```cpp
#include <iostream>

//
// anything outside of export will be global
//
export {

  //
  // implicitly private
  //
  int num = 10;
  import awesome "./cc_modules/awesome/index.cc";

  //
  // explicitly public
  //
  public:

    void doSomething(string s) {
      std::cout << s << awesome.stuff(num) << endl;
    }

  //
  // explicitly private
  //
  private:
    int x = 0;
}
```

## AWESOME.CC
```cpp
export {
  public:
    int stuff(int i) {
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

