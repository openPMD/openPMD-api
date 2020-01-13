#pragma once
#include <future>
#include <memory>
#include <utility>
#include <thread>

namespace openPMD
{
namespace auxiliary
{
    /**
     * Subclass template for std::future that wraps
     * the std::packaged_task creating the future
     * Useful to extend the lifetime of the task for
     * exactly as long as the future lives.
     */
    template< typename A, typename... Args >
    class ConsumingFuture : public std::future< A >
    {
    private:
        std::packaged_task< A( Args &&... ) > m_task;
        std::unique_ptr< std::thread > m_thread;

    public:
        /**
         * @brief  Upon creation of a ConsumingFuture, the wrapped 
         * std::packaged_tasked is not automatically run.
         * ConsumingFuture::operator()() or ConsumingFuture::run_threaded()
         * has to be invoked in order to do so. This boolean flag indicates
         * whether the std::packaged_task is running already.
         * 
         */
        bool isRunning = false;

        ConsumingFuture( std::packaged_task< A( Args &&... ) > task ) :
            std::future< A >( task.get_future() ),
            m_task( std::move( task ) )
        {
        }

        ConsumingFuture ( ConsumingFuture && ) = default;

        ~ConsumingFuture( )
        {
            if ( m_thread )
            {
                m_thread->join( );
            }
        }

        /**
         * @brief Run the contained std::packaged_task in blocking manner.
         *        The future should be available after this returns.
         * 
         * @param args The arguments for the std::packaged_task.
         */
        void
        operator()( Args &&... args )
        {
            m_task( std::forward< Args >( args )... );
            isRunning = true;
        }

        /**
         * @brief Run the contained std::packaged_task in threaded manner.
         *        Will return immediately. Use std::future::wait() to know
         *        when computation has ended.
         *        The destructor will wait for the created thread to join.
         * 
         * @param args The arguments for the std::packaged_task.
         */
        void
        run_as_thread( Args &&... args )
        {
            m_thread = std::unique_ptr< std::thread >(
                new std::thread(
                    std::move( m_task ),
                    std::forward< Args >( args )...
                )
            );
            isRunning = true;
        }
    };

    namespace detail
    {
        template< typename A, typename B >
        struct AvoidVoid
        {
            using type = B( A );

            template< typename Future >
            static void
            run_task(
                Future & first,
                std::packaged_task< type > & second )
            {
                second( first.get() );
            }
        };

        template< typename B >
        struct AvoidVoid< void, B >
        {
            using type = B();

            template< typename Future >
            static void
            run_task(
                Future &,
                std::packaged_task< type > & second )
            {
                second();
            }
        };
    } // namespace detail

    struct RunFutureNoop
    {
        template< typename Future >
        static void
        run( Future && )
        {
        }
    };

    struct RunFutureNonThreaded
    {
        template< typename Future >
        static void run( Future && fut ){
            fut();
        }
    };

    struct RunFutureThreaded
    {
        template< typename Future >
        static void run( Future && fut ){
            fut.run_as_thread( );
        }
    };

    /**
     * @brief Chain a std::future with a successive std::packaged_task.
     *        Warning: This is not the most efficient implementation and will,
     *        unlike sophisticated async frameworks, build up a chain of nested
     *        futures if used excessively.
     *
     * @tparam A Value produced by the first future. May be void.
     * @tparam B
     * @tparam RunFuture< RunFutureStrategy::DoNotRun > This function template
     *         facultatively supports activating the future passed to it.
     *         Since this future may also be a ConsumingFuture, it may not be
     *         running yet. It is advised to set this template parameter to
     *         RunFuture< RunFutureStrategy::RunNonThreaded > in this case.
     *         This avoids creating a new thread alone for execution of the
     *         first future which is unnecessary since running it can be
     *         embedded into the created std::packaged_task.
     * @tparam std::future< A >
     * @param first The future that runs first. Produces a value of
     *              type A.
     * @param second A packaged task that consumes the value created by the
     *               future.
     * @return ConsumingFuture< B > A ConsumingFuture representing the chained
     *         computation. It is important to note that the task is not running
     *         yet. Running it has to be triggered by calling either
     *         ConsumingFuture::operator()() or
     *         ConsumingFuture::run_as_thread().
     */
    template<
        typename A,
        typename B,
        typename RunFuture_T = RunFutureNoop,
        typename Future = std::future< A > >
    ConsumingFuture< B >
    chain_futures(
        Future first,
        std::packaged_task< typename detail::AvoidVoid< A, B >::type > second )
    {
        // capture by move is only possible using generalized lambda capture,
        // i.e. requires C++14
        // work around this by using shared pointers
        auto first_ptr = std::make_shared< Future >( std::move( first ) );
        auto second_ptr =
            std::make_shared< decltype( second ) >( std::move( second ) );
        std::packaged_task< B() > ptask( [first_ptr, second_ptr]() {
            RunFuture_T::template run( *first_ptr );
            if( first_ptr->valid() )
                first_ptr->wait();
            auto future = second_ptr->get_future();
            detail::AvoidVoid< A, B >::template run_task< Future >( 
                *first_ptr, *second_ptr );
            future.wait();
            return future.get();
        } );
        return ConsumingFuture< B >( std::move( ptask ) );
    }
} // namespace auxiliary

using auxiliary::ConsumingFuture;
} // namespace openPMD