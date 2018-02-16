#pragma once

#include "RecordComponent.hpp"

#include <vector>


class MeshRecordComponent : public RecordComponent
{
    template<
            typename T,
            typename T_key,
            typename T_container
    >
    friend
    class Container;

    friend class Mesh;

private:
    MeshRecordComponent();
    void read() override;

public:
    template< typename T >
    std::vector< T > position() const;
    template< typename T >
    MeshRecordComponent& setPosition(std::vector< T >);

    template< typename T >
    MeshRecordComponent& makeConstant(T);
};


template< typename T >
std::vector< T >
MeshRecordComponent::position() const
{ return readVectorFloatingpoint< T >("position"); }

template< typename T >
inline MeshRecordComponent&
MeshRecordComponent::makeConstant(T value)
{
    RecordComponent::makeConstant(value);
    return *this;
}
