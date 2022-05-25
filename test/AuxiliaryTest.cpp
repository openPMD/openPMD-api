// expose private and protected members for invasive testing
#if openPMD_USE_INVASIVE_TESTS
#define OPENPMD_private public
#define OPENPMD_protected public
#endif
#include "openPMD/Dataset.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerHelper.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/Option.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/config.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace openPMD;

namespace openPMD
{
namespace test
{
    struct TestHelper : public LegacyAttributable
    {
        TestHelper()
        {
            writable().IOHandler =
                createIOHandler(".", Access::CREATE, Format::JSON);
        }
    };
} // namespace test
} // namespace openPMD

TEST_CASE("optional", "[auxiliary]")
{
    using namespace auxiliary;

    Option<int> opt;

    REQUIRE_THROWS_AS(opt.get(), variantSrc::bad_variant_access);
    REQUIRE_THROWS_AS(opt.get() = 42, variantSrc::bad_variant_access);
    REQUIRE(!opt);
    REQUIRE(!opt.has_value());

    opt = 43;
    REQUIRE(opt);
    REQUIRE(opt.has_value());
    REQUIRE(opt.get() == 43);

    Option<int> opt2{opt};
    REQUIRE(opt2);
    REQUIRE(opt2.has_value());
    REQUIRE(opt2.get() == 43);

    Option<int> opt3 = makeOption(3);
    REQUIRE(opt3);
    REQUIRE(opt3.has_value());
    REQUIRE(opt3.get() == 3);
}

TEST_CASE("deref_cast_test", "[auxiliary]")
{
    using namespace auxiliary;

    struct A
    {
        double m_x;
        A(double x) : m_x(x)
        {}
        virtual ~A() = default;
    };
    struct B : virtual A
    {
        B(double x) : A(x)
        {}
    };
    struct C
    {
        float m_x;
    };

    B const value = {123.45};
    B const *const ptr = &value;

    auto const a = deref_dynamic_cast<A const>(ptr);
    auto const &ra = deref_dynamic_cast<A const>(ptr);
    (void)a;
    (void)ra;

    REQUIRE_THROWS_AS(deref_dynamic_cast<C const>(ptr), std::runtime_error);

    A *const nptr = nullptr;
    REQUIRE_THROWS_AS(deref_dynamic_cast<B>(nptr), std::runtime_error);
}

TEST_CASE("string_test", "[auxiliary]")
{
    using namespace auxiliary;

    std::string s =
        "Man muss noch Chaos in sich haben, "
        "um einen tanzenden Stern gebaeren zu koennen.";
    REQUIRE(starts_with(s, 'M'));
    REQUIRE(starts_with(s, "Man"));
    REQUIRE(starts_with(s, "Man muss noch"));
    REQUIRE(!starts_with(s, ' '));

    REQUIRE(ends_with(s, '.'));
    REQUIRE(ends_with(s, "koennen."));
    REQUIRE(ends_with(s, "gebaeren zu koennen."));

    REQUIRE(contains(s, 'M'));
    REQUIRE(contains(s, '.'));
    REQUIRE(contains(s, "noch Chaos"));
    REQUIRE(!contains(s, "foo"));

    REQUIRE("String" == replace_first("string", "s", "S"));
    REQUIRE("sTRING" == replace_first("string", "tring", "TRING"));
    REQUIRE("string" == replace_first("string", " ", "_"));
    REQUIRE(
        "strinGstringstring" == replace_first("stringstringstring", "g", "G"));
    REQUIRE(
        "#stringstring" == replace_first("stringstringstring", "string", "#"));

    REQUIRE(
        "stringstringstrinG" == replace_last("stringstringstring", "g", "G"));
    REQUIRE(
        "stringstring#" == replace_last("stringstringstring", "string", "#"));

    REQUIRE("/normal/path" == replace_all("////normal//////path", "//", "/"));

    std::vector<std::string> expected1{"0", "string", " ", "1234", "te st"};
    std::vector<std::string> expected2{
        "0_DELIM_", "string_DELIM_", " _DELIM_", "1234_DELIM_", "te st_DELIM_"};
    std::vector<std::string> expected3{"path", "to", "relevant", "data"};
    std::string s2 =
        "_DELIM_0_DELIM_string_DELIM_ _DELIM_1234_DELIM_te st_DELIM_";
    REQUIRE(expected1 == split(s2, "_DELIM_", false));
    REQUIRE(expected2 == split(s2, "_DELIM_", true));
    REQUIRE(expected3 == split("/path/to/relevant/data/", "/"));

    REQUIRE(
        "stringstringstring" ==
        strip("\t string\tstring string\0", {'\0', '\t', ' '}));
    REQUIRE("stringstringstring" == strip("stringstringstring", {}));

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
    struct S : public TestHelper
    {
        S() : TestHelper()
        {}
    };
} // namespace test
} // namespace openPMD

