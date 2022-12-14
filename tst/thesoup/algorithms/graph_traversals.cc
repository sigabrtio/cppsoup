#define CATCH_CONFIG_MAIN

#include <catch2/catch_all.hpp>

#include <thesoup/types/graph.hpp>
#include <thesoup/types/indexedgraph.hpp>
#include <thesoup/algorithms/graph_traversals.hpp>

/*
using thesoup::types::Edge;
using thesoup::types::IndexedPropertyDiGraph;
using thesoup::algorithms::bfs;

enum class EdgeType {
    E1,
    E2,
    E3
};

namespace std {
    template <> struct hash<EdgeType> {
        std::size_t operator()(const EdgeType &e) const noexcept {
            return hash<std::size_t>()(static_cast<std::size_t>(e));
        }
    };
}
SCENARIO("BFS") {

    GIVEN("I have a di-graph") {

        IndexedPropertyDiGraph<int, EdgeType> my_graph;

        AND_GIVEN("I have filled it out with some nodes and edges") {

            std::size_t v1_id {my_graph.insert_vertex(1)};
            std::size_t v2_id {my_graph.insert_vertex(2)};
            std::size_t v3_id {my_graph.insert_vertex(3)};
            std::size_t v4_id {my_graph.insert_vertex(4)};

            std::size_t e1_type_id {my_graph.register_edge(EdgeType::E1)};
            std::size_t e2_type_id {my_graph.register_edge(EdgeType::E2)};
            std::size_t e3_type_id {my_graph.register_edge_type(EdgeType::E3)};

            my_graph.insert_edge({v1_id, e1_type_id, v2_id});

            my_graph.insert_edge({1, EdgeType::E2, 2});

            my_graph.insert_edge({1, EdgeType::E1, 3});
            my_graph.insert_edge({1, EdgeType::E2, 3});

            my_graph.insert_edge({2, EdgeType::E1, 3});
            my_graph.insert_edge({2, EdgeType::E2, 3});

            my_graph.insert_edge({3, EdgeType::E1, 4});
            my_graph.insert_edge({3, EdgeType::E2, 4});

            WHEN("I do a BFS on it.") {

                std::set<int> visited;
                auto visit_callback = [&](const int& _, const int& v) {
                    (void)_;
                    visited.insert(v);
                };

                thesoup::types::Graph<int, EdgeType, DirectedGraph<int, EdgeType>> g = DirectedGraph<int, EdgeType> {};
                bfs<int, EdgeType, DirectedGraph<int, EdgeType>>(g, 1, visit_callback);
            }
        }
    }
}
 */