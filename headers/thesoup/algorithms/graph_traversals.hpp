#include <functional>
#include <future>
#include <optional>
#include <unordered_set>

#include <thesoup/types/types.hpp>
#include <thesoup/types/graph.hpp>
#include <thesoup/async/futures.hpp>

/**
 * \namespace thesoup
 *
 * \brief The root namespace.
 * */
namespace thesoup {

    /**
     * \namespace thesoup::algorithms
     *
     * \brief Sub namespace with all algorithms.
     * */
    namespace algorithms {
        template <typename Callable, typename V_TYPE>
        concept is_bfs_callback = requires (Callable callable, std::optional<V_TYPE> v1, V_TYPE v2) {
            {callable(v1, v2)};
        };

        template<class Impl, class ExecutorImpl, typename V_TYPE, typename E_TYPE, typename ERR, typename Callable, typename VID_TYPE=V_TYPE, typename EID_TYPE=E_TYPE>
        thesoup::async::SingleValueCoroTask<thesoup::types::Result<thesoup::types::Unit, ERR>, ExecutorImpl> bfs_coroutine(
                std::reference_wrapper<thesoup::async::CoroExecutorInterface<ExecutorImpl>> executor,
                std::reference_wrapper<const thesoup::types::Graph<Impl, V_TYPE, E_TYPE, ERR, VID_TYPE, EID_TYPE>> graph,
                const V_TYPE start,
                const Callable visit_callback) noexcept requires thesoup::algorithms::is_bfs_callback<Callable, V_TYPE> {

            std::unordered_set<V_TYPE> visited {};
            std::vector<V_TYPE> frontier {};
            std::vector<V_TYPE> next_frontier;

            co_await executor;

            frontier.push_back(start);
            visited.insert(start);
            visit_callback(std::nullopt, start);

            while (frontier.size() > 0) {
                for (const auto& u : frontier) {
                    auto res_fut {graph.get().get_neighbours(u)};

                    // Poll the future
                    while (!thesoup::async::is_ready(res_fut)) {
;                        co_await thesoup::types::Unit::unit;
                    }
                    auto res {res_fut.get()};
                    if (!res) {
                        co_return thesoup::types::Result<thesoup::types::Unit, ERR>::failure(res.error());
                    }
                    for(const auto&n : res.unwrap()) {
                        const auto& v = n.vertex;
                        if (visited.find(v) == visited.end()) {
                            visited.insert(v);
                            next_frontier.push_back(v);
                            visit_callback(u, v);
                        }
                    }
                }
                frontier.swap(next_frontier);
                next_frontier.clear();
            }
            co_return thesoup::types::Result<thesoup::types::Unit, ERR>::success(thesoup::types::Unit::unit);

        }

        /**
         * \brief Implement the bfs graph traversal method.
         *
         * This function implements a BFS algorithm on an input graph. On each visit, a user defined function is called with
         * the parent-child pair.
         *
         * TODO: Implement setting of max level and a filter function for neighbours.
         *
         * \tparam Impl - The Impl parameter for the thesoup::types::Graph argument. Don't worry about this. This will be inferred from the argument.
         * \tparam V_TYPE - The V_TYPE parameter for the thesoup::types::Graph argument. Don't worry about this. This will be inferred from the argument.
         * \tparam E_TYPE - The E_TYPE parameter for the thesoup::types::Graph argument. Don't worry about this. This will be inferred from the argument.
         * \tparam ERR - The ERR parameter for the thesoup::types::Graph argument. Don't worry about this. This will be inferred from the argument.
         * \tparam VID_TYPE - The VID_TYPE parameter for the thesoup::types::Graph argument. Don't worry about this. This will be inferred from the argument.
         * \tparam EID_TYPE - The EID_TYPE parameter for the thesoup::types::Graph argument. Don't worry about this. This will be inferred from the argument.
         *
         * \param graph - The input graph (CRTP specialization of thesoup::types::Graph).
         * \param start - The starting point of the traversal.
         * \param visit_callback - The callback method to call on each node. It is called with (parent_node, child_node) args. If there is no parent (starting node), std::nullopt is passed.
         * \return Void
         */
        template<class Impl, typename V_TYPE, typename E_TYPE, typename ERR, typename ExecutorImpl, typename Callable, typename VID_TYPE=V_TYPE, typename EID_TYPE=E_TYPE>
        std::future<thesoup::types::Result<thesoup::types::Unit, ERR>>
        bfs(
                const thesoup::types::Graph<Impl, V_TYPE, E_TYPE, ERR, VID_TYPE, EID_TYPE>& graph,
                const V_TYPE& start,
                Callable visit_callback,
                thesoup::async::CoroExecutorInterface<ExecutorImpl>& executor) noexcept requires thesoup::algorithms::is_bfs_callback<Callable, V_TYPE> {
            return bfs_coroutine<Impl, ExecutorImpl, V_TYPE, E_TYPE, ERR, Callable, VID_TYPE, EID_TYPE>(
                    std::reference_wrapper(executor),
                    std::reference_wrapper(graph),
                    start,
                    visit_callback).future;
        }
    }
}