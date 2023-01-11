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

        template <typename VID_TYPE, typename EID_TYPE> struct Neighbour {
            EID_TYPE edge;
            VID_TYPE vertex;
        };

        template <typename VID_TYPE, typename EID_TYPE> struct Edge {
            VID_TYPE from;
            EID_TYPE edge_type;
            VID_TYPE to;
        };

        template <typename V_TYPE,typename E_TYPE, typename E, typename=void> struct IsEdgeOfType {
            static constexpr bool value {false};
        };

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

        template<class Child, typename V_TYPE, typename E_TYPE, typename ERR, typename VID_TYPE=V_TYPE, typename EID_TYPE=E_TYPE>
        class Graph {
        public:
            thesoup::types::Result<std::vector<Neighbour<VID_TYPE, EID_TYPE>>, ERR> get_neighbours(const VID_TYPE& vertex) const noexcept {
                return static_cast<const Child*>(this)->get_neighbours(vertex);
            }

            thesoup::types::Result<std::vector<VID_TYPE>, ERR> get_neighbours(const VID_TYPE& vertex, const EID_TYPE& edge_type) const noexcept{
                return static_cast<const Child*>(this)->get_neighbours(vertex, edge_type);
            }

            thesoup::types::Result<VID_TYPE, ERR> insert_vertex(const V_TYPE& vertex) noexcept {
                return static_cast<Child*>(this)->insert_vertex(vertex);
            }

            thesoup::types::Result<thesoup::types::Unit, ERR> insert_edge(const Edge<VID_TYPE, EID_TYPE>& edge) noexcept {
                return static_cast<Child*>(this)->insert_edge(edge);
            }

            thesoup::types::Result<thesoup::types::Unit, ERR> delete_vertex(const VID_TYPE& vertex) noexcept {
                return static_cast<Child*>(this)->delete_vertex(vertex);
            }

            thesoup::types::Result<thesoup::types::Unit, ERR> delete_edge(const Edge<VID_TYPE, EID_TYPE>& edge) noexcept {
                return static_cast<Child*>(this)->delete_edge(edge);
            }
        };

    }
}


namespace std {
    template <typename VID_TYPE, typename EID_TYPE> struct hash<thesoup::types::Neighbour<VID_TYPE, EID_TYPE>> {
        std::size_t operator()(const thesoup::types::Neighbour<VID_TYPE, EID_TYPE>& n) const noexcept {
            auto hash1 {hash<VID_TYPE>()(n.vertex)};
            auto hash2 {hash<EID_TYPE>()(n.edge)};
            return hash1 ^ (hash2 + (hash1<<6) + (hash1>>2) + 0x9e3779b9);
        }
    };
}
