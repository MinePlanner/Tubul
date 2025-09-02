//
// Created by Carlos Acosta on 19-10-23.
//


#pragma once
#include <vector>
#include <tuple>
#include <ranges>

namespace TU {

    /** This is basically the same std concept to check that something is an actual
     * range that can be iterated with begin() and end().
     */
    template<typename C>
    concept TubulIterable = std::ranges::range<C>;

    /** This templated function receives an standard iterable container (via begin/end)
	 * and returns a simple wrapper that will return tuples (i,item), where i is the index
	 * of item inside the container. This follows the same idea as Python's enumerate.
	 * The template type T is the container to be iterated.
	 * Because this is built using the std::ranges library, this should be composable with
	 * other ranges without too much problem.
	 */
	template <std::ranges::input_range T>
	constexpr auto enumerate(T&& iterable)
	{
		size_t aux = 0;
		return std::views::all(std::forward<T>(iterable) |
			std::ranges::views::transform(
				[idx=aux](const auto& i) mutable
				{
					return std::make_tuple(idx++,i);
				})
			);
	}

    /** This was originally influenced by the zip view implementation at https://github.com/alemuntoni/zip-views
     * It has several changes to allow compliance with std::ranges and allow composition for
  	 * example with std::views::filter and newer changes to simplify a bit the handling of
	 * iterators, although it's still quite complicated. The base idea is that we create
	 * a range that contains a tuple of views to all the passed ranges. From that, we create
	 * iterators that contain a tuple of real iterators from each container, and another of the
	 * end iterators. When comparing with the sentinel (as it is done to loop over all the range)
	 * we simply have to check if any of the iterators is equal to its corresponding end.
	**/
	template <std::ranges::input_range... T>
	struct zipper_view: public std::ranges::view_interface<zipper_view<T...>>
	{
		template<bool IsConst>
		struct zip_iterator_impl
		{
			//Because ranges do something weird with the const-ness, we have to go to extreme lengths to
			//ensure we are getting a real const-ref to the underlying item of each range.
			//For this we will do the following template trickery
			// get the range_reference_t ->  remove the ref -> add const to the type -> add a ref.
			// using value_type        = std::tuple<std::ranges::range_reference_t<T&>...>;
			using value_type = std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t< std::ranges::range_reference_t<T&>>>>...>;
			using reference         = value_type;
			using iterator_category = std::input_iterator_tag;
			using difference_type   = std::ptrdiff_t;
			using zip_iter_tuple = std::conditional_t<IsConst,
				std::tuple<decltype(std::ranges::begin(std::declval<const T&>()))...>,
				std::tuple<decltype(std::ranges::begin(std::declval<T&>()))...>>;
			using zip_sent_tuple = std::conditional_t<IsConst,
				std::tuple<decltype(std::ranges::end(std::declval<const T&>()))...>,
				std::tuple<decltype(std::ranges::end(std::declval<T&>()))...>>;

			zip_iter_tuple its_tuple;
			zip_sent_tuple ends_tuple;

			auto operator++() -> zip_iterator_impl&
			{
				std::apply([](auto &&...args)
						   { ((++args), ...); },
						   its_tuple);
				return *this;
			}

			auto operator++(int) -> zip_iterator_impl
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}

			auto operator*() const -> reference
			{
				return std::apply([](auto &&...args)
								  { return reference(*args...); },
								  its_tuple);
			}
			template <std::size_t... I>
			bool check_end(std::index_sequence<I...>) const
			{
				return ((std::get<I>(its_tuple) == std::get<I>(ends_tuple)) || ...);
			}
			// Sentinel comparison: directly check if any iterator reached the end of its container
			bool operator==(std::default_sentinel_t) const
			{
				return check_end(std::index_sequence_for<T...>{});
			}
			// Because the iterator is just a pair of tuples, we can just compare member-wise.
			bool operator==(const zip_iterator_impl &) const = default;
			auto operator!=(zip_iterator_impl const &other) const
			{
				return not this->operator==(other);
			}
		};

		using zip_iterator = zip_iterator_impl<false>;
		using const_zip_iterator = zip_iterator_impl<true>;

		std::tuple<std::ranges::views::all_t<T>...> iterables;

		explicit zipper_view(T &&...rs) :
			iterables(std::views::all(std::forward<T>(rs))...)
		{
		}
		// Helper: get begin iterator tuple, const version using cbegin/cend
		auto begin_impl() const
		{
			return std::apply([](auto &...rs)
							  { return std::tuple(std::ranges::begin(rs)...); },
							  iterables);
		}
		auto end_impl() const
		{
			return std::apply([](auto &...rs)
							  { return std::tuple(std::ranges::end(rs)...); },
							  iterables);
		}
		// Helper: get begin iterator tuple, non-const version using begin/end
		auto begin_impl()
		{
			return std::apply([](auto &...rs)
							  { return std::tuple(std::ranges::begin(rs)...); },
							  iterables);
		}
		auto end_impl()
		{
			return std::apply([](auto &...rs)
							  { return std::tuple(std::ranges::end(rs)...); },
							  iterables);
		}

		//Selecting const and non-const iterator accordingly. End is always the same
		//and doesn't really have trouble with const-ness.
		auto begin() const { return const_zip_iterator{begin_impl(), end_impl()}; }
		auto begin() { return zip_iterator{begin_impl(), end_impl()}; }
		auto end() const { return std::default_sentinel; }
	};
	// Deduction guide
	template <typename... Ranges>
	zipper_view(Ranges &&...) -> zipper_view<Ranges...>;

	/** This function will return a view object to allow iterating a set of collections
	 * in such a way that the first item of ths view is a tuple containing refs to
	 * the first element of each collection. The second item of the view is a tuple
	 * of refs to the second element of each collection and so on. The view will keep
	 * going until any of the underlying collections reaches its end iterator.
	 */
    template <std::ranges::input_range... T>
    constexpr auto zip(T&& ...  iterables) {
        return zipper_view( std::forward<T>(iterables)... );
    }


}// end namespace TU



