#include "../include/bind/stl11/boost/ratio"
#include "../include/bind/stl11/std/ratio"
#include "../include/boost/test/unit_test.hpp"

BOOST_AUTO_TEST_SUITE(bind)

BOOST_AUTO_TEST_CASE(bind / works, "Tests that the STL binds work as advertised")
{
  // In this context use Boost's ratio
  {
    using namespace boost_lite::bind::boost::ratio;
    using two_thirds = ratio<2, 3>;
    using one_sixth = ratio<1, 6>;
    BOOST_CHECK((ratio_less<one_sixth, two_thirds>::value));
  }
  // In this context use STL11's ratio
  {
    using namespace boost_lite::bind::std::ratio;
    using two_thirds = ratio<2, 3>;
    using one_sixth = ratio<1, 6>;
    BOOST_CHECK((ratio_less<one_sixth, two_thirds>::value));
  }
}

BOOST_AUTO_TEST_SUITE_END()
