// References:
//   llvm/include/llvm/ADT/SmallVector.h
//     https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/ADT/SmallVector.h
//   folly/container/FBVector.h
//     https://github.com/facebook/folly/blob/main/folly/container/FBVector.h

#pragma once

#include <cstdlib>              // malloc, realloc, free, size_t
#include <cstddef>              // std::max_align_t
#include <cstdint>              // SIZE_MAX
#include <type_traits>          // is_trivially_copyable
#include <initializer_list>     // 
#include <cstring>              // memcpy
#include <stdexcept>            // std::out_of_range
#include <utility>              // std::swap
#include <algorithm>            // fill_n
#include <new>                  // std::bad_alloc
#include <iterator>             // std::distance


namespace TU {
template <typename T>
struct PODVector{
    static_assert(std::is_trivially_copyable<T>::value,
        "PODVector<T> requires T to be trivially copyable type."
    );
    // malloc/realloc only guarantee max_align_t alignment; over-aligned T would not work.
    static_assert(alignof(T) <= alignof(std::max_align_t),
        "PODVector<T> requires alignof(T) <= alignof(std::max_align_t)."
    );

public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Commons
    bool empty() const { return size_ == 0; }

    size_type size() const { return size_; }
    
    size_type capacity() const { return capacity_; }

    void reserve(size_type new_cap) {
        if (new_cap <= capacity_) return;
        grow(new_cap);
    }

    // Construction
    PODVector() : size_(0), capacity_(0), ptr_(nullptr) {}

    explicit PODVector(size_type count, const T& value = T()) : PODVector() {
        resize(count, value);
    }

    // Copy
    PODVector(const PODVector& other) : PODVector() {
        reserve(other.size_);
    	// check if other is not an empty pointer
        if (other.size_) std::memcpy(data(), other.data(), other.size_ * sizeof(T));
        size_ = other.size_;
    }

    PODVector(PODVector&& other) noexcept : PODVector() {
        move_from(other);
    }

    PODVector& operator=(const PODVector& other) {
        if (this == &other) return *this;
        reserve(other.size_);
    	// check if other is not an empty pointer
        if (other.size_) std::memcpy(data(), other.data(), other.size_ * sizeof(T));
        size_ = other.size_;
        return *this;
    }

    PODVector(std::initializer_list<T> init) : PODVector() {
        reserve(init.size());
        if (init.size()) std::memcpy(data(), init.begin(), init.size() * sizeof(T));
        size_ = init.size();
    }

    PODVector& operator=(PODVector&& other) noexcept {
        if (this == &other) return *this;
    	std::free(ptr_);
        move_from(other);
        return *this;
    }

    // Destruction
    ~PODVector() { 
    	std::free(ptr_);
    }


    // Element access
    T* data() { return ptr_; }
    const T* data() const { return ptr_; }

    reference operator[](size_type i) { return data()[i]; }
    const_reference operator[](size_type i) const { return data()[i]; }
    
    reference at(size_type i) {
        if(i >= size_) throw std::out_of_range("PODVector::at");
        return data()[i];
    }
    const_reference at(size_type i) const {
        if(i >= size_) throw std::out_of_range("PODVector::at");
        return data()[i];
    }

    reference front() { return data()[0]; }
    const_reference front() const { return data()[0]; }
    reference back() { return data()[size_ - 1]; }
    const_reference back() const { return data()[size_ - 1]; }

    // Iterators
    iterator begin() { return data(); }
    const_iterator begin() const { return data(); }
    iterator end() { return data() + size_; }
    const_iterator end() const { return data() + size_; }

    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }


    // Modifiers
    void clear() {
        size_ = 0;
    }

    void push_back(const T& value) {
        T tmp = value;
        if (size_ == capacity_) grow(next_capacity(capacity_ + 1));
        data()[size_++] = tmp;
    }

