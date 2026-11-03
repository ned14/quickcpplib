#include <cstddef>
#include <cstdio>

int main()
{
  std::byte b{42};
  std::printf("std::byte ok, __cplusplus=%ld\n", static_cast<long>(__cplusplus));
  return static_cast<int>(b) == 42 ? 0 : 1;
}
