#include <functional>
#include <future>
#include <optional>
#include <unordered_set>

#include <thesoup/types/types.hpp>
#include <thesoup/types/graph.hpp>
#include <thesoup/algorithms/utils.hpp>

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
        template<class Impl, typename V_TYPE, typename E_TYPE, typename ERR, typename VID_TYPE=V_TYPE, typename EID_TYPE=E_TYPE>
        std::future<thesoup::types::Result<thesoup::types::Unit, ERR>>
        bfs(
                const thesoup::types::Graph<Impl, V_TYPE, E_TYPE, ERR, VID_TYPE, EID_TYPE>& graph,
                const V_TYPE& start,
                std::function<void(const std::optional<V_TYPE>&, const V_TYPE&)> visit_callback
                ) noexcept {

            std::unordered_set<V_TYPE> visited {};
            std::vector<V_TYPE> frontier {};
            std::vector<V_TYPE> next_frontier;

            frontier.push_back(start);
            visited.insert(start);
            visit_callback(std::nullopt, start);

            while (frontier.size() > 0) {
                for (const auto& u : frontier) {
                    auto res {graph.get_neighbours(u).get()};
                    if (!res) {
                        return thesoup::async::make_ready_future(
                                thesoup::types::Result<thesoup::types::Unit, ERR>::failure(res.error()));
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
            return thesoup::async::make_ready_future(thesoup::types::Result<thesoup::types::Unit, ERR>::success(thesoup::types::Unit::unit));
        }
    }
}