#pragma once

#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <unordered_set>

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
        template<typename V_TYPE, typename WEIGHT_TYPE>
        bool operator==(const Neighbour<V_TYPE, WEIGHT_TYPE> &left, const Neighbour<V_TYPE, WEIGHT_TYPE> &right) {
            return left.vertex == right.vertex && left.edge == right.edge;
        }

        namespace SimpleWeightedGraphAttributes {
            enum class ErrorCode : char {
                NON_EXISTENT_VERTEX,
                NON_EXISTENT_EDGE,
                CONNECTED_VERTEX
            };
        }

        template<typename V_TYPE, typename WEIGHT_TYPE>
        class SimpleWeightedGraph
                : public Graph<V_TYPE, WEIGHT_TYPE, SimpleWeightedGraphAttributes::ErrorCode, V_TYPE, WEIGHT_TYPE> {
        private:
            std::unordered_map<V_TYPE, std::unordered_set<thesoup::types::Neighbour<V_TYPE, WEIGHT_TYPE>>> adj_list {};
            std::unordered_map<V_TYPE, unsigned long> incoming_edges_count {};
        public:
            thesoup::types::Result<std::vector<Neighbour<V_TYPE, WEIGHT_TYPE>>, SimpleWeightedGraphAttributes::ErrorCode>
            get_neighbours(const V_TYPE &vertex) const noexcept {
                if (adj_list.find(vertex) == adj_list.end()) {
                    return thesoup::types::Result<std::vector<Neighbour<V_TYPE, WEIGHT_TYPE>>, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                            SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else {
                    std::vector<Neighbour<V_TYPE, WEIGHT_TYPE>> neighbours;
                    neighbours.reserve(adj_list.at(vertex).size());
                    std::copy(adj_list.at(vertex).begin(), adj_list.at(vertex).end(), std::back_inserter(neighbours));
                    return thesoup::types::Result<std::vector<Neighbour<V_TYPE, WEIGHT_TYPE>>, SimpleWeightedGraphAttributes::ErrorCode>::success(
                            neighbours);
                }
            }

            thesoup::types::Result<std::vector<V_TYPE>, SimpleWeightedGraphAttributes::ErrorCode>
            get_neighbours(const V_TYPE &vertex, const WEIGHT_TYPE &edge_type) const noexcept {
                if (adj_list.find(vertex) == adj_list.end()) {
                    return thesoup::types::Result<std::vector<V_TYPE>, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                            SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else {
                    std::vector<V_TYPE> neighbours;
                    for (const auto &neighbour: adj_list.at(vertex)) {
                        if (neighbour.edge == edge_type) {
                            neighbours.push_back(neighbour.vertex);
                        }
                    }
                    return thesoup::types::Result<std::vector<V_TYPE>, SimpleWeightedGraphAttributes::ErrorCode>::success(
                            neighbours);
                }
            }

            thesoup::types::Result<V_TYPE, SimpleWeightedGraphAttributes::ErrorCode>
            insert_vertex(const V_TYPE &vertex) noexcept {
                if (adj_list.find(vertex) == adj_list.end()) {
                    adj_list[vertex];
                    incoming_edges_count.emplace(vertex, 0);
                }
                return thesoup::types::Result<V_TYPE, SimpleWeightedGraphAttributes::ErrorCode>::success(vertex);
            }

            thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>
            insert_edge(const Edge<V_TYPE, WEIGHT_TYPE> &edge) noexcept {
                if (adj_list.find(edge.from) == adj_list.end() || adj_list.find(edge.to) == adj_list.end()) {
                    return thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                            SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else {
                    if (adj_list.at(edge.from).find({edge.edge_type, edge.to}) == adj_list.at(edge.from).end()) {
                        adj_list.at(edge.from).insert({edge.edge_type, edge.to});
                        incoming_edges_count.at(edge.to)++;

                    }
                    return thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::success(
                            thesoup::types::Unit::unit);
                }
            }

            thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>
            delete_vertex(const V_TYPE &vertex) noexcept {
                if (adj_list.find(vertex) == adj_list.end()) {
                    return thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                            SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else if (adj_list.at(vertex).size() > 0 || incoming_edges_count.at(vertex) > 0) {
                    return thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                            SimpleWeightedGraphAttributes::ErrorCode::CONNECTED_VERTEX);
                } else {
                    adj_list.erase(vertex);
                    incoming_edges_count.erase(vertex);
                    return thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::success(
                            thesoup::types::Unit::unit);
                }
            }

            thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>
            delete_edge(const Edge<V_TYPE, WEIGHT_TYPE> &edge) noexcept {
                if (adj_list.find(edge.from) == adj_list.end() || adj_list.find(edge.to) == adj_list.end()) {
                    return thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                            SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else {
                    auto it{adj_list.at(edge.from).find({edge.edge_type, edge.to})};
                    if (it == adj_list.at(edge.from).end()) {
                        return thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                                SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_EDGE);
                    } else {
                        adj_list.at(edge.from).erase(it);
                        incoming_edges_count.at(edge.to)--;
                        return thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::success(
                                thesoup::types::Unit::unit);
                    }
                }
            }

        };
    }
}