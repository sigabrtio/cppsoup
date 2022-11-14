#pragma once

#include <algorithm>
#include <exception>
#include <functional>
#include <iterator>
#include <iostream>
#include <vector>

#include <thesoup/types/types.hpp>

/**
 * \namespace thesoup
 *
 * \brief The root namespace.
 * */
namespace thesoup {
    /**
     * \namespace thesoup::types
     *
     * \brief Sub namespace with all numeric classes and functions.
     * */
    namespace types {
        struct PageOffsetBits {
            using type = std::size_t;
        };

        struct PageIndexBits {
            using type = std::size_t;
        };

        /**
         * \class VectorCache
         * \tparam T The success type.
         * \tparam page_offset_bits The page size exponent. The page size will be therefore 2^page_offset_bits, and the number of bytes will be page_size * sizeof(T)
         *
         * \brief A vector implementation with a hybrid of linked-list and contiguous memory array.
         *
         * Usage:
         * ------
         * VectorCache<int, 4> my_vec;
         * my_vec.push_back(1);
         * my_vec.push_back(2);
         * my_vec.push_back(3);
         *
         * for (auto& item : my_vec) {
         *     std::cout << item << "\n";
         * }
         *
         * */
        template <typename T, PageOffsetBits::type page_offset_bits, PageIndexBits::type page_index_bits> class VectorCache {
            static_assert(page_offset_bits + page_index_bits < sizeof (std::size_t), "Sum of page offset width and page index width cannot be greater than size of index.");

        private:
            struct Page {
                thesoup::types::Slice<T> slice {nullptr, 0};
                bool valid {false};
                std::size_t page_number {0};
            };

            constexpr static std::size_t page_size {(1 << page_offset_bits)};
            constexpr static std::size_t num_items_per_page {1 << page_offset_bits};
            constexpr static std::size_t page_offset_bit_mask {num_items_per_page - 1};

            constexpr static std::size_t page_number_bit_mask {~page_offset_bit_mask};

            constexpr static std::size_t page_table_size {1 << page_index_bits};
            constexpr static std::size_t page_table_entry_bit_mask {page_table_size-1};

            const std::function<thesoup::types::Result<bool, int>(const thesoup::types::Slice<T>& page, const std::size_t& page_number)> saver;
            const std::function<thesoup::types::Result<thesoup::types::Slice<T>, int>(const std::size_t& page_number)> loader;

            std::vector<Page> page_table {page_table_size};
            std::size_t _size {};
            std::size_t num_pages {};

        public:
            VectorCache(
                    const std::function<thesoup::types::Result<bool, int>(const thesoup::types::Slice<T>&, const std::size_t& page_num)>& saver,
                    const std::function<thesoup::types::Result<thesoup::types::Slice<T>, int>(const std::size_t&)>& loader
                    ) : saver {saver}, loader {loader} {}
            ~VectorCache() {
                for (auto& page: page_table) {
                    delete page.slice.start;
                }
            }
            VectorCache(VectorCache<T, page_offset_bits, page_index_bits>& other)=delete;
            VectorCache(VectorCache<T, page_offset_bits, page_index_bits>&& other) noexcept : page_table {std::move(other.page_table)}, _size {other._size}, num_pages {other.num_pages} {
                other.page_table.clear();
                other._size = 0;
                other.num_pages = 0;
            }

            VectorCache<T, page_offset_bits, page_index_bits>& operator=(VectorCache<T, page_offset_bits, page_index_bits>& other)=delete;
            VectorCache<T, page_offset_bits, page_index_bits>& operator=(VectorCache<T, page_offset_bits, page_index_bits>&& other)=delete;
            VectorCache<T, page_offset_bits, page_index_bits>& operator*() {return *this;}
            VectorCache<T, page_offset_bits, page_index_bits>* operator->() {return this;}

            void push_back(const T& item) noexcept {
                std::size_t page_number_to_insert_to {(_size & page_number_bit_mask) >> page_offset_bits};
                std::size_t page_offset {_size & page_offset_bit_mask};
                std::size_t page_table_entry {page_number_to_insert_to & page_table_entry_bit_mask};

                // Page swap is needed
                if (page_table[page_table_entry].page_number != page_number_to_insert_to || page_number_to_insert_to >= num_pages) {

                    if (page_table[page_table_entry].valid) {
                        saver(page_table[page_table_entry].slice, page_table[page_table_entry].page_number);
                    }
                    // New page needed
                    if (page_number_to_insert_to >= num_pages) {
                        T* new_page = new T[page_size];
                        page_table[page_table_entry].slice.start = new_page;
                        page_table[page_table_entry].slice.size = 0;
                        num_pages++;
                    } else {
                        // Just a simple swap will suffice
                        thesoup::types::Slice<T> slice {std::move(loader(page_number_to_insert_to).unwrap())};
                        page_table[page_table_entry].slice.start = slice.start;
                        page_table[page_table_entry].slice.size = slice.size;
                        page_table[page_table_entry].page_number = page_number_to_insert_to;
                    }

                    page_table[page_table_entry].valid = true;
                    page_table[page_table_entry].page_number = page_number_to_insert_to;
                }


                page_table[page_table_entry].slice.size++;
                page_table[page_table_entry].slice[page_offset] = item;
                _size++;
            }

            T& operator[](const std::size_t idx) {
                if (idx >= _size) {
                    std::stringstream ss;
                    ss << "Array index out of bounds for VectorCache of size " << _size;
                    throw std::out_of_range(ss.str());
                }
                std::size_t page_number {(idx & page_number_bit_mask) >> page_offset_bits};
                std::size_t page_offset {idx & page_offset_bit_mask};
                std::size_t page_table_entry {page_number & page_table_entry_bit_mask};

                if(page_table[page_table_entry].page_number == page_number && page_table[page_table_entry].valid) {
                    // Ideal case. Cache hit
                    std::cout << "Cache hit for idx " << idx << "\n";

                } else if(page_table[page_table_entry].page_number != page_number && page_table[page_table_entry].valid) {
                    std::cout << "Cache miss for idx " << idx << " Swapping valid page for page number " << page_table[page_table_entry].page_number << "\n";

                    // Some other valid page
                    // TODO: Throw on save failure
                    saver(page_table[page_table_entry].slice, page_table[page_table_entry].page_number);
                    // TODO: Throw on load failure
                    thesoup::types::Slice<T> slice {std::move(loader(page_number).unwrap())};
                    page_table[page_table_entry].slice.start = slice.start;
                    page_table[page_table_entry].slice.size = slice.size;
                    page_table[page_table_entry].page_number = page_number;
                } else {
                    // Invalid page
                    // TODO: Throw on load failure
                    std::cout << "Cache miss fpr idx " << idx << ". Invalid page!\n";
                    thesoup::types::Slice<T> slice {std::move(loader(page_number).unwrap())};
                    page_table[page_table_entry].slice.start = slice.start;
                    page_table[page_table_entry].slice.size = slice.size;
                    page_table[page_table_entry].page_number = page_number;
                    page_table[page_table_entry].valid = true;
                }
                std::cout << page_table[page_table_entry].slice.size << " === " << page_offset << " ---------------------------------- \n";
                return page_table[page_table_entry].slice[page_offset];
            }

            std::size_t size() const noexcept {
                return _size;
            }

            std::size_t bytes() const noexcept {
                return num_pages * page_size * sizeof(T);
            }

            std::size_t num_partitions() const noexcept {
                return num_pages;
            }

            thesoup::types::Slice<T> get_partition(const std::size_t page_number) {
                if (page_number >= num_pages) {
                    throw std::out_of_range("Partition absent.");
                }
                std::size_t page_table_entry {page_number & page_table_entry_bit_mask};

                // Load page if needed
                if (page_table[page_table_entry].page_number != page_number && page_table[page_table_entry].valid) {
                    // TODO: Throw on save failure
                    saver(page_table[page_table_entry].slice, page_table[page_table_entry].page_number);
                    thesoup::types::Slice<T> slice {std::move(loader(page_number).unwrap())};
                    page_table[page_table_entry].slice.start = slice.start;
                    page_table[page_table_entry].slice.size = slice.size;
                    page_table[page_table_entry].page_number = page_number;
                } else if (!page_table[page_table_entry].valid) {
                    thesoup::types::Slice<T> slice {std::move(loader(page_number).unwrap())};
                    page_table[page_table_entry].slice.start = slice.start;
                    page_table[page_table_entry].slice.size = slice.size;
                    page_table[page_table_entry].page_number = page_number;
                }
                return Slice<T> {
                    page_table[page_table_entry].slice.start,
                    page_table[page_table_entry].slice.size
                };
            }

            // TODO: Optimize ++ and -- operators at least
            struct Iterator {
                using iterator_category [[maybe_unused]] = std::random_access_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type [[maybe_unused]] = T;
                using pointer           = T*;
                using reference         = T&;

                VectorCache<T, page_offset_bits, page_index_bits>* enclosing;
                std::size_t idx {};

                Iterator(
                        VectorCache<T, page_offset_bits, page_index_bits>* enclosing,
                        const std::size_t& idx) : enclosing {enclosing}, idx {idx} {}

                reference operator*() noexcept {return (*enclosing)[idx];}
                pointer operator->() noexcept {return &(*enclosing)[idx];}

                Iterator& operator++() noexcept {idx++; return *this;}
                Iterator operator++(int) noexcept { Iterator tmp {enclosing, idx}; idx++; return tmp;}

                Iterator& operator--() noexcept { idx--; return *this;}
                Iterator operator--(int) noexcept { Iterator tmp {enclosing, idx}; idx--; return tmp;}

                Iterator operator+(const std::size_t& val) noexcept {return Iterator {enclosing, idx+val};}
                Iterator operator-(const std::size_t& val) noexcept {return Iterator {enclosing, idx-val};}

                difference_type operator-(const Iterator& other) noexcept {return other.idx - idx;}

                Iterator& operator+=(const std::size_t& val) noexcept {idx+=val; return *this;}
                Iterator& operator-=(const std::size_t& val) noexcept {idx-=val; return *this;}
                bool operator== (const Iterator& other) const noexcept { return enclosing == other.enclosing && idx == other.idx; };
                bool operator!= (const Iterator& other) const noexcept { return enclosing != other.enclosing || idx != other.idx; };
            };

            Iterator begin() noexcept {return Iterator(this, 0);}
            Iterator end() noexcept {return Iterator(this, _size);}
        };
    }
}