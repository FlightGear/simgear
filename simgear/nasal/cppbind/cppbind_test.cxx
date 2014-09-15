#define BOOST_TEST_MODULE cppbind
#include <BoostTestTargetConfig.h>

#include "Ghost.hxx"
#include "NasalHash.hxx"
#include "NasalString.hxx"

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/map.hxx>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <cstring>

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

  unsigned long getThis() const { return (unsigned long)this; }
  bool genericSet(const std::string& key, const std::string& val)
  {
    return key == "test";
  }
  bool genericGet(const std::string& key, std::string& val_out) const
  {
    if( key != "get_test" )
      return false;

    val_out = "generic-get";
    return true;
  }
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

naRef f_derivedGetRandom(const Derived&, naContext)
{
  return naNil();
}
void f_derivedSetX(Derived& d, naContext, naRef r)
{
  d._x = static_cast<int>(naNumValue(r).num);
}

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

class SGReferenceBasedClass:
  public SGReferenced
{

};

class SGWeakReferenceBasedClass:
  public SGWeakReferenced
{

};

typedef boost::shared_ptr<Derived> DerivedPtr;
typedef boost::shared_ptr<DoubleDerived> DoubleDerivedPtr;
typedef boost::shared_ptr<DoubleDerived2> DoubleDerived2Ptr;
typedef SGSharedPtr<SGReferenceBasedClass> SGRefBasedPtr;
typedef SGSharedPtr<SGWeakReferenceBasedClass> SGWeakRefBasedPtr;

typedef boost::weak_ptr<Derived> DerivedWeakPtr;

naRef derivedFreeMember(Derived&, const nasal::CallContext&) { return naNil(); }
naRef f_derivedGetX(const Derived& d, naContext c)
{
  return nasal::to_nasal(c, d.getX());
}
naRef f_freeFunction(nasal::CallContext c) { return c.requireArg<naRef>(0); }

