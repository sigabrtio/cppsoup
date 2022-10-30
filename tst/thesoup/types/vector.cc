#define CATCH_CONFIG_MAIN

#include <thesoup/types/vector.hpp>
#include <catch2/catch_all.hpp>

using thesoup::type::Vector;

struct Weird3byteStruct {
    char a;
    char b;
    char c;
};

bool operator==(const Weird3byteStruct& lhs, const Weird3byteStruct& rhs) {
    return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c;
}

SCENARIO("Vector happy case.") {

    GIVEN("I have a vector.") {

        Vector<Weird3byteStruct, 4> my_vec {};

        WHEN("I insert some items in it.") {

            my_vec.push_back({'a','b','c'});

            THEN("The idem should be index able.") {

                REQUIRE(Weird3byteStruct{'a','b','c'} == my_vec[0]);

                AND_THEN("The size and capacity should match.") {

                    REQUIRE(my_vec.size() == 1);
                    REQUIRE(my_vec.bytes() == 4);
                }
            }

            AND_WHEN("I insert some more items.") {

                my_vec.push_back({'a','b','d'});
                my_vec.push_back({'a','b','e'});

                THEN("The items should be indexed correctly.") {

                    REQUIRE(Weird3byteStruct{'a','b','d'} == my_vec[1]);
                    REQUIRE(Weird3byteStruct{'a','b','e'} == my_vec[2]);

                    AND_THEN("The sized and the capacity should work out.") {

                        REQUIRE(3 == my_vec.size());
                        REQUIRE(12 == my_vec.bytes());
                    }
                }
            }
        }
    }
}
