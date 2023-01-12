#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <unordered_set>

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
        /**
         * \class DisjointSets
         *
         * \brief A class implementing the disjoint sets data structure.
         *
         * A disjoint set is an interesting data structures with a few properties. It is useful for grouping things
         * together into sets.
         * Following is the description of its state:
         *
         *   - At any point in time, there are a bunch (say N) elements inside the structure. N does not change over time.
         *   - The N elements are grouped into M(t) disjoint groups, meaning 1 element can only belong to 1 set at a time.
         *     The subscript `t` implies that the number of sets change over time. It is to be noted that the number of sets
         *     can only decrease. Details below in the operations section.
         *   - Each set has a set leader. A set leader is 1 of the N elements in the set, and acts as the "representative"
         *     (ID) for that set
         *
         * Following are the operations:
         *
         *  - Query the set leader for an item. This operation accepts any 1 of the N elements and returns the leader of
         *    the set to which this element belongs.
         *  - Merge 2 sets. This operation accepts 2 out of the N elements, and if they belong to different sets, will
         *    merge the 2. The set leader of the unified set will be leader of the set, to which the first element belonged.
         *    Note that we can only merge sets. Hence the total number of sets must only decrease or stay the same over time.
         *
         * One example where this data structure is used is Kruskal's algorithm to find the minimum cost spanning tree.
         * See `thesoup::algorithms::kruskal`
         *
         * \tparam T The content element type
         */
        template <typename T> class DisjointSets {
        private:
            std::unordered_map<T, std::unordered_set<T>> sets {};
            std::unordered_map<T, T> set_leaders {};

        public:
            /**
             * \enum ErrorCode
             *
             * \brief Error codes for DisjointSets
             *
             * This enum helps us return relevant error information for any operations on the `DisjointSets` class.
             * Here it is used in conjunction of `thesoup::types::Result`.
             *
             */
            enum class ErrorCode {
                ELEMENT_DOES_NOT_EXIST
            };

            /**
             * \brief Constructor
             *
             * This constructs the DisjointSets object from  collection, and sets it to it's initial sets. In the initial
             * state, the object contains N elements and N sets. Each set has only 1 element, and that element is the set
             * leader. It uses iterators to construct the object. Use the example below to construct an object.
             *
             * ```
             * std::vector<char> elements {'a', 'b', 'c', 'd'};
             * DisjointSets<char> ds {elements.begin(), elements.end()};
             * ```
             *
             * \tparam IT The iterator type that iterates over elements of type `T`. No need to specify this. It will be deduced.
             * \param begin The start iterator
             * \param end The end iterator
             */
            template <typename IT> DisjointSets(const IT& begin, const IT& end) {
                static_assert(thesoup::types::IsForwardIteratorOfType<IT, T>::value);
                std::for_each(
                        begin,
                        end,
                        [&](const T& item) {
                            sets.emplace(item, std::unordered_set<T>());
                            sets.at(item).insert(item);
                            set_leaders.emplace(item, item);
                        });
            }

            /**
             * \brief Get the set leader
             *
             * This method accepts an item and returns the leader of the set to which it belongs. If the item is not
             * present, an error code indicating as such is returned.
             *
             * \param item
             * \return Result<T, ErrorCode>
             */
            thesoup::types::Result<T, ErrorCode> get_set_leader(const T& item) const noexcept {
                if (set_leaders.find(item) == set_leaders.end()) {
                    return thesoup::types::Result<T, ErrorCode>::failure(ErrorCode::ELEMENT_DOES_NOT_EXIST);
                } else {
                    return thesoup::types::Result<T, ErrorCode>::success(set_leaders.at(item));
                }
            }

            /**
             * \brief Return the number of sets
             *
             * Returns the number of sets. Since the number of elements remains constant, the size is defined by this
             * parameter.
             *
             * \return Number of sets (std::size_t)
             */
            std::size_t size() const noexcept {
                return sets.size();
            }

            /**
             * \brief Merge 2 sets
             *
             * This method accepts 2 items, and if they belong to different sets, merges them. If say `sl1` was the set
             * leader for elem1's set, `sl1` is the set leader of the new merged set.
             *
             * \param elem1 The first item
             * \param elem2 The second item
             * \return Result<Unit, ErrorCode>
             */
            thesoup::types::Result<thesoup::types::Unit, ErrorCode> merge_sets(const T& elem1, const T& elem2) noexcept {
                if (set_leaders.find(elem1) == set_leaders.end() || set_leaders.find(elem2) == set_leaders.end()) {
                    return thesoup::types::Result<thesoup::types::Unit, ErrorCode>::failure(ErrorCode::ELEMENT_DOES_NOT_EXIST);
                }
                const T sl1 {set_leaders.at(elem1)};
                const T sl2 {set_leaders.at(elem2)};

                if (sl1 != sl2) {
                    sets.at(sl1).insert(sets.at(sl2).begin(), sets.at(sl2).end());
                    std::for_each(
                            sets.at(sl2).begin(),
                            sets.at(sl2).end(),
                            [&](const T& item){
                                set_leaders.at(item) = sl1;
                            });
                    sets.erase(sl2);
                }
                return thesoup::types::Result<thesoup::types::Unit, ErrorCode>::success(thesoup::types::Unit::unit);
            }
        };

    }
}