//
// Created by Alicanto Nano on 13-07-26.
//

#ifndef TUBUL_TUBUL_SMALLVECTOR_H
#define TUBUL_TUBUL_SMALLVECTOR_H

#endif // TUBUL_TUBUL_SMALLVECTOR_H

#pragma once

// Reference:
//   folly/container/small_vector.h
//	   https://github.com/facebook/folly/blob/main/folly/container/small_vector.h#L1444
//   folly/container/FBVector.h
//     https://github.com/facebook/folly/blob/main/folly/container/FBVector.h
//   llvm/include/llvm/ADT/SmallVector.h
//     https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/ADT/SmallVector.h


#include <cstdlib>
#include <cstddef>
#include <type_traits>
#include <initializer_list>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <memory>
#include <new>
#include <iterator>

namespace TU {

namespace detail {
	template <typename T>
	inline constexpr bool is_relocatable_v = std::is_trivially_copyable_v<T>;


	// Just as LLVM to pass as value instead of the reference. It is sometimes cheaper to do this.
	template <typename T>
	inline constexpr bool should_pass_by_value = std::is_trivially_copyable_v<T> && sizeof(T)<= 2 * sizeof(void *);

	template <typename T>
	using value_param_t = std::conditional_t<should_pass_by_value<T>, T, const T&>;
} // namespace detail


template <typename T, std::size_t N>
struct SmallVector {
	static_assert(
		std::is_trivially_copyable_v<T> ||
		(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>),
		"SmallVector<T, N> requires to be trivially copyable, or nothrow move constructible and assignable."
	);
	static_assert(N > 0,
		"N must be greater than 0."
	);

public:
	using value_type      = T;
	using size_type       = std::size_t;
	using reference       = T&;
	using const_reference = const T&;
	using iterator        = T*;
	using const_iterator  = const T*;
	using value_param     = detail::value_param_t<T>;

private:
	size_t size_;
	size_t capacity_;
	struct HeapBuf { T *ptr; };
	union {
		HeapBuf heapbuf_;
		alignas(T )std::byte stackbuf_[N * sizeof(T)];
	};

	T* stack_data() { return reinterpret_cast<T*>(stackbuf_); }
	const T* stack_data() const { return reinterpret_cast<const T*>(stackbuf_); }

	size_type check_insert_pos(const_iterator pos) const {
		size_type index = pos - begin();
		if (index > size_ )throw std::out_of_range("SmallVector::insert: index out of range");
		return index;
	}

	size_type next_capacity(size_type min_required) const {
		size_type grown = capacity_ + capacity_ / 2;
		return grown > min_required ? grown : min_required;
	}

	// copied from llvm implementation
	static void destroy_range(T* first, T* last) noexcept {
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (; first != last; ++first) first->~T();
		}
	}

	// move the range [first, first + count) to dest and destroy the original elements
	static void relocate_range(T* dest, T* first, size_type count) noexcept {
		if constexpr (detail::is_relocatable_v<T>) {
			std::memcpy(static_cast<void*>(dest), static_cast<const void*>(first), count * sizeof(T));
		}
		else {
			// move element by element
			std::uninitialized_move(first,first + count, dest);
			destroy_range(first, first + count);
		}
	}

	static void uninitialized_copy_range(T* dest, const T* src, size_type count) {
		if constexpr (std::is_trivially_copy_constructible_v<T>) {
			std::memcpy(static_cast<void*>(dest), static_cast<const void*>(src), count * sizeof(T));
		}
		else {
			std::uninitialized_copy(src, src + count, dest);
		}
	}

	void make_gap(size_type index, size_type count) noexcept {
		T* p = data();
		if constexpr (detail::is_relocatable_v<T>) {
			std::memmove(static_cast<void*>(p + index + count), static_cast<const void*>(p + index), (size_ - index) * sizeof(T));
		}
		else {
			// tail is the number of elements to the right of index
			size_type tail = size_ - index;
			// if the gap is big enough, move everything to the right without intercepting itself
			if (tail <= count) relocate_range(p + index + count, p + index, tail);
			else
			{
				// the gap is not big enough.
				// 1. Move the last elements [size_ - count, size_) to the uninitialized [size_, size_ + count)
				// 2. Then, move the rest of the elements [index, size_ - count) to [size_ - count, size_)
				// 3. Finally, destroy the range.
				// NOTE: It is not enough a move_backward because the move_backward
				// requires initialized elements to move.
				std::uninitialized_move(p + size_ - count, p + size_, p + size_);
				std::move_backward(p + index, p + size_ - count, p + size_);
				destroy_range(p + index, p + index + count);
			}
		}
	}

