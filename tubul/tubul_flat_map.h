#pragma once


#include <memory>
#include <algorithm>
#include <functional>
#include <vector>
#include <utility>

namespace TU {

    namespace detail {
        //This object is used by the FlatMap to wrap a comparison function
        //in a way that's more flexible for the container.
        template<class Key, class Value, class CompareFunc>
        class FlatMapCompare : public CompareFunc {
            //Note this is not the value mapped, but the pair stored in the FlatMap
            //which is used to compare the pairs directly instead of having to unwrap
            //the pair contained in the vector.
            using StoredData = std::pair<Key, Value>;

        public:
            FlatMapCompare() = default;

            explicit FlatMapCompare(const CompareFunc &src) :
                    CompareFunc(src) {}

            bool operator()(const Key& lhs, const Key& rhs) const {
                return CompareFunc::operator()(lhs, rhs);
            }

            bool operator()(const StoredData &lhs, const StoredData &rhs) const {
                return operator()(lhs.first, rhs.first);
            }

            bool operator()(const StoredData& lhs, const Key& rhs) const {
                return operator()(lhs.first, rhs);
            }

            bool operator()(const Key &lhs, const StoredData &rhs) const {
                return operator()(lhs, rhs.first);
            }
        };
    }

/** class template FlatMap is a class adapted from Andrei Alexandrescu's example
 * implementation in his Loki Library. It is an associative vector built as a
 * syntactic drop-in replacement for std::map, but built on top of a vector to handle
 * the storage, instead of the STL common implementation of rbtrees. This makes it
 * friendlier to memory cache and allocation. It is built directly on top of vector
 * so it defers a lot of work to stl's vector, but presents an interface most commonly
 * seen in associative containers (like map/unordered_map).
 *
 * BEWARE: FlatMap doesn't respect all std::map's guarantees, the most important being:
 *  * iterators are invalidated by insert and erase operations
 *  * the complexity of insert/erase is O(N) not O(log N)
 *  * value_type is std::pair<KeyType, ValueType> not std::pair<const KeyType, ValueType>
 *  * iterators are random
 */


