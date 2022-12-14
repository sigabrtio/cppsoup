#include <thesoup/types/graph.hpp>

#include <functional>
#include <unordered_set>

namespace thesoup {

    namespace algorithms {

        template<class Impl, typename V_TYPE, typename E_TYPE, typename ERR, typename VID_TYPE=V_TYPE, typename EID_TYPE=E_TYPE>
        void bfs(
                const thesoup::types::Graph<Impl, V_TYPE, ERR, E_TYPE, VID_TYPE, EID_TYPE>& graph,
                const V_TYPE& start,
                std::function<void(const V_TYPE&, const V_TYPE&)> visit_callback
                ) noexcept {

            std::unordered_set<V_TYPE> visited {};
            std::vector<V_TYPE> frontier {};
            std::vector<V_TYPE> next_frontier;

            frontier.push_back(start);

            while (frontier.size() > 0) {
                for (const auto& u : frontier) {
                    for(const auto&n : graph.get_neighbours(u).unwrap()) {
                        V_TYPE v = n.vertex;
                        if (visited.find(v) != visited.end()) {
                            visited.insert(v);
                            next_frontier.push_back(v);
                            visit_callback(u, v);
                        }
                    }
                }
                frontier.swap(next_frontier);
                next_frontier.clear();
            }
        }
    }
}