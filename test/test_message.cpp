#include "include/import.h"

BOOST_BINDLIB_NOTE("This is a note")
BOOST_BINDLIB_MESSAGE("This is a message, and it is a good message")
BOOST_BINDLIB_WARNING("This is a warning, and it is a neutral warning")
#ifdef _MSC_VER
BOOST_BINDLIB_ERROR("This is an error, and it is a bad error")
#endif

int main(void)
{
  return 0;
}
