//
// Created by Carlos Acosta on 16-01-23.
//

#pragma once
#include <concepts>
#include <assert.h>

namespace TU{
namespace details{

	template <std::integral T>
	class range {
	private:
		class iter {
		public:
			iter(T at) : at_(at) {}
			bool operator!=(iter const& other) const { return at_ != other.at_; }
			T const& operator*() const { return at_; }
			iter& operator++() { ++at_; return *this; }
		private:
			T at_;
		};

	public:
		range(T begin, T end) :
			begin_val_(begin),
			end_val_(end)
		{ }
		iter begin() { return iter(begin_val_); }
		iter end() { return iter(end_val_); }

		T begin_val_;
		T end_val_;
	};

	template <std::integral T>
	class skip_range {
	private:
		class iter {
		public:
			iter(T at, T step) :
				at_(at),
				step_(step)
			{}
			bool operator!=(iter const& other) const { return at_ != other.at_; }
			T const& operator*() const { return at_; }
			iter& operator++() { at_+=step_; return *this; }
		private:
			T at_;
			T step_;
		};

	public:
		skip_range(T begin, T end, T step) :
			begin_val_(begin),
			end_val_(end),
			step_val_(step)
		{  assert( (end-begin)%step == 0);}
		iter begin() { return iter(begin_val_, step_val_); }
		iter end() { return iter(end_val_, step_val_); }

		T begin_val_;
		T end_val_;
		T step_val_;
	};
}

using tubul_range = details::range<size_t>;
using tubul_skip_range = details::skip_range<size_t>;

}