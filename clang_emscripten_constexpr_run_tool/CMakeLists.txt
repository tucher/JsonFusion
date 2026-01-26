add_clang_executable(clang-constexpr-run
  clang-constexpr-run.cpp
)

target_link_libraries(clang-constexpr-run PRIVATE
  clangAST
  clangBasic
  clangFrontend
  clangLex
  clangParse
  clangSema
  clangSerialization
  LLVMSupport
)
