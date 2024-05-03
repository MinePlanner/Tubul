#pragma once


#include <memory>
#include <algorithm>
#include <functional>
#include <vector>
#include <utility>

namespace TU {

    namespace detail {
        template<class Value, class CompareFunc>
        class FlatSetCompare : public CompareFunc {

        public:
            FlatSetCompare() = default;

            explicit FlatSetCompare(const CompareFunc &src) :
                    CompareFunc(src) {}

            bool operator()(const Value&lhs, const Value& rhs) const {
                return CompareFunc::operator()(lhs, rhs);
            }

        };
    }

/** class template FlatSet is a class adapted from Andrei Alexandrescu's example
 * implementation in his Loki Library. It is an associative vector built as a
 * syntactic drop-in replacement for std::set, but built on top of a vector to handle
 * the storage, instead of the STL common implementation of rbtrees. This makes it
 * friendlier to memory cache and allocation. It is built directly on top of vector
 * so it defers a lot of work to stl's vector, but presents an interface most commonly
 * seen in associative containers (like set/unordered_set).
 *
 * BEWARE: FlatSet doesn't respect all std::set's guarantees, the most important being:
 *  * iterators are invalidated by insert and erase operations
 *  * the complexity of insert/erase is O(N) not O(log N)
 *  * value_type is std::pair<KeyType, ValueType> not std::pair<const KeyType, ValueType>
 *  * iterators are random
 */


    template
            <
                    class ValueType,
                    class C = std::less<ValueType>,
                    class A = std::allocator<ValueType>
            >
    class FlatSet
            : private std::vector<ValueType, A>, private detail::FlatSetCompare<ValueType, C> {
        using Base = std::vector<ValueType, A>;
        using MyCompare = detail::FlatSetCompare<ValueType, C>;

    public:
        using key_type = ValueType;
        using value_type = ValueType;

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
            friend class FlatSet;

        protected:
            explicit value_compare(key_compare pred) : key_compare(pred) {}

        public:
            bool operator()(const value_type &lhs, const value_type &rhs) const {
                return key_compare::operator()(lhs.first, rhs.first);
            }
        };

        // construct/copy/destroy
        explicit
        FlatSet(const key_compare &comp = key_compare(),
                const A &alloc = A())
                : Base(alloc), MyCompare(comp) {}

        template<class InputIterator>
        FlatSet(InputIterator first, InputIterator last,
                const key_compare &comp = key_compare(),
                const A &alloc = A())
                : Base(first, last, alloc), MyCompare(comp) {
            MyCompare &me = *this;
            std::sort(begin(), end(), me);
        }

        FlatSet( std::initializer_list<value_type> slist,const key_compare &comp = key_compare(),
                const A &alloc = A())
                : Base(slist,alloc), MyCompare(comp) {
            MyCompare &me = *this;
            std::sort(begin(), end(), me);
        }

