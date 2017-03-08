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

class structure : public Attributable
{
public:
    std::string string_ = "Hello, world!";
    int int_ = 42;
    float float_ = 3.14f;

    std::string text() const { return boost::get< std::string >(getAttribute("text").getResource()); }
    structure& setText(std::string text) { setAttribute("text", text); return *this; }
};

BOOST_AUTO_TEST_CASE(container_retrieve_test)
{
    Container< structure > c = Container< structure >();

    structure s;
    std::string text = "The openPMD standard, short for open standard for particle-mesh data files is not a file format per se. It is a standard for meta data and naming schemes.";
    s.setText(text);
    c["entry"] = s;
    BOOST_TEST(c["entry"].string_ == "Hello, world!");
    BOOST_TEST(c["entry"].int_ == 42);
    BOOST_TEST(c["entry"].float_ == 3.14f);
    BOOST_TEST(c["entry"].text() == text);
    BOOST_TEST(s.text() == text);


    structure s2 = c["entry"];
    BOOST_TEST(s2.string_ == "Hello, world!");
    BOOST_TEST(s2.int_ == 42);
    BOOST_TEST(s2.float_ == 3.14f);
    BOOST_TEST(s2.text() == text);
    BOOST_TEST(c["entry"].text() == text);


    s2.string_ = "New string";
    s2.int_ = -1;
    s2.float_ = 0.0f;
    text = "New text";
    s2.setText(text);
    c["entry"] = s2;
    BOOST_TEST(c["entry"].string_ == "New string");
    BOOST_TEST(c["entry"].int_ == -1);
    BOOST_TEST(c["entry"].float_ == 0.0f);
    BOOST_TEST(c["entry"].text() == text);
    BOOST_TEST(s2.text() == text);

    s = c["entry"];
    BOOST_TEST(s.string_ == "New string");
    BOOST_TEST(s.int_ == -1);
    BOOST_TEST(s.float_ == 0.0f);
    BOOST_TEST(s.text() == text);
    BOOST_TEST(c["entry"].text() == text);

    c["entry"].setText("Different text");
    BOOST_TEST(s.text() == text);
    BOOST_TEST(c["entry"].text() != text);

    s.setText("Also different text");
    BOOST_TEST(s.text() == "Also different text");
    BOOST_TEST(c["entry"].text() == "Different text");
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
        return getAttribute(key).getResource();
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

    a.setComment("This is a comment");
    BOOST_TEST(a.comment() == "This is a comment");
    BOOST_TEST(a.numAttributes() == 1);
}

class Dotty : public Attributable
{
public:
    Dotty()
    {
        setAtt1(1);
        setAtt2(2);
        setAtt3("3");
    }

    int att1() const { return boost::get< int >(getAttribute("att1").getResource()); }
    double att2() const { return boost::get< double >(getAttribute("att2").getResource()); }
    std::string att3() const { return boost::get< std::string >(getAttribute("att3").getResource()); }
    Dotty& setAtt1(int i) { setAttribute("att1", i); return *this; }
    Dotty& setAtt2(double d) { setAttribute("att2", d); return *this; }
    Dotty& setAtt3(std::string s) { setAttribute("att3", s); return *this; }
};

BOOST_AUTO_TEST_CASE(dot_test)
{
    Dotty d;
    BOOST_TEST(d.att1() == 1);
    BOOST_TEST(d.att2() == static_cast<double>(2));
    BOOST_TEST(d.att3() == "3");

    d.setAtt1(10).setAtt2(20).setAtt3("30");
    BOOST_TEST(d.att1() == 10);
    BOOST_TEST(d.att2() == static_cast<double>(20));
    BOOST_TEST(d.att3() == "30");

}
