#define CATCH_CONFIG_MAIN

#include <algorithm>
#include <future>
#include <string>
#include <tuple>

#include <catch2/catch_all.hpp>
#include <thesoup/async/functions.hpp>
#include <thesoup/async/round_robin_threadpool.hpp>

using thesoup::async::collect_futures;
using thesoup::async::is_ready;
using thesoup::async::make_ready_future;
using thesoup::async::make_bad_future;
using thesoup::async::RoundRobinCoroExecutor;
using thesoup::async::FutureComposer;

SCENARIO("Future map, flatmap test test.") {

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

SCENARIO("Future composer join test.") {

    GIVEN("I have a executor, and some futures.") {

        RoundRobinCoroExecutor exec;
        std::promise<int> p1;
        std::promise<bool> p2;

        std::future<int> f1 {p1.get_future()};
        std::future<bool> f2 {p2.get_future()};

        WHEN("I join them.") {

            std::future<std::tuple<int, bool>> final {
                FutureComposer<int, RoundRobinCoroExecutor>(exec, std::move(f1))
                        .join(std::move(f2))
                        .get_future()

            };

            THEN("I can wait on the joined future.") {

                REQUIRE_FALSE(is_ready(final));
                p1.set_value(123);
                p2.set_value(false);

                exec.step();

                REQUIRE(is_ready(final));
                const auto& [i,b] = final.get();
                REQUIRE(123 == i);
                REQUIRE_FALSE(b);
            }
        }
    }
}

SCENARIO("Future composer collect test.") {

    GIVEN("I have an executor and a vector of futures and an output vector.") {

        RoundRobinCoroExecutor exec;
        std::vector<std::promise<int>> promises {};
        std::vector<std::future<int>> futures {};

        for (int i = 0; i < 10; i++) {
            promises.emplace_back();
            futures.push_back(promises[i].get_future());
        }

        WHEN("I collect the futures into a new vector.") {

            std::vector<int> collected {};
            std::future<thesoup::types::Unit> final{
                    FutureComposer<int, RoundRobinCoroExecutor>::collect(exec, futures.begin(), futures.end(), std::back_inserter(collected))
                            .get_future()};

            THEN("I should get back a vector of collected results.") {

                REQUIRE_FALSE(is_ready(final));
                for (int i = 0; i < 10; i++) {
                    promises[i].set_value(i);
                }
                exec.step();
                REQUIRE(is_ready(final));
                for (int i = 0; i < 10; i++) {
                    REQUIRE(i == collected[i]);
                }
            }
        }
    }
}