#define BOOST_TEST_MODULE libopenpmd_aux_test


#include <boost/test/included/unit_test.hpp>

#include "backend/Attributable.hpp"
#include "auxiliary/StringManip.hpp"
#include "backend/Container.hpp"
#include "../include/Dataset.hpp"
#include "backend/Writable.hpp"


BOOST_AUTO_TEST_CASE(string_test)
{
    std::string s = "Man muss noch Chaos in sich haben, "
                    "um einen tanzenden Stern gebären zu können.";
    BOOST_TEST(starts_with(s, "M"));
    BOOST_TEST(starts_with(s, "Man"));
    BOOST_TEST(starts_with(s, "Man muss noch"));
    BOOST_TEST(!starts_with(s, " "));

    BOOST_TEST(ends_with(s, "."));
    BOOST_TEST(ends_with(s, "können."));
    BOOST_TEST(ends_with(s, "gebären zu können."));

    BOOST_TEST(contains(s, "M"));
    BOOST_TEST(contains(s, "."));
    BOOST_TEST(contains(s, "noch Chaos"));
    BOOST_TEST(!contains(s, "foo"));

    BOOST_TEST("String" == replace_first("string", "s", "S"));
    BOOST_TEST("sTRING" == replace_first("string", "tring", "TRING"));
    BOOST_TEST("string" == replace_first("string", " ", "_"));
    BOOST_TEST("strinGstringstring" == replace_first("stringstringstring", "g", "G"));
    BOOST_TEST("#stringstring" == replace_first("stringstringstring", "string", "#"));

    BOOST_TEST("stringstringstrinG" == replace_last("stringstringstring", "g", "G"));
    BOOST_TEST("stringstring#" == replace_last("stringstringstring", "string", "#"));

    BOOST_TEST("/normal/path" == replace_all("////normal//////path", "//", "/"));

    std::vector< std::string > expected1{"0", "string", " ",  "1234", "te st"};
    std::vector< std::string > expected2{"0_DELIM_", "string_DELIM_", " _DELIM_",  "1234_DELIM_", "te st_DELIM_"};
    std::vector< std::string > expected3{"path", "to", "relevant", "data"};
    std::string s2 = "_DELIM_0_DELIM_string_DELIM_ _DELIM_1234_DELIM_te st_DELIM_";
    BOOST_TEST(expected1 == split(s2, "_DELIM_", false));
    BOOST_TEST(expected2 == split(s2, "_DELIM_", true));
    BOOST_TEST(expected3 == split("/path/to/relevant/data/", "/"));
}

BOOST_AUTO_TEST_CASE(container_default_test)
{
    struct S : public Writable
    {
        int val;
        bool written;
    };
    Container< S > c = Container< S >();

    BOOST_TEST(c.size() == 0);
    BOOST_TEST(c.erase("nonExistentKey") == false);
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

struct Widget : public Writable
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
    BOOST_TEST(c.erase("firstWidget") == true);
    BOOST_TEST(c.size() == 1);
    BOOST_TEST(c.erase("nonExistentWidget") == false);
    BOOST_TEST(c.size() == 1);
    BOOST_TEST(c.erase("secondWidget") == true);
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

    a.setAttribute("key", std::string("value"));
    BOOST_TEST(a.numAttributes() == 1);
    BOOST_TEST(boost::get< std::string >(a.get("key")) == "value");

    a.setAttribute("key", std::string("newValue"));
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
