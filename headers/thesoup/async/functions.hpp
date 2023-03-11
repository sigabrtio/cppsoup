#pragma once

#include <future>

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
            return fut.wait_for(std::chrono::seconds(0));
        }

        template<typename T>
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
        };

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
    }
}