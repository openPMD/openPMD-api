#define BOOST_TEST_MODULE libopenpmd_aux_test


#include <boost/test/included/unit_test.hpp>

#include "../include/Attributable.hpp"
#include "../include/Container.hpp"


BOOST_AUTO_TEST_CASE(container_default_test)
{
    Container< int > c = Container< int >();

    BOOST_TEST(c.size() == 0);
    BOOST_TEST(c.remove("nonExistentKey") == false);
}

struct Widget
{
    Widget()
    { }

    Widget(int)
    { }
};

BOOST_AUTO_TEST_CASE(container_access_test)
{
    Container< Widget > c = Container< Widget >();

    c["firstWidget"] = Widget(0);
    BOOST_TEST(c.size() == 1);

    c["firstWidget"] = Widget(1);
    BOOST_TEST(c.size() == 1);

    c["secondWidget"] = Widget(2);
    BOOST_TEST(c.size() == 2);
    BOOST_TEST(c.remove("firstWidget") == true);
    BOOST_TEST(c.size() == 1);
    BOOST_TEST(c.remove("nonExistentWidget") == false);
    BOOST_TEST(c.size() == 1);
    BOOST_TEST(c.remove("secondWidget") == true);
    BOOST_TEST(c.size() == 0);
}

BOOST_AUTO_TEST_CASE(attributable_default_test)
{
    Attributable a = Attributable();

    BOOST_TEST(a.numAttributes() == 0);
}

class AttributedWidget : public Attributable
{
public:
    Attribute::resource get(std::string key)
    {
        return getAttribute(key);
    }
};

BOOST_AUTO_TEST_CASE(attributable_access_test)
{
    AttributedWidget a = AttributedWidget();

    a.setAttribute("key", "value");
    BOOST_TEST(a.numAttributes() == 1);
    BOOST_TEST(boost::get< std::string >(a.get("key")) == "value");

    a.setAttribute("key", "newValue");
    BOOST_TEST(a.numAttributes() == 1);
    BOOST_TEST(boost::get< std::string >(a.get("key")) == "newValue");

    using array_t = std::array< double, 7 >;
    array_t arr{{1, 2, 3, 4, 5, 6, 7}};
    a.setAttribute("array", arr);
    BOOST_TEST(a.numAttributes() == 2);
    BOOST_TEST(boost::get< array_t >(a.get("array")) == arr);
    BOOST_TEST(a.deleteAttribute("nonExistentKey") == false);
    BOOST_TEST(a.numAttributes() == 2);
    BOOST_TEST(a.deleteAttribute("key") == true);
    BOOST_TEST(a.numAttributes() == 1);
    BOOST_TEST(a.deleteAttribute("array") == true);
    BOOST_TEST(a.numAttributes() == 0);
}