	void grow(size_type new_cap) {
		if constexpr (detail::is_relocatable_v<T>) {
			if (is_small()) {
				T* new_data = static_cast<T*>(std::malloc(new_cap * sizeof(T)));
				if (!new_data) throw std::bad_alloc();
				std::memcpy(static_cast<void*>(new_data), static_cast<const void*>(stack_data()), size_ * sizeof(T));
				heapbuf_.ptr = new_data;
			}
			else {
				T* new_data = static_cast<T*>(std::realloc(heapbuf_.ptr, new_cap * sizeof(T)));
				if (!new_data) throw std::bad_alloc();
				heapbuf_.ptr = new_data;
			}
		}
		else {
			T* new_data = static_cast<T*>(std::malloc(new_cap * sizeof(T)));
			if (!new_data) throw std::bad_alloc();
			T* old = data();
			relocate_range(new_data, old, size_);
			free_heap();
			heapbuf_.ptr = new_data;
		}
		capacity_ = new_cap;
	}

	void free_heap() {
		if (!is_small()) std::free(heapbuf_.ptr);
	}

	void move_from(SmallVector& other) noexcept {
		if (other.is_small()) {
			relocate_range(stack_data(), other.stack_data(), other.size_);
			capacity_ = N;
		}
		else {
			heapbuf_.ptr = other.heapbuf_.ptr;
			capacity_ = other.capacity_;
		}
		size_ = other.size_;
		other.size_ = 0;
		other.capacity_ = N;
	}

public:
	// Commons
    bool is_small() const {
        return capacity_ <= N;
    }

    bool empty() const { return size_ == 0; }

    size_type size() const { return size_; }

    size_type capacity() const { return capacity_; }

    void reserve(size_type new_cap) {
        if (new_cap <= capacity_) return;
        grow(new_cap);
    }

    // Construction
    SmallVector() : size_(0), capacity_(N) {}

    explicit SmallVector(size_type count, value_param value = T()) : SmallVector() {
        resize(count, value);
    }

    // Copy
    SmallVector(const SmallVector& other) : SmallVector() {
        reserve(other.size_);
        uninitialized_copy_range(data(), other.data(), other.size_);
        size_ = other.size_;
    }

    SmallVector(SmallVector&& other) noexcept : SmallVector() {
        move_from(other);
    }

    SmallVector& operator=(const SmallVector& other) {
        if (this == &other) return *this;
        // destroy-then-copy: if T's copy constructor throws, the vector is left
        // empty but valid
        destroy_range(data(), data() + size_);
        size_ = 0;
        reserve(other.size_);
        uninitialized_copy_range(data(), other.data(), other.size_);
        size_ = other.size_;
        return *this;
    }

    SmallVector(std::initializer_list<T> init) : SmallVector() {
        reserve(init.size());
        uninitialized_copy_range(data(), init.begin(), init.size());
        size_ = init.size();
    }

    SmallVector& operator=(SmallVector&& other) noexcept {
        if (this == &other) return *this;
        destroy_range(data(), data() + size_);
        free_heap();
        move_from(other);
        return *this;
    }

    // Destruction
    ~SmallVector() {
        destroy_range(data(), data() + size_);
        free_heap();
    }


    // Element access
    T* data() {
        return is_small()? stack_data() : heapbuf_.ptr;
    }
    const T* data() const {
        return is_small()? stack_data() : heapbuf_.ptr;
    }

    reference operator[](size_type i) { return data()[i]; }
    const_reference operator[](size_type i) const { return data()[i]; }

