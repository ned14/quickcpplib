namespace boost { namespace afio { inline namespace v1_std_boost_asio { inline namespace stl11 {

int foo()
{
  return 1;
}

} } } }

extern "C" void printf(const char *, ...);
int main(void)
{
  printf("foo=%d\n", boost :: afio :: v1_std_boost_asio::foo());
  return 0;
}
