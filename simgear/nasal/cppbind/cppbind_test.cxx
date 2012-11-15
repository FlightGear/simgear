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
  naRef member(naContext, int, naRef*) { return naNil(); }
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

naRef member(Derived&, naContext, int, naRef*) { return naNil(); }

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

  typedef boost::shared_ptr<Base> BasePtr;
  typedef boost::shared_ptr<Derived> DerivedPtr;
  typedef boost::shared_ptr<DoubleDerived> DoubleDerivedPtr;
  typedef boost::shared_ptr<DoubleDerived2> DoubleDerived2Ptr;

  Ghost<BasePtr>::init("BasePtr");
  Ghost<DerivedPtr>::init("DerivedPtr")
    .bases<BasePtr>();
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

  // TODO actuall do something with the ghosts...

  naFreeContext(c);

  return 0;
}
