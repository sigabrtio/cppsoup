#define CATCH_CONFIG_MAIN

#include <exception>
#include <iterator>
#include <thesoup/types/vector.hpp>
#include <catch2/catch_all.hpp>

using thesoup::types::PartitionedVector;

struct Weird3byteStruct {
    char a;
    char b;
    char c;
};

bool operator==(const Weird3byteStruct& lhs, const Weird3byteStruct& rhs) {
    return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c;
}

SCENARIO("PartitionedVector happy case.") {

    GIVEN("I have a vector.") {

        PartitionedVector<Weird3byteStruct, 4> my_vec {};

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

            AND_WHEN("I construct another vector from this one via a move.") {

                auto other_vec {std::move(my_vec)};

                THEN("The other vector should be properly initialized.") {

                    REQUIRE(Weird3byteStruct{'a','b','c'} == other_vec[0]);
                    REQUIRE(1 == other_vec.size());
                    REQUIRE(4 == other_vec.bytes());

                    AND_THEN("The original vector should have been reset.") {

                        REQUIRE(0 == my_vec.size());
                        REQUIRE(0 == my_vec.bytes());
                    }
                }
            }

            AND_WHEN("I assign another vector from this one via a move.") {

                PartitionedVector<Weird3byteStruct, 4> other_vec {};
                other_vec = std::move(my_vec);

                THEN("The other vector should be properly initialized.") {

                    REQUIRE(Weird3byteStruct{'a','b','c'} == other_vec[0]);
                    REQUIRE(1 == other_vec.size());
                    REQUIRE(4 == other_vec.bytes());

                    AND_THEN("The original vector should have been reset.") {

                        REQUIRE(0 == my_vec.size());
                        REQUIRE(0 == my_vec.bytes());
                    }
                }
            }
        }
    }
}

SCENARIO("PartitionedVector iterations.") {

    GIVEN("I have a vector with some elements in it.") {

        PartitionedVector<Weird3byteStruct, 4> my_vec;
        my_vec.push_back({'a', 'b', 'c'});
        my_vec.push_back({'a', 'b', 'd'});
        my_vec.push_back({'a', 'b', 'e'});
        my_vec.push_back({'a', 'b', 'f'});
        my_vec.push_back({'a', 'b', 'g'});

        WHEN("I iterate through it.") {

            auto it {my_vec.begin()};

            THEN("The iterator operations should work as expected.") {

                REQUIRE(Weird3byteStruct{'a', 'b', 'c'} == *it);
                it++;
                REQUIRE(Weird3byteStruct{'a', 'b', 'd'} == *it);
                it++;
                REQUIRE(Weird3byteStruct{'a', 'b', 'e'} == *it);
                it++;
                REQUIRE(Weird3byteStruct{'a', 'b', 'f'} == *it);
                it--;
                REQUIRE(Weird3byteStruct{'a', 'b', 'e'} == *it);
                it += 2;
                REQUIRE(Weird3byteStruct{'a', 'b', 'g'} == *it);
                it -= 2;
                REQUIRE(Weird3byteStruct{'a', 'b', 'e'} == *it);

                std::size_t acc {};
                for ([[maybe_unused]] auto& _ : my_vec) {
                    acc++;
                }
                REQUIRE(my_vec.size() == acc);
            }

            THEN("The distance functions should work.") {

                REQUIRE(static_cast<long>(my_vec.size()) == std::distance(my_vec.end(), my_vec.begin()));
            }

        }
    }
}

SCENARIO("Partitions test.") {

    GIVEN("I have a vector of type int.") {

        PartitionedVector<int, sizeof (int) * 4> my_vec {};

        WHEN("I push back a number of items into it.") {

            my_vec.push_back(1);
            my_vec.push_back(2);
            my_vec.push_back(3);
            my_vec.push_back(4);
            my_vec.push_back(5);
            my_vec.push_back(6);
            my_vec.push_back(7);

            AND_WHEN("I query the number of partitions.") {

                THEN("It should be correct (2 in this case).") {

                    REQUIRE(2 == my_vec.num_partitions());
                }
            }

            AND_WHEN("I get the first partition.") {

                THEN("It should be as expected in terms of size and contents.") {

                    auto partition {my_vec.get_partition(0)};
                    REQUIRE(4 == partition.size);
                    REQUIRE(1 == partition[0]);
                    REQUIRE(2 == partition[1]);
                    REQUIRE(3 == partition[2]);
                    REQUIRE(4 == partition[3]);
                }
            }

            AND_WHEN("I get the second partition.") {

                THEN("It should be as expected in terms of size and contents.") {

                    auto partition {my_vec.get_partition(1)};
                    REQUIRE(3 == partition.size);
                    REQUIRE(5 == partition[0]);
                    REQUIRE(6 == partition[1]);
                    REQUIRE(7 == partition[2]);
                }
            }

            AND_WHEN("I get the third partition.") {

                THEN("It should throw an error.") {
                    
                    REQUIRE_THROWS_AS(my_vec.get_partition(2), std::out_of_range);
                }
            }
        }
    }
}
