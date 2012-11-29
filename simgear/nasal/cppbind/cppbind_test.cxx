#include "Ghost.hxx"
#include "NasalHash.hxx"

#include <boost/shared_ptr.hpp>

#include <cstring>
#include <iostream>

#define VERIFY(a) \
  if( !(a) ) \
  { \
    std::cerr << "failed: line " << __LINE__ << ": " << #a << std::endl; \
    return 1; \
  }

struct Base
{
  naRef member(const nasal::CallContext&) { return naNil(); }
  virtual ~Base(){};
};
struct Derived:
  public Base
{
  int _x;
  int getX() const { return _x; }
  void setX(int x) { _x = x; }
};
struct DoubleDerived:
  public Derived
{

};
struct DoubleDerived2:
  public Derived
{

};

typedef boost::shared_ptr<Base> BasePtr;
typedef boost::shared_ptr<Derived> DerivedPtr;
typedef boost::shared_ptr<DoubleDerived> DoubleDerivedPtr;
typedef boost::shared_ptr<DoubleDerived2> DoubleDerived2Ptr;

naRef member(Derived&, const nasal::CallContext&) { return naNil(); }

int main(int argc, char* argv[])
{
  naContext c = naNewContext();
  naRef r;

  using namespace nasal;

  r = to_nasal(c, "Test");
  VERIFY( strncmp("Test", naStr_data(r), naStr_len(r)) == 0 );
  VERIFY( from_nasal<std::string>(c, r) == "Test" );

  r = to_nasal(c, std::string("Test"));
  VERIFY( strncmp("Test", naStr_data(r), naStr_len(r)) == 0 );
  VERIFY( from_nasal<std::string>(c, r) == "Test" );

  r = to_nasal(c, 42);
  VERIFY( naNumValue(r).num == 42 );
  VERIFY( from_nasal<int>(c, r) == 42 );

  r = to_nasal(c, 4.2f);
  VERIFY( naNumValue(r).num == 4.2f );
  VERIFY( from_nasal<float>(c, r) == 4.2f );

  std::vector<int> vec;
  r = to_nasal(c, vec);

  r = to_nasal(c, "string");
  try
  {
    from_nasal<int>(c, r);

    std::cerr << "failed: Expected bad_nasal_cast to be thrown" << std::endl;
    return 1;
  }
  catch(nasal::bad_nasal_cast&)
  {}

  Hash hash(c);
  hash.set("vec", r);
  hash.set("vec2", vec);
  hash.set("name", "my-name");
  hash.set("string", std::string("blub"));

  r = to_nasal(c, hash);
  VERIFY( naIsHash(r) );

  VERIFY( hash.get<std::string>("name") == "my-name" );
  VERIFY( naIsString(hash.get("name")) );

  Hash mod = hash.createHash("mod");
  mod.set("parent", hash);

  Ghost<Base>::init("Base")
    .method<&Base::member>("member");
  Ghost<Derived>::init("Derived")
    .bases<Base>()
    .member("x", &Derived::getX, &Derived::setX)
    .method_func<&member>("free_member");

  naRef derived = Ghost<Derived>::create(c);
  VERIFY( naIsGhost(derived) );
  VERIFY( std::string("Derived") ==  naGhost_type(derived)->name );

  Ghost<BasePtr>::init("BasePtr");
  Ghost<DerivedPtr>::init("DerivedPtr")
    .bases<BasePtr>()
    .member("x", &Derived::getX, &Derived::setX)
    .method_func<&member>("free_member");
  Ghost<DoubleDerivedPtr>::init("DoubleDerivedPtr")
    .bases<DerivedPtr>();
  Ghost<DoubleDerived2Ptr>::init("DoubleDerived2Ptr")
    .bases<DerivedPtr>();

  BasePtr d( new Derived );
  derived = Ghost<BasePtr>::create(c, d);
  VERIFY( naIsGhost(derived) );
  VERIFY( std::string("DerivedPtr") == naGhost_type(derived)->name );

  BasePtr d2( new DoubleDerived );
  derived = Ghost<BasePtr>::create(c, d2);
  VERIFY( naIsGhost(derived) );
  VERIFY( std::string("DoubleDerivedPtr") ==  naGhost_type(derived)->name );

  BasePtr d3( new DoubleDerived2 );
  derived = Ghost<BasePtr>::create(c, d3);
  VERIFY( naIsGhost(derived) );
  VERIFY( std::string("DoubleDerived2Ptr") ==  naGhost_type(derived)->name );

  VERIFY( Ghost<BasePtr>::isBaseOf(derived) );
  VERIFY( Ghost<DerivedPtr>::isBaseOf(derived) );
  VERIFY( Ghost<DoubleDerived2Ptr>::isBaseOf(derived) );

  VERIFY( Ghost<BasePtr>::fromNasal(c, derived) == d3 );
  VERIFY( Ghost<BasePtr>::fromNasal(c, derived) != d2 );
  VERIFY(    Ghost<DerivedPtr>::fromNasal(c, derived)
          == boost::dynamic_pointer_cast<Derived>(d3) );
  VERIFY(    Ghost<DoubleDerived2Ptr>::fromNasal(c, derived)
          == boost::dynamic_pointer_cast<DoubleDerived2>(d3) );
  VERIFY( !Ghost<DoubleDerivedPtr>::fromNasal(c, derived) );

  // Check converting to Ghost if using Nasal hashes with actual ghost inside
  // the hashes parents vector
  std::vector<naRef> parents;
  parents.push_back(hash.get_naRef());
  parents.push_back(derived);

  Hash obj(c);
  obj.set("parents", parents);
  VERIFY( Ghost<BasePtr>::fromNasal(c, obj.get_naRef()) == d3 );

  // Check recursive parents (aka parent-of-parent)
  std::vector<naRef> parents2;
  parents2.push_back(obj.get_naRef());
  Hash derived_obj(c);
  derived_obj.set("parents", parents2);
  VERIFY( Ghost<BasePtr>::fromNasal(c, derived_obj.get_naRef()) == d3 );

  naRef args[] = {
    to_nasal(c, std::string("test-arg"))
  };
  CallContext cc(c, sizeof(args)/sizeof(args[0]), args);
  VERIFY( cc.requireArg<std::string>(0) == "test-arg" );
  VERIFY( cc.getArg<std::string>(0) == "test-arg" );
  VERIFY( cc.getArg<std::string>(1) == "" );

  // TODO actually do something with the ghosts...

  naFreeContext(c);

  return 0;
}
