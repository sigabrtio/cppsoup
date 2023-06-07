#pragma once

#include <exception>
#include <future>
#include <functional>
#include <type_traits>

#include <thesoup/async/types.hpp>
#include <thesoup/types/types.hpp>

/**
 * \namespace thesoup
 *
 * \brief The root namespace.
 * */
namespace thesoup {

    /**
     * \namespace thesoup::async
     *
     * \brief Sub namespace with some async utilities.
     * */
    namespace async {

        /**
         * \brief Make a future that is already ready.
         *
         * This function returns a future with it's value already set. This is useful in testing async functions.
         *
         * @tparam T The type of future
         * @param val the value that the future will contain.
         * @return A `std::future<T>` object.
         */
        template<typename T>
        std::future<T> make_ready_future(const T &val) {
            std::promise<T> promise;
            std::future<T> fut{promise.get_future()};
            promise.set_value(val);
            return fut;
        }

        /**
         * \brief Make a future that is an error.
         *
         * This function returns a future with it's exception already set. This is useful in testing async functions.
         *
         * @tparam T The type of future
         * @tparam E The exception type
         * @param val the value that the future will contain.
         * @return A `std::future<T>` object.
         */
        template<typename T, typename E>
        std::future<T> make_bad_future(const E &exception) {
            std::promise<T> promise;
            std::future<T> fut{promise.get_future()};
            promise.set_exception(std::make_exception_ptr(exception));
            return fut;
        }

        /**
         * \brief Test if a future is ready.
         *
         * This function tests if a future is ready.
         *
         * @tparam T The future type to test.
         * @param fut The future object being tested.
         * @return true if ready else false
         */
        template <typename T> bool is_ready(const std::future<T>& fut) {
            if (!fut.valid()) {
                std::cout << "Future is INVALID!!!\n";
            }
            return fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }

        //!\cond NO_DOC
        template <typename Callable, typename T, typename U>
        concept is_map_function = requires (Callable callable, T input, U output) {
            {callable(input)} -> std::same_as<U>;
        };

        template <typename Callable, typename T, typename U>
        concept is_flatmap_function = requires (Callable callable, T input, U output) {
            {callable(input)} -> std::same_as<std::future<U>>;
        };

        template <typename T, typename U, class ExecutorImpl, typename Callable>
        thesoup::async::SingleValueCoroTask<U, ExecutorImpl> map_coroutine(
                std::reference_wrapper<CoroExecutorInterface<ExecutorImpl>> executor,
                std::future<T> input_fut,
                const Callable function) requires is_map_function<Callable, T, U> {
            co_await executor;
            while (!is_ready(input_fut)) {
                co_await thesoup::types::Unit::unit;
            }
            co_return function(input_fut.get());
        }

