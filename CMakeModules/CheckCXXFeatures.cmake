check_cxx_source_compiles("
  #include <utility>
  #include <type_traits>
  std::make_index_sequence<0> t;
  int main() {}" HAVE_STD_INDEX_SEQUENCE
)

check_cxx_source_compiles("
  #include <type_traits>
  std::remove_cv_t<const int> t;
  int main() {}" HAVE_STD_REMOVE_CV_T
)

check_cxx_source_compiles("
  #include <type_traits>
  std::remove_cvref_t<const int&> t;
  int main() {}" HAVE_STD_REMOVE_CVREF_T
)

check_cxx_source_compiles("
  #include <type_traits>
  std::enable_if_t<true, int> t;
  int main() {}" HAVE_STD_ENABLE_IF_T
)

check_cxx_source_compiles("
  #include <type_traits>
  std::bool_constant<true> t;
  int main() {}" HAVE_STD_BOOL_CONSTANT
)