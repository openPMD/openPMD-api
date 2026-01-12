#include "openPMD/backend/ScientificDefaults.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include <iostream>
#include <utility>

namespace openPMD::internal
{
template <typename Child>
auto ScientificDefaults<Child>::asChild() -> Child &
{
    return *static_cast<Child *>(this);
}

template <typename Child>
auto ScientificDefaults<Child>::asChild() const -> Child const &
{
    return *static_cast<Child const *>(this);
}

template <typename Child>
template <typename Setter, typename F>
void ScientificDefaults<Child>::addDefaultFor(
    char const *key,
    Child &(Child::*setter)(std::conditional_t<
                            std::is_void_v<Setter>,
                            std::remove_reference_t<detail::CallResult_t<F>>,
                            Setter>),
    F &&get_value)
{
    if (asChild().containsAttribute(key))
    {
        return;
    }
    auto value = [&]() {
        if constexpr (detail::IsCallable_v<F>)
        {
            return get_value();
        }
        else
        {
            return get_value;
        }
    }();
    std::cout << "\tInitializing default for '" << key << "' = '";
    if constexpr (auxiliary::IsVector_v<
                      std::remove_reference_t<decltype(value)>>)
    {
        auxiliary::write_vec_to_stream(std::cout, value);
    }
    else
    {
        std::cout << value;
    }
    std::cout << "'" << std::endl;
    (asChild().*setter)(std::move(value));
}
// template <typename Child>
// template <typename F>
// void ScientificDefaults<Child>::addDefaultFor(char const *key, F &&get_value)
// {
//     if (asChild().containsAttribute(key))
//     {
//         return;
//     }
//     auto value = [&]() {
//         if constexpr (detail::IsCallable_v<F>)
//         {
//             return get_value();
//         }
//         else
//         {
//             return get_value;
//         }
//     }();
//     asChild().setAttribute(key, std::move(value));
// }

template <typename Child>
void ScientificDefaults<Child>::finalize(Access at)
{
    if (access::write(at))
    {
        addDefaults();
    }
    if constexpr (IsContainer_v<Child>)
    {
        using Container_t = AsContainer_t<Child>;
        using value_t = typename Container_t::value_type;
        if constexpr (HasScientificDefaults_v<value_t>)
        {
            for (auto &[_, right] : *this)
            {
                (void)_;
                right.ScientificDefaults<value_t>::finalize(at);
            }
        }
    }
}

template <typename Child>
void ScientificDefaults<Child>::addDefaults()
{
    std::cout << "Adding defaults for '" << asChild().myPath().openPMDPath()
              << "'" << std::endl;

    if constexpr (std::is_same_v<Child, Mesh>)
    {
        auto dimensionality = asChild().retrieveDimensionality();
        // std::cout << "Dimensionality is " << dimensionality << " for '"
        //           << asChild().myPath().openPMDPath() << "'" << std::endl;

        addDefaultFor("timeOffset", &Mesh::setTimeOffset, 0.f);
        addDefaultFor(
            "geometry", &Mesh::setGeometry, Mesh::Geometry::cartesian);
        addDefaultFor("dataOrder", &Mesh::setDataOrder, Mesh::DataOrder::C);
        addDefaultFor<std::vector<std::string> const &>(
            "axisLabels",
            &Mesh::setAxisLabels,
            [&]() -> std::vector<std::string> {
                switch (dimensionality)
                {
                case 1:
                    return {"x"};
                case 2:
                    return {"x", "y"};
                case 3:
                    return {"x", "y", "z"};
                default: {
                    if (dimensionality < 100)
                    {
                        std::vector<std::string> res;
                        res.reserve(dimensionality);
                        for (uint64_t i = 0; i < dimensionality; ++i)
                        {
                            res.emplace_back("x" + std::to_string(i));
                        }
                        return res;
                    }
                    else
                    {
                        return {
                            "Please verify dimensionality. Was inferred as '" +
                            std::to_string(dimensionality) + "'."};
                    }
                }
                }
                return std::vector<std::string>{"x", "y", "z"};
            });
        addDefaultFor<std::vector<double> const &>(
            "gridSpacing", &Mesh::setGridSpacing, [&]() {
                if (dimensionality < 100)
                {
                    return std::vector<double>(dimensionality, 1.0);
                }
                else
                {
                    return std::vector<double>{1.0};
                }
            });
        addDefaultFor<std::vector<double> const &>(
            "gridGlobalOffset", &Mesh::setGridGlobalOffset, [&]() {
                if (dimensionality < 100)
                {
                    return std::vector<double>(dimensionality, 0.0);
                }
                else
                {
                    return std::vector<double>{0.0};
                }
            });
    }
}

template class ScientificDefaults<Iteration>;
template class ScientificDefaults<Mesh>;
template class ScientificDefaults<MeshRecordComponent>;
template class ScientificDefaults<ParticleSpecies>;
} // namespace openPMD::internal
