#define CATCH_CONFIG_MAIN

#include <vector>
#include <catch2/catch_all.hpp>

#include <thesoup/types/graph.hpp>
#include <thesoup/algorithms/kruskal.hpp>

#include <test_utils.hpp>

using thesoup::types::Edge;
using thesoup::algorithms::kruskal;
using thesoup::testutils::similar;

namespace thesoup{

    namespace types {

        template <typename V, typename E> constexpr bool operator==(Edge<V,E> left, const Edge<V,E> right) noexcept {
            return left.from == right.from && left.to == right.to && left.edge_type == right.edge_type;
        }
    }
}

SCENARIO("Kruskal test.") {

    GIVEN("I have a graph's list of edges.") {

        /*
         *
         * A---(10)-----B--(2)----E
         * |           |          |
         * |           | (1)      | (4)
         * (2)         C---(3)----D
         * |           |
         * |           | (1)
         * `-----------F
         */
        std::vector<Edge<char, float>> edges {
                {'A', 10.0f, 'B'},
                {'B', 2.0f, 'E'},
                {'B', 1.0f, 'C'},
                {'C', 3.0f, 'D'},
                {'C', 1.0f, 'F'},
                {'E', 4.0f, 'D'},
                {'A', 2.0f, 'F'}
        };

        WHEN("I run the list of edges through a kruskal's algorithm.") {

            std::vector<Edge<char, float>> reduced_edges {kruskal(edges.begin(), edges.end())};

            THEN("I should get back the edges that indeed sum up to min weight while keeping everything connected.") {

                REQUIRE(5 == reduced_edges.size());

                std::vector<Edge<char, float>> expected_edges {
                        {'B', 1.0f,  'C'},
                        {'C', 1.0f,  'F'},
                        {'A', 2.0, 'F'},
                        {'B', 2.0f, 'E'},
                        {'C', 3.0f, 'D'},
                };

                REQUIRE(similar(expected_edges.begin(), expected_edges.end(), reduced_edges.begin(), reduced_edges.end()));
            }
        }
    }
}