add_executable(tests tests.cpp)

target_link_libraries(tests PRIVATE
  toydb_lib
  Catch2::Catch2WithMain
)

target_compile_options(tests PRIVATE
  -fsanitize=address -fsanitize=undefined
  -g -O0
)
target_link_options(tests PRIVATE
  -fsanitize=address -fsanitize=undefined
)

catch_discover_tests(tests)