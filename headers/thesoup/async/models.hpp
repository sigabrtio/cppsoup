#pragma once

#include <iostream>

#include <coroutine>
#include <future>

#include <thesoup/types/types.hpp>

namespace thesoup {

    namespace async {
        template <typename T> bool is_ready(const std::future<T>& fut) {
            return fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }

        template <class Impl> class CoroExecutorInterface {
        public:
            void schedule(std::coroutine_handle<>&& handle) noexcept {
                static_cast<Impl*>(this)->schedule(std::forward<std::coroutine_handle<>&&>(handle));
            }
        };

        template <typename T, class ExecutorImpl> struct [[nodiscard]] SingleValueCoroTask {
            struct promise_type {
                CoroExecutorInterface<ExecutorImpl>* executor_handle;
                std::promise<T> promise {};

                promise_type(CoroExecutorInterface<ExecutorImpl>* executor_handle, ...) : executor_handle {executor_handle} {}

                SingleValueCoroTask get_return_object() {
                    std::coroutine_handle<promise_type> handle {std::coroutine_handle<promise_type>::from_promise(*this)};
                    executor_handle->schedule(handle);
                    return SingleValueCoroTask {
                            .future = promise.get_future()
                    };
                }

                std::suspend_always initial_suspend() const noexcept {return {};}
                std::suspend_always await_transform(const thesoup::types::Unit _) const noexcept {std::ignore = _; return {};}
                std::suspend_always final_suspend() const noexcept {return {};}

                void return_value(const T& ret_val) noexcept {
                    promise.set_value(std::move(ret_val));
                }

                void unhandled_exception() {
                    promise.set_exception(std::current_exception());
                }
            };

            std::future<T> future;

        };
    }
}