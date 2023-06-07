#pragma once

#include <algorithm>
#include <functional>
#include <future>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <iostream>

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

        /**
         * \brief Struct representing the neighbour of a vertex
         *
         * This struct represents the neighbour of a vertex. A vertex in a graph will have typically a list of these. This
         * has 2 elements: The neighbouring vertex ID and an edge type ID.
         *
         * Note that the vertex "ID" and the edge type "ID" do not have to be of the same type as the vertex and edge type
         * respectively. That will depend on the implementation. For example, this project has 2 graph implementations:
         *   - thesoup::types::IndexedPropertyDiGraph and
         *   - thesoup::types::SimpleWeightedGraph
         *
         * In the first, vertex and vertex_id are different. The vertex ID is std::size_t always, and the vertex type is
         * templated. In the second use, the vertex ID and vertex is the same type (something that represents weight).
         *
         * \tparam VID_TYPE The datatype for the vertex ID
         * \tparam EID_TYPE The datatype for the edge ID
         */
        template <typename VID_TYPE, typename EID_TYPE> struct Neighbour {
            EID_TYPE edge;
            VID_TYPE vertex;
        };

        /**
         * \brief A type representing an edge
         *
         * This struct represents a directed edge. It has three fields {from, edge_type, to}. The edge_type does not
         * necessarily have to be a "type", and can be a literal. It is named like that because of original use with
         * property graph in mind (see thesoup::types::IndexedPropertyDiGraph). However nothing is stopping it from being
         * used in a weighted graph (see usage in thesoup::types::SimpleWeightedGraph).
         *
         * Note that the vertex "ID" and the edge type "ID" do not have to be of the same type as the vertex and edge type
         * respectively. That will depend on the implementation. For example, this project has 2 graph implementations:
         *   - thesoup::types::IndexedPropertyDiGraph and
         *   - thesoup::types::SimpleWeightedGraph
         *
         * In the first, vertex and vertex_id are different. The vertex ID is std::size_t always, and the vertex type is
         * templated. In the second use, the vertex ID and vertex is the same type (something that represents weight).
         *
         * @tparam VID_TYPE The datatype for the vertex ID
         * @tparam EID_TYPE The datatype for the edge ID
         */
        template <typename VID_TYPE, typename EID_TYPE> struct Edge {
            VID_TYPE from;
            EID_TYPE edge_type;
            VID_TYPE to;
        };

        /**
         * \brief Static assert helper to determine if a type is an edge of type V_TYPE, E_TYPE
         *
         * This struct helps us determine at compile time whether something is an Edge of type V_TYPE, E_TYPE. Use it
         * as below.
         *
         * ```
         * using EE = thesoup::types::Edge<char, float>;
         * thesoup::types::IsEdgeOfType<char, float, EE>::value; // true
         * thesoup::types::IsEdgeOfType<char, char, EE>::value; // false
         * ```
         *
         * \tparam V_TYPE
         * \tparam E_TYPE
         * \tparam E
         */
        template <typename V_TYPE,typename E_TYPE, typename E, typename=void> struct IsEdgeOfType {
            static constexpr bool value {false};
        };

        //!\cond NO_DOC
        template <typename V_TYPE, typename E_TYPE, typename E>
        struct IsEdgeOfType <
                V_TYPE,
                E_TYPE,
                E,
                typename std::enable_if<
                        std::is_same<typename thesoup::types::Edge<V_TYPE, E_TYPE>, E>::value
                >::type
        > {
            static constexpr bool value {true};
        };
        //!\endcond

        /**
         * \brief An interface for a graph class
         *
         * This represents a CRTP interface for a graph class. This provides a few basic interfaces common to most graphs:
         *   - Query the neighbours of a vertex.
         *   - Query the neighbours of a vertex, filtered by some edge type.
         *   - Insert and delete vertices and edges.
         *
         * This interface allows for separation of actual vertices and edges and their IDs. What that means is that some
         * implementations can use complicated structs as vertices and edge types, and use some integral IDs to deal
         * with queries and traversals. That makes it much more efficient. For example, one might use the
         * thesoup::types::IndexedPropertyDiGraph class with RDF nodes as vertices and edges. In that case, it is best
         * to assign a hash like ID to nodes and predicates. Makes it much more efficient.
         *
         * On the other hand, simple weighted graphs need none of that as the vertices and edges are usually primitives.
         * In that case V_TYPE and VID_TYPE can be same, and so on and so forth.
         *
         * \tparam Child The implementation for this CRTP interface
         * \tparam V_TYPE Vertex type
         * \tparam E_TYPE Edge type
         * \tparam ERR Error type. Every implementation in this library implements some error code enum
         * \tparam VID_TYPE Vertex ID type
         * \tparam EID_TYPE Edge ID type
         */
        template<class Child, typename V_TYPE, typename E_TYPE, typename ERR, typename VID_TYPE=V_TYPE, typename EID_TYPE=E_TYPE>
        class Graph {
        public:
            virtual /**
             * \brief Method that returns all neighbours of a vertex
             *
             * This method returns all neighbours of a given vertex, regardless of the type of edge they are connected
             * with. The return value ia a `thesoup::types::Result` object.
             *
             * @param vertex
             * @return A `Result` object containing a `std::vector` of all thee connected vertices.
             */
            std::future<thesoup::types::Result<std::vector<Neighbour<VID_TYPE, EID_TYPE>>, ERR>>
            get_neighbours(const VID_TYPE& vertex) const noexcept {
                return static_cast<const Child*>(this)->get_neighbours(vertex);
            }

            virtual /**
             * \brief Return all neighbours that are connected to a given vertex via a certain edge type.
             *
             * This method returns a `std::vector` of all the vertices connected to a given vertex via a certain edge type.
             * The return value ia a `thesoup::types::Result` object.
             *
             * @param vertex
             * @param edge_type
             * @return
             */
            std::future<thesoup::types::Result<std::vector<VID_TYPE>, ERR>>
            get_neighbours(const VID_TYPE& vertex, const EID_TYPE& edge_type) const noexcept {
                return static_cast<const Child*>(this)->get_neighbours(vertex, edge_type);
            }

            virtual /**
             * \brief Insert a vertex.
             *
             * This method inserts a new vertex in the graph. The return value ia a `thesoup::types::Result` object.
             * @param vertex
             * @return
             */
            std::future<thesoup::types::Result<VID_TYPE, ERR>>
            insert_vertex(const V_TYPE& vertex) noexcept {
                return static_cast<Child*>(this)->insert_vertex(vertex);
            }

            virtual /**
             * \brief Insert a new edge between 2 vertices.
             *
             * This method inserts an edge between 2 existing vertices. The return value ia a `thesoup::types::Result`
             * object. Of course this fails when 1 or both of the vertices do not exist.
             *
             * @param edge
             * @return
             */
            std::future<thesoup::types::Result<thesoup::types::Unit, ERR>>
                insert_edge(const Edge<VID_TYPE, EID_TYPE>& edge) noexcept {
                return static_cast<Child*>(this)->insert_edge(edge);
            }

            virtual /**
             * \brief Delete a given vertex.
             *
             * This method helps delete a given vertex. The return value ia a `thesoup::types::Result` object. The
             * implementation is free to make this an idempotent operation or something that causes an error on invalid
             * deletes
             *
             * @param vertex The vertex to delete.
             * @return Nothing
             */
            std::future<thesoup::types::Result<thesoup::types::Unit, ERR>>
            delete_vertex(const VID_TYPE& vertex) noexcept {
                return static_cast<Child*>(this)->delete_vertex(vertex);
            }

            virtual /**
             * \brief Delete an edge.
             *
             * This method deletes an edge. The return value ia a `thesoup::types::Result` object. The implementation is
             * free to make this an idempotent operation or just throw an error when things are invalid.
             *
             * @param edge
             * @return
             */
            std::future<thesoup::types::Result<thesoup::types::Unit, ERR>>
            delete_edge(const Edge<VID_TYPE, EID_TYPE>& edge) noexcept {
                return static_cast<Child*>(this)->delete_edge(edge);
            }
        };

    }
}

//!\cond NO_DOC
namespace std {
    template <typename VID_TYPE, typename EID_TYPE> struct hash<thesoup::types::Neighbour<VID_TYPE, EID_TYPE>> {
        std::size_t operator()(const thesoup::types::Neighbour<VID_TYPE, EID_TYPE>& n) const noexcept {
            auto hash1 {hash<VID_TYPE>()(n.vertex)};
            auto hash2 {hash<EID_TYPE>()(n.edge)};
            return hash1 ^ (hash2 + (hash1<<6) + (hash1>>2) + 0x9e3779b9);
        }
    };
}
//!\endcond
