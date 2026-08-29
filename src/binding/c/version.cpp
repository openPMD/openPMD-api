#include <openPMD/binding/c/version.h>

#include <openPMD/version.hpp>

#include <string>
#include <vector>

const int openPMDapi_version_major = OPENPMDAPI_VERSION_MAJOR;
const int openPMDapi_version_minor = OPENPMDAPI_VERSION_MINOR;
const int openPMDapi_version_patch = OPENPMDAPI_VERSION_PATCH;
const char *const openPMDapi_version_label = OPENPMDAPI_VERSION_LABEL;

const int openPMD_standard_major = OPENPMD_STANDARD_MAJOR;
const int openPMD_standard_minor = OPENPMD_STANDARD_MINOR;
const int openPMD_standard_patch = OPENPMD_STANDARD_PATCH;

const int openPMD_standard_min_major = OPENPMD_STANDARD_MIN_MAJOR;
const int openPMD_standard_min_minor = OPENPMD_STANDARD_MIN_MINOR;
const int openPMD_standard_min_patch = OPENPMD_STANDARD_MIN_PATCH;

const char *openPMD_getVersion()
{
    static const std::string version = openPMD::getVersion();
    return version.c_str();
}

const char *openPMD_getStandard()
{
    static const std::string standard = openPMD::getStandardMaximum();
    return standard.c_str();
}

const char *openPMD_getStandardDefault()
{
    static const std::string standard = openPMD::getStandardDefault();
    return standard.c_str();
}

const char *openPMD_getStandardMaximum()
{
    static const std::string standard = openPMD::getStandardMaximum();
    return standard.c_str();
}

const char *openPMD_getStandardMinimum()
{
    static std::string standard_minimum = openPMD::getStandardMinimum();
    return standard_minimum.c_str();
}

const openPMD_Variant *openPMD_getVariants()
{
    static const std::map<std::string, bool> variants = openPMD::getVariants();
    static const std::vector<openPMD_Variant> c_variants = [&]() {
        std::vector<openPMD_Variant> c_vars;
        for (const auto &[var, supp] : variants)
            c_vars.push_back(openPMD_Variant{var.c_str(), supp});
        c_vars.push_back(openPMD_Variant{nullptr, false});
        return c_vars;
    }();
    return c_variants.data();
}

const char *const *openPMD_getFileExtensions()
{
    static const std::vector<std::string> file_extensions =
        openPMD::getFileExtensions();
    static const std::vector<const char *> c_file_extensions = [&]() {
        std::vector<const char *> c_exts;
        for (const auto &ext : file_extensions)
            c_exts.push_back(ext.c_str());
        c_exts.push_back(nullptr);
        return c_exts;
    }();
    return c_file_extensions.data();
}
