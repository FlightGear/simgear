#include <simgear/math/SGMath.hxx>

#include "Ghost.hxx"
#include "NasalHash.hxx"
#include "NasalString.hxx"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <cstring>
#include <iostream>

#define VERIFY(a) \
  if( !(a) ) \
  { \
    std::cerr << "failed: line " << __LINE__ << ": " << #a << std::endl; \
    return 1; \
  }

enum MyEnum
{
  ENUM_FIRST,
  ENUM_ANOTHER,
  ENUM_LAST
};
struct Base
{
  naRef member(const nasal::CallContext&) { return naNil(); }
  virtual ~Base(){};

  std::string getString() const { return ""; }
  void setString(const std::string&) {}
  void constVoidFunc() const {}
  size_t test1Arg(const std::string& str) const { return str.length(); }
  bool test2Args(const std::string& s, bool c) { return c && s.empty(); }

  std::string var;
  const std::string& getVar() const { return var; }
  void setVar(const std::string v) { var = v; }
};

void baseVoidFunc(Base& b) {}
void baseConstVoidFunc(const Base& b) {}
size_t baseFunc2Args(Base& b, int x, const std::string& s) { return x + s.size(); }
std::string testPtr(Base& b) { return b.getString(); }
void baseFuncCallContext(const Base&, const nasal::CallContext&) {}

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

typedef boost::shared_ptr<Base> BasePtr;
typedef std::vector<BasePtr> BaseVec;

struct DoubleDerived2:
  public Derived
{
  const BasePtr& getBase() const{return _base;}
  BasePtr _base;
  BaseVec doSomeBaseWork(const BaseVec& v) { return v; }
};

typedef boost::shared_ptr<Derived> DerivedPtr;
typedef boost::shared_ptr<DoubleDerived> DoubleDerivedPtr;
typedef boost::shared_ptr<DoubleDerived2> DoubleDerived2Ptr;

typedef boost::weak_ptr<Derived> DerivedWeakPtr;

naRef derivedFreeMember(Derived&, const nasal::CallContext&) { return naNil(); }
naRef f_derivedGetX(naContext c, const Derived& d)
{
  return nasal::to_nasal(c, d.getX());
}
naRef f_freeFunction(nasal::CallContext c) { return c.requireArg<naRef>(0); }