TEST_CASE("container_default_test", "[auxiliary]")
{
#if openPMD_USE_INVASIVE_TESTS
    Container<openPMD::test::S> c = Container<openPMD::test::S>();
    c.writable().IOHandler = createIOHandler(".", Access::CREATE, Format::JSON);

    REQUIRE(c.empty());
    REQUIRE(c.erase("nonExistentKey") == false);
#else
    std::cerr << "Invasive tests not enabled. Hierarchy is not visible.\n";
#endif
}

namespace openPMD
{
namespace test
{
    struct structure : public TestHelper
    {
        structure() : TestHelper()
        {}

        std::string string_ = "Hello, world!";
        int int_ = 42;
        float float_ = 3.14f;

        std::string text() const
        {
            return variantSrc::get<std::string>(
                getAttribute("text").getResource());
        }
        structure &setText(std::string newText)
        {
            setAttribute("text", newText);
            return *this;
        }
    };
} // namespace test
} // namespace openPMD

TEST_CASE("container_retrieve_test", "[auxiliary]")
{
#if openPMD_USE_INVASIVE_TESTS
    using structure = openPMD::test::structure;
    Container<structure> c = Container<structure>();
    c.writable().IOHandler = createIOHandler(".", Access::CREATE, Format::JSON);

    structure s;
    std::string text =
        "The openPMD standard, short for open standard for particle-mesh data "
        "files is not a file format per se. It is a standard for meta data and "
        "naming schemes.";
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
#else
    std::cerr << "Invasive tests not enabled. Hierarchy is not visible.\n";
#endif
}

namespace openPMD
{
namespace test
{
    struct Widget : public TestHelper
    {
        Widget() : TestHelper()
        {}

        Widget(int) : TestHelper()
        {}
    };
} // namespace test
} // namespace openPMD

TEST_CASE("container_access_test", "[auxiliary]")
{
#if openPMD_USE_INVASIVE_TESTS
    using Widget = openPMD::test::Widget;
    Container<Widget> c = Container<Widget>();
    c.writable().IOHandler = createIOHandler(".", Access::CREATE, Format::JSON);

    c["1firstWidget"] = Widget(0);
    REQUIRE(c.size() == 1);

    c["1firstWidget"] = Widget(1);
    REQUIRE(c.size() == 1);

    c["2secondWidget"] = Widget(2);
    c["3thirdWidget"] = Widget(3);
    c["4fourthWidget"] = Widget(4);
    c["5fifthWidget"] = Widget(5);

    REQUIRE(c.size() == 5);
    REQUIRE(c.erase("1firstWidget") == true);
    REQUIRE(c.size() == 4);
    REQUIRE(c.erase("nonExistentWidget") == false);
    REQUIRE(c.size() == 4);
    REQUIRE(c.erase("2secondWidget") == true);
    REQUIRE(c.size() == 3);
    REQUIRE(c.erase(c.find("5fifthWidget")) == c.end());
    REQUIRE(c.size() == 2);
    REQUIRE(c.erase(c.find("3thirdWidget")) == c.find("4fourthWidget"));
    REQUIRE(c.size() == 1);
    REQUIRE(c.erase(c.find("4fourthWidget")) == c.end());
    REQUIRE(c.size() == 0);
    REQUIRE(c.empty());
#else
    std::cerr << "Invasive tests not enabled. Hierarchy is not visible.\n";
#endif
}

TEST_CASE("attributable_default_test", "[auxiliary]")
{
    LegacyAttributable a;

    REQUIRE(a.numAttributes() == 0);
}

namespace openPMD
{
namespace test
{
    struct AttributedWidget : public TestHelper
    {
        AttributedWidget() : TestHelper()
        {}

        Attribute::resource get(std::string key)
        {
            return getAttribute(key).getResource();
        }
    };
} // namespace test
} // namespace openPMD

TEST_CASE("attributable_access_test", "[auxiliary]")
{
    using AttributedWidget = openPMD::test::AttributedWidget;
    AttributedWidget a = AttributedWidget();

    a.setAttribute("key", std::string("value"));
    REQUIRE(a.numAttributes() == 1);
    REQUIRE(variantSrc::get<std::string>(a.get("key")) == "value");

    a.setAttribute("key", std::string("newValue"));
    REQUIRE(a.numAttributes() == 1);
    REQUIRE(variantSrc::get<std::string>(a.get("key")) == "newValue");

    using array_t = std::array<double, 7>;
    array_t arr{{1, 2, 3, 4, 5, 6, 7}};
    a.setAttribute("array", arr);
    REQUIRE(a.numAttributes() == 2);
    REQUIRE(variantSrc::get<array_t>(a.get("array")) == arr);
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
    struct Dotty : public TestHelper
    {
        Dotty() : TestHelper()
        {
            setAtt1(1);
            setAtt2(2);
            setAtt3("3");
        }

        int att1() const
        {
            return variantSrc::get<int>(getAttribute("att1").getResource());
        }
        double att2() const
        {
            return variantSrc::get<double>(getAttribute("att2").getResource());
        }
        std::string att3() const
        {
            return variantSrc::get<std::string>(
                getAttribute("att3").getResource());
        }
        Dotty &setAtt1(int i)
        {
            setAttribute("att1", i);
            return *this;
        }
        Dotty &setAtt2(double d)
        {
            setAttribute("att2", d);
            return *this;
        }
        Dotty &setAtt3(std::string s)
        {
            setAttribute("att3", s);
            return *this;
        }
    };
} // namespace test
} // namespace openPMD