        FlatSet &operator=(const FlatSet &rhs) {
            FlatSet(rhs).swap(*this);
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

        // modifiers:
        std::pair<iterator, bool> insert(const value_type &val) {
            bool found(true);
            iterator i(lower_bound(val));

            if (i == end() || this->operator()(val, *i)) {
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

        void swap(FlatSet &other) {
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

        // set operations:
        iterator find(const key_type &k) {
            iterator i(lower_bound(k));
            if (i != end() && this->operator()(k, *i)) {
                i = end();
            }
            return i;
        }

        const_iterator find(const key_type &k) const {
            const_iterator i(lower_bound(k));
            if (i != end() && this->operator()(k, *i)) {
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

        void add_sorted_range_at_tail( iterator first, iterator last) {
            while (first != last) {
                const auto pos = end();
                const auto& val = *first;
                if ((pos == begin() || this->operator()(*(pos - 1), val)) &&
                    (pos == end() || this->operator()(val, *pos))) {
                    Base::push_back( val );
                }
                ++first;
            }
        }

        void add_sorted_range_at_head( iterator first, iterator last) {
            if ( first == last )
                return;
            //insert as many elements as needed.
            auto orig_size = size();
            //Add the elements at the end
            Base::push_back(*first);
            ++first;
            add_sorted_range_at_tail(first, last);
            //Rotate the elements so that the new elements are at the beginning
            std::rotate( begin(), begin()+orig_size,end());
        }

        void add_sorted_range_overlapped( iterator first, iterator last) {
            auto size_pre = size();
            if ( size_pre == 0) {
                add_sorted_range_at_tail(first, last);
                return;
            }
            //store the second range at the end.
            Base::push_back(*first);
            ++first;
            add_sorted_range_at_tail(first, last);
            //use inplace_merge to correctly merge 2 sorted ranges.
            std::inplace_merge(begin(), begin()+size_pre, end(), key_comp());
            //Ensure elements are unique
            auto new_end = std::unique(begin(), end() );
            Base::erase( new_end, end());
        }

        void add_sorted_range( iterator first, iterator last) {
            //Adding empty ranges does nothing
            if (first == last)
                return;
            //If first element in the range is bigger than our current last element (or empty)
            if (empty() || this->operator()(Base::back(), *first)) {
                add_sorted_range_at_tail(first, last);
                return;
            }
            //If our current first element is bigger than the last element we are supposed to add.
            if ( this->operator()(*(last-1), Base::front() )) {
                add_sorted_range_at_head(first, last);
                return;
            }
            add_sorted_range_overlapped(first, last);
        }

        void remove_sorted_range(iterator first, iterator last) {
            if (empty() or first == last) //Checking the easiest of cases.
                return;
            //Because both sets are sorted, we can try to look for the intersection range of A and B.
            //This helps to reduce the size of B (because if B contains elements outside the range of A
            //we dont care about them as we won't find them in A).

            //We check if there's any chance of intersection between the ranges.
            if ( *begin() > *(last-1) or *(end()-1) < *first )
                return;
            //We check where we may start seeing the intersection of elements.
            auto found = std::lower_bound(begin(), end(), *first);
            //There's no point in continue looking because elements can't intersect.
            if (found == end())
                return;
            // We check the other range until which point there could be an intersection with us
            auto endOfOtherRange = std::upper_bound(first, last, *rbegin());

            auto writeIt = found;
            auto i = found;
            auto j = first;
            while (i != end() and j != endOfOtherRange) {
                //The same element from the range is in contained. We have to not store the element
                //pointed by i, and advance both pointers
                if (*i == *j) {
                    ++i;
                    ++j;
                }
                //Element pointed by i was smaller (i.e. not the same), so we can store it and
                //move pointers forward.
                else if (*i < *j) {
                    *writeIt = *i;
                    ++writeIt;
                    ++i;
                } else //element pointed by i was bigger than pointed by j
                {
                    ++j;
                }
            }

            // Copy remaining elements from A if any
            while (i < end())
                *writeIt++ = *i++;

            // Drop elements beyond what we wrote (note writeIt points to *AFTER* the last place we wrote).
            Base::resize(std::distance(begin(), writeIt));
        }
        //Operators that are extremely useful
        template<class V1, class C1, class A1>
        friend bool operator==(const FlatSet<V1, C1, A1> &lhs,
                               const FlatSet<V1, C1, A1> &rhs);

        bool operator<(const FlatSet &rhs) const {
            const Base &me = *this;
            const Base &yo = rhs;
            return me < yo;
        }

        template<class V1, class C1, class A1>
        friend bool operator!=(const FlatSet<V1, C1, A1> &lhs,
                               const FlatSet<V1, C1, A1> &rhs);

        template<class V1, class C1, class A1>
        friend bool operator>(const FlatSet<V1, C1, A1> &lhs,
                              const FlatSet<V1, C1, A1> &rhs);

        template<class V1, class C1, class A1>
        friend bool operator>=(const FlatSet<V1, C1, A1> &lhs,
                               const FlatSet<V1, C1, A1> &rhs);

        template<class V1, class C1, class A1>
        friend bool operator<=(const FlatSet<V1, C1, A1> &lhs,
                               const FlatSet<V1, C1, A1> &rhs);
    }; //end of class FlatSet



    template<class V, class C, class A>
    inline bool operator==(const FlatSet<V, C, A> &lhs,
                           const FlatSet<V, C, A> &rhs) {
        const std::vector<V, A> &me = lhs;
        return me == rhs;
    }

    template<class V, class C, class A>
    inline bool operator!=(const FlatSet<V, C, A> &lhs,
                           const FlatSet<V, C, A> &rhs) { return !(lhs == rhs); }

    template<class V, class C, class A>
    inline bool operator>(const FlatSet<V, C, A> &lhs,
                          const FlatSet<V, C, A> &rhs) { return rhs < lhs; }

    template<class V, class C, class A>
    inline bool operator>=(const FlatSet<V, C, A> &lhs,
                           const FlatSet<V, C, A> &rhs) { return !(lhs < rhs); }

    template<class V, class C, class A>
    inline bool operator<=(const FlatSet<V, C, A> &lhs,
                           const FlatSet<V, C, A> &rhs) { return !(rhs < lhs); }


    // specialized algorithms:
    template<class V, class C, class A>
    void swap(FlatSet<V, C, A> &lhs, FlatSet<V, C, A> &rhs) { lhs.swap(rhs); }

} // namespace TU 


