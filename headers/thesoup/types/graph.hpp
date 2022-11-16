#pragma once

#include <algorithm>
#include <functional>
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

        enum class ErrorCode {
            NON_EXISTENT_EDGE,
            NON_EXISTENT_VERTEX
        };

        template <typename V_TYPE, typename E_TYPE> struct Edge {
            V_TYPE from;
            E_TYPE edge;
            V_TYPE to;
        };

        template <typename V_TYPE, typename E_TYPE> struct Neighbour {
            V_TYPE vertex;
            E_TYPE edge;
        };

        template<typename V_TYPE, typename E_TYPE>
        bool operator==(const Neighbour<V_TYPE, E_TYPE>& left, const Neighbour<V_TYPE, E_TYPE>& right) {
            return left.edge == right.edge && left.vertex == right.vertex;
        }

        template<typename V_TYPE, typename E_TYPE>
        bool operator==(const Edge<V_TYPE, E_TYPE>& left, const Edge<V_TYPE, E_TYPE>& right) {
            return left.edge == right.edge && left.from == right.from && left.to == right.to;
        }

        template <typename V_TYPE, typename E_TYPE, class Child> class Graph {
        public:
            const thesoup::types::Result<std::reference_wrapper<std::unordered_set<V_TYPE>>, ErrorCode>
            get_neighbours(const V_TYPE& vertex, const E_TYPE& edge_type) const noexcept {
                return static_cast<Child*>(this)->get_neighbours(vertex, edge_type);
            }

            const thesoup::types::Result<std::reference_wrapper<std::unordered_set<Neighbour<V_TYPE, E_TYPE>>>, ErrorCode>
            get_neighbours(const V_TYPE& vertex) const noexcept {
                return static_cast<Child*>(this)->get_neighbours(vertex);
            }

            void insert_vertex(const V_TYPE& vertex) noexcept {
                return static_cast<Child*>(this)->insert_vertex(vertex);
            }

            thesoup::types::Result<thesoup::types::Unit, ErrorCode> insert_edge(const Edge<V_TYPE, E_TYPE>& edge) noexcept {
                return static_cast<Child*>(this)->insert_edge(edge);
            }

            std::vector<Edge<V_TYPE, E_TYPE>> get_edges() noexcept {
                return static_cast<Child*>(this)->get_edges();
            }

            std::vector<V_TYPE> get_vertices() {
                return static_cast<Child*>(this)->get_vertices();
            }
        };
    }
}

namespace std {
    template <typename V_TYPE, typename E_TYPE> struct hash<thesoup::types::Neighbour<V_TYPE, E_TYPE>> {
        std::size_t operator()(const thesoup::types::Neighbour<V_TYPE, E_TYPE>& n) const noexcept {
            auto hash1 {hash<V_TYPE>()(n.vertex)};
            auto hash2 {hash<E_TYPE>()(n.edge)};
            return hash1 ^ (hash2 + (hash1<<6) + (hash1>>2) + 0x9e3779b9);
        }
    };
}