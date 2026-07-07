// References:
//   llvm/include/llvm/ADT/SmallVector.h
//     https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/ADT/SmallVector.h
//   folly/container/FBVector.h
//     https://github.com/facebook/folly/blob/main/folly/container/FBVector.h

#pragma once

#include <cstdlib>              // malloc, realloc, free, size_t
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

public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;

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
        std::memcpy(data(), other.data(), other.size_ * sizeof(T));
        size_ = other.size_;
    }

    PODVector(PODVector&& other) noexcept : PODVector() {
        move_from(other);
    }

    PODVector& operator=(const PODVector& other) {
        if (this == &other) return *this;
        reserve(other.size_);
        std::memcpy(data(), other.data(), other.size_ * sizeof(T));
        size_ = other.size_;
        return *this;
    }

    PODVector(std::initializer_list<T> init) : PODVector() {
        reserve(init.size());
        std::memcpy(data(), init.begin(), init.size() * sizeof(T));
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
    

    // Modifiers
    void clear() {
        size_ = 0;
    }

    void push_back(const T& value) {
        T tmp = value;
        if (size_ == capacity_) grow(next_capacity(capacity_ + 1));
        data()[size_++] = tmp;
    }

    void pop_back() noexcept {
        if(size_ > 0) --size_;
    }

    void resize(size_type new_size, const T& fill_value = T()) {
        if (new_size > capacity_) grow(next_capacity(new_size));
        if (new_size > size_) {
            size_t count = new_size - size_;
            std::fill_n(data() + size_, count, fill_value);
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
	requires std::input_iterator<InputIt>
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


private:
    size_t size_;
    size_t capacity_;
	T* ptr_;

    size_type next_capacity(size_type min_required) const {
        size_type grown = capacity_ + capacity_ / 2;
        return grown > min_required ? grown : min_required;
    }

    void grow(size_type new_cap) {
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

} // namespace TU
