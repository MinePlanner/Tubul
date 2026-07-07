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

namespace TU {
template <typename T, std::size_t N>
struct PODVector{
    static_assert(std::is_trivially_copyable<T>::value, 
        "PODVector<T, N> requires T to be trivially copyable type."
    );
    static_assert(N > 0, 
        "N must be greater than 0."
    );

public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;

    // Commons
    bool is_small() const {
        return capacity_ <= N;
    }

    bool empty() { return size_ == 0; }
    
    size_type size() const { return size_; }
    
    size_type capacity() const { return capacity_; }

    void reserve(size_type new_cap) {
        if (new_cap <= capacity_) return;
        grow(next_capacity(new_cap));
    }

    // Construction
    PODVector() : size_(0), capacity_(N) {}

    explicit PODVector(size_type count, const T& value = T()) : PODVector() {
        resize(count, value);
    }

    // Copy
    PODVector(const PODVector& other) : PODVector() {
        reserve(other.size_);
        std::memcpy(data(), other.data(), other.size_ * sizeof(T));
        size_ = other.size_;
    }

    PODVector(PODVector&& other) : PODVector() {
        move_from(other);
    }

    PODVector& operator=(const PODVector& other) {
        if (this == &other) return *this;
        clear();
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
        free_heap();
        move_from(other);
        return *this;
    }

    // Destruction
    ~PODVector() { 
        free_heap(); 
    }


    // Element access
    T* data() {
        return is_small()? stackbuf_ : heapbuf_.ptr;
    }
    const T* data() const {
        return is_small()? stackbuf_ : heapbuf_.ptr;
    }

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
        if (size_ == capacity_) grow(next_capacity(capacity_ + 1));
        data()[size_++] = value;   
    }

    void pop_back() {
        if(size_ > 0) --size_;
    }

    void resize(size_type new_size, const T& fill_value = T()) {
        if (new_size > capacity_) grow(next_capacity(new_size));
        if (new_size > size_) {
            T* p = data();
            size_t count = new_size - size_;
            // memset works cast internally to unsigned char (1 byte), so it may not work with 
            // some values of the type. it could work with 0, but not with 5
            // memcpmp returns 0 if there is no difference between the casting of unsigned char of both values
            static const T kZero{};
            if (std::memcmp(&fill_value, &kZero, sizeof(T)) == 0) {
                std::memset(p + size_, 0, count * sizeof(T));
            }
            else {
                std::fill_n(p + size_, count, fill_value);
            }
        }
        size_ = new_size;
    }

    void swap(PODVector& other) noexcept {
        if (is_small() && other.is_small()) {
            T tmp_buf[N];
            std::memcpy(tmp_buf, stackbuf_, size_ * sizeof(T));
            std::memcpy(stackbuf_, other.stackbuf_, other.size_ * sizeof(T));
            std::memcpy(other.stackbuf_, tmp_buf, size_ * sizeof(T));
            std::swap(size_, other.size_);
        } else if (!is_small() && !other.is_small()) {
            std::swap(heapbuf_.ptr, other.heapbuf_.ptr);
            std::swap(size_, other.size_);
            std::swap(capacity_, other.capacity_);
        } else {
            PODVector tmp(std::move(*this));
            *this = std::move(other);
            other = std::move(tmp);
        }
    }

    iterator insert(const_iterator pos, const T& value) {
        size_type index = pos - begin();
        if (index > size_) throw std::out_of_range("PODVector::insert: pos fuera de rango");

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
        if (index > size_) throw std::out_of_range("PODVector::insert: pos fuera de rango");
        if (count == 0) return begin() + index;


        T tmp = value; // misma razón que arriba

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
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        size_type index = static_cast<size_type>(pos - begin());
        size_type count  = static_cast<size_type>(std::distance(first, last));
        
        if (index > size_) throw std::out_of_range("PODVector::insert: pos fuera de rango");
        
        if (count == 0) return begin() + index;

        if (size_ + count > capacity_) grow(next_capacity(size_ + count));

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
    // Evaluar poner el capacity_ dentro del struct dentro heapbuf_
    size_t capacity_;
    union {
        struct {
            T *ptr;
        } heapbuf_;
        T stackbuf_[N];
    };

    size_type next_capacity(size_type min_required) const {
        size_type grown = capacity_ + capacity_ / 2;
        return grown > min_required ? grown : min_required;
    }

    void grow(size_type new_cap) {
        if (is_small()) {
            T* new_data = static_cast<T*>(std::malloc(new_cap * sizeof(T)));
            if (!new_data) throw std::bad_alloc();
            // Reallocate the memory to the heapbuffer from the stackbuffer
            std::memcpy(new_data, stackbuf_, size_ * sizeof(T));
            heapbuf_.ptr = new_data;
        }
        else {
            T* new_data = static_cast<T*>(std::realloc(heapbuf_.ptr, new_cap * sizeof(T)));
            if (!new_data) throw std::bad_alloc();
            // Change the pointer
            heapbuf_.ptr = new_data;
        }
        capacity_ = new_cap;
    }

    void free_heap() {
        if (!is_small()) std::free(heapbuf_.ptr);
    }

    void move_from(PODVector& other) {
        if (other.is_small()) {
            std::memcpy(stackbuf_, other.stackbuf_, other.size_ * sizeof(T));
            capacity_ = N;
        }
        else {
            heapbuf_.ptr = other.heapbuf_.ptr;
            capacity_ = other.capacity_;
        }
        size_ = other.size_;

        // other verctor as an small vector
        other.size_ = 0;
        other.capacity_ = N;
    }
};

} // namespace TU