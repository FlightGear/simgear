////////////////////////////////////////////////////////////////////////
// Test harness.
////////////////////////////////////////////////////////////////////////

#include <simgear_config.h>
#include <simgear/compiler.h>

#include <iostream>

#include <simgear/misc/test_macros.hxx>

#include "intern.hxx"

class Base {};
class Derived : public Base {};

struct Base2 { virtual void foo() {} }; // polymorphic
struct Derived2 : Base2 {};

template <typename T>
std::string Type(T type) {
    return simgear::getTypeName<T>();
}

int main(int ac, char ** av)
{
    Derived d1;
    Base& b1 = d1;
    Derived2 d2;
    Base2& b2 = d2;
    int i;

    SG_VERIFY(Type(d1) == "Derived");
    SG_VERIFY(Type(b1) == "Base");
    SG_VERIFY(Type(d2) == "Derived2");
    SG_VERIFY(Type(b2) == "Base2");
    SG_VERIFY(Type(i) == "int");

    std::cout << "all tests passed" << std::endl;
    return 0;
}
