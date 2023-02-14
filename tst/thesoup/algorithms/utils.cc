#define CATCH_CONFIG_MAIN

#include <algorithm>
#include <future>

#include <catch2/catch_all.hpp>
#include <thesoup/algorithms/utils.hpp>

using thesoup::async::collect_futures;

template <typename T>
std::future<T> make_ready_future(const T& val) {
    std::promise<T> promise;
    std::future<T> fut {promise.get_future()};
    promise.set_value(val);
    return fut;
}

SCENARIO("Collect test.") {

    GIVEN("When I have a list of futures.") {

        std::vector<int> inputs {1,2,3,4,5};
        std::vector<std::future<int>> futures;

        std::transform(
                inputs.begin(),
                inputs.end(),
                std::back_inserter(futures),
                make_ready_future<int>);

        std::future<std::vector<int>> collected_fut {collect_futures(futures)};
        REQUIRE(collected_fut.get() == inputs);
    }
}