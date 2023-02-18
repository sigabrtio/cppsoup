#define CATCH_CONFIG_MAIN

#include <optional>
#include <unordered_set>

#include <catch2/catch_all.hpp>

#include <thesoup/types/types.hpp>
#include <thesoup/types/graph.hpp>
#include <thesoup/types/simplegraph.hpp>
#include <thesoup/algorithms/graph_traversals.hpp>

using thesoup::types::Unit;
using thesoup::types::Edge;
using thesoup::types::SimpleWeightedGraph;
using thesoup::types::SimpleWeightedGraphAttributes::ErrorCode;
using thesoup::algorithms::bfs;

SCENARIO("BFS") {

    GIVEN("I have a di-graph without caring about weights.") {

        SimpleWeightedGraph<char, Unit> my_graph;

        AND_GIVEN("I have filled it out with some nodes and edges") {
            /*
             *
             * A --------->B--------->E
             * |           |          |
             * |           C--------->D
             * |           |
             * `---------->F
             */

            my_graph.insert_vertex('A');
            my_graph.insert_vertex('B');
            my_graph.insert_vertex('C');
            my_graph.insert_vertex('D');
            my_graph.insert_vertex('E');
            my_graph.insert_vertex('F');

            my_graph.insert_edge({'A', Unit::unit, 'B'});
            my_graph.insert_edge({'A', Unit::unit, 'F'});
            my_graph.insert_edge({'B', Unit::unit, 'E'});
            my_graph.insert_edge({'B', Unit::unit, 'C'});
            my_graph.insert_edge({'C', Unit::unit, 'D'});
            my_graph.insert_edge({'C', Unit::unit, 'F'});
            my_graph.insert_edge({'E', Unit::unit, 'D'});

            WHEN("I do a BFS on it.") {

                std::unordered_set<char> visited;
                std::unordered_set<char> visited2;
                std::function<void(const std::optional<char>&, const char&)> visit_callback = [&](const std::optional<char>& _, const char& v) {
                    (void)_;
                    visited.insert(v);
                };

                std::function<void(const std::optional<char>&, const char&)> visit_callback_2 = [&](const std::optional<char>& _, const char& v) {
                    (void)_;
                    visited2.insert(v);
                };

                bfs(my_graph,'A', visit_callback).get().unwrap();
                bfs(my_graph,'C', visit_callback_2).get().unwrap();

                THEN("The I should have traversed all reachable vertices.") {

                    std::unordered_set<char> expected_vertices {'A', 'B', 'C', 'D', 'E', 'F'};
                    std::unordered_set<char> expected_vertices_2 {'C', 'D', 'F'};

                    REQUIRE(expected_vertices == visited);
                    REQUIRE(expected_vertices_2 == visited2);
                }
            }

            AND_WHEN("I start a BFS with an incorrect node.") {

                std::function<void(const std::optional<char>&, const char&)> visit_callback = [&](const std::optional<char>& _, const char& v) {
                    (void)_;
                    (void) v;
                };
                auto res {bfs(my_graph, 'Z', visit_callback)};

                THEN("I should get the original error back from the underlying graph's get_neighbour method.") {

                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == res.get().error());
                }
            }
        }
    }
}
