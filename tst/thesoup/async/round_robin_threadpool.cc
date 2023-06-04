#define CATCH_CONFIG_MAIN

#include <exception>

#include <catch2/catch_all.hpp>

#include <thesoup/async/types.hpp>
#include <thesoup/async/round_robin_threadpool.hpp>
#include <thesoup/types/types.hpp>
#include <thesoup/async/futures.hpp>

using thesoup::async::RoundRobinCoroExecutor;
using thesoup::async::SingleValueCoroTask;
using thesoup::async::is_ready;
using thesoup::types::Unit;

SingleValueCoroTask<const int, RoundRobinCoroExecutor> my_routine(
        std::reference_wrapper<RoundRobinCoroExecutor> executor,
        const int start_val) {
    co_await executor;
    int acc {0};
    for (int i = start_val; i < 10; i++) {
        acc++;
        co_await Unit::unit;
    }
    co_return acc;
};

SingleValueCoroTask<int, RoundRobinCoroExecutor> my_throwable_routine(
        std::reference_wrapper<RoundRobinCoroExecutor> executor,
        bool should_throw) {
    co_await executor;
    int acc {0};
    if (should_throw) {
        throw std::runtime_error("error");
    }
    for (int i = 0; i < 10; i++) {
        acc++;
        co_await Unit::unit;
    }
    co_return acc;
};

SingleValueCoroTask<const Unit, RoundRobinCoroExecutor> my_routine_accidental_schedule_twice(
        std::reference_wrapper<RoundRobinCoroExecutor> executor) {
    co_await executor;
    co_await executor;
    co_return Unit::unit;
};

SCENARIO("Round robin executor test") {

    GIVEN("I have a round robin coroutine executor.") {

        RoundRobinCoroExecutor executor {};

        AND_GIVEN("I have a some coroutines that does some trivial things in a loop.") {

            auto task1 {my_routine(executor, 8)};
            auto task2 {my_routine(executor, 6)};
            auto task3 {my_routine(executor, 4)};
            auto task4 {my_routine(executor, 2)};

            WHEN("I test the size of my queue.") {

                THEN("The size should reflect the number of open tasks (4).") {

                    REQUIRE(4 == executor.size());

                }
            }

            WHEN("I check if the futures are ready.") {

                THEN("They should still be not ready.") {

                    REQUIRE_FALSE(is_ready(task1.future));
                    REQUIRE_FALSE(is_ready(task2.future));
                    REQUIRE_FALSE(is_ready(task3.future));
                    REQUIRE_FALSE(is_ready(task4.future));
                }
            }

            WHEN("I step through until 1 task finishes.") {

                while(executor.size() == 4) {
                    executor.step();
                }

                THEN("The first task should have been the first one to finish.") {

                    REQUIRE(is_ready(task1.future));
                    REQUIRE_FALSE(is_ready(task2.future));
                    REQUIRE_FALSE(is_ready(task3.future));
                    REQUIRE_FALSE(is_ready(task4.future));

                    AND_THEN("The results should have been correct.") {

                        REQUIRE((10-8) == task1.future.get());
                    }
                }

                AND_WHEN("I run through all the remaining coroutines.") {

                    while(executor.size() > 0) {
                        executor.step();
                    }

                    THEN("All futures should be ready.") {

                        REQUIRE(is_ready(task1.future));
                        REQUIRE(is_ready(task2.future));
                        REQUIRE(is_ready(task3.future));
                        REQUIRE(is_ready(task4.future));

                        AND_THEN("All the values should have been returned correctly.") {

                            REQUIRE((10-8) == task1.future.get());
                            REQUIRE((10-6) == task2.future.get());
                            REQUIRE((10-4) == task3.future.get());
                            REQUIRE((10-2) == task4.future.get());
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Coroutine throws exceptions.") {

    GIVEN("I have a round robin coroutine executor.") {

        RoundRobinCoroExecutor executor{};

        WHEN("I have some coroutines, and one of them throws.") {

            auto task_1 {my_throwable_routine(executor, false)};
            auto task_2 {my_throwable_routine(executor, true)};

            AND_WHEN("I have finished all tasks in my executor.") {

                while (executor.size() > 0) {
                    executor.step();
                }

                AND_WHEN("I access the results") {

                    THEN("1 of them should be error.") {

                        REQUIRE(10 == task_1.future.get());
                        REQUIRE_THROWS_AS(task_2.future.get(), std::runtime_error);
                    }
                }
            }
        }
    }
}

SCENARIO("Double await on the executor in the coroutine.") {

    GIVEN("I have a round robin coroutine executor.") {

        RoundRobinCoroExecutor executor{};

        AND_GIVEN("I have a co-routine that accidentally co-awaits twice on the executor, potentially scheduling the task twice.") {

            WHEN("I schedule that coroutine") {
                auto task1 {my_routine_accidental_schedule_twice(executor)};

                THEN("The task queue size should never be more than 1.") {

                    // Initially queued by first co_await
                    REQUIRE(1 == executor.size());
                    executor.step();
                    // second co_await. Task waits here, but nothing is re-queued]
                    REQUIRE(1 == executor.size());
                    executor.step();
                    // Task finish, stopped to be cleaned on final_suspend
                    REQUIRE(1 == executor.size());
                    executor.step();
                    // Task cleaned up
                    REQUIRE(0 == executor.size());

                }
            }
        }
    }
}

