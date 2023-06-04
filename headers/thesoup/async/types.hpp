#pragma once

#include <coroutine>
#include <future>

#include <iostream>

#include <thesoup/types/types.hpp>

/**
 * \namespace thesoup
 *
 * \brief The root namespace.
 * */
namespace thesoup {

    /**
     * \namespace thesoup::async
     *
     * \brief Sub namespace with some async utilities.
     * */
    namespace async {
        /**
         * \brief The co-routine executor interface.
         *
         * This is the interface to the coroutine executor. Any implementation has to implement just one method:
         * `schedule`.
         *
         * @tparam Impl
         */
        template <class Impl> class CoroExecutorInterface {
        public:
            /**
             * \brief Schedule a coroutine.
             *
             * Any implementation of this interface will be passed a r-value reference. The implementation can then take
             * it and put it in whatever the implementation wants to use as a task queue.
             *
             * @param handle The co-routine handle to schedule.
             */
            virtual void schedule(std::coroutine_handle<>&& handle) noexcept {
                static_cast<Impl*>(this)->schedule(std::forward<std::coroutine_handle<>&&>(handle));
            }
        };

        /**
         * \brief A coroutine task type where the coroutine does some work and returns a single value.
         *
         * If you have dealt with coroutines in c++20, you are aware of how much boiler plate code is needed to get it
         * going. This class tries to take care of that for a very specific use case: co-routines that do something and
         * return a SINGLE value. There are lot more use cases of co-routines. Generators are the first thing that come
         * to mind. However this task type will NOT help with those.
         *
         * That is not to say that the use cases of this are limited. Want to write a co-routine that polls on some SQL
         * connection occasionally and returns when the result when it is done? Or want to write a co-routine that does \
         * a BFS on a graph, where the neighbour list is slow to return (a build system resolving the dependency graph
         * comes to mind)? Just write it, so that it takes a reference_wrapper of the executor, and returns this task type!
         *
         * NOTE: The first statement in the co-routine has to be `co_await the_executor_reference_wrapper`. This will
         * schedule the co-routine you wrote. Sure you can have statements before that. But if you pause the co-routine
         * via co_await (this task type does not support co_yield), you are responsible for resuming it. Otherwise this
         * is never getting scheduled.
         *
         * Also, please be a decent human being and do not write a co-routine that wastes a large amount of time between
         * 2 successive co_awaits. In that time, no other co-routines in the executor's queue will progress. The following
         * example shows how to use this.
         *
         * ```
         * SingleValueCoroTask<const int, RoundRobinCoroExecutor> my_routine(
         *     std::reference_wrapper<RoundRobinCoroExecutor> executor, const int start_val) {
         *     co_await executor;
         *     int acc {0};
         *     for (int i = start_val; i < 10; i++) {
         *         acc++;
         *         co_await Unit::unit;
         *     }
         *     co_return acc;
         * };
         * ```
         *
         * @tparam T
         * @tparam ExecutorImpl
         */
        template <typename T, class ExecutorImpl> struct [[nodiscard]] SingleValueCoroTask {
            //!\cond NO_DOC
            struct promise_type {
                enum class Status {
                    PENDING, SCHEDULED, COMPLETE
                };
                std::coroutine_handle<promise_type> handle {};
                std::promise<T> promise {};
                Status status {Status::PENDING};

                promise_type() = default;
                promise_type(promise_type&& other)  noexcept : handle {std::move(other.handle)}, promise {std::move(other.promise)} {}
                promise_type(const promise_type& other)=delete;

                SingleValueCoroTask get_return_object() {
                    this->handle = {std::coroutine_handle<promise_type>::from_promise(*this)};
                    return SingleValueCoroTask {
                            .future = promise.get_future()
                    };
                }

                [[nodiscard]] std::suspend_never initial_suspend() const noexcept {return {};}
                [[nodiscard]] std::suspend_always await_transform(const thesoup::types::Unit _) const noexcept {std::ignore = _; return {};}
                [[nodiscard]] std::suspend_always await_transform(std::reference_wrapper<CoroExecutorInterface<ExecutorImpl>> executor_handle) {
                    if (status == Status::PENDING) {
                        executor_handle.get().schedule(std::move(handle));
                        status = Status::SCHEDULED;
                    }
                    return {};
                }
                [[nodiscard]] std::suspend_always final_suspend() const noexcept {return {};}

                void return_value(const T& ret_val) noexcept {
                    status = Status::COMPLETE;
                    promise.set_value(std::move(ret_val));
                }

                void unhandled_exception() {
                    promise.set_exception(std::current_exception());
                }
            };

            std::future<T> future;
            //!\endcond

        };
    }
}
