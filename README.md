# SYNOPSIS
A transpiler/module system polyfill for `C++`.


# MOTIVATION
C++ does not have a real module system. One is in the works but in the
meanwhile, here is a future-compatible polyfill. Inspired by 
[`this`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n2073.pdf), 
[`this`](http://isocpp.org/files/papers/n4214.pdf), clang's 
[`experimental module implementation`](http://clang.llvm.org/docs/Modules.html)
and the node.js [`module system`](http://nodejs.org/api/modules.html).


# PROJECT STATUS
Experimental. A work in progress.


# EXAMPLE
A modue is a discrete unit of code, encapsulated in a file. It exposes at 
least one public value, function, class, etc. Let's use the following file 
strcture as an example...

```
index.cc
pizza.cc
cheese.cc
cc_modules/
  serve/
    index.cc
```


## ./INDEX.CC
`index.cc` is the typical entry point for most programs.

```cpp
//
// import the file "pizza.cc" as "pizza".
//
import pizza "./pizza.cc";

//
// import the module "serve" as "serve".
//
import serve "serve";

int main() {
  serve.plate(pizza.make());
}
```
To understand how the math `module` is imported, read 
[`this`](http://nodejs.org/api/modules.html#modules_loading_from_node_modules_folders).


## ./PIZZA.CC
The module imported by the code in `index.cc`.

```cpp
#include <iostream>
#include <math.h>

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
  const double pi = M_PI;

  //
  // import the file "./cheese.cc" as "chz"
  //
  import chz "./cheese.cc";

  //
  // Variables found after the "public:" label are exported!
  //
  public:

    int make(int qty) {
      return qty * pi + chz.add(secret_sauce);
    }

  //
  // You can switch back and forth between public and private
  // by using the "private:" label.
  //
  private:
    int secret_sauce = 42;
}
```


## ./CHEESE.CC
This file happens to be included by `pizza.cc`.

```cpp
export {
  public:
    int add(int i) {
      return i*i;
    }
}
```


## ./CC_MODULES/SERVE/INDEX.CC
```cpp
export {
  public:
    void plate(int pizza) {
      cout << pizza << endl;
    }
}
```


# USAGE
```bash
bale ./test/input/index.cc
```

