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
        template <typename T> class DisjointSets {
        private:
            std::unordered_map<T, std::unordered_set<T>> sets {};
            std::unordered_map<T, T> set_leaders {};

        public:
            enum class ErrorCode {
                ELEMENT_DOES_NOT_EXIST
            };

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

            thesoup::types::Result<T, ErrorCode> get_set_leader(const T& item) const noexcept {
                if (set_leaders.find(item) == set_leaders.end()) {
                    return thesoup::types::Result<T, ErrorCode>::failure(ErrorCode::ELEMENT_DOES_NOT_EXIST);
                } else {
                    return thesoup::types::Result<T, ErrorCode>::success(set_leaders.at(item));
                }
            }

            std::size_t size() const noexcept {
                return sets.size();
            }

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