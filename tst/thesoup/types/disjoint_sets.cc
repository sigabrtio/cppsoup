#define CATCH_CONFIG_MAIN

#include <algorithm>
#include <unordered_set>

#include <catch2/catch_all.hpp>
#include <thesoup/types/disjoint_sets.hpp>

using thesoup::types::DisjointSets;

SCENARIO("Disjoint sets test happy cases.") {

    GIVEN("I have a disjoint sets of characters.") {

        std::unordered_set<char> elems {'a', 'b', 'c', 'd'};
        DisjointSets<char> ds {elems.begin(), elems.end()};

        WHEN(" I query the size.") {

            std::size_t sz {ds.size()};

            THEN("The initial size should equal the size of the input set.") {

                REQUIRE(elems.size() == sz);
            }
        }

        WHEN("I query the leader sets.") {

            THEN("Initially all leader set values should bethe elements themselves.") {

                std::for_each(
                       elems.begin(),
                       elems.end(),
                       [&](const char& item) {
                           REQUIRE(item == ds.get_set_leader(item).unwrap());
                       });

                AND_WHEN("I merge some sets.") {

                    ds.merge_sets('a', 'b');
                    ds.merge_sets('c', 'd');

                    THEN("The size should have reduced accordingly.") {

                        REQUIRE(2== ds.size());

                        AND_THEN("The set leaders should have changed.") {

                            REQUIRE('a' == ds.get_set_leader('a').unwrap());
                            REQUIRE('a' == ds.get_set_leader('b').unwrap());
                            REQUIRE('c' == ds.get_set_leader('c').unwrap());
                            REQUIRE('c' == ds.get_set_leader('d').unwrap());
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Disjoint sets error cases.") {

    GIVEN("I have a disjoint sets of characters.") {

        std::unordered_set<char> elems{'a', 'b', 'c', 'd'};
        DisjointSets<char> ds{elems.begin(), elems.end()};

        WHEN("I query set leader of non-existent element.") {

            auto res {ds.get_set_leader('x')};

            THEN("I should get an error.") {

                REQUIRE(DisjointSets<char>::ErrorCode::ELEMENT_DOES_NOT_EXIST == res.error());
            }
        }

        WHEN("I try to merge items which do not exist.") {

            auto res1 {ds.merge_sets('a', 'x')};
            auto res2 {ds.merge_sets('x', 'a')};
            auto res3 {ds.merge_sets('x', 'y')};

            THEN("I should get back errors.") {

                REQUIRE(DisjointSets<char>::ErrorCode::ELEMENT_DOES_NOT_EXIST == res1.error());
                REQUIRE(DisjointSets<char>::ErrorCode::ELEMENT_DOES_NOT_EXIST == res2.error());
                REQUIRE(DisjointSets<char>::ErrorCode::ELEMENT_DOES_NOT_EXIST == res3.error());
            }
        }
    }
}