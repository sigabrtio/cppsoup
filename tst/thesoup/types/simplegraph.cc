#define CATCH_CONFIG_MAIN

#include <algorithm>

#include <catch2/catch_all.hpp>
#include <thesoup/types/simplegraph.hpp>

using thesoup::types::SimpleWeightedGraph;
using thesoup::types::Neighbour;
using thesoup::types::SimpleWeightedGraphAttributes::ErrorCode;

using WeightedNeighbour = Neighbour<char, float>;

template <typename T>
bool similar(const std::vector<T>& v1, const std::vector<T>& v2) {
    if (v1.size() != v2.size()) {
        return false;
    } else {
        for (const auto& item : v1) {
            if (std::find(v2.begin(), v2.end(), item) == v2.end()) {
                return false;
            }
        }
        return true;
    }
}

SCENARIO("Simple graph happy case") {

    GIVEN("I have a simple weighted graph, where nodes are char and weights are float.") {

        SimpleWeightedGraph<char, float> graph {};

        WHEN("I insert some vertices ad edges.") {
            /*
            *   A -(1)-- B-(1)--D
            *   |        |      |
            *   |       (2)    (2)
            *   |        |      |
            *   |        C-(1)--|
            *   `----(3)--------'
            */

            graph.insert_vertex('A');
            graph.insert_vertex('B');
            graph.insert_vertex('C');
            graph.insert_vertex('D');

            graph.insert_edge({'A', 1.0f, 'B'});
            graph.insert_edge({'B', 1.0f, 'D'});
            graph.insert_edge({'B', 2.0f, 'C'});
            graph.insert_edge({'A', 5.0f, 'D'});
            graph.insert_edge({'C', 3.0f, 'D'});

            AND_WHEN("I query the neighbours") {

                auto n_set_a {graph.get_neighbours('A').unwrap()};
                auto n_set_b {graph.get_neighbours('B').unwrap()};
                auto n_set_c {graph.get_neighbours('C').unwrap()};
                auto n_set_d {graph.get_neighbours('D').unwrap()};

                THEN("I should get back the correct ones.") {

                    REQUIRE(similar(std::vector<Neighbour<char, float>> {WeightedNeighbour {1.0f, 'B'},WeightedNeighbour {5.0f, 'D'}}, n_set_a));
                    REQUIRE(similar(std::vector<Neighbour<char, float>> {WeightedNeighbour {1.0f, 'D'},WeightedNeighbour {2.0f, 'C'}}, n_set_b));
                    REQUIRE(similar(std::vector<Neighbour<char, float>> {WeightedNeighbour {3.0f, 'D'}}, n_set_c));
                    REQUIRE(similar(std::vector<Neighbour<char, float>> {}, n_set_d));
                }
            }

            AND_WHEN("I query the neighbours by weight") {

                auto n_set_a5 {graph.get_neighbours('A', 5).unwrap()};
                auto n_set_a1 {graph.get_neighbours('A', 1).unwrap()};
                auto n_set_b2 {graph.get_neighbours('B', 2).unwrap()};
                auto n_set_b1 {graph.get_neighbours('B', 1).unwrap()};

                THEN("I should get back the correct ones.") {

                    REQUIRE(similar(std::vector<char> {'D'}, n_set_a5));
                    REQUIRE(similar(std::vector<char> {'B'}, n_set_a1));
                    REQUIRE(similar(std::vector<char> {'C'}, n_set_b2));
                    REQUIRE(similar(std::vector<char> {'D'}, n_set_b1));

                }
            }

            AND_WHEN("I delete an edge.") {

                graph.delete_edge({'A', 5, 'D'});

                THEN("That neighbour should be deleted.") {

                    std::vector<Neighbour<char, float>> neighbours {graph.get_neighbours('A')};
                    REQUIRE(1 == neighbours.size());
                    REQUIRE(std::find(neighbours.begin(), neighbours.end(),  Neighbour<char, float> {5.0f, 'D'}) == neighbours.end());
                }
            }
        }

        AND_WHEN("I add a lone vertex and then delete it.") {

            graph.insert_vertex('Z');
            auto res {graph.delete_vertex('Z')};

            THEN("The operation should succeed.") {

                REQUIRE(res);

                AND_THEN("That vertex should truly be gone.") {

                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == graph.get_neighbours('Z').error());
                }
            }
        }
    }
}


SCENARIO("Simple weighted graph failure scenarios.") {

    GIVEN("I have a simple weighted graph, where nodes are char and weights are float.") {

        SimpleWeightedGraph<char, float> graph{};

        WHEN("I insert some vertices ad edges.") {

            /*
             *   A -(1)-- B-(1)--D
             *   |        |      |
             *   |       (2)    (2)
             *   |        |      |
             *   |        C-(1)--|
             *   `----(3)--------'
             */
            graph.insert_vertex('A');
            graph.insert_vertex('B');
            graph.insert_vertex('C');
            graph.insert_vertex('D');

            graph.insert_edge({'A', 1.0f, 'B'});
            graph.insert_edge({'B', 1.0f, 'D'});
            graph.insert_edge({'B', 2.0f, 'C'});
            graph.insert_edge({'A', 5.0f, 'D'});
            graph.insert_edge({'C', 3.0f, 'D'});

            AND_WHEN("I query the neighbours of a non existent vertex.") {

                auto res {graph.get_neighbours('Z')};

                THEN("I should get an error.") {

                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == res.error());
                }
            }

            AND_WHEN("I query neighbours with incorrect weight.") {

                std::vector<char> neighbours {graph.get_neighbours('A', 10.0f).unwrap()};

                THEN("I should get an empty list.") {

                    REQUIRE(0 == neighbours.size());
                }
            }

            AND_WHEN("I delete a vertex with outgoing edges.") {

                auto res {graph.delete_vertex('A')};

                THEN("I should get an error.") {

                    REQUIRE(ErrorCode::CONNECTED_VERTEX == res.error());
                }
            }

            AND_WHEN("I delete a vertex with incoming edges.") {

                auto res {graph.delete_vertex('D')};

                THEN("I should get an error.") {

                    REQUIRE(ErrorCode::CONNECTED_VERTEX == res.error());

                    AND_WHEN("I delete the edges pointing to it.") {

                        graph.delete_edge({'A', 5.0, 'D'});
                        graph.delete_edge({'B', 1.0, 'D'});
                        graph.delete_edge({'C', 3.0, 'D'});

                        THEN("I should be able to delete the vertex.") {

                            REQUIRE(graph.delete_vertex('D'));
                        }
                    }
                }
            }

            AND_WHEN("I delete an edge connecting n on existent vertices") {

                auto res1 {graph.delete_edge({'Z', 100.0f, 'Q'})};
                auto res2 {graph.delete_edge({'A', 100.0f, 'Q'})};
                auto res3 {graph.delete_edge({'Z', 100.0f, 'D'})};

                THEN("I should get an error.") {

                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == res1.error());
                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == res2.error());
                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == res3.error());
                }
            }

            AND_WHEN("I delete an edge connecting actual vertices, but a bad weight.") {

                auto res {graph.delete_edge({'A', 1.0, 'D'})};

                THEN("I should get an error.") {

                    REQUIRE(ErrorCode::NON_EXISTENT_EDGE == res.error());
                }
            }
        }
    }
}
