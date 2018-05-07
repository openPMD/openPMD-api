#define CATCH_CONFIG_MAIN

/* make Writable::parent visible for hierarchy check */
#define protected public
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/Dataset.hpp"
#undef protected
using namespace openPMD;

#include <catch/catch.hpp>


TEST_CASE( "string_test", "[auxiliary]" )
{
    using namespace auxiliary;

    std::string s = "Man muss noch Chaos in sich haben, "
                    "um einen tanzenden Stern gebaeren zu koennen.";
    REQUIRE(starts_with(s, "M"));
    REQUIRE(starts_with(s, "Man"));
    REQUIRE(starts_with(s, "Man muss noch"));
    REQUIRE(!starts_with(s, " "));

    REQUIRE(ends_with(s, "."));
    REQUIRE(ends_with(s, "koennen."));
    REQUIRE(ends_with(s, "gebaeren zu koennen."));

    REQUIRE(contains(s, "M"));
    REQUIRE(contains(s, "."));
    REQUIRE(contains(s, "noch Chaos"));
    REQUIRE(!contains(s, "foo"));

    REQUIRE("String" == replace_first("string", "s", "S"));
    REQUIRE("sTRING" == replace_first("string", "tring", "TRING"));
    REQUIRE("string" == replace_first("string", " ", "_"));
    REQUIRE("strinGstringstring" == replace_first("stringstringstring", "g", "G"));
    REQUIRE("#stringstring" == replace_first("stringstringstring", "string", "#"));

    REQUIRE("stringstringstrinG" == replace_last("stringstringstring", "g", "G"));
    REQUIRE("stringstring#" == replace_last("stringstringstring", "string", "#"));

    REQUIRE("/normal/path" == replace_all("////normal//////path", "//", "/"));

    std::vector< std::string > expected1{"0", "string", " ",  "1234", "te st"};
    std::vector< std::string > expected2{"0_DELIM_", "string_DELIM_", " _DELIM_",  "1234_DELIM_", "te st_DELIM_"};
    std::vector< std::string > expected3{"path", "to", "relevant", "data"};
    std::string s2 = "_DELIM_0_DELIM_string_DELIM_ _DELIM_1234_DELIM_te st_DELIM_";
    REQUIRE(expected1 == split(s2, "_DELIM_", false));
    REQUIRE(expected2 == split(s2, "_DELIM_", true));
    REQUIRE(expected3 == split("/path/to/relevant/data/", "/"));

    REQUIRE("stringstringstring" == strip("\t string\tstring string\0", { '\0', '\t', ' '}));
    REQUIRE("stringstringstring" == strip("stringstringstring", { }));

    REQUIRE("1,2,3,4" == join({"1", "2", "3", "4"}, ","));
    REQUIRE("1234" == join({"1", "2", "3", "4"}, ""));
    REQUIRE("" == join({}, ","));
    REQUIRE("1" == join({"1"}, ","));
    REQUIRE("1" == join({"1"}, ""));
    REQUIRE("1,2" == join({"1", "2"}, ","));
}

namespace openPMD
{
namespace test
{
struct S : public Attributable
{ };
} // test
} // openPMD

TEST_CASE( "container_default_test", "[auxiliary]")
{
    Container< openPMD::test::S > c = Container< openPMD::test::S >();

    REQUIRE(c.size() == 0);
    REQUIRE(c.erase("nonExistentKey") == false);
}

namespace openPMD
{
namespace test
{
class structure : public Attributable
{
public:
    std::string string_ = "Hello, world!";
    int int_ = 42;
    float float_ = 3.14f;

    std::string text() const { return variantSrc::get< std::string >(getAttribute("text").getResource()); }
    structure& setText(std::string text) { setAttribute("text", text); return *this; }
};
} // test
} // openPMD

TEST_CASE( "container_retrieve_test", "[auxiliary]" )
{
    using structure = openPMD::test::structure;
    Container< structure > c = Container< structure >();

    structure s;
    std::string text = "The openPMD standard, short for open standard for particle-mesh data files is not a file format per se. It is a standard for meta data and naming schemes.";
    s.setText(text);
    c["entry"] = s;
    REQUIRE(c["entry"].string_ == "Hello, world!");
    REQUIRE(c["entry"].int_ == 42);
    REQUIRE(c["entry"].float_ == 3.14f);
    REQUIRE(c["entry"].text() == text);
    REQUIRE(s.text() == text);


    structure s2 = c["entry"];
    REQUIRE(s2.string_ == "Hello, world!");
    REQUIRE(s2.int_ == 42);
    REQUIRE(s2.float_ == 3.14f);
    REQUIRE(s2.text() == text);
    REQUIRE(c["entry"].text() == text);


    s2.string_ = "New string";
    s2.int_ = -1;
    s2.float_ = 0.0f;
    text = "New text";
    s2.setText(text);
    c["entry"] = s2;
    REQUIRE(c["entry"].string_ == "New string");
    REQUIRE(c["entry"].int_ == -1);
    REQUIRE(c["entry"].float_ == 0.0f);
    REQUIRE(c["entry"].text() == text);
    REQUIRE(s2.text() == text);

    s = c["entry"];
    REQUIRE(s.string_ == "New string");
    REQUIRE(s.int_ == -1);
    REQUIRE(s.float_ == 0.0f);
    REQUIRE(s.text() == text);
    REQUIRE(c["entry"].text() == text);

    text = "Different text";
    c["entry"].setText(text);
    REQUIRE(s.text() == text);
    REQUIRE(c["entry"].text() == text);

    text = "Also different text";
    s.setText(text);
    REQUIRE(s.text() == text);
    REQUIRE(c["entry"].text() == text);
}

