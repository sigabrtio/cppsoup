#define CATCH_CONFIG_MAIN

#include <algorithm>
#include <exception>
#include <future>
#include <string>
#include <iostream>

#include <catch2/catch_all.hpp>
#include <thesoup/async/functions.hpp>
#include <thesoup/async/round_robin_threadpool.hpp>

using thesoup::async::collect_futures;
using thesoup::async::is_ready;
using thesoup::async::make_ready_future;
using thesoup::async::make_bad_future;
using thesoup::async::RoundRobinCoroExecutor;
using thesoup::async::FutureComposer;
/*
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
*/

SCENARIO("Future map test.") {

    GIVEN("I have a future and an executor.") {

        std::promise<int> test_promise {};
        std::future<int> test_future {test_promise.get_future()};

        std::promise<bool> flatmap_promise {};
        std::future<bool> flatmap_future {flatmap_promise.get_future()};

        RoundRobinCoroExecutor exec;

        AND_GIVEN("I have a functions that maps an into to a string, and one that maps a string to std::future<bool>") {

            int map_invocations {0};
            int flat_map_invocations {0};
            std::string flat_map_arg {};

            std::function<std::string(const int&)> map_f = [&](const int& value) {
                map_invocations++;
                return std::to_string(value);
            };

            std::function<std::future<bool>(const std::string&)> flatmap_f = [&](const std::string& val) {
                flat_map_invocations++;
                flat_map_arg = val;
                return std::move(flatmap_future);
            };


            WHEN("I compose some futures using maps and flat-maps.") {

                std::future<bool> final {
                            FutureComposer<int, RoundRobinCoroExecutor>(std::reference_wrapper(exec), std::move(test_future))
                                    .map(map_f)
                                    .flatmap(flatmap_f)
                                    .get_future()
                    };

                THEN("Initially it should be not ready.") {

                    REQUIRE_FALSE(is_ready(final));

                    AND_THEN("I can step through the executor a couple of times, and observe that the final future is still not ready.") {

                        exec.step();
                        exec.step();
                        exec.step();
                        exec.step();

                        REQUIRE_FALSE(is_ready(final));
                        REQUIRE(0 == map_invocations);

                        AND_WHEN("I set the first future, then the map function should be invoked.") {

                            test_promise.set_value(123);
                            exec.step();

                            REQUIRE(1 == map_invocations);
                            exec.step();

                            REQUIRE(1 == flat_map_invocations);
                            REQUIRE(std::string("123") == flat_map_arg);

                            AND_WHEN("I set the last promise.") {

                                flatmap_promise.set_value(false);

                                THEN("The flatmap should finally complete.") {

                                    exec.step();
                                    REQUIRE(is_ready(final));
                                    REQUIRE_FALSE(final.get());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}