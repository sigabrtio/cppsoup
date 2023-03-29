#pragma once

#include <coroutine>
#include <future>

#include <iostream>

#include <thesoup/types/types.hpp>

namespace thesoup {

    namespace async {
        template <class Impl> class CoroExecutorInterface {
        public:
            void schedule(std::coroutine_handle<>&& handle) noexcept {
                static_cast<Impl*>(this)->schedule(std::forward<std::coroutine_handle<>&&>(handle));
            }
        };

        template <typename T, class ExecutorImpl> struct [[nodiscard]] SingleValueCoroTask {
            struct promise_type {
                std::coroutine_handle<promise_type> handle {};
                std::promise<T> promise {};

                promise_type() {}
                promise_type(promise_type&& other) : handle {std::move(other.handle)}, promise {std::move(other.promise)} {}
                promise_type(const promise_type& other)=delete;

                SingleValueCoroTask get_return_object() {
                    std::coroutine_handle<promise_type> handle {std::coroutine_handle<promise_type>::from_promise(*this)};
                    this->handle = handle;
                    return SingleValueCoroTask {
                            .future = promise.get_future()
                    };
                }

                std::suspend_never initial_suspend() const noexcept {return {};}
                std::suspend_always await_transform(const thesoup::types::Unit _) const noexcept {std::ignore = _; return {};}
                std::suspend_always await_transform(std::reference_wrapper<CoroExecutorInterface<ExecutorImpl>> executor_handle) {
                    executor_handle.get().schedule(std::move(handle));
                    return {};
                }
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
