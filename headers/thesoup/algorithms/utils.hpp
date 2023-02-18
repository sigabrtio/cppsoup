#pragma once

#include <future>
#include <vector>

/**
 * \namespace thesoup
 *
 * \brief The root namespace.
 * */
namespace thesoup {

    /**
     * \namespace thesoup::async
     *
     * \brief Sub namespace some async utilities.
     * */
    namespace async {

        template <typename T>
        std::future<std::vector<T>> collect_futures(std::vector<std::future<T>>& futures) {
            return std::async(
                    std::launch::deferred,
                    [&]() {
                        std::vector<T> results;
                        results.reserve(futures.size());
                        for (auto& fut : futures) {
                            results.push_back(fut.get());
                        }
                        return results;
                    });
        }

        template <typename T>
        std::future<T> make_ready_future(const T& val) {
            std::promise<T> promise;
            std::future<T> fut {promise.get_future()};
            promise.set_value(val);
            return fut;
        }

        template <typename T, typename E>
        std::future<T> make_bad_future(const E& exception) {
            std::promise<T> promise;
            std::future<T> fut {promise.get_future()};
            promise.set_exception(std::make_exception_ptr(exception));
            return fut;
        }
    }
}