BOOST_AUTO_TEST_CASE( cppbind_misc_testing )
{
  naContext c = naNewContext();
  naRef r;

  using namespace nasal;

  r = to_nasal(c, ENUM_ANOTHER);
  BOOST_CHECK_EQUAL(from_nasal<int>(c, r), ENUM_ANOTHER);

  r = to_nasal(c, "Test");
  BOOST_CHECK( strncmp("Test", naStr_data(r), naStr_len(r)) == 0 );
  BOOST_CHECK_EQUAL(from_nasal<std::string>(c, r), "Test");

  r = to_nasal(c, std::string("Test"));
  BOOST_CHECK( strncmp("Test", naStr_data(r), naStr_len(r)) == 0 );
  BOOST_CHECK_EQUAL(from_nasal<std::string>(c, r), "Test");

  r = to_nasal(c, 42);
  BOOST_CHECK_EQUAL(naNumValue(r).num, 42);
  BOOST_CHECK_EQUAL(from_nasal<int>(c, r), 42);

  r = to_nasal(c, 4.2f);
  BOOST_CHECK_EQUAL(naNumValue(r).num, 4.2f);
  BOOST_CHECK_EQUAL(from_nasal<float>(c, r), 4.2f);

  float test_data[3] = {0, 4, 2};
  r = to_nasal(c, test_data);

  SGVec2f vec(0,2);
  r = to_nasal(c, vec);
  BOOST_CHECK_EQUAL(from_nasal<SGVec2f>(c, r), vec);

  std::vector<int> std_vec;
  r = to_nasal(c, std_vec);

  r = to_nasal(c, "string");
  BOOST_CHECK_THROW(from_nasal<int>(c, r), bad_nasal_cast);

  Hash hash(c);
  hash.set("vec", r);
  hash.set("vec2", vec);
  hash.set("name", "my-name");
  hash.set("string", std::string("blub"));
  hash.set("func", &f_freeFunction);

  BOOST_CHECK_EQUAL(hash.size(), 5);
  for(Hash::const_iterator it = hash.begin(); it != hash.end(); ++it)
    BOOST_CHECK_EQUAL( hash.get<std::string>(it->getKey()),
                       it->getValue<std::string>() );

  Hash::iterator it1, it2;
  Hash::const_iterator it3 = it1, it4(it2);
  it1 = it2;
  it3 = it2;

  r = to_nasal(c, hash);
  BOOST_REQUIRE( naIsHash(r) );

  simgear::StringMap string_map = from_nasal<simgear::StringMap>(c, r);
  BOOST_CHECK_EQUAL(string_map.at("vec"), "string");
  BOOST_CHECK_EQUAL(string_map.at("name"), "my-name");
  BOOST_CHECK_EQUAL(string_map.at("string"), "blub");

  BOOST_CHECK_EQUAL(hash.get<std::string>("name"), "my-name");
  BOOST_CHECK(naIsString(hash.get("name")));

  Hash mod = hash.createHash("mod");
  mod.set("parent", hash);


  // 'func' is a C++ function registered to Nasal and now converted back to C++
  boost::function<int (int)> f = hash.get<int (int)>("func");
  BOOST_REQUIRE( f );
  BOOST_CHECK_EQUAL(f(3), 3);

  boost::function<std::string (int)> fs = hash.get<std::string (int)>("func");
  BOOST_REQUIRE( fs );
  BOOST_CHECK_EQUAL(fs(14), "14");

  typedef boost::function<void (int)> FuncVoidInt;
  FuncVoidInt fvi = hash.get<FuncVoidInt>("func");
  BOOST_REQUIRE( fvi );
  fvi(123);

  typedef boost::function<std::string (const std::string&, int, float)> FuncMultiArg;
  FuncMultiArg fma = hash.get<FuncMultiArg>("func");
  BOOST_REQUIRE( fma );
  BOOST_CHECK_EQUAL(fma("test", 3, .5), "test");

  typedef boost::function<naRef (naRef)> naRefMemFunc;
  naRefMemFunc fmem = hash.get<naRefMemFunc>("func");
  BOOST_REQUIRE( fmem );
  naRef ret = fmem(hash.get_naRef()),
        hash_ref = hash.get_naRef();
  BOOST_CHECK( naIsIdentical(ret, hash_ref) );

  // Check if nasal::Me gets passed as self/me and remaining arguments are
  // passed on to function
  typedef boost::function<int (Me, int)> MeIntFunc;
  MeIntFunc fmeint = hash.get<MeIntFunc>("func");
  BOOST_CHECK_EQUAL(fmeint(naNil(), 5), 5);

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
    .method("str_ptr", &testPtr)
    .method("this", &Base::getThis)
    ._set(&Base::genericSet)
    ._get(&Base::genericGet);
  Ghost<DerivedPtr>::init("DerivedPtr")
    .bases<BasePtr>()
    .member("x", &Derived::getX, &Derived::setX)
    .member("x_alternate", &f_derivedGetX)
    .member("x_mixed", &f_derivedGetRandom, &Derived::setX)
    .member("x_mixed2", &Derived::getX, &f_derivedSetX)
    .member("x_w", &f_derivedSetX)
    .method("free_fn", &derivedFreeMember)
    .method("free_member", &derivedFreeMember)
    .method("baseDoIt", &baseFuncCallContext);
  Ghost<DoubleDerivedPtr>::init("DoubleDerivedPtr")
    .bases<DerivedPtr>();
  Ghost<DoubleDerived2Ptr>::init("DoubleDerived2Ptr")
    .bases< Ghost<DerivedPtr> >()
    .member("base", &DoubleDerived2::getBase)
    .method("doIt", &DoubleDerived2::doSomeBaseWork);

  Ghost<SGRefBasedPtr>::init("SGRefBasedPtr");
  Ghost<SGWeakRefBasedPtr>::init("SGWeakRefBasedPtr");

  SGWeakRefBasedPtr weak_ptr(new SGWeakReferenceBasedClass());
  naRef nasal_ref = to_nasal(c, weak_ptr),
        nasal_ptr = to_nasal(c, weak_ptr.get());

  BOOST_REQUIRE( naIsGhost(nasal_ref) );
  BOOST_REQUIRE( naIsGhost(nasal_ptr) );

  SGWeakRefBasedPtr ptr1 = from_nasal<SGWeakRefBasedPtr>(c, nasal_ref),
                    ptr2 = from_nasal<SGWeakRefBasedPtr>(c, nasal_ptr);

  BOOST_CHECK_EQUAL(weak_ptr, ptr1);
  BOOST_CHECK_EQUAL(weak_ptr, ptr2);


  BOOST_REQUIRE( Ghost<BasePtr>::isInit() );
  nasal::to_nasal(c, DoubleDerived2Ptr());

  BasePtr d( new Derived );
  naRef derived = to_nasal(c, d);
  BOOST_REQUIRE( naIsGhost(derived) );
  BOOST_CHECK_EQUAL( std::string("DerivedPtr"), naGhost_type(derived)->name );

  // Get member function from ghost...
  naRef thisGetter = naNil();
  BOOST_CHECK( naMember_get(c, derived, to_nasal(c, "this"), &thisGetter) );
  BOOST_CHECK( naIsFunc(thisGetter) );

  // ...and check if it really gets passed the correct instance
  typedef boost::function<unsigned long (Me)> MemFunc;
  MemFunc fGetThis = from_nasal<MemFunc>(c, thisGetter);
  BOOST_REQUIRE( fGetThis );
  BOOST_CHECK_EQUAL( fGetThis(derived), (unsigned long)d.get() );

  BasePtr d2( new DoubleDerived );
  derived = to_nasal(c, d2);
  BOOST_CHECK( naIsGhost(derived) );
  BOOST_CHECK_EQUAL( std::string("DoubleDerivedPtr"),
                     naGhost_type(derived)->name );

  BasePtr d3( new DoubleDerived2 );
  derived = to_nasal(c, d3);
  BOOST_CHECK( naIsGhost(derived) );
  BOOST_CHECK_EQUAL( std::string("DoubleDerived2Ptr"),
                     naGhost_type(derived)->name );

  SGRefBasedPtr ref_based( new SGReferenceBasedClass );
  naRef na_ref_based = to_nasal(c, ref_based.get());
  BOOST_CHECK( naIsGhost(na_ref_based) );
  BOOST_CHECK_EQUAL( from_nasal<SGReferenceBasedClass*>(c, na_ref_based),
                     ref_based.get() );
  BOOST_CHECK_EQUAL( from_nasal<SGRefBasedPtr>(c, na_ref_based), ref_based );

  BOOST_CHECK_EQUAL( from_nasal<BasePtr>(c, derived), d3 );
  BOOST_CHECK_NE( from_nasal<BasePtr>(c, derived), d2 );
  BOOST_CHECK_EQUAL( from_nasal<DerivedPtr>(c, derived),
                     boost::dynamic_pointer_cast<Derived>(d3) );
  BOOST_CHECK_EQUAL( from_nasal<DoubleDerived2Ptr>(c, derived),
                     boost::dynamic_pointer_cast<DoubleDerived2>(d3) );
  BOOST_CHECK_THROW( from_nasal<DoubleDerivedPtr>(c, derived), bad_nasal_cast );

  std::map<std::string, BasePtr> instances;
  BOOST_CHECK( naIsHash(to_nasal(c, instances)) );

  std::map<std::string, DerivedPtr> instances_d;
  BOOST_CHECK( naIsHash(to_nasal(c, instances_d)) );

  std::map<std::string, int> int_map;
  BOOST_CHECK( naIsHash(to_nasal(c, int_map)) );

  std::map<std::string, std::vector<int> > int_vector_map;
  BOOST_CHECK( naIsHash(to_nasal(c, int_vector_map)) );

  simgear::StringMap dict =
    simgear::StringMap("hello", "value")
                      ("key2", "value2");
  naRef na_dict = to_nasal(c, dict);
  BOOST_REQUIRE( naIsHash(na_dict) );
  BOOST_CHECK_EQUAL( Hash(na_dict, c).get<std::string>("key2"), "value2" );

  // Check converting to Ghost if using Nasal hashes with actual ghost inside
  // the hashes parents vector
  std::vector<naRef> parents;
  parents.push_back(hash.get_naRef());
  parents.push_back(derived);

  Hash obj(c);
  obj.set("parents", parents);
  BOOST_CHECK_EQUAL( from_nasal<BasePtr>(c, obj.get_naRef()), d3 );

  // Check recursive parents (aka parent-of-parent)
  std::vector<naRef> parents2;
  parents2.push_back(obj.get_naRef());
  Hash derived_obj(c);
  derived_obj.set("parents", parents2);
  BOOST_CHECK_EQUAL( from_nasal<BasePtr>(c, derived_obj.get_naRef()), d3 );

  std::vector<naRef> nasal_objects;
  nasal_objects.push_back( Ghost<BasePtr>::makeGhost(c, d) );
  nasal_objects.push_back( Ghost<BasePtr>::makeGhost(c, d2) );
  nasal_objects.push_back( Ghost<BasePtr>::makeGhost(c, d3) );
  naRef obj_vec = to_nasal(c, nasal_objects);

  std::vector<BasePtr> objects = from_nasal<std::vector<BasePtr> >(c, obj_vec);
  BOOST_CHECK_EQUAL( objects[0], d );
  BOOST_CHECK_EQUAL( objects[1], d2 );
  BOOST_CHECK_EQUAL( objects[2], d3 );

  {
    // Calling fallback setter for unset values
    const char* src_code = "me.test = 3;";
    int errLine = -1;
    naRef code = naParseCode( c, to_nasal(c, "source.nas"), 0,
                              (char*)src_code, strlen(src_code),
                              &errLine );
    ret = naCallMethod(code, derived, 0, 0, naNil());

    BOOST_REQUIRE( !naGetError(c) ); // TODO real error check (this seems to
                                     //      always return 0...
    BOOST_CHECK_EQUAL( from_nasal<int>(c, ret), 3 );
  }
  {
    // Calling generic (fallback) getter
    const char* src_code = "var a = me.get_test;";
    int errLine = -1;
    naRef code = naParseCode( c, to_nasal(c, "source.nas"), 0,
                              (char*)src_code, strlen(src_code),
                              &errLine );
    ret = naCallMethod(code, derived, 0, 0, naNil());

    BOOST_REQUIRE( !naGetError(c) ); // TODO real error check (this seems to
                                     //      always return 0...
    BOOST_CHECK_EQUAL( from_nasal<std::string>(c, ret), "generic-get" );
  }

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
  CallContext cc(c, naNil(), sizeof(args)/sizeof(args[0]), args);
  BOOST_CHECK_EQUAL( cc.requireArg<std::string>(0), "test-arg" );
  BOOST_CHECK_EQUAL( cc.getArg<std::string>(0), "test-arg" );
  BOOST_CHECK_EQUAL( cc.getArg<std::string>(10), "" );
  BOOST_CHECK( cc.isString(0) );
  BOOST_CHECK( !cc.isNumeric(0) );
  BOOST_CHECK( !cc.isVector(0) );
  BOOST_CHECK( !cc.isHash(0) );
  BOOST_CHECK( !cc.isGhost(0) );
  BOOST_CHECK( cc.isNumeric(1) );
  BOOST_CHECK( cc.isVector(2) );
  BOOST_CHECK( cc.isHash(3) );

  naRef args_vec = nasal::to_nasal(c, args);
  BOOST_CHECK( naIsVector(args_vec) );

  //----------------------------------------------------------------------------
  // Test nasal::String
  //----------------------------------------------------------------------------

  String string( to_nasal(c, "Test") );
  BOOST_CHECK_EQUAL( from_nasal<std::string>(c, string.get_naRef()), "Test" );
  BOOST_CHECK_EQUAL( string.c_str(), std::string("Test") );
  BOOST_CHECK( string.starts_with(string) );
  BOOST_CHECK( string.starts_with(String(c, "T")) );
  BOOST_CHECK( string.starts_with(String(c, "Te")) );
  BOOST_CHECK( string.starts_with(String(c, "Tes")) );
  BOOST_CHECK( string.starts_with(String(c, "Test")) );
  BOOST_CHECK( !string.starts_with(String(c, "Test1")) );
  BOOST_CHECK( !string.starts_with(String(c, "bb")) );
  BOOST_CHECK( !string.starts_with(String(c, "bbasdasdafasd")) );
  BOOST_CHECK( string.ends_with(String(c, "t")) );
  BOOST_CHECK( string.ends_with(String(c, "st")) );
  BOOST_CHECK( string.ends_with(String(c, "est")) );
  BOOST_CHECK( string.ends_with(String(c, "Test")) );
  BOOST_CHECK( !string.ends_with(String(c, "1Test")) );
  BOOST_CHECK( !string.ends_with(String(c, "abc")) );
  BOOST_CHECK( !string.ends_with(String(c, "estasdasd")) );
  BOOST_CHECK_EQUAL( string.find('e'), 1 );
  BOOST_CHECK_EQUAL( string.find('9'), String::npos );
  BOOST_CHECK_EQUAL( string.find_first_of(String(c, "st")), 2 );
  BOOST_CHECK_EQUAL( string.find_first_of(String(c, "st"), 3), 3 );
  BOOST_CHECK_EQUAL( string.find_first_of(String(c, "xyz")), String::npos );
  BOOST_CHECK_EQUAL( string.find_first_not_of(String(c, "Tst")), 1 );
  BOOST_CHECK_EQUAL( string.find_first_not_of(String(c, "Tse"), 2), 3 );
  BOOST_CHECK_EQUAL( string.find_first_not_of(String(c, "abc")), 0 );
  BOOST_CHECK_EQUAL( string.find_first_not_of(String(c, "abc"), 20), String::npos );

  naFreeContext(c);
}
