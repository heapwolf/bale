#ifndef MODULE2
#define MODULE2
#include <iostream>

class MODULE2 {

  // inlcudes get hoisted

  // introduce a second level of depth
  // in this case an implicitly private
  // instance of `B`.
  MODULE3 B;

  // implicitly private
  int pnum1 = 10;

  // explicitly private
  int pnum2 = 10;

  // explicitly public
  public:
    void func1(string s) {
      std::cout << B.func() << endl;
      std::cout << s << pb1 << endl;
    }

    // a class that can be 
    // instantiated after exporting
    class c1 {
      int ci = 10;
    };

  private:
    // a few more private things...
    bool pb1;
    void func2(string s) {
      std::cout << s << endl;
    }
};
#endif

