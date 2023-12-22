#include "openPMD/auxiliary/TemplateFile.hpp"
#include "openPMD/DatatypeHelpers.hpp"

#include <iostream>

namespace openPMD::auxiliary
{
namespace
{
    // Some forward declarations
    template <typename T>
    void initializeFromTemplate(
        Container<T> &initializeMe, Container<T> const &fromTemplate);

    struct SetAttribute
    {
        template <typename T>
        static void
        call(Attributable &object, std::string const &name, Attribute attr)
        {
            object.setAttribute(name, attr.get<T>());
        }

        template <unsigned n>
        static void call(Attributable &, std::string const &name, Attribute)
        {
            std::cerr << "Unknown datatype for template attribute '" << name
                      << "'. Will skip it." << std::endl;
        }
    };

    void copyAttributes(
        Attributable &target,
        Attributable const &source,
        std::vector<std::string> ignore = {})
    {
#if 0 // leave this in for potential future debugging
        std::cout << "COPYING ATTRIBUTES FROM '" << [&source]() -> std::string {
            auto vec = source.myPath().group;
            if (vec.empty())
            {
                return "[]";
            }
            std::stringstream sstream;
            auto it = vec.begin();
            sstream << "[" << *it++;
            for (; it != vec.end(); ++it)
            {
                sstream << ", " << *it;
            }
            sstream << "]";
            return sstream.str();
        }() << "'"
            << std::endl;
#endif
        auto shouldBeIgnored = [&ignore](std::string const &attrName) {
            // `ignore` is empty by default and normally has only a handful of
            // entries otherwise.
            // So just use linear search.
            for (auto const &ignored : ignore)
            {
                if (attrName == ignored)
                {
                    return true;
                }
            }
            return false;
        };

        for (auto const &attrName : source.attributes())
        {
            if (shouldBeIgnored(attrName))
            {
                continue;
            }
            auto attr = source.getAttribute(attrName);
            auto dtype = attr.dtype;
            switchType<SetAttribute>(dtype, target, attrName, std::move(attr));
        }
    }

    void initializeFromTemplate(
        BaseRecordComponent &initializeMe,
        BaseRecordComponent const &fromTemplate)
    {
        copyAttributes(initializeMe, fromTemplate);
    }

    void initializeFromTemplate(
        RecordComponent &initializeMe, RecordComponent const &fromTemplate)
    {
        if (fromTemplate.getDatatype() != Datatype::UNDEFINED)
        {
            initializeMe.resetDataset(
                Dataset{fromTemplate.getDatatype(), fromTemplate.getExtent()});
        }
        initializeFromTemplate(
            static_cast<BaseRecordComponent &>(initializeMe),
            static_cast<BaseRecordComponent const &>(fromTemplate));
    }

    void initializeFromTemplate(
        PatchRecordComponent &initializeMe,
        PatchRecordComponent const &fromTemplate)
    {
        if (fromTemplate.getDatatype() != Datatype::UNDEFINED)
        {
            initializeMe.resetDataset(
                Dataset{fromTemplate.getDatatype(), fromTemplate.getExtent()});
        }
        initializeFromTemplate(
            static_cast<BaseRecordComponent &>(initializeMe),
            static_cast<BaseRecordComponent const &>(fromTemplate));
    }

    template <typename T>
    void initializeFromTemplate(
        BaseRecord<T> &initializeMe, BaseRecord<T> const &fromTemplate)
    {
        if (fromTemplate.scalar())
        {
            initializeMe[RecordComponent::SCALAR];
            initializeFromTemplate(
                static_cast<T &>(initializeMe),
                static_cast<T const &>(fromTemplate));
        }
        else
        {
            initializeFromTemplate(
                static_cast<Container<T> &>(initializeMe),
                static_cast<Container<T> const &>(fromTemplate));
        }
    }

    void initializeFromTemplate(
        ParticleSpecies &initializeMe, ParticleSpecies const &fromTemplate)
    {
        if (!fromTemplate.particlePatches.empty())
        {
            initializeFromTemplate(
                static_cast<Container<PatchRecord> &>(
                    initializeMe.particlePatches),
                static_cast<Container<PatchRecord> const &>(
                    fromTemplate.particlePatches));
        }
        initializeFromTemplate(
            static_cast<Container<Record> &>(initializeMe),
            static_cast<Container<Record> const &>(fromTemplate));
    }

    template <typename T>
    void initializeFromTemplate(
        Container<T> &initializeMe, Container<T> const &fromTemplate)
    {
        copyAttributes(initializeMe, fromTemplate);
        for (auto const &pair : fromTemplate)
        {
            initializeFromTemplate(initializeMe[pair.first], pair.second);
        }
    }

    void initializeFromTemplate(
        Iteration &initializeMe, Iteration const &fromTemplate)
    {
        copyAttributes(initializeMe, fromTemplate, {"snapshot"});
        if (fromTemplate.hasMeshes())
        {
            initializeFromTemplate(initializeMe.meshes, fromTemplate.meshes);
        }
        if (fromTemplate.hasParticles())
        {
            initializeFromTemplate(
                initializeMe.particles, fromTemplate.particles);
        }
    }
} // namespace

Series &initializeFromTemplate(
    Series &initializeMe, Series const &fromTemplate, uint64_t iteration)
{
    if (!initializeMe.containsAttribute("from_template"))
    {
        copyAttributes(
            initializeMe,
            fromTemplate,
            {"basePath", "iterationEncoding", "iterationFormat", "openPMD"});
        initializeMe.setAttribute("from_template", fromTemplate.name());
    }

    uint64_t sourceIteration = iteration;
    if (!fromTemplate.iterations.contains(sourceIteration))
    {
        if (fromTemplate.iterations.empty())
        {
            std::cerr << "Template file has no iterations, will only fill in "
                         "global attributes."
                      << std::endl;
            return initializeMe;
        }
        else
        {
            sourceIteration = fromTemplate.iterations.begin()->first;
        }
    }

    initializeFromTemplate(
        initializeMe.iterations[iteration],
        fromTemplate.iterations.at(sourceIteration));
    return initializeMe;
}
} // namespace openPMD::auxiliary
