#pragma once


#include <memory>
#include "openPMD/Dataset.hpp"
#include <random>


namespace openPMD
{
    /**
     * An abstract class to create one iteration of data per thread.
     * @tparam T The type of data to produce.
     */
    template< typename T >
    class DatasetFiller
    {
    protected:
        Extent::value_type m_numberOfItems;
    public:
        using resultType = T;

        explicit DatasetFiller( Extent::value_type numberOfItems = 0 );

        /**
         * Create a shared pointer of m_numberOfItems items of type T.
         * Should take roughly the same amount of time per call as long as
         * m_numberOfItems does not change.
         * @return
         */
        virtual std::shared_ptr< T > produceData( ) = 0;

        /**
         * Set number of items to be produced.
         * @param numberOfItems The number.
         */
        virtual void setNumberOfItems( Extent::value_type numberOfItems ) = 0;
    };


    template< typename T >
    DatasetFiller< T >::DatasetFiller( Extent::value_type numberOfItems ) :
        m_numberOfItems( numberOfItems )
    {}


    template< typename DF >
    class SimpleDatasetFillerProvider
    {
    public:
        using resultType = typename DF::resultType;
    private:
        std::shared_ptr< DF > m_df;


        template<
            typename T,
            typename Dummy=void
        >
        struct Helper
        {
            std::shared_ptr< DatasetFiller< T>> operator()( std::shared_ptr<DF> & )
            {
                throw std::runtime_error(
                    "Can only create data of type " +
                    datatypeToString( determineDatatype< resultType >( ) )
                );
            }
        };

        template< typename Dummy >
        struct Helper<
            resultType,
            Dummy
        >
        {
            std::shared_ptr< DatasetFiller< resultType>> operator()(std::shared_ptr<DF> &df )
            {
                return df;
            }
        };

    public:


        explicit SimpleDatasetFillerProvider( DF df ) :
            m_df { std::make_shared< DF >( std::move( df ) ) }
        {}


        template< typename T >
        std::shared_ptr< DatasetFiller< T >> operator()( )
        {
            Helper< T > h;
            return h( m_df );
        }
    };


}

