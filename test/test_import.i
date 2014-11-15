namespace boost { namespace afio { inline namespace v1_std { inline namespace stl11 {

int foo()
{
  return 1;
}

} } } }

extern "C" void printf(const char *, ...);
int main(void)
{
  printf("foo=%d\n", boost :: afio :: v1_std::foo());
  return 0;
}