    template <typename... Args>
    reference emplace_back(Args&&... args) {
        // build the value before growing: args may reference existing elements
        T tmp(std::forward<Args>(args)...);
        if (size_ == capacity_) grow(next_capacity(capacity_ + 1));
        T* slot = data() + size_;
        *slot = tmp;
        ++size_;
        return *slot;
    }

    void pop_back() noexcept {
        if(size_ > 0) --size_;
    }

    void resize(size_type new_size, const T& fill_value = T()) {
    	T tmp = fill_value;
        if (new_size > capacity_) grow(next_capacity(new_size));
        if (new_size > size_) {
            size_t count = new_size - size_;
            std::fill_n(data() + size_, count, tmp);
        }
        size_ = new_size;
    }

    void swap(PODVector& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    iterator insert(const_iterator pos, const T& value) {
        size_type index = pos - begin();
        if (index > size_) throw std::out_of_range("PODVector::insert: pos out of range");

        // copy value before growing
        T tmp = value;
        if (size_ == capacity_) grow(next_capacity(capacity_ + 1));

        T* p = data();
        if (index < size_) {
            std::memmove(p + index + 1, p + index, (size_ - index) * sizeof(T));
        }
        p[index] = tmp;
        ++size_;
        return p + index;
    }

    iterator insert(const_iterator pos, size_type count, const T& value) {
        size_type index = static_cast<size_type>(pos - begin());
        if (index > size_) throw std::out_of_range("PODVector::insert: pos out of range");
        if (count == 0) return begin() + index;


        T tmp = value;

        if (size_ + count > capacity_) grow(next_capacity(size_ + count));

        T* p = data();
        if (index < size_) {
            std::memmove(p + index + count, p + index, (size_ - index) * sizeof(T));
        }
        std::fill_n(p + index, count, tmp);
        size_ += count;
        return p + index;
    }

    template <typename InputIt>
	requires std::forward_iterator<InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        size_type index = static_cast<size_type>(pos - begin());
        size_type count  = static_cast<size_type>(std::distance(first, last));

        if (index > size_) throw std::out_of_range("PODVector::insert: pos out of range");

        if (count == 0) return begin() + index;

        if (size_ + count > capacity_) grow(size_ + count);

        T* p = data();
        if (index < size_) {
            std::memmove(p + index + count, p + index, (size_ - index) * sizeof(T));
        }
        for(size_type i = 0; i < count; ++i, ++first) {
            p[index + i] = *first;
        }
        size_ += count;
        return p + index;
    }

    iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

    iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }

    iterator erase(const_iterator first, const_iterator last) {
        size_type index = static_cast<size_type>(first - begin());
        size_type count = static_cast<size_type>(last - first);
        if (count == 0) return begin() + index;

        T* p = data();
        // shift the tail [last, end) down onto the erased range
        std::memmove(p + index, p + index + count, (size_ - index - count) * sizeof(T));
        size_ -= count;
        return p + index;
    }

    void assign(size_type count, const T& value) {
        T tmp = value;   // copy before clear: value may alias an element of *this
        clear();
        resize(count, tmp);
    }


private:
    size_t size_;
    size_t capacity_;
	T* ptr_;

    size_type next_capacity(size_type min_required) const {
        size_type grown = capacity_ + capacity_ / 2;
        return grown > min_required ? grown : min_required;
    }

    void grow(size_type new_cap) {
        if (new_cap > SIZE_MAX / sizeof(T)) throw std::length_error("PODVector: capacity exceeds maximum size");
        T* new_data = static_cast<T*>(std::realloc(ptr_, new_cap * sizeof(T)));
        if (!new_data) throw std::bad_alloc();
        // Change the pointer
        ptr_ = new_data;
        capacity_ = new_cap;
    }

    void move_from(PODVector& other) noexcept {
        ptr_ = other.ptr_;
        capacity_ = other.capacity_;
        size_ = other.size_;

        // leave the other vector empty. don't free because the memory is now on this ptr.
        other.ptr_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
};

template <typename T>
void swap(PODVector<T>& a, PODVector<T>& b) noexcept {
    a.swap(b);
}

} // namespace TU
