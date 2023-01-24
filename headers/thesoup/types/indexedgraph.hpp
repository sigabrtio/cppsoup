#pragma once

#include <algorithm>
#include <iterator>
#include <limits>
#include <ranges>
#include <set>
#include <unordered_map>

#include <thesoup/types/graph.hpp>
#include <thesoup/types/types.hpp>

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
        // TODO: Re-evaluate this approach
        template <typename T> bool operator<(const thesoup::types::Neighbour<T, T> &left,
                       const thesoup::types::Neighbour<std::size_t, std::size_t> &right) {
            return left.vertex < right.vertex;
        }

        template <typename T> bool operator==(const thesoup::types::Neighbour<T, T> &left,
                        const thesoup::types::Neighbour<std::size_t, std::size_t> &right) {
            return left.vertex == right.vertex && left.edge == right.edge;
        }

        namespace IndexedPropertyDiGraphAttributes {
            enum class ErrorCode : char {
                TOO_MANY_DUPLICATE_IDS,
                NON_EXISTENT_VERTEX,
                NON_EXISTENT_EDGE,
                INVALID_EDGE_TYPE,
                CONNECTED_VERTEX,
                BROKEN_EDGE
            };
        }

       /**
        *
        * \brief A property graph type with indices into the vertices and edge types.
        *
        * This class implements the thesoup::types::Graph interface with some extra functionality, and using `std::size_t`
        * as the `VID_TYPE` and `EID_TYPE`. This graph models vertices of type V_TYPE connected by "properties". A good
        * use case id RDF graph, where the subject and object are vertices of V_TYPE and the predicate if of the type
        * E_TYPE. Since in RDF graphs the predicates are highly repeated, we are better off "registering" a predicate once
        * (using the register_edge_type method) and using the returned ID (std::size_t) again and again in the actual graph.
        *
        * Similarly for vertices, when we insert a vertex, an ID (std::size_t) is returned that then we use to construct
        * the actual graph. Since we expose the IDs to the user, traversal operations and hydration can be decoupled,
        * resulting in good performance opportunities.
        *
        * \tparam V_TYPE The vertex type
        * \tparam E_TYPE The edge type
        *
        */
        template<typename V_TYPE, typename E_TYPE>
        class IndexedPropertyDiGraph
                : public Graph<IndexedPropertyDiGraph<V_TYPE, E_TYPE>, V_TYPE, E_TYPE, IndexedPropertyDiGraphAttributes::ErrorCode, std::size_t, std::size_t> {

        private:
            // Encapsulates both the incoming and outgoing edges.
            // The point of maintaining incoming edges count is tp enable advanced queries  and deletion operations.
            struct _Edges {
                std::set<Neighbour<std::size_t, std::size_t>> outgoing_edges {};
                std::set<Neighbour<std::size_t, std::size_t>> incoming_edges {};
            };
            struct _VertexRecord {
                // This struct encapsulates a vertex and all the housekeeping information needed.
                V_TYPE vertex;
                unsigned long incoming_edges;
            };
            std::unordered_map<std::size_t, V_TYPE> vertex_index{};
            std::unordered_map<std::size_t, E_TYPE> edge_index{};
            std::unordered_map<std::size_t, _Edges> adj_list{};


        public:
            /**
             * \brief Register an edge.
             *
             * This method registers an edge type and returns an ID for the registered edge type. Subsequent additions of
             * edges will be of the type (from vertex ID, edge type, to vertex ID). As mentioned before, this is really
             * helpful when the edge types are large structs. This allows all operations other than the hydration to be
             * on long integers, making them efficient.
             *
             * If the edge type has been already registered, then the function will return a Success(previous_id). In case
             * of a collision, the function tries to rehash using an `attempt_id` number. In the rare case the collisions
             * are way too much and we run out of attempt ids, a failure is returned.
             *
             * The id type is std::size_t. So on a 64 bit system, you have 2^64 collisions, you have bigger problems.
             * This function expects to have a `struct hash with operator() defined to return a std::size_t in namespace std
             * (a pretty standard requirement to make anything hashable).
             *
             * Worst case complexity in case of a horrendous hash function is O(n). Example, if the hash function is a
             * constant, this will happen.
             *
             * Average case complexity with a good hash function should be O(1).
             *
             * NOTE: Once registered, an edge type cannot be unregistered, as it might leave the graph as inconsistent
             *
             * \param edge
             * \return Result<edge_type_id, error_code>
             */
            thesoup::types::Result<std::size_t, IndexedPropertyDiGraphAttributes::ErrorCode>
            register_edge_type(const E_TYPE &edge) noexcept {
                std::size_t attempt_id{};
                std::size_t id;
                while (attempt_id < std::numeric_limits<std::size_t>::max()) {
                    auto hash1{std::hash<E_TYPE>()(edge)};
                    id = hash1 ^ (attempt_id + (hash1 << 6) + (hash1 >> 2) + 0x9e3779b9);
                    if (edge_index.find(id) == edge_index.end()) {
                        edge_index[id] = edge;
                    } else if (edge_index.at(id) == edge) {
                        return thesoup::types::Result<std::size_t, IndexedPropertyDiGraphAttributes::ErrorCode>::success(
                                id);
                    } else {
                        attempt_id++;
                    }
                }

                return thesoup::types::Result<std::size_t, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                        IndexedPropertyDiGraphAttributes::ErrorCode::TOO_MANY_DUPLICATE_IDS);
            }

            /**
             * \brief Add a vertex
             *
             * This method adds a vertex to the graph and returns an ID. The vertex does not necessarily have to be attached
             * to another vertex. Since edge insertions are of the form (from ID, edge type, to ID), either keep this ID
             * stored somewhere of just query the ID with the hydration API.
             *
             * If the vertex has been already added, then the function will return a Success(previous_id). In case
             * of a collision, the function tries to rehash using an `attempt_id` number. In the rare case the collisions
             * are way too much and we run out of attempt ids, a failure is returned.
             *
             * The id type is std::size_t. So on a 64 bit system, you have 2^64 collisions, you have bigger problems.
             * This function expects to have a `struct hash with operator() defined to return a std::size_t in namespace std
             * (a pretty standard requirement to make anything hashable).
             *
             * Worst case complexity in case of a horrendous hash function is O(n). Example, if thee hash function is a
             * constant, this will happen.
             *
             * \param vertex
             * \return Result<vertex_id, error_code>
             */
            thesoup::types::Result<std::size_t, IndexedPropertyDiGraphAttributes::ErrorCode>
            insert_vertex(const V_TYPE &vertex) noexcept {
                std::size_t attempt_id {};
                std::size_t id;
                while (attempt_id < std::numeric_limits<std::size_t>::max()) {
                    auto hash1{std::hash<V_TYPE>()(vertex)};
                    id = hash1 ^ (attempt_id + (hash1 << 6) + (hash1 >> 2) + 0x9e3779b9);
                    if (vertex_index.find(id) == vertex_index.end()) {
                        vertex_index.emplace(id, vertex);
                        adj_list.emplace(id, _Edges{});
                    } else if (vertex_index.at(id) == vertex) {
                        return thesoup::types::Result<std::size_t, IndexedPropertyDiGraphAttributes::ErrorCode>::success(
                                id);
                    } else {
                        attempt_id++;
                    }
                }

                return thesoup::types::Result<std::size_t, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                        IndexedPropertyDiGraphAttributes::ErrorCode::TOO_MANY_DUPLICATE_IDS);
            }

            /**
             * \brief Delete a vertex
             *
             * This method accepts a vertex ID and delete the vertex both from the index and the graph. If the vertex
             * does not exist, an error specifying that is returned. Also, if a vertex is connected to something, it will
             * not be deleted. An error specifying that will be returned.
             *
             * \param vertex
             * \return Result<Unit, ErrorCode>
             */
            thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>
            delete_vertex(const std::size_t &vertex) noexcept {
                if (adj_list.find(vertex) == adj_list.end()) {
                    return thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else if (adj_list.at(vertex).incoming_edges.size() > 0 || adj_list.at(vertex).outgoing_edges.size() > 0) {
                    return thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::CONNECTED_VERTEX);
                } else {
                    adj_list.erase(vertex);
                    vertex_index.erase(vertex);
                    return thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>::success(
                            thesoup::types::Unit::unit);
                }
            }

            /**
             * \brief Hydrate a vertex
             *
             * This method accepts a vertex ID and returns the corresponding actual vertex. If the vertex is absent, an
             * error specifying that is returned.
             *
             * \param vertex_id
             * \return Result<vertex, ErrorCode>
             */
            thesoup::types::Result<const V_TYPE, IndexedPropertyDiGraphAttributes::ErrorCode>
            hydrate_vertex(const std::size_t &vertex_id) const noexcept {
                if (vertex_index.find(vertex_id) == vertex_index.end()) {
                    return thesoup::types::Result<const V_TYPE, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else {
                    return thesoup::types::Result<const V_TYPE, IndexedPropertyDiGraphAttributes::ErrorCode>::success(
                            vertex_index.at(vertex_id));
                }
            }

            /**
             * \brief Add an edge.
             *
             * This function takes an edge of type thesoup::types::Edge with the template parameters set to std::size_t.
             * If the vertices in the (from, edge, to) do not exist, an error is returned via the thesoup::types::Result
             * type. Same, if the ID corresponding the edge is not registered previously.
             *
             * \param edge The edge to insert of the form (from vertex ID, edge ID, to vertex ID)
             * \return Result<Unit, ErrorCode>
             */
            thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>
            insert_edge(const thesoup::types::Edge<std::size_t, std::size_t> &edge) noexcept {
                if (vertex_index.find(edge.from) == vertex_index.end() ||
                    vertex_index.find(edge.to) == vertex_index.end()) {
                    return thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else if (edge_index.find(edge.edge_type) == edge_index.end()) {
                    return thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::INVALID_EDGE_TYPE);
                } else {
                    adj_list.at(edge.from).outgoing_edges.insert({edge.edge_type, edge.to});
                    adj_list.at(edge.to).incoming_edges.insert({edge.edge_type, edge.from});
                    return thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>::success(
                            thesoup::types::Unit::unit);
                }
            }

            /**
             * \brief Delete an edge
             *
             * This method deletes an edge from the graph. NOTE: The edge type is not effected by this method. If the
             * edge is invalid because on of both of the vertices does not exist, an error is returned. If the edge type
             * happens to be invalid of the edge does not exist, an error is returned.
             *
             * \param edge
             * \return Result<Unit, ErrorCode>
             */
            thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>
            delete_edge(const thesoup::types::Edge<std::size_t, std::size_t>& edge) noexcept {
                std::size_t from_id {edge.from};
                std::size_t to_id {edge.to};
                std::size_t edge_type_id {edge.edge_type};

                if (adj_list.find(from_id) == adj_list.end() || adj_list.find(to_id) == adj_list.end()) {
                    return thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else if (edge_index.find(edge_type_id) == edge_index.end()) {
                    return thesoup::types::Result<thesoup::types::Unit, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::INVALID_EDGE_TYPE);
                } else {
                    auto it {adj_list.at(from_id).outgoing_edges.find({edge_type_id, to_id})};
                    auto it_back {adj_list.at(to_id).incoming_edges.find({edge_type_id, from_id})};
                    if (it == adj_list.at(from_id).outgoing_edges.end() && it_back == adj_list.at(to_id).incoming_edges.end()) {
                        return thesoup::types::Result < thesoup::types::Unit,
                                IndexedPropertyDiGraphAttributes::ErrorCode > ::failure(
                                        IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_EDGE);
                    } else if (it_back == adj_list.at(to_id).incoming_edges.end() || it == adj_list.at(to_id).outgoing_edges.end()) {
                        // TODO: See if we can return more meaningful error
                        return thesoup::types::Result < thesoup::types::Unit,
                                IndexedPropertyDiGraphAttributes::ErrorCode > ::failure(
                                IndexedPropertyDiGraphAttributes::ErrorCode::BROKEN_EDGE);
                    } else {
                        adj_list.at(from_id).outgoing_edges.erase(it);
                        adj_list.at(to_id).incoming_edges.erase(it_back);
                        return thesoup::types::Result < thesoup::types::Unit,
                                IndexedPropertyDiGraphAttributes::ErrorCode > ::success(
                                        thesoup::types::Unit::unit);
                    }
                }
            }


            /**
             * \brief Hydrate an edge type
             *
             * This method accepts an edge type ID and returns the corresponding actual edge type. If the edge type is
             * absent, an error specifying that is returned.
             *
             * \param edge_type_id
             * \return Result<edge_type, ErrorCode>
             */
            thesoup::types::Result<const E_TYPE, IndexedPropertyDiGraphAttributes::ErrorCode>
            hydrate_edge_type(const std::size_t& edge_type_id) const noexcept {
                if (edge_index.find(edge_type_id) == edge_index.end()) {
                    return thesoup::types::Result<const E_TYPE, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::INVALID_EDGE_TYPE);
                } else {
                    return thesoup::types::Result<const E_TYPE, IndexedPropertyDiGraphAttributes::ErrorCode>::success(
                            edge_index.at(edge_type_id));
                }
            }

            /**
             * \brief Get all neighbours of a vertex
             *
             * This method returns all neighbours of a vertex. The returned value is a list of type Neighbour<edge_type, vertex>.
             * This allows us to keep information about what neighbour is attached to this vertex by what type of edge.
             * This function can be used with const objects of the class.
             *
             * If the vertex is not present, an error code is returned.
             *
             * \param vertex The vertex whose neighbours to return
             * \return Result<List<Neighbour<vertex ID, edge type ID>>
             */
            thesoup::types::Result<std::vector<thesoup::types::Neighbour<std::size_t, std::size_t>>, IndexedPropertyDiGraphAttributes::ErrorCode>
            get_neighbours(const std::size_t &vertex) const noexcept {
                if (vertex_index.find(vertex) == vertex_index.end()) {
                    return thesoup::types::Result<std::vector<thesoup::types::Neighbour<std::size_t, std::size_t>>, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else {
                    std::vector<thesoup::types::Neighbour<std::size_t, std::size_t>> neighbours{};
                    neighbours.reserve(adj_list.at(vertex).outgoing_edges.size());
                    std::copy(adj_list.at(vertex).outgoing_edges.begin(), adj_list.at(vertex).outgoing_edges.end(), std::back_inserter(neighbours));
                    return thesoup::types::Result<std::vector<thesoup::types::Neighbour<std::size_t, std::size_t>>, IndexedPropertyDiGraphAttributes::ErrorCode>::success(
                            neighbours);
                }
            }

            /**
             * \brief Return all neighbour of a vertex, attached by a given edge type
             *
             * This method returns all neighbours of a given vertex, who are attached by a certain edge type only. The
             * returned value is a list of vertex IDs. This function can be used with const objects of the class.
             *
             * If a vertex is absent of the edge type is not registered, an error code is returned.
             *
             * @param vertex
             * @param edge_type
             * @return
             */
            thesoup::types::Result<std::vector<std::size_t>, IndexedPropertyDiGraphAttributes::ErrorCode>
            get_neighbours(const std::size_t &vertex, const std::size_t &edge_type) {
                if (vertex_index.find(vertex) == vertex_index.end()) {
                    return thesoup::types::Result<std::vector<std::size_t>, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else if (edge_index.find(edge_type) == edge_index.end()) {
                    return thesoup::types::Result<std::vector<std::size_t>, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::INVALID_EDGE_TYPE);
                } else {
                    std::vector<std::size_t> neighbours;
                    // TODO: Move to c++20 filter when available
                    for (const auto &neighbour: adj_list.at(vertex).outgoing_edges) {
                        if (neighbour.edge == edge_type) {
                            neighbours.push_back(neighbour.vertex);
                        }
                    }
                    return thesoup::types::Result<std::vector<std::size_t>, IndexedPropertyDiGraphAttributes::ErrorCode>::success(
                            neighbours);
                }
            }

            /**
             * \brief Get all incoming neighbours of a vertex
             *
             * This method returns all incoming edges to a vertex. The returned value is a list of type Neighbour<edge_type, vertex>.
             * Just like the `get_neighbours` method, this allows us to keep information about what neighbour is attached
             * to this vertex by what type of edge, but in the incoming direction. This is helpful for querying by object
             * in a property graph. This function can be used with const objects of the class.
             *
             * If the vertex is not present, an error code is returned.
             *
             * \param vertex The vertex whose neighbours to return
             * \return Result<List<Neighbour<vertex ID, edge type ID>>
             */
            thesoup::types::Result<std::vector<thesoup::types::Neighbour<std::size_t, std::size_t>>, IndexedPropertyDiGraphAttributes::ErrorCode>
            get_incoming_edges(const std::size_t &vertex) const noexcept {
                if (vertex_index.find(vertex) == vertex_index.end()) {
                    return thesoup::types::Result<std::vector<thesoup::types::Neighbour<std::size_t, std::size_t>>, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else {
                    std::vector<thesoup::types::Neighbour<std::size_t, std::size_t>> neighbours{};
                    neighbours.reserve(adj_list.at(vertex).incoming_edges.size());
                    std::copy(adj_list.at(vertex).incoming_edges.begin(), adj_list.at(vertex).incoming_edges.end(), std::back_inserter(neighbours));
                    return thesoup::types::Result<std::vector<thesoup::types::Neighbour<std::size_t, std::size_t>>, IndexedPropertyDiGraphAttributes::ErrorCode>::success(
                            neighbours);
                }
            }

            /**
             * \brief Return all incoming neighbour of a vertex, attached by a given edge type
             *
             * This method returns all incoming neighbours of a given vertex, who are attached by a certain edge type only. The
             * returned value is a list of vertex IDs. This function can be used with const objects of the class.
             *
             * If a vertex is absent of the edge type is not registered, an error code is returned.
             *
             * @param vertex
             * @param edge_type
             * @return
             */
            thesoup::types::Result<std::vector<std::size_t>, IndexedPropertyDiGraphAttributes::ErrorCode>
            get_incoming_edges(const std::size_t &vertex, const std::size_t &edge_type) {
                if (vertex_index.find(vertex) == vertex_index.end()) {
                    return thesoup::types::Result<std::vector<std::size_t>, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX);
                } else if (edge_index.find(edge_type) == edge_index.end()) {
                    return thesoup::types::Result<std::vector<std::size_t>, IndexedPropertyDiGraphAttributes::ErrorCode>::failure(
                            IndexedPropertyDiGraphAttributes::ErrorCode::INVALID_EDGE_TYPE);
                } else {
                    std::vector<std::size_t> neighbours;
                    // TODO: Move to c++20 filter when available
                    for (const auto &neighbour: adj_list.at(vertex).incoming_edges) {
                        if (neighbour.edge == edge_type) {
                            neighbours.push_back(neighbour.vertex);
                        }
                    }
                    return thesoup::types::Result<std::vector<std::size_t>, IndexedPropertyDiGraphAttributes::ErrorCode>::success(
                            neighbours);
                }
            }
        };

    }

}