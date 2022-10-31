#pragma once

#include <algorithm>
#include <iterator>
#include <vector>

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
    namespace type {

        template <typename T, std::size_t page_size> class Vector {

        private:
            std::vector<T*> pages {};
            std::size_t _size {};

            static constexpr std::size_t num_items_per_page = page_size / sizeof(T);

            void allocate_new_page() {
                T* new_page = reinterpret_cast<T*>(new char(page_size));
                pages.push_back(new_page);
            }
        public:
            Vector() {}
            ~Vector() {
                for (T* ptr: pages) {
                    delete ptr;
                }
            }
            Vector(Vector<T, page_size>& other)=delete;
            Vector(Vector<T,page_size>&& other) : pages {other.pages}, _size {other._size} {
                other.pages.clear();
                other._size = 0;
            }

            void operator=(Vector<T, page_size>& other)=delete;
            void operator=(Vector<T, page_size>&& other) {
                for (T* ptr: pages) {
                    delete ptr;
                }
                pages.clear();
                std::copy(other.pages.begin(), other.pages.end(), std::back_inserter(pages));
                _size = other._size;
                other.pages.clear();
                other._size = 0;
            }
            Vector<T, page_size>& operator*() {return *this;}
            Vector<T, page_size>* operator->() {return this;}

            void push_back(const T& item) noexcept {
                std::size_t page_number_to_insert_to {(_size)/num_items_per_page};
                std::size_t page_offset {_size - page_number_to_insert_to * num_items_per_page};
                if (page_number_to_insert_to >= pages.size()) {
                    allocate_new_page();
                }
                pages[page_number_to_insert_to][page_offset] = item;
                _size++;
            }

            T& operator[](const std::size_t idx) {
                std::size_t page_num {idx/num_items_per_page};
                std::size_t page_offset {idx - page_num * num_items_per_page};
                return pages[page_num][page_offset];
            }

            std::size_t size() const noexcept {
                return _size;
            }

            std::size_t bytes() const noexcept {
                return pages.size() * page_size;
            }

            struct Iterator {
                using iterator_category [[maybe_unused]] = std::random_access_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type [[maybe_unused]] = T;
                using pointer           = T*;
                using reference         = T&;

                Vector<T, page_size>* enclosing;
                std::size_t idx {};

                Iterator(
                        Vector<T, page_size>* enclosing,
                        const std::size_t& idx) : enclosing {enclosing}, idx {idx} {}

                reference operator*() noexcept {return (*enclosing)[idx];}
                pointer operator->() noexcept {return &(*enclosing)[idx];}

                Iterator& operator++() noexcept { idx++; return *this;}
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