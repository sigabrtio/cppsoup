#pragma once

#include <algorithm>
#include <iterator>
#include <cmath>
#include <unordered_set>

#include <thesoup/types/graph.hpp>
#include <thesoup/types/types.hpp>
#include <thesoup/types/disjoint_sets.hpp>

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
         * \brief Kruskal's algorithm to compute the minimum cost spanning tree.
         *
         * This function helps to construct a Minimum Cost Spanning Tree of a graph. The input is iterators to the
         * collection of edges (`thesoup::types::Edge`) that we are supposed to inspect.
         *
         * We are not taking a graph as input as the interface does not have a method to list edges (and it should not,
         * as it might be a very expensive operation).
         *
         * See the example below to learn the usage:
         * ```
         * std::vector<Edge<char, float>> edges {
         *       {'A', 10.0f, 'B'},
         *       {'B', 2.0f, 'E'},
         *       {'B', 1.0f, 'C'},
         *       {'C', 3.0f, 'D'},
         *       {'C', 1.0f, 'F'},
         *       {'E', 4.0f, 'D'},
         *       {'A', 2.0f, 'F'}
         *   };
         * std::vector<Edge<char, float>> reduced_edges {kruskal(edges.begin(), edges.end())};
         * ```
         *
         * \tparam IterType
         * \param begin
         * \param end
         * \return vector<Edge>
         */
        template <typename IterType>
        auto kruskal(const IterType& begin, const IterType& end) {
            using ValueType = typename std::iterator_traits<IterType>::value_type;
            static_assert(thesoup::types::IsTemplateSpecialization<ValueType, thesoup::types::Edge>::value);


            using V_TYPE = decltype(ValueType::from);
            using E_TYPE = decltype(ValueType::edge_type);
            static_assert(thesoup::types::IsEdgeOfType<V_TYPE, E_TYPE, ValueType>::value);


            // Copy over the edges first
            std::vector<thesoup::types::Edge<V_TYPE, E_TYPE>> edges {};
            std::unordered_set<V_TYPE> vertices {};
            edges.reserve(std::abs(std::distance(begin, end)));
            std::copy(begin, end, std::back_inserter(edges));
            std::for_each(
                    edges.begin(),
                    edges.end(),
                    [&](const thesoup::types::Edge<V_TYPE, E_TYPE>& edge) {
                        vertices.insert(edge.from);
                        vertices.insert(edge.to);
                    }
                    );

            // Sort them by weight
            std::sort(
                    edges.begin(),
                    edges.end(),
                    [&](const ValueType& left, const ValueType& right) {
                        return left.edge_type < right.edge_type;
                    });

            //Initialize the disjoint sets
            thesoup::types::DisjointSets<V_TYPE> ds {vertices.begin(), vertices.end()};
            std::vector<thesoup::types::Edge<V_TYPE, E_TYPE>> retval;
            for (const auto& edge: edges) {
                if (ds.get_set_leader(edge.from).unwrap() != ds.get_set_leader(edge.to).unwrap()) {
                    retval.push_back(edge);
                    ds.merge_sets(edge.from, edge.to);
                }
            }
            return retval;
        }
    }
}