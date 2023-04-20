#pragma once

#include <algorithm>
#include <future>
#include <iterator>
#include <unordered_map>
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

        /**
         * \brief A simple weighted graph
         *
         * This class represents a thesoup::types::Graph specialization, where the edge type is not a type, but a simple
         * literal that has ordering on it. This is useful for analyzing things like shortest path, etc. Here there is
         * no separate vertex and edge ID types (unlike thesoup::types::IndexedPropertyDiGraph). It is the same as the
         * vertex and edge types.
         *
         * \tparam V_TYPE The vertex type
         * \tparam WEIGHT_TYPE The edge type
         */
        template<typename V_TYPE, typename WEIGHT_TYPE>
        class SimpleWeightedGraph
                : public Graph<SimpleWeightedGraph<V_TYPE, WEIGHT_TYPE>, V_TYPE, WEIGHT_TYPE, SimpleWeightedGraphAttributes::ErrorCode, V_TYPE, WEIGHT_TYPE> {
        private:
            std::unordered_map<V_TYPE, std::unordered_set<thesoup::types::Neighbour<V_TYPE, WEIGHT_TYPE>>> adj_list {};
            std::unordered_map<V_TYPE, unsigned long> incoming_edges_count {};
        public:
            /**
             * \brief Get a list of all neighbours of a vertex
             *
             * This method returns all neighbours of a vertex, as promised in the interface (thesoup::types::Graph).
             * The return value has a list of neighbours (thesoup::types::Neighbour). Each neighbour has information about
             * the neighbouring vertex and the weight.
             *
             * \param vertex
             * \return Result<List<Neighbour>, ErrorCode>
             */
            std::future<thesoup::types::Result<std::vector<Neighbour<V_TYPE, WEIGHT_TYPE>>, SimpleWeightedGraphAttributes::ErrorCode>>
            get_neighbours(const V_TYPE &vertex) const noexcept {
                if (adj_list.find(vertex) == adj_list.end()) {
                    return thesoup::async::make_ready_future(
                            thesoup::types::Result<std::vector<Neighbour<V_TYPE, WEIGHT_TYPE>>, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                                    SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX));
                } else {
                    std::vector<Neighbour<V_TYPE, WEIGHT_TYPE>> neighbours;
                    neighbours.reserve(adj_list.at(vertex).size());
                    std::copy(adj_list.at(vertex).begin(), adj_list.at(vertex).end(), std::back_inserter(neighbours));
                    return thesoup::async::make_ready_future(
                            thesoup::types::Result<std::vector<Neighbour<V_TYPE, WEIGHT_TYPE>>, SimpleWeightedGraphAttributes::ErrorCode>::success(
                                    neighbours));
                }
            }

            /**
             * \brief Get neighbours of a vertex, connected by a certain edge
             *
             * This method returns the neighbours of a vertex connected by a certain edge, as promised in the interface
             * (thesoup::types::Graph). The return value has a list of vertices. Unlike the get all vertices method, this
             * just returns a vector of vertices instead of neighbour types, as the edge typee is already specified during
             * the method call.
             *
             * \param vertex Vertex whose neighbours are needed.
             * \param edge_type The edge type that is to bee applied as a filter to all edges coming out of the graph.
             * \return Result<Vector, ErrorCode>
             */
            std::future<thesoup::types::Result<std::vector<V_TYPE>, SimpleWeightedGraphAttributes::ErrorCode>>
            get_neighbours(const V_TYPE &vertex, const WEIGHT_TYPE &edge_type) const noexcept {
                if (adj_list.find(vertex) == adj_list.end()) {
                    return thesoup::async::make_ready_future(
                            thesoup::types::Result<std::vector<V_TYPE>, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                                    SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX));
                } else {
                    std::vector<V_TYPE> neighbours;
                    for (const auto &neighbour: adj_list.at(vertex)) {
                        if (neighbour.edge == edge_type) {
                            neighbours.push_back(neighbour.vertex);
                        }
                    }
                    return thesoup::async::make_ready_future(
                            thesoup::types::Result<std::vector<V_TYPE>, SimpleWeightedGraphAttributes::ErrorCode>::success(
                                    neighbours));
                }
            }

            /**
             * \brief Insert a vertex
             *
             * Insert a vertex. The same vertex is returned after insert. This is an unfortunate implementation issue and
             * cannot be helped. Ignore the return value.
             *
             * \param vertex Vertex to insert
             * \return Result<vertex, ErrorCode>
             */
            std::future<thesoup::types::Result<V_TYPE, SimpleWeightedGraphAttributes::ErrorCode>>
            insert_vertex(const V_TYPE &vertex) noexcept {
                if (adj_list.find(vertex) == adj_list.end()) {
                    adj_list[vertex];
                    incoming_edges_count.emplace(vertex, 0);
                }
                return thesoup::async::make_ready_future(
                        thesoup::types::Result<V_TYPE, SimpleWeightedGraphAttributes::ErrorCode>::success(vertex));
            }

            /**
             * \brief Insert an edge
             *
             * Insert an edge. The return value on success is a Unit type. The types that the edge will accept is the
             * vertex and edge types, as there is no separate ID types here unlike thesoup::types::IndexedPropertyDiGraph
             *
             * \param edge a thesoup::types::Edge object
             * \return
             */
            std::future<thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>>
            insert_edge(const Edge<V_TYPE, WEIGHT_TYPE> &edge) noexcept {
                if (adj_list.find(edge.from) == adj_list.end() || adj_list.find(edge.to) == adj_list.end()) {
                    return thesoup::async::make_ready_future(
                            thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                                    SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX));
                } else {
                    if (adj_list.at(edge.from).find({edge.edge_type, edge.to}) == adj_list.at(edge.from).end()) {
                        adj_list.at(edge.from).insert({edge.edge_type, edge.to});
                        incoming_edges_count.at(edge.to)++;

                    }
                    return thesoup::async::make_ready_future(
                            thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::success(
                                    thesoup::types::Unit::unit));
                }
            }

            /**
             * \brief Delete a vertex
             *
             * Delete a vertex. If the vertex is not found, an error code is returned.
             *
             * \param vertex The vertex to delete
             * @return Result<Unit, ErrorCode>
             */
            std::future<thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>>
            delete_vertex(const V_TYPE &vertex) noexcept {
                if (adj_list.find(vertex) == adj_list.end()) {
                    return thesoup::async::make_ready_future(
                            thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                                    SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX));
                } else if (adj_list.at(vertex).size() > 0 || incoming_edges_count.at(vertex) > 0) {
                    return thesoup::async::make_ready_future(
                            thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                                    SimpleWeightedGraphAttributes::ErrorCode::CONNECTED_VERTEX));
                } else {
                    adj_list.erase(vertex);
                    incoming_edges_count.erase(vertex);
                    return thesoup::async::make_ready_future(
                            thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::success(
                                    thesoup::types::Unit::unit));
                }
            }

            /**
             * \brief Delete an edge
             *
             * This method deletes an edge. If the edge is not found, maybe because of non-existent vertices of unconnected
             * vertices, an error code is returned indicating the problem.
             *
             * NOTE: The reasons for failure are:
             *   - Non existent vertices (either from or to or both)
             *   - The vertices exist bit are not connected by an edge of the specified weight.
             *
             * \param edge The edge to delete
             * \return Result<Unit, ErrorCode>
             */
            std::future<thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>>
            delete_edge(const Edge<V_TYPE, WEIGHT_TYPE> &edge) noexcept {
                if (adj_list.find(edge.from) == adj_list.end() || adj_list.find(edge.to) == adj_list.end()) {
                    return thesoup::async::make_ready_future(
                            thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                                    SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX));
                } else {
                    auto it{adj_list.at(edge.from).find({edge.edge_type, edge.to})};
                    if (it == adj_list.at(edge.from).end()) {
                        return thesoup::async::make_ready_future(
                                thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::failure(
                                        SimpleWeightedGraphAttributes::ErrorCode::NON_EXISTENT_EDGE));
                    } else {
                        adj_list.at(edge.from).erase(it);
                        incoming_edges_count.at(edge.to)--;
                        return thesoup::async::make_ready_future(
                                thesoup::types::Result<thesoup::types::Unit, SimpleWeightedGraphAttributes::ErrorCode>::success(
                                        thesoup::types::Unit::unit));
                    }
                }
            }
        };
    }
}