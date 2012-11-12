#include "Ghost.hxx"
#include "NasalHash.hxx"

#include <cstring>
#include <iostream>

#define VERIFY(a) \
  if( !(a) ) \
  { \
    std::cerr << "failed:" << #a << std::endl; \
    return 1; \
  }

struct Base
{
  naRef member(int, naRef*) { return naNil(); }
};
struct Derived:
  public Base
{
  int _x;
  int getX() const { return _x; }
  void setX(int x) { _x = x; }
};

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
    .member("x", &Derived::getX, &Derived::setX);

  naRef derived = Ghost<Derived>::create(c);
  VERIFY( naIsGhost(derived) );
  // TODO actuall do something with the ghosts...

  naFreeContext(c);

  return 0;
}