TEST_CASE("dot_test", "[auxiliary]")
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

TEST_CASE("filesystem_test", "[auxiliary]")
{
    using auxiliary::create_directories;
    using auxiliary::file_exists;
    using auxiliary::directory_exists;
    using auxiliary::list_directory;
    using auxiliary::remove_directory;
    using auxiliary::remove_file;

    auto contains = [](std::vector<std::string> const &entries,
                       std::string const &path) -> bool {
        return std::find(entries.cbegin(), entries.cend(), path) !=
            entries.cend();
    };

    auto random_string = [](std::string::size_type length) -> std::string {
        auto randchar = []() -> char {
            char const charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
            size_t const max_index = (sizeof(charset) - 1);
            return charset[rand() % max_index];
        };
        std::string str(length, 0);
        std::generate_n(str.begin(), length, randchar);
        return str;
    };

#ifdef _WIN32
    REQUIRE(directory_exists("C:\\"));
    REQUIRE(directory_exists("C:\\Program Files"));
    REQUIRE(directory_exists("C:\\Windows"));
    REQUIRE(!directory_exists("C:\\nonexistent_folder_in_C_drive"));

    auto dir_entries = list_directory("C:\\");
    REQUIRE(!dir_entries.empty());
    REQUIRE(contains(dir_entries, "Program Files"));
    REQUIRE(contains(dir_entries, "Windows"));
    REQUIRE(!contains(dir_entries, "nonexistent_folder_in_C_drive"));

    std::string new_directory = random_string(10);
    while (directory_exists(new_directory))
        new_directory = random_string(10);
    REQUIRE(create_directories(new_directory));
    REQUIRE(create_directories(new_directory));
    REQUIRE(directory_exists(new_directory));

    std::string new_file = new_directory + "\\abc.txt";
    std::fstream fs;
    fs.open(new_file, std::ios::out);
    fs.close();

    REQUIRE(file_exists(new_file));
    REQUIRE(remove_file(new_file));
    REQUIRE(!file_exists(new_file));

    REQUIRE(remove_directory(new_directory));
    REQUIRE(!directory_exists(new_directory));
    REQUIRE(!remove_directory(new_directory));

    REQUIRE(!remove_file(".\\nonexistent_file_in_cmake_bin_directory"));
#else
    REQUIRE(directory_exists("/"));
    // REQUIRE(directory_exists("/boot"));
    // REQUIRE(directory_exists("/etc"));
    // REQUIRE(directory_exists("/home"));
    REQUIRE(!directory_exists("/nonexistent_folder_in_root_directory"));
    REQUIRE(directory_exists("../bin"));

    REQUIRE(file_exists("./AuxiliaryTests"));
    REQUIRE(!file_exists("./nonexistent_file_in_cmake_bin_directory"));

    auto dir_entries = list_directory("/");
    REQUIRE(!dir_entries.empty());
    // REQUIRE(contains(dir_entries, "boot"));
    // REQUIRE(contains(dir_entries, "etc"));
    // REQUIRE(contains(dir_entries, "home"));
    // REQUIRE(contains(dir_entries, "root"));
    REQUIRE(!contains(dir_entries, "nonexistent_folder_in_root_directory"));

    std::string new_directory = random_string(10);
    while (directory_exists(new_directory))
        new_directory = random_string(10);
    std::string new_sub_directory = new_directory + "/" + random_string(10);
    REQUIRE(create_directories(new_sub_directory));
    REQUIRE(create_directories(new_directory));
    REQUIRE(directory_exists(new_sub_directory));
    REQUIRE(directory_exists(new_directory));

    std::string new_file = new_directory + "/abc.txt";
    std::fstream fs;
    fs.open(new_file, std::ios::out);
    fs.close();

    REQUIRE(file_exists(new_file));
    REQUIRE(remove_file(new_file));
    REQUIRE(!file_exists(new_file));

    REQUIRE(remove_directory(new_directory));
    REQUIRE(!directory_exists(new_directory));
    REQUIRE(!directory_exists(new_sub_directory));
    REQUIRE(!remove_directory(new_directory));
    REQUIRE(!remove_directory(new_sub_directory));

    REQUIRE(!remove_file("./nonexistent_file_in_cmake_bin_directory"));
#endif
}
