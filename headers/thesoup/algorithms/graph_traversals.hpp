#include <functional>
#include <optional>
#include <unordered_set>

#include <thesoup/types/types.hpp>
#include <thesoup/types/graph.hpp>


namespace thesoup {

    namespace algorithms {

        template<class Impl, typename V_TYPE, typename E_TYPE, typename ERR, typename VID_TYPE=V_TYPE, typename EID_TYPE=E_TYPE>
        thesoup::types::Result<thesoup::types::Unit, ERR> bfs(
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
                    auto res {graph.get_neighbours(u)};
                    if (!res) {
                        return thesoup::types::Result<thesoup::types::Unit, ERR>::failure(res.error());
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
            return thesoup::types::Result<thesoup::types::Unit, ERR>::success(thesoup::types::Unit::unit);
        }
    }
}