#include "NasalHash.hxx"

#include <cstring>
#include <iostream>

#define VERIFY(a) \
  if( !(a) ) \
  { \
    std::cerr << "failed:" << #a << std::endl; \
    return 1; \
  }

int main(int argc, char* argv[])
{
  naContext c = naNewContext();
  naRef r;

  using namespace nasal;

  r = to_nasal(c, "Test");
  VERIFY( strncmp("Test", naStr_data(r), naStr_len(r)) == 0 );

  r = to_nasal(c, std::string("Test"));
  VERIFY( strncmp("Test", naStr_data(r), naStr_len(r)) == 0 );

  r = to_nasal(c, 42);
  VERIFY( naNumValue(r).num == 42 );

  r = to_nasal(c, 4.2);
  VERIFY( naNumValue(r).num == 4.2 );

  std::vector<int> vec;
  r = to_nasal(c, vec);

  Hash hash(c);
  hash.set("vec", r);
  hash.set("vec2", vec);
  hash.set("name", "my-name");
  hash.set("string", std::string("blub"));

  r = to_nasal(c, hash);
  VERIFY( naIsHash(r) );

  Hash mod = hash.createHash("mod");
  mod.set("parent", hash);

  naFreeContext(c);

  return 0;
}
