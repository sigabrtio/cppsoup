#pragma once

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <thesoup/types/types.hpp>
#include <thesoup/types/graph.hpp>

/**
 * \namespace thesoup
 *
 * \brief The root namespace.
 * */
namespace thesoup {

    /**
     * \namespace thesoup::types
     *
     * \brief Sub namespace with all numeric classes and functions.
     * */
    namespace types {

        template <typename V_TYPE, typename E_TYPE> class DirectedGraph : public Graph<V_TYPE, E_TYPE, DirectedGraph<V_TYPE, E_TYPE>> {
        protected:
            struct _Edges {
                std::unordered_map<E_TYPE, std::unordered_set<V_TYPE>> neighbours {};
                std::unordered_set<Neighbour<V_TYPE, E_TYPE>> all_neighbours {};

                bool contains(const V_TYPE& vertex) {
                    return all_neighbours.find(vertex) != all_neighbours.end();
                }
            };
            std::unordered_map<V_TYPE, _Edges> store;

        public:
            DirectedGraph() {}

            const thesoup::types::Result<std::reference_wrapper<std::unordered_set<V_TYPE>>, ErrorCode>
            get_neighbours(const V_TYPE& vertex, const E_TYPE& edge_type) const noexcept {
                if (store.find(vertex) != store.end()) {
                    return thesoup::types::Result<std::reference_wrapper<std::unordered_set<V_TYPE>>, ErrorCode>::failure(ErrorCode::NON_EXISTENT_VERTEX);
                } else if(!store.at(vertex).contains(edge_type)) {
                    return thesoup::types::Result<std::reference_wrapper<std::unordered_set<V_TYPE>>, ErrorCode>::failure(ErrorCode::NON_EXISTENT_EDGE);
                } else {
                    return std::reference_wrapper(store.at(vertex).neighbours.at(edge_type));
                }
            }

            const thesoup::types::Result<std::reference_wrapper<std::unordered_set<Neighbour<V_TYPE, E_TYPE>>>, ErrorCode>
            get_neighbours(const V_TYPE& vertex) const noexcept {
                if (store.find(vertex) != store.end()) {
                    return thesoup::types::Result<std::reference_wrapper<std::unordered_set<V_TYPE>>, ErrorCode>::failure(
                            ErrorCode::NON_EXISTENT_VERTEX);
                } else {
                    return std::reference_wrapper(store.at(vertex).all_neighbours);
                }
            }

            void insert_vertex(const V_TYPE& vertex) noexcept {
                if (store.find(vertex) == store.end()) {
                    store.emplace(vertex, _Edges());
                }
            }

            thesoup::types::Result<thesoup::types::Unit, ErrorCode> insert_edge(const Edge<V_TYPE, E_TYPE>& edge) noexcept {
                if (this->store.find(edge.from) == this->store.end() || this->store.find(edge.to) == this->store.end()) {
                    return thesoup::types::Result<thesoup::types::Unit, ErrorCode>::failure(ErrorCode::NON_EXISTENT_VERTEX);
                }
                if (this->store.at(edge.from).neighbours.find(edge.edge) == this->store.at(edge.from).neighbours.end()) {
                    this->store.at(edge.from).neighbours.emplace(edge.edge, std::unordered_set<V_TYPE>());
                }
                this->store.at(edge.from).neighbours.at(edge.edge).insert(edge.to);
                this->store.at(edge.from).all_neighbours.insert({edge.to, edge.edge});
                return thesoup::types::Result<thesoup::types::Unit, ErrorCode>::success(thesoup::types::Unit::unit);
            }

            std::vector<Edge<V_TYPE, E_TYPE>> get_edges() noexcept {
                std::vector<Edge<V_TYPE, E_TYPE>> edges;
                for (const auto& [k,v] : store) {
                    std::for_each(
                            v.all_neighbours.begin(),
                            v.all_neighbours.end(),
                            [&](Neighbour<V_TYPE,E_TYPE> n) {
                                edges.push_back(Edge<V_TYPE,E_TYPE>{k, n.edge, n.vertex});
                            });
                }
                return edges;
            }

            std::vector<V_TYPE> get_vertices() {
                std::vector<V_TYPE> vertices {};
                vertices.reserve(store.size());
                for (auto& [k,_] : store) {
                    vertices.push_back(k);
                }
                return vertices;
            }
        };
    }
}