        template <typename T, typename U, class ExecutorImpl, typename Callable>
        SingleValueCoroTask<U, ExecutorImpl> flatmap_coroutine(
                std::reference_wrapper<CoroExecutorInterface<ExecutorImpl>> executor,
                std::future<T> input_fut,
                Callable function) requires is_flatmap_function<Callable, T, U> {
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

        template <typename T, typename U, class ExecutorImpl>
        SingleValueCoroTask<std::tuple<T, U>, ExecutorImpl> join_coroutine(
                std::reference_wrapper<CoroExecutorInterface<ExecutorImpl>> executor,
                std::future<T> input_fut_left,
                std::future<U> input_fut_right) {
            co_await executor;
            while(!is_ready(input_fut_left)) {
                co_await thesoup::types::Unit::unit;
            }
            while(!is_ready(input_fut_right)) {
                co_await thesoup::types::Unit::unit;
            }
            co_return std::make_tuple(
                    input_fut_left.get(),
                    input_fut_right.get());
        }

        template <typename T, class ExecutorImpl, typename InputIterType, typename OutputIterType>
        SingleValueCoroTask<thesoup::types::Unit, ExecutorImpl> collect_coroutine(
                std::reference_wrapper<CoroExecutorInterface<ExecutorImpl>> executor,
                InputIterType begin,
                InputIterType end,
                OutputIterType dest) {
            static_assert(thesoup::types::IsForwardIteratorOfType<InputIterType, std::future<T>>::value, "Expect a forward iterator of type std::future<T> for input.");
            static_assert(thesoup::types::IsOutputIteratorOfType<OutputIterType, T>::value, "Expect a output iterator of type T for output.");

            co_await executor;
            for (auto it = begin; it < end; it++) {
                while (!is_ready(*it)) {
                    co_await thesoup::types::Unit::unit;
                }
                *dest = (*it).get();
                dest++;
            }
            co_return thesoup::types::Unit::unit;
        }

        template <typename T, typename ExecutorImpl=void, typename=void> class FutureComposer;
        //!\endcond


        /**
         * \brief A class to help compose futures.
         *
         * This class helps compose futures. This uses a `CoroExecutorInterface` implementation to wait on futures.
         * It supports the following operations:
         *   - map
         *   - flat-map
         *   - join
         *   - collect
         *   The details of each is in the doc of the functions themselves. Here is some broad usage examples:
         *
         *   ```
         *   std::future<std::string> search_id_by_name_from_sql(const std::string& name); // Returns a string representing a int like "123"
         *   std::future<Record> fetch_record_from_sql_by_id(int id);
         *
         *   std::future<std::string> fut = search_id_by_name_from_sql("Amartya Datta Gupta");
         *   std::function<int(const std::string& val) f = [&](const std::string& id) {return to_int(id);};
         *
         *   std::future<Record> rec = FutureComposer<std::string, RoundRobinCoroExecutor>(exec, std::move(fut))
         *       .map(f)
         *       .flat_map(fetch_record_from_sql_by_id)
         *       .get_future();
         *
         *   ```
         *
         * @tparam T
         * @tparam ExecutorImpl n_set_a {graph.get_neighbours('A').get().unwrap()};
         */
        template <typename T, typename ExecutorImpl>
        class FutureComposer<T, ExecutorImpl, typename std::enable_if<std::is_base_of<CoroExecutorInterface<ExecutorImpl>, ExecutorImpl>::value>::type> {
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
            FutureComposer(const FutureComposer&& other)  noexcept : fut {std::move(other.fut)}, is_valid {other.is_valid} {
                other.is_valid = false;
            }

            FutureComposer(const FutureComposer<T, void, void>&)=delete;
            FutureComposer operator=(const FutureComposer&)=delete;
            FutureComposer operator=(const FutureComposer&&)=delete;


            /**
             * \brief Map composer.
             *
             * This method maps the return value of a future via an input function. It returns a future which when ready
             * is supposed to return the mapped value. Basically this allows us to return a future, while we wait on the
             * original future to complete in the background. Then we apply the mapping and set the value off the returned
             * future.
             *
             * Example usage:
             *
             * ```
             *
             * std::future<Record> rec = fetch_record_from_sql_by_id(123);
             *
             * std::future<std::string> name_fut = FutureComposer<int, RoundRobinCoroExecutor>(exec, std::move(rec))
             *     .map<std::string>(Record::get_name)
             *     .get_future();
             *
             * ```
             *
             * @tparam U The mapped future's type
             * @param function The mapping function
             * @return A FutureComposer<U>
             */
            template<typename U, typename Callable> FutureComposer<U, ExecutorImpl> map(const Callable function)
            requires is_map_function<Callable, T, U> {
                auto task {map_coroutine<T, U, ExecutorImpl, Callable>(executor, std::move(fut), function)};
                return FutureComposer<U, ExecutorImpl>(executor, std::move(task.future));
            }

            /**
             * \brief Flatmap composer.
             *
             * This function allows us to wait on a future in the background, get it's return value and pass it to a
             * function that returns another future in response. We return this final future.
             *
             * Example usage:
             *
             * ```
             *
             * std::future<int> search_id_by_name(const std::string& name);
             * std::future<Record> fetch_record_from_sql_by_id(123);
             *
             * std::future<int> id_fut = search_id_by_name("Amartya Datta Gupta");
             *
             * std::future<std::string> name_fut = FutureComposer<int, RoundRobinCoroExecutor>(exec, std::move(id_fut))
             *     .flat_map<Record>(fetch_record_from_sql_by_id)
             *     .map<std::string>(Record::get_name)
             *     .get_future();
             *
             * ```
             *
             * NOTE: Please note that the particular co routine that is used to implement this is alive till the second
             * future completes. So if you use an algorithm that chains flat_maps, you might end with a lot of active
             * co routines in the thread-pool.
             *
             * @tparam U The flat-mapped future's type
             * @param function The function that generates the next future
             * @return A FutureComposer<U>
             */
            template<typename U, typename Callable> FutureComposer<U, ExecutorImpl> flatmap(Callable function)
            requires is_flatmap_function<Callable, T, U> {
                auto task {flatmap_coroutine<T, U, ExecutorImpl>(executor, std::move(fut), function)};
                return FutureComposer<U, ExecutorImpl>(executor, std::move(task.future));
            }

            /**
             * \brief Join 2 futures.
             *
             * This function helps us join 2 futures of type T and U and return a future of type std::tuple<T, U>.
             *
             * Example usage:
             *
             * ```
             *
             * std::future<int> f1 = ...;
             * std::future<std::string> f2 = ...;
             *
             * std::future<std::tuple<int, std::string>> combined {
             *   FutureComposer<int, RoundRobinCoroExecutor>(exec, f1)
             *       .join(f2)
             *       .get_future()
             * };
             *
             * ```
             *
             * @tparam U The type of the future that we want to join with.
             * @param other
             * @return A FutureComposer<std::tuple<T, U>, ExecutorImpl> object
             */
            template <typename U> FutureComposer<std::tuple<T, U>, ExecutorImpl> join(std::future<U>&& other) {
                auto task {join_coroutine<T, U, ExecutorImpl>(executor, std::move(fut), std::move(other))};
                return FutureComposer<std::tuple<T, U>, ExecutorImpl>(executor, std::move(task.future));
            }

            /**
             * \brief Collect futures.
             *
             * This static function allows us to collect futures into 1 collection. Imagine we have a collection of
             * std::future<int> typed. We want to convert it to a single collection of ints. See example below to see
             * how this might work.
             *
             * ```
             * std::vector<std::future<int>> id_futures = ...;
             * std::vector<int> ids {};
             *
             * std::future<Unit> fut {
             *     FutureComposer<int, RoundRobinCoroExecutor>::collect(exec, id_futures.begin(), id_futures.end(), std::back_inserter(ids));
             * };
             * ```
             *
             * In the above example, when the `fut` is ready, the ids vector should be expected to be filled out.
             *
             * @tparam InputIterType Input iterator type. This is inferred. No need to explicitly provide this parameter.
             * @tparam OutputIterType Output iterator type. This is inferred. No need to explicitly provide this parameter.
             * @param exec The coroutine thread pool executor.
             * @param begin Start iterator for input collection.
             * @param end End iterator for input collection.
             * @param dest Output iterator for output collection.
             * @return A FutureComposer<Unit> object
             */
            template <typename InputIterType, typename OutputIterType>
            static FutureComposer<thesoup::types::Unit, ExecutorImpl> collect(std::reference_wrapper<ExecutorImpl> exec, InputIterType begin, InputIterType end, OutputIterType dest) {
                auto task {collect_coroutine<T, ExecutorImpl, InputIterType, OutputIterType>(exec, begin, end, dest)};
                return FutureComposer<thesoup::types::Unit, ExecutorImpl>(exec, std::move(task.future));
            }

            /**
             * \brief Get the future.
             *
             * This function returns the future object inside the composer.
             *
             * @return
             */
            std::future<T> get_future() {
                if (!is_valid) {
                    throw std::runtime_error("Invalid future.");
                } else {
                    is_valid = false;
                    return std::move(fut);
                }
            }
        };

        /**
         * \brief A class to help compose futures.
         *
         * This class helps compose futures. As opposed to the implementation that uses a `CoroExecutorInterface`
         * implementation to wait on futures, this implementation uses `std::async` to launch threads in the background
         * that call `wait` functions on the future and are blocked in the background.
         *
         * It supports the following operations:
         *   - map
         *   - flat-map
         *   - join
         *   - collect
         *   The details of each is in the doc of the functions themselves. Here is some broad usage examples:
         *
         *   ```
         *   std::future<std::string> search_id_by_name_from_sql(const std::string& name); // Returns a string representing a int like "123"
         *   std::future<Record> fetch_record_from_sql_by_id(int id);
         *
         *   std::future<std::string> fut = search_id_by_name_from_sql("Amartya Datta Gupta");
         *   std::function<int(const std::string& val) f = [&](const std::string& id) {return to_int(id);};
         *
         *   std::future<Record> rec = FutureComposer<std::string>(std::move(fut))
         *       .map(f)
         *       .flat_map(fetch_record_from_sql_by_id)
         *       .get_future();
         *
         *   ```
         *
         * @tparam T
         * @tparam ExecutorImpl n_set_a {graph.get_neighbours('A').get().unwrap()};
         */
        template <typename T>
        class FutureComposer<T, void, void> {
        private:
            std::future<T> fut;
            bool is_valid {true};
        public:
            explicit FutureComposer(std::future<T>&& fut) : fut {std::move(fut)} {}
            FutureComposer(FutureComposer&& other)  noexcept : fut {std::move(other.fut)}, is_valid {other.is_valid} {
                other.is_valid = false;
            }

            FutureComposer(const FutureComposer&)=delete;
            FutureComposer operator=(const FutureComposer&)=delete;
            FutureComposer operator=(const FutureComposer&&)=delete;

            /**
             * \brief Map composer.
             *
             * This method maps the return value of a future via an input function. It returns a future which when ready
             * is supposed to return the mapped value. Basically this allows us to return a future, while we wait on the
             * original future to complete in the background. Then we apply the mapping and set the value off the returned
             * future.
             *
             * Example usage:
             *
             * ```
             *
             * std::future<Record> rec = fetch_record_from_sql_by_id(123);
             *
             * std::future<std::string> name_fut = FutureComposer<int>(std::move(rec))
             *     .map<std::string>(Record::get_name)
             *     .get_future();
             *
             * ```
             *
             * @tparam U The mapped future's type
             * @param function The mapping function
             * @return A FutureComposer<U>
             */
            template<typename U, typename Callable> FutureComposer<U> map(const Callable function)
            requires is_map_function<Callable, T, U> {
                std::future<U> returned_future {std::async(std::launch::async, [moved_fut = std::move(fut), function]() mutable {
                    return function(moved_fut.get());
                })};
                return FutureComposer<U>(std::move(returned_future));
            }

            /**
             * \brief Flatmap composer.
             *
             * This function allows us to wait on a future in the background, get it's return value and pass it to a
             * function that returns another future in response. We return this final future.
             *
             * Example usage:
             *
             * ```
             *
             * std::future<int> search_id_by_name(const std::string& name);
             * std::future<Record> fetch_record_from_sql_by_id(123);
             *
             * std::future<int> id_fut = search_id_by_name("Amartya Datta Gupta");
             *
             * std::future<std::string> name_fut = FutureComposer<int>(std::move(id_fut))
             *     .flat_map<Record>(fetch_record_from_sql_by_id)
             *     .map<std::string>(Record::get_name)
             *     .get_future();
             *
             * ```
             *
             * @tparam U The flat-mapped future's type
             * @param function The function that generates the next future
             * @return A FutureComposer<U>
             */
            template<typename U, typename Callable> FutureComposer<U> flatmap(Callable function)
            requires is_flatmap_function<Callable, T, U> {
                std::future<U> return_future {
                    std::async([moved_fut = std::move(fut), function]() mutable -> U {
                        return function(moved_fut.get()).get();
                    })
                };
                return FutureComposer<U>(std::move(return_future));
            }

            /**
             * \brief Join 2 futures.
             *
             * This function helps us join 2 futures of type T and U and return a future of type std::tuple<T, U>.
             *
             * Example usage:
             *
             * ```
             *
             * std::future<int> f1 = ...;
             * std::future<std::string> f2 = ...;
             *
             * std::future<std::tuple<int, std::string>> combined {
             *   FutureComposer<int>(f1)
             *       .join(f2)
             *       .get_future()
             * };
             *
             * ```
             *
             * @tparam U The type of the future that we want to join with.
             * @param other
             * @return A FutureComposer<std::tuple<T, U>, ExecutorImpl> object
             */
            template <typename U> FutureComposer<std::tuple<T, U>, void, void> join(std::future<U>&& other) {
                std::future<std::tuple<T, U>> return_future {
                    std::async([moved_fut = std::move(fut), moved_other = std::move(other)] () mutable {
                        return std::make_tuple(moved_fut.get(), moved_other.get());
                    })
                };
                return FutureComposer<std::tuple<T, U>>(std::move(return_future));
            }

            /**
             * \brief Collect futures.
             *
             * This static function allows us to collect futures into 1 collection. Imagine we have a collection of
             * std::future<int> typed. We want to convert it to a single collection of ints. See example below to see
             * how this might work.
             *
             * ```
             * std::vector<std::future<int>> id_futures = ...;
             * std::vector<int> ids {};
             *
             * std::future<Unit> fut {
             *     FutureComposer<int>::collect(exec, id_futures.begin(), id_futures.end(), std::back_inserter(ids));
             * };
             * ```
             *
             * In the above example, when the `fut` is ready, the ids vector should be expected to be filled out.
             *
             * @tparam InputIterType Input iterator type. This is inferred. No need to explicitly provide this parameter.
             * @tparam OutputIterType Output iterator type. This is inferred. No need to explicitly provide this parameter.
             * @param exec The coroutine thread pool executor.
             * @param begin Start iterator for input collection.
             * @param end End iterator for input collection.
             * @param dest Output iterator for output collection.
             * @return A FutureComposer<Unit> object
             */
            template <typename InputIterType, typename OutputIterType>
            static FutureComposer<thesoup::types::Unit, void, void> collect(InputIterType begin, InputIterType end, OutputIterType dest) {
                static_assert(thesoup::types::IsForwardIteratorOfType<InputIterType, std::future<T>>::value, "Expect a forward iterator of type std::future<T> for input.");
                static_assert(thesoup::types::IsOutputIteratorOfType<OutputIterType, T>::value, "Expect a output iterator of type T for output.");

                std::future<thesoup::types::Unit> return_future {
                    std::async([begin, end, dest]() mutable {
                        for (auto it = begin; it < end; it++) {
                            *dest = (*it).get();
                            dest++;
                        }
                        return thesoup::types::Unit::unit;
                    }
                )};
                return FutureComposer<thesoup::types::Unit, void, void>(std::move(return_future));
            }

            /**
             * \brief Get the future.
             *
             * This function returns the future object inside the composer.
             *
             * @return
             */
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