    template
            <
                    class KeyType,
                    class ValueType,
                    class C = std::less<KeyType>,
                    class A = std::allocator<std::pair<KeyType, ValueType> >
            >
    class FlatMap
            : private std::vector<std::pair<KeyType, ValueType>, A>, private detail::FlatMapCompare<KeyType, ValueType, C> {
        using Base = std::vector<std::pair<KeyType, ValueType>, A>;
        using MyCompare = detail::FlatMapCompare<KeyType, ValueType, C>;

    public:
        using key_type = KeyType;
        using mapped_type = ValueType;
        using value_type = typename Base::value_type;

        using key_compare = C;
        using allocator_type = A;
        using iterator = typename Base::iterator;
        using const_iterator = typename Base::const_iterator;
        using size_type = typename Base::size_type;
        using difference_type = typename Base::difference_type;
        using reverse_iterator = typename Base::reverse_iterator;
        using const_reverse_iterator = typename Base::const_reverse_iterator;

        class value_compare
                : private key_compare {
            friend class FlatMap;

        protected:
            explicit value_compare(key_compare pred) : key_compare(pred) {}

        public:
            bool operator()(const value_type &lhs, const value_type &rhs) const {
                return key_compare::operator()(lhs.first, rhs.first);
            }
        };

        // construct/copy/destroy
        explicit
        FlatMap(const key_compare &comp = key_compare(),
                const A &alloc = A())
                : Base(alloc), MyCompare(comp) {}

        template<class InputIterator>
        FlatMap(InputIterator first, InputIterator last,
                const key_compare &comp = key_compare(),
                const A &alloc = A())
                : Base(first, last, alloc), MyCompare(comp) {
            MyCompare &me = *this;
            std::sort(begin(), end(), me);
        }

        FlatMap( std::initializer_list<value_type> slist,const key_compare &comp = key_compare(),
                const A &alloc = A())
                : Base(slist, alloc), MyCompare(comp) {
            MyCompare& me = *this;
            std::sort(begin(), end(), me);
        }

        FlatMap &operator=(const FlatMap &rhs) {
            FlatMap(rhs).swap(*this);
            return *this;
        }

        // iterators:
        // The following are here because MWCW gets 'using' wrong
        iterator begin() { return Base::begin(); }
        const_iterator begin() const { return Base::begin(); }
        iterator end() { return Base::end(); }
        const_iterator end() const { return Base::end(); }
        reverse_iterator rbegin() { return Base::rbegin(); }
        const_reverse_iterator rbegin() const { return Base::rbegin(); }
        reverse_iterator rend() { return Base::rend(); }
        const_reverse_iterator rend() const { return Base::rend(); }

        // capacity:
        bool empty() const { return Base::empty(); }
        size_type size() const { return Base::size(); }
        size_type max_size() const { return Base::max_size(); }
        void reserve(size_type new_cap){ Base::reserve(new_cap); }
        size_type capacity() const { return Base::capacity(); }

        // element access:
        mapped_type &operator[](const key_type &key) {
            return insert(value_type(key, mapped_type())).first->second;
        }

        // modifiers:
        std::pair<iterator, bool> insert(const value_type &val) {
            bool found(true);
            iterator i(lower_bound(val.first));

            if (i == end() || this->operator()(val.first, i->first)) {
                i = Base::insert(i, val);
                found = false;
            }
            return std::make_pair(i, !found);
        }

        iterator insert(iterator pos, const value_type &val) {
            if ((pos == begin() || this->operator()(*(pos - 1), val)) &&
                (pos == end() || this->operator()(val, *pos))) {
                return Base::insert(pos, val);
            }
            return insert(val).first;
        }

        template<class InputIterator>
        void insert(InputIterator first, InputIterator last) {
            for (; first != last; ++first)
                insert(*first);
        }

        void erase(iterator pos) {
            Base::erase(pos);
        }

        size_type erase(const key_type &k) {
            iterator i(find(k));
            if (i == end()) return 0;
            erase(i);
            return 1;
        }

        void erase(iterator first, iterator last) {
            Base::erase(first, last);
        }

        void swap(FlatMap &other) {
            Base::swap(other);
            MyCompare &me = *this;
            MyCompare &rhs = other;
            std::swap(me, rhs);
        }

        void clear() { Base::clear(); }

        // observers:
        key_compare key_comp() const { return *this; }

        value_compare value_comp() const {
            const key_compare &comp = *this;
            return value_compare(comp);
        }

        // map operations:
        iterator find(const key_type &k) {
            iterator i(lower_bound(k));
            if (i != end() && this->operator()(k, i->first)) {
                i = end();
            }
            return i;
        }

        const_iterator find(const key_type &k) const {
            const_iterator i(lower_bound(k));
            if (i != end() && this->operator()(k, i->first)) {
                i = end();
            }
            return i;
        }

        size_type count(const key_type &k) const {
            return find(k) != end();
        }

        bool contains(const key_type &k) const {
            return find(k) != end();
        }

        iterator lower_bound(const key_type &k) {
            MyCompare &me = *this;
            return std::lower_bound(begin(), end(), k, me);
        }

        const_iterator lower_bound(const key_type &k) const {
            const MyCompare &me = *this;
            return std::lower_bound(begin(), end(), k, me);
        }

        iterator upper_bound(const key_type &k) {
            MyCompare &me = *this;
            return std::upper_bound(begin(), end(), k, me);
        }

        const_iterator upper_bound(const key_type &k) const {
            const MyCompare &me = *this;
            return std::upper_bound(begin(), end(), k, me);
        }

        std::pair<iterator, iterator> equal_range(const key_type &k) {
            MyCompare &me = *this;
            return std::equal_range(begin(), end(), k, me);
        }

        std::pair<const_iterator, const_iterator> equal_range(
                const key_type &k) const {
            const MyCompare &me = *this;
            return std::equal_range(begin(), end(), k, me);
        }



        //Operators that are extremely useful
        template<class K1, class V1, class C1, class A1>
        friend bool operator==(const FlatMap<K1, V1, C1, A1> &lhs,
                               const FlatMap<K1, V1, C1, A1> &rhs);

        bool operator<(const FlatMap &rhs) const {
            const Base &me = *this;
            const Base &yo = rhs;
            return me < yo;
        }

        template<class K1, class V1, class C1, class A1>
        friend bool operator!=(const FlatMap<K1, V1, C1, A1> &lhs,
                               const FlatMap<K1, V1, C1, A1> &rhs);

        template<class K1, class V1, class C1, class A1>
        friend bool operator>(const FlatMap<K1, V1, C1, A1> &lhs,
                              const FlatMap<K1, V1, C1, A1> &rhs);

        template<class K1, class V1, class C1, class A1>
        friend bool operator>=(const FlatMap<K1, V1, C1, A1> &lhs,
                               const FlatMap<K1, V1, C1, A1> &rhs);

        template<class K1, class V1, class C1, class A1>
        friend bool operator<=(const FlatMap<K1, V1, C1, A1> &lhs,
                               const FlatMap<K1, V1, C1, A1> &rhs);
    }; //end of class FlatMap



    template<class K, class V, class C, class A>
    inline bool operator==(const FlatMap<K, V, C, A> &lhs,
                           const FlatMap<K, V, C, A> &rhs) {
        const std::vector<std::pair<K, V>, A> &me = lhs;
        return me == rhs;
    }

    template<class K, class V, class C, class A>
    inline bool operator!=(const FlatMap<K, V, C, A> &lhs,
                           const FlatMap<K, V, C, A> &rhs) { return !(lhs == rhs); }

    template<class K, class V, class C, class A>
    inline bool operator>(const FlatMap<K, V, C, A> &lhs,
                          const FlatMap<K, V, C, A> &rhs) { return rhs < lhs; }

    template<class K, class V, class C, class A>
    inline bool operator>=(const FlatMap<K, V, C, A> &lhs,
                           const FlatMap<K, V, C, A> &rhs) { return !(lhs < rhs); }

    template<class K, class V, class C, class A>
    inline bool operator<=(const FlatMap<K, V, C, A> &lhs,
                           const FlatMap<K, V, C, A> &rhs) { return !(rhs < lhs); }


    // specialized algorithms:
    template<class K, class V, class C, class A>
    void swap(FlatMap<K, V, C, A> &lhs, FlatMap<K, V, C, A> &rhs) { lhs.swap(rhs); }

} // namespace TU 


