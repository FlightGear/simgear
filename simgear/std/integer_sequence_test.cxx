#include <simgear/std/integer_sequence.hxx>
#include <iostream>

template<class T>
void print(const T& v, std::size_t i)
{
  std::cout << "arg #" << i << ": '" << v << "', ";
}

template<class... Args, std::size_t... Is>
void doIt_impl(Args ... args, std::index_sequence<Is...>)
{
  std::initializer_list<char>{(print(args, Is), '0')...};
}

template<class... Args>
void doIt(Args ... args)
{
  static_assert(sizeof...(Args) == std::index_sequence_for<Args...>::size(), "");

  doIt_impl<Args...>(args..., std::index_sequence_for<Args...>{});
}

int main(int argc, char* argv[])
{
  static_assert(std::is_same<std::integer_sequence<char>::value_type, char>::value, "");
  static_assert(std::is_same<std::integer_sequence<long long>::value_type, long long>::value, "");
  static_assert(std::is_same<std::index_sequence<>::value_type, std::size_t>::value, "");

  static_assert(std::is_same<std::make_index_sequence<0>, std::index_sequence<>>::value, "");
  static_assert(std::is_same<std::make_index_sequence<1>, std::index_sequence<0>>::value, "");
  static_assert(std::is_same<std::make_index_sequence<2>, std::index_sequence<0, 1>>::value, "");
  static_assert(std::is_same<std::make_index_sequence<3>, std::index_sequence<0, 1, 2>>::value, "");
  static_assert(std::is_same<std::make_index_sequence<4>, std::index_sequence<0, 1, 2, 3>>::value, "");
  static_assert(std::is_same<std::make_index_sequence<5>, std::index_sequence<0, 1, 2, 3, 4>>::value, "");
  static_assert(std::is_same<std::make_index_sequence<6>, std::index_sequence<0, 1, 2, 3, 4, 5>>::value, "");
  static_assert(std::is_same<std::make_index_sequence<7>, std::index_sequence<0, 1, 2, 3, 4, 5, 6>>::value, "");
  static_assert(std::is_same<std::make_index_sequence<8>, std::index_sequence<0, 1, 2, 3, 4, 5, 6, 7>>::value, "");

  static_assert(std::make_index_sequence<5>::size() == 5, "");
  static_assert(std::index_sequence_for<float, int, double>::size() == 3, "");

  std::cout << std::make_integer_sequence<int, 5>::size() << std::endl;
  doIt(1, 2, 3, "hallo", 3.4, 3.52f);
  return 0;
}
