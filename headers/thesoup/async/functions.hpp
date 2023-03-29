#pragma once

#include <exception>
#include <future>
#include <functional>
#include <type_traits>

#include <thesoup/async/types.hpp>
#include <thesoup/types/types.hpp>

namespace thesoup {

    namespace async {

        template<typename T>
        std::future<T> make_ready_future(const T &val) {
            std::promise<T> promise;
            std::future<T> fut{promise.get_future()};
            promise.set_value(val);
            return fut;
        }

        template<typename T, typename E>
        std::future<T> make_bad_future(const E &exception) {
            std::promise<T> promise;
            std::future<T> fut{promise.get_future()};
            promise.set_exception(std::make_exception_ptr(exception));
            return fut;
        }

        template <typename T> bool is_ready(const std::future<T>& fut) {
            if (!fut.valid()) {
                std::cout << "Future is INVALID!!!\n";
            }
            return fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }

        /*template<typename T>
        class FutureComposer {
        private:
            std::future<T> fut;
        public:
            FutureComposer(T &&fut) : fut{fut} {}

            std::future<T> future() {
                return std::move(fut);
            }

            template<typename Transformed>
            FutureComposer<Transformed> map(const std::function<Transformed(const T &)> &function) {
                return FutureComposer{
                        std::async(
                                std::launch::deferred,
                                [&]() {
                                    function(fut.get());
                                })
                };
            }

            template<typename Transformed>
            FutureComposer flat_map(const std::function<std::future<Transformed>(const T &)> &function) {
                return FutureComposer{
                        std::async(
                                std::launch::deferred,
                                [&]() {
                                    function(fut.get());
                                })
                };
            }
        };*/

        template<typename T>
        std::future<std::vector<T>> collect_futures(std::vector<std::future<T>> &futures) {
            return std::async(
                    std::launch::deferred,
                    [&]() {
                        std::vector<T> results;
                        results.reserve(futures.size());
                        for (auto &fut: futures) {
                            results.push_back(fut.get());
                        }
                        return results;
                    });
        }

        template <typename T, typename U, class ExecutorImpl>
        SingleValueCoroTask<U, ExecutorImpl> map_coroutine(
                std::reference_wrapper<CoroExecutorInterface<ExecutorImpl>> executor,
                std::future<T> input_fut, const typename std::function<U(const T&)> function) {
            co_await executor;
            while (!is_ready(input_fut)) {
                co_await thesoup::types::Unit::unit;
            }
            co_return function(input_fut.get());
        }

        template <typename T, typename U, class ExecutorImpl>
        SingleValueCoroTask<U, ExecutorImpl> flatmap_coroutine(
                std::reference_wrapper<CoroExecutorInterface<ExecutorImpl>> executor,
                std::future<T> input_fut,
                std::function<std::future<U>(const T&)> function) {
            co_await executor;
            while (!is_ready(input_fut)) {
                co_await thesoup::types::Unit::unit;
            }
            std::future<U> next_fut {function(input_fut.get())};
            while(!is_ready(next_fut)) {
                co_await thesoup::types::Unit::unit;
            }
            co_return next_fut.get();
        }


        template <typename T, typename ExecutorImpl>
        class FutureComposer {
        private:
            static_assert(
                    std::is_base_of<CoroExecutorInterface<ExecutorImpl>, ExecutorImpl>::value,
                    "The ExecutorImpl needs to implement the CoroExecutorInterface CRTP interface");
            std::reference_wrapper<CoroExecutorInterface<ExecutorImpl>> executor;
            std::future<T> fut;
            bool is_valid {true};
        public:
            FutureComposer(
                    const std::reference_wrapper<CoroExecutorInterface<ExecutorImpl>>& executor,
                    std::future<T>&& fut) : executor {executor}, fut {std::move(fut)} {}

            template<typename U> FutureComposer<U, ExecutorImpl> map(const std::function<U(const T& val)> function) {
                auto task {map_coroutine<T, U, ExecutorImpl>(executor, std::move(fut), function)};
                return FutureComposer<U, ExecutorImpl>(executor, std::move(task.future));
            }

            template<typename U> FutureComposer<U, ExecutorImpl> flatmap(std::function<std::future<U>(const T& val)> function) {
                auto task {flatmap_coroutine<T, U, ExecutorImpl>(executor, std::move(fut), function)};
                return FutureComposer<U, ExecutorImpl>(executor, std::move(task.future));
            }

            std::future<T> get_future() {
                if (!is_valid) {
                    throw std::runtime_error("Invalid future.");
                } else {
                    is_valid = false;
                    return std::move(fut);
                }
            }
        };
    }
}