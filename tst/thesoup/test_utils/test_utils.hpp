#include <algorithm>
#include <iterator>
#include <vector>

namespace thesoup {

    namespace testutils {
        /**
         * \brief utility function to check if 2 collections are similar.
         *
         * Here, 2 collections are defined to be similar if both of them contain the same elements, just not in necessarily
         * in the same order. Tf you look at the implementation, you will notice that it is very inefficient. The rational
         * behind the chosen implementation method is to avoid hashing any items. We do not to burden the user with
         * having to implement std::hash for their type. The performance hit is OK, as this function is for testing only.
         *
         * @tparam IterType
         * @param b1
         * @param e1
         * @param b2
         * @param e2
         * @return
         */
        template <typename IterType> bool similar(const IterType& b1, const IterType& e1, const IterType& b2, const IterType& e2) {
            using ValueType = typename std::iterator_traits<IterType>::value_type;
            if (std::distance(b1, e1) != std::distance(b2, e2)) {
                return false;
            } else {
                auto len {std::distance(b1, e1)};
                std::vector<ValueType> v1;
                std::vector<ValueType> v2;
                v1.reserve(len);
                v2.reserve(len);
                std::copy(b1, e1, std::back_inserter(v1));
                std::copy(b2, e2, std::back_inserter(v2));

                for (auto item: v1) {
                    auto it {std::find(v2.begin(), v2.end(), item)};
                    if (it == v2.end()) {
                        return false;
                    } else {
                        v2.erase(it);
                    }
                }
                return true;
            }
        }
    }
}