    reference at(size_type i) {
        if(i >= size_) throw std::out_of_range("SmallVector::at");
        return data()[i];
    }
    const_reference at(size_type i) const {
        if(i >= size_) throw std::out_of_range("SmallVector::at");
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
    void clear() noexcept {
        destroy_range(data(), data() + size_);
        size_ = 0;
    }

    void push_back(const T& value) {
        // copy value before growing
        T tmp = value;
        if (size_ == capacity_) grow(next_capacity(capacity_ + 1));
        new (data() + size_) T(std::move(tmp));
        ++size_;
    }

    void push_back(T&& value) {
        T tmp = std::move(value);
        if (size_ == capacity_) grow(next_capacity(capacity_ + 1));
        new (data() + size_) T(std::move(tmp));
        ++size_;
    }

    void pop_back() noexcept {
        if(size_ > 0) {
            --size_;
            destroy_range(data() + size_, data() + size_ + 1);
        }
    }

    void resize(size_type new_size, value_param fill_value = T()) {
        if (new_size < size_) {
            destroy_range(data() + new_size, data() + size_);
        }
        else if (new_size > size_) {
            // copy value before growing
            T tmp = fill_value;
            if (new_size > capacity_) grow(next_capacity(new_size));
            T* p = data();
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::fill_n(p + size_, new_size - size_, tmp);
            }
        	else {
                std::uninitialized_fill_n(p + size_, new_size - size_, tmp);
            }
        }
        size_ = new_size;
    }

    void swap(SmallVector& other) noexcept {
        if (is_small() && other.is_small()) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                alignas(T) std::byte tmp_buf[N * sizeof(T)];
                std::memcpy(tmp_buf, stackbuf_, size_ * sizeof(T));
                std::memcpy(stackbuf_, other.stackbuf_, other.size_ * sizeof(T));
                std::memcpy(other.stackbuf_, tmp_buf, size_ * sizeof(T));
                std::swap(size_, other.size_);
            }
        	else {
                SmallVector tmp(std::move(*this));
                *this = std::move(other);
                other = std::move(tmp);
            }
        }
    	else if (!is_small() && !other.is_small()) {
            std::swap(heapbuf_.ptr, other.heapbuf_.ptr);
            std::swap(size_, other.size_);
            std::swap(capacity_, other.capacity_);
        }
    	else {
            SmallVector tmp(std::move(*this));
            *this = std::move(other);
            other = std::move(tmp);
        }
    }

    iterator insert(const_iterator pos, const T& value) {
        size_type index = check_insert_pos(pos);

        // copy value before growing
        T tmp = value;
        if (size_ == capacity_) grow(next_capacity(capacity_ + 1));

        make_gap(index, 1);
        T* p = data();
        new (p + index) T(std::move(tmp));
        ++size_;
        return p + index;
    }

    iterator insert(const_iterator pos, T&& value) {
        size_type index = check_insert_pos(pos);

        T tmp = std::move(value);
        if (size_ == capacity_) grow(next_capacity(capacity_ + 1));

        make_gap(index, 1);
        T* p = data();
        new (p + index) T(std::move(tmp));
        ++size_;
        return p + index;
    }

    iterator insert(const_iterator pos, size_type count, value_param value) {
        size_type index = check_insert_pos(pos);
        if (count == 0) return begin() + index;

        T tmp = value;

        if (size_ + count > capacity_) grow(size_ + count);

        make_gap(index, count);
        T* p = data();
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::fill_n(p + index, count, tmp);
        }
    	else {
            try {
                std::uninitialized_fill_n(p + index, count, tmp);
            }
    		catch (...) {
                // T's copy constructor threw halfway: the elements already
                // shifted past the gap would leave a hole, so drop everything
                // from the gap onwards (weak guarantee, same policy as folly's
                // undo_window)
                destroy_range(p + index + count, p + size_ + count);
                size_ = index;
                throw;
            }
        }
        size_ += count;
        return p + index;
    }

    template <typename InputIt>
	requires std::forward_iterator<InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        size_type index = check_insert_pos(pos);
        size_type count  = static_cast<size_type>(std::distance(first, last));

        if (count == 0) return begin() + index;

        if (size_ + count > capacity_) grow(size_ + count);

        make_gap(index, count);
        T* p = data();
        if constexpr (std::is_trivially_copyable_v<T>) {
            for(size_type i = 0; i < count; ++i, ++first) {
                p[index + i] = *first;
            }
        }
    	else {
            try {
                std::uninitialized_copy(first, last, p + index);
            } catch (...) {
                destroy_range(p + index + count, p + size_ + count);
                size_ = index;
                throw;
            }
        }
        size_ += count;
        return p + index;
    }

    iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }

};



} // namespace TU







