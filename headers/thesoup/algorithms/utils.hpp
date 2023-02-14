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
                    [&]() {
                        std::vector<T> results;
                        results.reserve(futures.size());
                        for (auto& fut : futures) {
                            results.push_back(fut.get());
                        }
                        return results;
                    });
        }
    }
}