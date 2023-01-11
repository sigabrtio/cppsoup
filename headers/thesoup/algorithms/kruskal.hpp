#pragma once

#include <algorithm>
#include <iterator>
#include <cmath>
#include <unordered_set>

#include <thesoup/types/graph.hpp>
#include <thesoup/types/types.hpp>
#include <thesoup/types/disjoint_sets.hpp>

namespace thesoup {

    namespace algorithms {
        template <typename IterType>
        auto kruskal(const IterType& begin, const IterType& end) {
            using ValueType = typename std::iterator_traits<IterType>::value_type;
            static_assert(thesoup::types::IsInstance<ValueType, thesoup::types::Edge>::value);


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