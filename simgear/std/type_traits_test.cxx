#include <simgear/std/type_traits.hxx>

using namespace std;

template<class T, class U>
void assert_same()
{
  static_assert(is_same<T, U>::value, "");
}

int main(int argc, char* argv[])
{
  assert_same<remove_cv_t<int const volatile>, int>();
  assert_same<remove_const_t<int const volatile>, int volatile>();
  assert_same<remove_volatile_t<int const volatile>, int const>();
  assert_same<remove_reference_t<int const volatile&>, int const volatile>();
  assert_same<remove_pointer_t<int const volatile*>, int const volatile>();
  assert_same<remove_cvref_t<int const volatile&>, int>();

  assert_same<enable_if_t<true, double>, double>();
  assert_same<bool_constant<true>, integral_constant<bool, true>>();

  return 0;
}
