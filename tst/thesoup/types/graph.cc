#define CATCH_CONFIG_MAIN

#include <algorithm>

#include <catch2/catch_all.hpp>
#include <thesoup/types/graph.hpp>

using thesoup::types::DirectedGraph;
using thesoup::types::Edge;
using thesoup::types::Neighbour;


SCENARIO("Graph happy case") {

    GIVEN("I have a graph with vertex of type size_t and edges of type EdgeType enum") {

        enum class EdgeType {
            E1,
            E2,
            E3
        };
        DirectedGraph<std::size_t, EdgeType> my_graph {};

        WHEN("I insert some vertices and edges.") {

            my_graph.insert_vertex(1);
            my_graph.insert_vertex(1);
            my_graph.insert_vertex(2);
            my_graph.insert_vertex(3);
            my_graph.insert_vertex(4);

            my_graph.insert_edge({1, EdgeType::E1, 2});
            my_graph.insert_edge({1, EdgeType::E2, 2});
            my_graph.insert_edge({1, EdgeType::E2, 2}); // Redundant edge

            my_graph.insert_edge({1, EdgeType::E1, 3});
            my_graph.insert_edge({1, EdgeType::E2, 3});

            my_graph.insert_edge({2, EdgeType::E1, 3});
            my_graph.insert_edge({2, EdgeType::E2, 3});

            my_graph.insert_edge({3, EdgeType::E1, 4});
            my_graph.insert_edge({3, EdgeType::E2, 4});

            AND_WHEN("I retrieved the edges.") {

                std::vector<Edge<std::size_t, EdgeType>> actual_edges {my_graph.get_edges()};

                THEN("I should get the correct edges back. No duplicates.") {

                    std::vector<Edge<std::size_t, EdgeType>> expected_edges{
                            {1, EdgeType::E1, 2},
                            {1, EdgeType::E2, 2},
                            {1, EdgeType::E1, 3},
                            {1, EdgeType::E2, 3},
                            {2, EdgeType::E1, 3},
                            {2, EdgeType::E2, 3},
                            {3, EdgeType::E1, 4},
                            {3, EdgeType::E2, 4}
                    };

                    REQUIRE(expected_edges.size() == actual_edges.size());
                    std::for_each(
                            expected_edges.begin(),
                            expected_edges.end(),
                            [&](auto item) {
                                REQUIRE(std::find(actual_edges.begin(), actual_edges.end(), item) != actual_edges.end());
                            });
                }
            }

            AND_WHEN("I retrieve the vertices.") {

                std::vector<std::size_t> actual_vertices {my_graph.get_vertices()};

                THEN("I should get beck the correct vertices. No duplicates.") {

                    std::vector<std::size_t> expected_vertices {1,2,3,4};

                    REQUIRE(expected_vertices.size() == actual_vertices.size());
                    std::for_each(
                            expected_vertices.begin(),
                            expected_vertices.end(),
                            [&](auto& item) {
                                REQUIRE(std::find(actual_vertices.begin(), actual_vertices.end(), item) != actual_vertices.end());
                            });
                }
            }
        }
    }

}