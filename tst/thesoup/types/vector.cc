#define CATCH_CONFIG_MAIN

#include <exception>
#include <iterator>
#include <iostream>
#include <unordered_map>

#include <thesoup/types/vector_cache.hpp>
#include <catch2/catch_all.hpp>

using thesoup::types::VectorCache;
using thesoup::types::Slice;
using thesoup::types::Result;
using thesoup::types::PageOffsetBits;
using thesoup::types::PageIndexBits;

struct Weird3byteStruct {
    char a;
    char b;
    char c;
};

bool operator==(const Weird3byteStruct& lhs, const Weird3byteStruct& rhs) {
    return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c;
}

template <typename T> class InMemDB {
private:
    std::unordered_map<std::size_t, Slice<T>> store {};
public:
    void save(const Slice<T>& slice, const std::size_t& page_num) {
        if (store.find(page_num) != store.end()) {
            store[page_num].start = slice.start;
            store[page_num].size = slice.size;
        } else {
            store.try_emplace(page_num, slice.start, slice.size);
        }
    }

    Result<Slice<T>, int> load(const std::size_t& page_num) const {
        if (store.find(page_num) != store.end()) {
            return Result<Slice<T>, int>::success(Slice(store.at(page_num).start, store.at(page_num).size));
        } else {
            return Result<Slice<T>, int>::failure(1);
        }
    }

    std::size_t size() const noexcept {
        return store.size();
    }

    auto begin() noexcept {
        return store.begin();
    }
};

SCENARIO("VectorCache happy case.") {

    GIVEN("I have a vector with the page size width set to 2, implying that each page contains 4 elements.") {

        InMemDB<Weird3byteStruct> database;
        auto saver = [&](const Slice<Weird3byteStruct>& slice, const std::size_t& page_num) {
            database.save(slice, page_num);
            return Result<bool, int>::success(true);
        };
        auto loader = [&](const std::size_t page_num) {return database.load(page_num);};

        const PageIndexBits::type INDEX_BITS = 2;
        const PageOffsetBits::type OFFSET_BITS = 2;

        VectorCache<Weird3byteStruct, OFFSET_BITS, INDEX_BITS> my_vec(saver, loader);

        WHEN("I insert some items in it.") {

            my_vec.push_back({'a','b','c'});

            THEN("The idem should be index able.") {

                REQUIRE(Weird3byteStruct{'a','b','c'} == my_vec[0]);

                AND_THEN("The size and capacity should match.") {

                    REQUIRE(my_vec.size() == 1);
                    REQUIRE(my_vec.bytes() == 4*sizeof(Weird3byteStruct));
                }
            }

            AND_WHEN("I insert some more items.") {

                my_vec.push_back({'a','b','d'});
                my_vec.push_back({'a','b','e'});
                my_vec.push_back({'a','b','f'});
                my_vec.push_back({'a','b','g'});


                THEN("The items should be indexed correctly.") {

                    REQUIRE(Weird3byteStruct{'a','b','d'} == my_vec[1]);
                    REQUIRE(Weird3byteStruct{'a','b','e'} == my_vec[2]);
                    REQUIRE(Weird3byteStruct{'a','b','f'} == my_vec[3]);
                    REQUIRE(Weird3byteStruct{'a','b','g'} == my_vec[4]);

                    AND_THEN("The size and the capacity should work out. The capacity should be now of holding 8 elements.") {

                        REQUIRE(5 == my_vec.size());
                        REQUIRE(4*sizeof(Weird3byteStruct)*2 == my_vec.bytes());
                    }
                }
            }

            AND_WHEN("I construct another vector from this one via a move.") {
                auto other_vec {std::move(my_vec)};

                THEN("The other vector should be properly initialized.") {

                    REQUIRE(Weird3byteStruct{'a','b','c'} == other_vec[0]);
                    REQUIRE(1 == other_vec.size());
                    REQUIRE((1<<OFFSET_BITS)*sizeof(Weird3byteStruct) == other_vec.bytes());

                    AND_THEN("The original vector should have been reset.") {

                        REQUIRE(0 == my_vec.size());
                        REQUIRE(0 == my_vec.bytes());
                    }
                }
            }

        }
    }
}

SCENARIO("VectorCache iterations.") {

    GIVEN("I have a vector with some elements in it.") {

        InMemDB<Weird3byteStruct> database;
        auto saver = [&](const Slice<Weird3byteStruct>& slice, const std::size_t& page_num) {
            database.save(slice, page_num);
            return Result<bool, int>::success(true);
        };
        auto loader = [&](const std::size_t page_num) {return database.load(page_num);};

        VectorCache<Weird3byteStruct, 4, 2> my_vec{saver, loader};
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
        InMemDB<int> database;
        auto saver = [&](const Slice<int>& slice, const std::size_t& page_num) {
            database.save(slice, page_num);
            return Result<bool, int>::success(true);
        };
        auto loader = [&](const std::size_t page_num) {return database.load(page_num);};

        VectorCache<int, 2, 2> my_vec {saver, loader};

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

SCENARIO("Swap test.") {

    GIVEN("I have an in memory database and a vector cache with a 2-bit offset and a 2-bit index.") {

        InMemDB<int> database;
        auto saver = [&](const Slice<int>& slice, const std::size_t& page_num) {
            database.save(slice, page_num);
            return Result<bool, int>::success(true);
        };
        auto loader = [&](const std::size_t page_num) {return database.load(page_num);};

        constexpr std::size_t offset_bits {2};
        constexpr std::size_t index_bits {3};
        VectorCache<int, offset_bits, index_bits> test_vector {saver, loader};

        WHEN("I fill it to the in-memory capacity.") {

            std::size_t num_pages_allowed {1<<index_bits};
            std::size_t entries_per_page {1<<offset_bits};
            std::size_t total_capacity {num_pages_allowed * entries_per_page};

            for (std::size_t i = 0; i < total_capacity; i++) {
                test_vector.push_back(i);
            }

            THEN("The external database should be empty at this point.") {
                REQUIRE(0 == database.size());

                AND_WHEN("I add another item to the vector.") {

                    test_vector.push_back(100);

                    THEN("By my calculation, the first page should have been swapped out.") {

                        REQUIRE(1 == database.size());
                        const auto& [page_num, items] {*(database.begin())};
                        REQUIRE(0 == page_num);
                        REQUIRE(4 == items.size);
                        for (std::size_t i = 0; i < 4; i++) {
                            REQUIRE(static_cast<int>(i) == items[i]);
                        }
                    }
                }
            }
        }
    }
}
