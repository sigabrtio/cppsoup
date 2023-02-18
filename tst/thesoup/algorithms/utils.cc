#define CATCH_CONFIG_MAIN

#include <algorithm>
#include <exception>
#include <future>

#include <catch2/catch_all.hpp>
#include <thesoup/algorithms/utils.hpp>

using thesoup::async::collect_futures;
using thesoup::async::make_ready_future;
using thesoup::async::make_bad_future;

SCENARIO("Collect test.") {

    GIVEN("When I have a list of futures.") {

        std::vector<int> inputs {1,2,3,4,5};
        std::vector<std::future<int>> futures;

        std::transform(
                inputs.begin(),
                inputs.end(),
                std::back_inserter(futures),
                make_ready_future<int>);

        WHEN("I collect the futures.") {

            std::future<std::vector<int>> collected_fut {collect_futures(futures)};

            THEN("Then the result should be 1 future that yields a vector of the individual values.") {

                REQUIRE(collected_fut.get() == inputs);
            }
        }

    }

    GIVEN("I have a list of futures, one of which throws an exception.") {

        std::vector<std::future<int>> futures;
        futures.emplace_back(make_ready_future<int>(1));
        futures.emplace_back(make_ready_future<int>(2));
        futures.emplace_back(make_bad_future<int, std::runtime_error>(std::runtime_error("")));
        futures.emplace_back(make_ready_future<int>(4));
        futures.emplace_back(make_ready_future<int>(5));

        WHEN("I do a get on the collected futures.") {

            THEN("The future should be an error.") {

                REQUIRE_THROWS_AS(collect_futures(futures).get(), std::runtime_error);
            }
        }

    }
}