#define CATCH_CONFIG_MAIN

#include <algorithm>
#include <catch2/catch_all.hpp>

#include <thesoup/types/graph.hpp>
#include <thesoup/types/indexedgraph.hpp>

using thesoup::types::IndexedPropertyDiGraph;
using thesoup::types::IndexedPropertyDiGraphAttributes::ErrorCode;
using thesoup::types::Neighbour;

enum EdgeType {
    NAME,
    DEPT,
    PERFORMANCE,
    IS
};

using Attribute = Neighbour<std::size_t, std::size_t>;

namespace std {
    template <> struct hash<EdgeType> {
        std::size_t operator()(const EdgeType &e) const noexcept {
            return hash<std::size_t>()(static_cast<std::size_t>(e));
        }
    };
}

SCENARIO("Indexed property graph happy case with a office database.") {

    GIVEN("I have a indexed property graph where vertices are string and edges are of type `EdgeType`.") {

        IndexedPropertyDiGraph<std::string, EdgeType> database;

        WHEN("I register the edges, and insert some vertices.") {

            std::size_t name {database.register_edge_type(EdgeType::NAME).unwrap()};
            std::size_t dept {database.register_edge_type(EdgeType::DEPT).unwrap()};
            std::size_t performance {database.register_edge_type(EdgeType::PERFORMANCE).unwrap()};

            std::size_t emp001 {database.insert_vertex(std::string("emp001")).unwrap()};
            std::size_t amartya {database.insert_vertex(std::string("Amartya")).unwrap()};
            std::size_t engineering {database.insert_vertex(std::string("Engineering")).unwrap()};
            std::size_t good_performance {database.insert_vertex(std::string("A+")).unwrap()};

            std::size_t emp002 {database.insert_vertex(std::string("emp002")).unwrap()};
            std::size_t bob {database.insert_vertex(std::string("Bob")).unwrap()};
            std::size_t clown {database.insert_vertex(std::string("Clown")).unwrap()};
            std::size_t ok_performance {database.insert_vertex(std::string("B+")).unwrap()};

            AND_WHEN("I try to register the same edge again.") {

                std::size_t name2 {database.register_edge_type(EdgeType::NAME).unwrap()};

                THEN("It should be a no op.") {

                    REQUIRE(name2 == name);
                }
            }

            AND_WHEN("I try to insert the same vertex again.") {

                std::size_t emp001_2 {database.insert_vertex(std::string("emp001")).unwrap()};

                THEN("It should be a no-op") {

                    REQUIRE(emp001_2 == emp001);
                }
            }

            AND_WHEN("I hydrate some vertices.") {

                std::string emp001_str {database.hydrate_vertex(emp001).unwrap()};

                THEN("I should get back the correct vertex.") {

                    REQUIRE(emp001_str.compare("emp001") == 0);
                }
            }

            AND_WHEN("I try to hydrate an edge type.") {

                EdgeType name_edge {database.hydrate_edge_type(name).unwrap()};

                THEN("I should be able to do so.") {

                    REQUIRE(EdgeType::NAME == name_edge);
                }
            }

            AND_WHEN("I connect the vertices in the form of a database.") {

                database.insert_edge({emp001, name, amartya}).unwrap();
                database.insert_edge({emp001, dept, engineering}).unwrap();
                database.insert_edge({emp001, performance, good_performance}).unwrap();

                database.insert_edge({emp002, name, bob}).unwrap();
                database.insert_edge({emp002, dept, clown}).unwrap();
                database.insert_edge({emp002, performance, ok_performance}).unwrap();

                AND_WHEN("I query all neighbours.") {

                    std::vector<Neighbour<std::size_t, std::size_t>> emp001_attributes {database.get_neighbours(emp001).unwrap()};
                    std::vector<Neighbour<std::size_t, std::size_t>> emp002_attributes {database.get_neighbours(emp002).unwrap()};

                    THEN("The correct neighbours should be returned.") {

                        REQUIRE(3 == emp001_attributes.size());
                        REQUIRE(std::find(emp001_attributes.begin(), emp001_attributes.end(), Attribute {name, amartya}) != emp001_attributes.end());
                        REQUIRE(std::find(emp001_attributes.begin(), emp001_attributes.end(), Attribute {dept, engineering}) != emp001_attributes.end());
                        REQUIRE(std::find(emp001_attributes.begin(), emp001_attributes.end(), Attribute {performance, good_performance}) != emp001_attributes.end());

                        REQUIRE(3 == emp002_attributes.size());
                        REQUIRE(std::find(emp001_attributes.begin(), emp002_attributes.end(), Attribute {name, bob}) != emp002_attributes.end());
                        REQUIRE(std::find(emp001_attributes.begin(), emp002_attributes.end(), Attribute {dept, clown}) != emp002_attributes.end());
                        REQUIRE(std::find(emp001_attributes.begin(), emp002_attributes.end(), Attribute {performance, ok_performance}) != emp002_attributes.end());

                    }
                }

                AND_WHEN("I query by a certain neighbour.") {

                    std::vector<std::size_t> emp001_name {database.get_neighbours(emp001, name).unwrap()};
                    std::vector<std::size_t> emp001_dept {database.get_neighbours(emp001, dept).unwrap()};
                    std::vector<std::size_t> emp001_perf {database.get_neighbours(emp001, performance).unwrap()};

                    THEN("I should get the ID of the correct attribute.") {

                        REQUIRE(1 == emp001_name.size());
                        REQUIRE(amartya == emp001_name[0]);

                        REQUIRE(1 == emp001_dept.size());
                        REQUIRE(engineering == emp001_dept[0]);

                        REQUIRE(1 == emp001_perf.size());
                        REQUIRE(good_performance == emp001_perf[0]);
                    }
                }

                AND_WHEN("I try to delete an edge.") {

                    auto res {database.delete_edge({emp001, name, amartya})};
                    std::vector<Attribute> neighbours {database.get_neighbours(emp001, name)};

                    THEN("I should be able to do so.") {

                        REQUIRE(res);
                        REQUIRE(std::find(neighbours.begin(), neighbours.end(), Attribute {name, amartya}) == neighbours.end());
                    }
                }
            }

            AND_WHEN("I insert a solo vertex.") {

                std::size_t solo {database.insert_vertex("solo").unwrap()};

                THEN("I should be successfully able to delete it.") {

                    REQUIRE(database.delete_vertex(solo));

                    AND_WHEN("I try to hydrate it.") {

                        auto res {database.hydrate_vertex(solo)};

                        THEN("I should not be able to do it") {

                            REQUIRE_FALSE(res);
                            REQUIRE(thesoup::types::IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX == res.error());
                        }
                    }

                    AND_WHEN("I try to re-delete it.") {

                        auto res {database.delete_vertex(solo)};

                        THEN("I should not be able to do it") {

                            REQUIRE_FALSE(res);
                            REQUIRE(thesoup::types::IndexedPropertyDiGraphAttributes::ErrorCode::NON_EXISTENT_VERTEX == res.error());
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Indexed property graph failure cases with a office database.") {

    GIVEN("I have a indexed property graph where vertices are string and edges are of type `EdgeType`.") {

        IndexedPropertyDiGraph<std::string, EdgeType> database;

        WHEN("I register the edges.") {

            std::size_t name {database.register_edge_type(EdgeType::NAME).unwrap()};

            AND_WHEN("I try to insert an edge connecting non-existent vertices.") {

                THEN("The operation should fail.") {

                    auto fail{database.insert_edge({0, name, 1})};
                    REQUIRE_FALSE(fail);
                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == fail.error());
                }
            }

            AND_WHEN("I insert 1 vertex and try to insert an edge.") {

                std::size_t emp001 {database.insert_vertex(std::string("emp001")).unwrap()};

                THEN("It should still fail.") {

                    auto fail{database.insert_edge({emp001, name, emp001+1})};
                    REQUIRE_FALSE(fail);
                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == fail.error());
                }

                AND_WHEN("I insert more vertices and an edge and attempt to query by incorrect edge type.") {

                    std::size_t amartya {database.insert_vertex(std::string("Amartya")).unwrap()};
                    database.insert_edge({emp001, name, amartya}).unwrap();

                    THEN("The operation should fail.") {
                        auto fail{database.get_neighbours(emp001, name + 1)};
                        REQUIRE_FALSE(fail);
                        REQUIRE(ErrorCode::INVALID_EDGE_TYPE == fail.error());
                    }
                }
            }
        }

        WHEN("I try to hydrate a non-existent vertex.") {

            auto res {database.hydrate_vertex(0)};

            THEN("I should get an error.") {

                REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == res.error());
            }
        }

        WHEN("I try to hydrate a non existent edge type.") {

            auto res {database.hydrate_edge_type(0)};

            THEN("I should get an error.") {

                REQUIRE(ErrorCode::INVALID_EDGE_TYPE == res.error());
            }
        }

        WHEN("I register an edge type.") {

            std::size_t name {database.register_edge_type(EdgeType::NAME).unwrap()};
            std::size_t emp001 {database.insert_vertex("emp001").unwrap()};


            AND_WHEN("I try to delete an edge connecting non-existent vertices.") {

                auto res1 {database.delete_edge({emp001+1, name, emp001+2})};
                auto res2 {database.delete_edge({emp001, name, emp001+2})};
                auto res3 {database.delete_edge({emp001+1, name, emp001})};

                THEN("I should get an error.") {

                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == res1.error());
                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == res2.error());
                    REQUIRE(ErrorCode::NON_EXISTENT_VERTEX == res3.error());
                }
            }

            AND_WHEN("we insert an actual edge.") {

                std::size_t is {database.register_edge_type(EdgeType::IS).unwrap()};
                std::size_t fat {database.insert_vertex("fat").unwrap()};
                std::size_t amartya {database.insert_vertex("amartya").unwrap()};

                database.insert_edge({emp001, name, amartya});

                AND_WHEN("We try to delete an edge between actual vertices, but unconnected.") {

                    auto res {database.delete_edge({amartya, is, fat})};

                    THEN("I should get an error.") {

                        REQUIRE(ErrorCode::NON_EXISTENT_EDGE == res.error());
                    }
                }

                AND_WHEN("We try to delete a vertex that has an outgoing edge.") {

                    auto res {database.delete_vertex(emp001)};

                    THEN("I should get an error.") {

                        REQUIRE(ErrorCode::CONNECTED_VERTEX == res.error());
                    }
                }

                AND_WHEN("We try to delete a vertex that has an incoming edge.") {

                    auto res {database.delete_vertex(amartya)};

                    THEN("I should get an error.") {

                        REQUIRE(ErrorCode::CONNECTED_VERTEX == res.error());
                    }
                }
            }
        }
    }
}