int main(int argc, char* argv[])
{
  naContext c = naNewContext();
  naRef r;

  using namespace nasal;

  r = to_nasal(c, ENUM_ANOTHER);
  VERIFY( from_nasal<int>(c, r) == ENUM_ANOTHER );

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

  float test_data[3] = {0, 4, 2};
  r = to_nasal(c, test_data);

  SGVec2f vec(0,2);
  r = to_nasal(c, vec);
  VERIFY( from_nasal<SGVec2f>(c, r) == vec );

  std::vector<int> std_vec;
  r = to_nasal(c, std_vec);

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
  hash.set("func", &f_freeFunction);

  r = to_nasal(c, hash);
  VERIFY( naIsHash(r) );

  VERIFY( hash.get<std::string>("name") == "my-name" );
  VERIFY( naIsString(hash.get("name")) );

  Hash mod = hash.createHash("mod");
  mod.set("parent", hash);


  // 'func' is a C++ function registered to Nasal and now converted back to C++
  boost::function<int (int)> f = hash.get<int (int)>("func");
  VERIFY( f );
  VERIFY( f(3) == 3 );

  boost::function<std::string (int)> fs = hash.get<std::string (int)>("func");
  VERIFY( fs );
  VERIFY( fs(14) == "14" );

  typedef boost::function<void (int)> FuncVoidInt;
  FuncVoidInt fvi = hash.get<FuncVoidInt>("func");
  VERIFY( fvi );
  fvi(123);

  typedef boost::function<std::string (const std::string&, int, float)> FuncMultiArg;
  FuncMultiArg fma = hash.get<FuncMultiArg>("func");
  VERIFY( fma );
  VERIFY( fma("test", 3, .5) == "test" );

  //----------------------------------------------------------------------------
  // Test exposing classes to Nasal
  //----------------------------------------------------------------------------

  Ghost<BasePtr>::init("BasePtr")
    .method("member", &Base::member)
    .method("strlen", &Base::test1Arg)
    .member("str", &Base::getString, &Base::setString)
    .method("str_m", &Base::getString)
    .method("void", &Base::constVoidFunc)
    .member("var_r", &Base::getVar)
    .member("var_w", &Base::setVar)
    .member("var", &Base::getVar, &Base::setVar)
    .method("void", &baseVoidFunc)
    .method("void_c", &baseConstVoidFunc)
    .method("int2args", &baseFunc2Args)
    .method("bool2args", &Base::test2Args)
    .method("str_ptr", &testPtr);
  Ghost<DerivedPtr>::init("DerivedPtr")
    .bases<BasePtr>()
    .member("x", &Derived::getX, &Derived::setX)
    .member("x_alternate", &f_derivedGetX)
    .method("free_fn", &derivedFreeMember)
    .method("free_member", &derivedFreeMember)
    .method("baseDoIt", &baseFuncCallContext);
  Ghost<DoubleDerivedPtr>::init("DoubleDerivedPtr")
    .bases<DerivedPtr>();
  Ghost<DoubleDerived2Ptr>::init("DoubleDerived2Ptr")
    .bases< Ghost<DerivedPtr> >()
    .member("base", &DoubleDerived2::getBase)
    .method("doIt", &DoubleDerived2::doSomeBaseWork);

  Ghost<DerivedWeakPtr>::init("DerivedWeakPtr");

  VERIFY( Ghost<BasePtr>::isInit() );
  nasal::to_nasal(c, DoubleDerived2Ptr());

  BasePtr d( new Derived );
  naRef derived = Ghost<BasePtr>::create(c, d);
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

  std::map<std::string, BasePtr> instances;
  VERIFY( naIsHash(to_nasal(c, instances)) );

  std::map<std::string, DerivedPtr> instances_d;
  VERIFY( naIsHash(to_nasal(c, instances_d)) );

  std::map<std::string, int> int_map;
  VERIFY( naIsHash(to_nasal(c, int_map)) );

  std::map<std::string, std::vector<int> > int_vector_map;
  VERIFY( naIsHash(to_nasal(c, int_vector_map)) );

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

  std::vector<naRef> nasal_objects;
  nasal_objects.push_back( Ghost<BasePtr>::create(c, d) );
  nasal_objects.push_back( Ghost<BasePtr>::create(c, d2) );
  nasal_objects.push_back( Ghost<BasePtr>::create(c, d3) );
  naRef obj_vec = to_nasal(c, nasal_objects);

  std::vector<BasePtr> objects = from_nasal<std::vector<BasePtr> >(c, obj_vec);
  VERIFY( objects[0] == d );
  VERIFY( objects[1] == d2 );
  VERIFY( objects[2] == d3 );

  // TODO actually do something with the ghosts...

  //----------------------------------------------------------------------------
  // Test nasal::CallContext
  //----------------------------------------------------------------------------


  int int_vec[] = {1,2,3};
  std::map<std::string, std::string> map;
  naRef args[] = {
    to_nasal(c, std::string("test-arg")),
    to_nasal(c, 4),
    to_nasal(c, int_vec),
    to_nasal(c, map)
  };
  CallContext cc(c, sizeof(args)/sizeof(args[0]), args);
  VERIFY( cc.requireArg<std::string>(0) == "test-arg" );
  VERIFY( cc.getArg<std::string>(0) == "test-arg" );
  VERIFY( cc.getArg<std::string>(10) == "" );
  VERIFY( cc.isString(0) );
  VERIFY( !cc.isNumeric(0) );
  VERIFY( !cc.isVector(0) );
  VERIFY( !cc.isHash(0) );
  VERIFY( !cc.isGhost(0) );
  VERIFY( cc.isNumeric(1) );
  VERIFY( cc.isVector(2) );
  VERIFY( cc.isHash(3) );

  naRef args_vec = nasal::to_nasal(c, args);
  VERIFY( naIsVector(args_vec) );

  //----------------------------------------------------------------------------
  // Test nasal::String
  //----------------------------------------------------------------------------

  String string( to_nasal(c, "Test") );
  VERIFY( from_nasal<std::string>(c, string.get_naRef()) == "Test" );
  VERIFY( string.c_str() == std::string("Test") );
  VERIFY( string.starts_with(string) );
  VERIFY( string.starts_with(String(c, "T")) );
  VERIFY( string.starts_with(String(c, "Te")) );
  VERIFY( string.starts_with(String(c, "Tes")) );
  VERIFY( string.starts_with(String(c, "Test")) );
  VERIFY( !string.starts_with(String(c, "Test1")) );
  VERIFY( !string.starts_with(String(c, "bb")) );
  VERIFY( !string.starts_with(String(c, "bbasdasdafasd")) );
  VERIFY( string.ends_with(String(c, "t")) );
  VERIFY( string.ends_with(String(c, "st")) );
  VERIFY( string.ends_with(String(c, "est")) );
  VERIFY( string.ends_with(String(c, "Test")) );
  VERIFY( !string.ends_with(String(c, "1Test")) );
  VERIFY( !string.ends_with(String(c, "abc")) );
  VERIFY( !string.ends_with(String(c, "estasdasd")) );
  VERIFY( string.find('e') == 1 );
  VERIFY( string.find('9') == String::npos );
  VERIFY( string.find_first_of(String(c, "st")) == 2 );
  VERIFY( string.find_first_of(String(c, "st"), 3) == 3 );
  VERIFY( string.find_first_of(String(c, "xyz")) == String::npos );
  VERIFY( string.find_first_not_of(String(c, "Tst")) == 1 );
  VERIFY( string.find_first_not_of(String(c, "Tse"), 2) == 3 );
  VERIFY( string.find_first_not_of(String(c, "abc")) == 0 );
  VERIFY( string.find_first_not_of(String(c, "abc"), 20) == String::npos );

  naFreeContext(c);

  return 0;
}