namespace openPMD
{
namespace test
{
struct Widget : public Attributable
{
    Widget()
    { }

    Widget(int)
    { }
};
} // test
} // openPMD

TEST_CASE( "container_access_test", "[auxiliary]" )
{
    using Widget = openPMD::test::Widget;
    Container< Widget > c = Container< Widget >();

    c["firstWidget"] = Widget(0);
    REQUIRE(c.size() == 1);

    c["firstWidget"] = Widget(1);
    REQUIRE(c.size() == 1);

    c["secondWidget"] = Widget(2);
    REQUIRE(c.size() == 2);
    REQUIRE(c.erase("firstWidget") == true);
    REQUIRE(c.size() == 1);
    REQUIRE(c.erase("nonExistentWidget") == false);
    REQUIRE(c.size() == 1);
    REQUIRE(c.erase("secondWidget") == true);
    REQUIRE(c.size() == 0);
}

TEST_CASE( "attributable_default_test", "[auxiliary]" )
{
    Attributable a;

    REQUIRE(a.numAttributes() == 0);
}

namespace openPMD
{
namespace test
{
class AttributedWidget : public Attributable
{
public:
    Attribute::resource get(std::string key)
    {
        return getAttribute(key).getResource();
    }
};
} // test
} // openPMD

TEST_CASE( "attributable_access_test", "[auxiliary]" )
{
    using AttributedWidget = openPMD::test::AttributedWidget;
    AttributedWidget a = AttributedWidget();

    a.setAttribute("key", std::string("value"));
    REQUIRE(a.numAttributes() == 1);
    REQUIRE(variantSrc::get< std::string >(a.get("key")) == "value");

    a.setAttribute("key", std::string("newValue"));
    REQUIRE(a.numAttributes() == 1);
    REQUIRE(variantSrc::get< std::string >(a.get("key")) == "newValue");

    using array_t = std::array< double, 7 >;
    array_t arr{{1, 2, 3, 4, 5, 6, 7}};
    a.setAttribute("array", arr);
    REQUIRE(a.numAttributes() == 2);
    REQUIRE(variantSrc::get< array_t >(a.get("array")) == arr);
    REQUIRE(a.deleteAttribute("nonExistentKey") == false);
    REQUIRE(a.numAttributes() == 2);
    REQUIRE(a.deleteAttribute("key") == true);
    REQUIRE(a.numAttributes() == 1);
    REQUIRE(a.deleteAttribute("array") == true);
    REQUIRE(a.numAttributes() == 0);

    a.setComment("This is a comment");
    REQUIRE(a.comment() == "This is a comment");
    REQUIRE(a.numAttributes() == 1);
}

namespace openPMD
{
namespace test
{
class Dotty : public Attributable
{
public:
    Dotty()
    {
        setAtt1(1);
        setAtt2(2);
        setAtt3("3");
    }

    int att1() const { return variantSrc::get< int >(getAttribute("att1").getResource()); }
    double att2() const { return variantSrc::get< double >(getAttribute("att2").getResource()); }
    std::string att3() const { return variantSrc::get< std::string >(getAttribute("att3").getResource()); }
    Dotty& setAtt1(int i) { setAttribute("att1", i); return *this; }
    Dotty& setAtt2(double d) { setAttribute("att2", d); return *this; }
    Dotty& setAtt3(std::string s) { setAttribute("att3", s); return *this; }
};
} // test
} // openPMD

TEST_CASE( "dot_test", "[auxiliary]" )
{
    openPMD::test::Dotty d;
    REQUIRE(d.att1() == 1);
    REQUIRE(d.att2() == static_cast<double>(2));
    REQUIRE(d.att3() == "3");

    d.setAtt1(10).setAtt2(20).setAtt3("30");
    REQUIRE(d.att1() == 10);
    REQUIRE(d.att2() == static_cast<double>(20));
    REQUIRE(d.att3() == "30");

}
