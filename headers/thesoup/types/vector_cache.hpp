#pragma once

#include <algorithm>
#include <exception>
#include <functional>
#include <iterator>
#include <string>
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
        //!\cond NO_DOC
        struct PageOffsetBits {
            using type = std::size_t;
        };

        struct PageIndexBits {
            using type = std::size_t;
        };
        //!\endcond

        /**
         * \class VectorCache
         *
         * \brief A cached vector implementation
         *
         * Ever wished you had a data structure that behaved like a vector, but you have the capability to load only a
         * part of it into memory at a time? A sort of memory managed std::vector? Me neither! Here is a data structure
         * that can do that anyway!
         *
         * The VectorCache class has the ability to do just that. This class emulates a 1 way set associative caching mechanism
         * to allow a part of the vector to be saved to memory. Here is an explaination of how that works.
         *
         * Say you want a page size of N, and you want M pages saved in memory.
         * NOTE: N and M can only be powers of 2, for reasons that will become clear later.
         *
         * Consider a 64 bit index into our vector. Since the page size (N) is a power of 2, we will need a K bit number
         * to address anything inside of a page, where 2^K = N.
         *
         * The remaining 64 - K bits will constitute the page number, allowing us to have 2 ^ (64 - K) pages.
         * We use a page table to keep records of which page is saved where in the memory. Now we probably do not want
         * ALL the pages in memory (we want M of them), so a M sized page table is good enough. If you recall, M is also
         * a power of 2. Hence we can index into the page table using R bits where (2^R) = M.
         *
         * Given this, we will use R of the remaining (64 - K) bits from a item's index to figure out where exactly in the
         * page table, we can find the information about the page where our element resides.
         *
         * To recap, when we do `my_arr[idx]`,
         *   - First we split up the idx into 3 parts:
         *     - Offset bits (lowest K bits)
         *     - Index bits (Lowest K+R bits to lowest K bits)
         *     - Tag bits (The highest remaining bits that we do not care about)
         *
         *   - We first use the index bits to query the page table to get in formation about the page with a page number
         *     equal to the `idx` with the lowest K bits masked to 0.
         *   - Note that many different pages can map to the same page table entry. Specifically 2^(64-K-R) pages can map
         *     to the same page table entry. So, if we detect that the wrong page is loaded, we save that page somewhere
         *     and load the correct one (swap).
         *   - If we find that this page was never loaded at all (the page table is entry is invalid), we will just load
         *     the relevant page from somewhere and fix the page table entry.
         *   - Now that we have the page information, we need to go to the memory location where the page is loaded, and
         *     index into that using the page offset bits, to retrieve the final element.
         *
         * Notes:
         *   - How to save and load a page is specified by the user via a `saver` and `loader` function. The saver function,
         *     when called will be handed a memory slice (thesoup::types::Slice), and the actual page number. This function
         *     has the responsibility of storing the data in a way that can be retrieved later. Similarly, the load function
         *     is just provided a page number and must return a slice (thesoup::types::Slice) after loading the data to
         *     memory. Use MYSQL, local file or whatever suits you for offloading pages.
         *   - Keeping the page size and number of pages powers of 2, by defining them as exponents of 2 help us keep the
         *    computations of page number and page offset fast. We just rely on bitwise operations as opposed to expensive
         *    modulo arithmetic.
         *   - DO NOT expect performance of the `[]` operator iterators even remotely close to std::vector. Each operation
         *     requires a LOT of extra computations as compared to a plain std::vector.
         *   - However if you are working locally within a page, and want fast access, this class does provides methods that
         *     with return a slice corresponding to an index. Then use the slice to access elements i  the page.
         *   - Copy constructor, and both copy and move assignment operators have been deleted. Copying incurs a massive
         *     headache as we not only have to copy all the loaded slices, but also whatever the hell is offloaded via
         *     the `save` method. Assignment operators have been deleted for this reason + the fact that even in case of
         *     move assignment, we will have to clean up the original object.
         *
         * Usage:
         * ```
         * VectorCache<int, 4> my_vec;
         * my_vec.push_back(1);
         * my_vec.push_back(2);
         * my_vec.push_back(3);
         *
         * for (auto& item : my_vec) {
         *     std::cout << item << "\n";
         * }
         *```
         * \tparam T The success type.
         * \tparam page_offset_bits The page size exponent. The page size will be therefore 2^page_offset_bits,
         *                           and the number of bytes a page can hold will be page_size * sizeof(T)
         * \tparam page_index_bits The page table index exponent. The page table size will be therefore 2^page_index_bits,
         *                           implying that the total in-memory capacity of the vector cache class will be
         *                           page table size * page size * sizeof (T)
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

            constexpr static std::size_t page_size {1 << page_offset_bits};
            constexpr static std::size_t page_offset_bit_mask {page_size - 1};

            constexpr static std::size_t page_number_bit_mask {~page_offset_bit_mask};

            constexpr static std::size_t page_table_size {1 << page_index_bits};
            constexpr static std::size_t page_table_entry_bit_mask {page_table_size-1};

            const std::function<thesoup::types::Result<bool, int>(const thesoup::types::Slice<T>& page, const std::size_t& page_number)> saver;
            const std::function<thesoup::types::Result<thesoup::types::Slice<T>, int>(const std::size_t& page_number)> loader;

            std::vector<Page> page_table {page_table_size};
            std::size_t _size {};
            std::size_t num_pages {};

        public:
            /**
             * \brief  Constructor
             * \param saver The saver function as described above, is responsible for saving pages during swaps / object destruction.
             * \param loader The loader function, as described above, is responsible for loading a page from persistence
             *               on a swap.
             */
            VectorCache(
                    const std::function<thesoup::types::Result<bool, int>(const thesoup::types::Slice<T>&, const std::size_t& page_num)>& saver,
                    const std::function<thesoup::types::Result<thesoup::types::Slice<T>, int>(const std::size_t&)>& loader
                    ) : saver {saver}, loader {loader} {}

             /**
              * \brief Destructor
              *
              * The destructor actually flushes all the pages in memory before destroying the page table contents, and
              * the in memory stuff.
              */
            ~VectorCache() {
                for (auto& page: page_table) {
                    if (page.slice.start != nullptr) {
                        if (page.valid) {
                            saver(page.slice, page.page_number);
                        }
                        delete page.slice.start;
                    }
                }
            }


            /**
             * \brief Move constructor
             *
             * Move constructor. Please do not try to use the object out of which everything was just moved.
             *
             * \param other The object from which to move
             */
            VectorCache(VectorCache<T, page_offset_bits, page_index_bits>&& other) noexcept :
                saver {other.saver},
                loader {other.loader},
                page_table {std::move(other.page_table)},
                _size {other._size},
                num_pages {other.num_pages} {
                other.page_table.clear();
                other._size = 0;
                other.num_pages = 0;
            }

            //!\cond NO_DOC
            VectorCache<T, page_offset_bits, page_index_bits>& operator*() {return *this;}
            VectorCache<T, page_offset_bits, page_index_bits>* operator->() {return this;}
            VectorCache<T, page_offset_bits, page_index_bits>& operator=(VectorCache<T, page_offset_bits, page_index_bits>& other)=delete;
            VectorCache<T, page_offset_bits, page_index_bits>& operator=(VectorCache<T, page_offset_bits, page_index_bits>&& other)=delete;
            VectorCache(VectorCache<T, page_offset_bits, page_index_bits>& other)=delete;
            //!\endcond

            /**
             * \brief Push an item onto the vector.
             *
             * Appends one item to the end of the vector.
             *
             * \param item Item to push onto the vector.
             */
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

            /**
             * \brief The random access operator
             *
             * This operator allows us to access the ith element like this: array[i]. This is just like the `[]` operator
             * of std::vector, except that this can be very slow, when a swap is needed.
             *
             * \param idx Index to access
             * \return a reference to the idx-th element in the vector.
             */
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
                } else if(page_table[page_table_entry].page_number != page_number && page_table[page_table_entry].valid) {
                    // Some other valid page
                    auto save_res {saver(page_table[page_table_entry].slice, page_table[page_table_entry].page_number)};
                    if (!save_res) {
                        throw std::runtime_error(std::to_string(save_res.error()));
                    }
                    auto load_res {loader(page_number)};
                    if (!load_res) {
                        throw std::runtime_error(std::to_string(load_res.error()));
                    }
                    thesoup::types::Slice<T> slice {std::move(load_res.unwrap())};
                    page_table[page_table_entry].slice.start = slice.start;
                    page_table[page_table_entry].slice.size = slice.size;
                    page_table[page_table_entry].page_number = page_number;
                } else {
                    auto load_res {loader(page_number)};
                    if (!load_res) {
                        throw std::runtime_error(std::to_string(load_res.error()));
                    }
                    thesoup::types::Slice<T> slice {std::move(load_res.unwrap())};
                    page_table[page_table_entry].slice.start = slice.start;
                    page_table[page_table_entry].slice.size = slice.size;
                    page_table[page_table_entry].page_number = page_number;
                    page_table[page_table_entry].valid = true;
                }
                return page_table[page_table_entry].slice[page_offset];
            }

            /**
             * \brief Number of elements in the vector
             *
             * This method returns the TOTAL number of elements in the vector, regardless of whether they are loaded in
             * memory or not.
             *
             * \return Total number of elements (std::size_t)
             */
            std::size_t size() const noexcept {
                return _size;
            }

            /**
             * \brief Total bytes being used
             *
             * This method returns the total number of bytes (loaded in memory or not) being used. Note that once a page
             * is allocated, it might not be full. So the total bytes being used might be greater than total number of elements
             * multiplied by the size of each element.
             *
             * \return Total bytes (std::size_t)
             */
            std::size_t bytes() const noexcept {
                return num_pages * page_size * sizeof(T);
            }

            /**
             * \brief Total number of pages.
             *
             * This method returns the total number of pages, regardless of whether they are in memory or not.
             *
             * \return Total number of pages (std::size_t)
             */
            std::size_t num_partitions() const noexcept {
                return num_pages;
            }

            /**
             * \brief Get the slice for a partition
             *
             * As mentioned before, both random, and sequential access is slow for this class, as a large number of
             * computations need to be made. However if one id working mostly within a page (spatial locality) and needs
             * performance, he can extract the slice information for a page and and then work with that. That will provide
             * full performance of that of std::vector.
             *
             * \param page_number The page number
             * \return A slice for that partition
             */
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
            //!\cond NO_DOC
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
            //!\endcond

            Iterator begin() noexcept {return Iterator(this, 0);}
            Iterator end() noexcept {return Iterator(this, _size);}
        };
    }
}