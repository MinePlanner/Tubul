
#pragma once
#include <unordered_map>
#include <string_view>
#include <string>
#include <vector>
#include <ranges>

namespace TU
{
/**
 * /brief This class provides a 2-way map between strings and numeric id's. The idea is that you
 *      can defer the ownership of the strings to this class, and then query for string id's to use just the id instead
 *      of the full string.
 * This is a variation of the "original" string index that tries a more tightly controlled
 * approach to how to store the strings that are indexed in this structure. The normal approach
 * would be to keep the strings in the vector (as in the v1), but that creates a lot of small
 * allocations to nicely store each single string stored. This version uses a single big buffer
 * where all strings are stored consecutively like a very long chunk of text. The vector where
 * we were storing the strings, now stores something similar to a string_view that points to the
 * specific portion of the buffer that contains the characters that forms the string.
 * A couple of extra changes are required to keep compatibility to check for string_views coming
 * from the outside (remember that string->string_view conversion is trivial).
 * Also do note that this structure immediately pre-allocates some space to store the strings.
 */
struct StringIndex
{
	static constexpr size_t POOL_STARTING_SIZE = 10 * 1024 * 1024; // 10mb
	using Id                                   = size_t;
	StringIndex();

	explicit StringIndex(size_t bufferSize);

	/**
	 * /brief Adds a string to the index. Do note this is added for performance as it doesn't validate the existence
	 *  of val in the index. If you are unsure, use tryGetId instead
	 * @param val value to be added
	 * @return the id assigned to the added value.
	 */
	Id addStr(std::string_view val);

	/**
	 * /brief Tries to recover the id for val, but if val is new in this index it will be added and then will return
	 * the newly assigned index.
	 * @param val value to be added/queried
	 * @return the id assigned to val.
	 */
	Id tryGetId(std::string_view val);

	/**
	 * /brief Gets id of val. Do note that at this point, if val is not added, it will throw an exception
	 * @param val value to be queried
	 * @return the id assigned to val.
	 */
	Id getId(std::string_view val) const;

	/**
	 * /brief Gets string assigned to id. Do note at this point, if id is not added, it will throw.
	 * @param val value to be added/queried
	 * @return a reference to the string assigned to id.
	 */
	std::string_view getStr(size_t id) const;

	/**
	 * /brief Checks if a given string is already contained.
	 * @param val value to be added/queried
	 * @return true if string is contained.
	 */
	bool contains(const std::string_view& val) const;

	/** \brief Returns a range with the strings contained in this index */
	auto strings() const;

	/** \brief Returns a range with the ids of the strings stored in this index */
	auto keys() const;
	/**
	 * \brief Clears all internal structures and releases the memory used.
	 */
	void clear();

	/** \brief size of this index, in other words, how many strings are stored here */
	size_t size() const;

	/** \brief size in bytes of the buffer used to store the strings.*/
	size_t bufferCurrentSize() const;

private:
	using StringPool = std::vector<char>;

	// String_view-like object, that points to a given index in a buffer and the size
	// of the string that starts at that index. This object _does not_ hold the reference
	// to the pool actually containing the data, as it is expected this is just valid
	// along the pool containing the related data, so these objects should never leak outside
	// this StringIndex.
	struct StringPoolElement
	{
		size_t start;
		size_t size;

		bool operator==(StringPoolElement const & other) const
		{
			return start == other.start && size == other.size;
		}
		bool operator!=(StringPoolElement const & other) const
		{
			return not(*this == other);
		}
	};

	// Build a string_view from a StringPoolElement and a pool
	static std::string_view strView(StringPoolElement const & e, StringPool const & pool)
	{
		return std::string_view(pool.data() + e.start, e.size);
	}

	// Build a string_view from the StringPoolElement owned by this index.
	std::string_view strView(StringPoolElement const & e) const
	{
		return strView(e, m_pool);
	}

	// Very simple hasher for string and string_view, with extra support for the
	// PooledStrings (assuming we have a pool first). This will let us use find
	// directly either with string, string_views and/or StringPoolElements
	struct PooledStringHasher
	{
		using is_transparent = void; // Tells unordered_map we support heterogeneous lookup

		StringPool const & pool;

		template<typename StringType>
		size_t easyHash(StringType const & str) const
		{
			return std::hash<StringType>()(str);
		}

		size_t operator()(StringPoolElement const & e) const
		{
			return easyHash(strView(e, pool));
		}
		size_t operator()(std::string const & str) const
		{
			return easyHash(str);
		}
		size_t operator()(std::string_view strv) const
		{
			return easyHash(strv);
		}
	};

	// Equivalent structure to the hasher to handle equality. The sad part, is that I didn't
	// find a concise way to write this without handling the combinations explicitly.
	struct PooledStringEqual
	{
		using is_transparent = void;

		StringPool const & pool;

		bool operator()(std::string_view lhs, std::string_view rhs) const
		{
			return lhs == rhs;
		}
		bool operator()(std::string_view lhs, std::string const & rhs) const
		{
			return lhs == rhs;
		}
		bool operator()(std::string_view lhs, StringPoolElement rhs) const
		{
			return lhs == strView(rhs, pool);
		}

		bool operator()(std::string const & lhs, std::string_view rhs) const
		{
			return lhs == rhs;
		}
		bool operator()(std::string const & lhs, std::string const & rhs) const
		{
			return lhs == rhs;
		}
		bool operator()(std::string const & lhs, StringPoolElement rhs) const
		{
			return lhs == strView(rhs, pool);
		}

		bool operator()(StringPoolElement lhs, std::string_view rhs) const
		{
			return strView(lhs, pool) == rhs;
		}
		bool operator()(StringPoolElement lhs, std::string const & rhs) const
		{
			return strView(lhs, pool) == rhs;
		}
		bool operator()(StringPoolElement lhs, StringPoolElement rhs) const
		{
			return lhs.start == rhs.start and lhs.size == rhs.size;
		}
	};

	// Buffer where we will actually store the different strings.
	StringPool m_pool;
	// This is the string->id map, but uses the StringPoolElement as that is what we need as key
	// so the hash and equal must be customized to take this into account. This has the extra
	// functionality that you can use find/counts with strings and string_view transparently,
	// but we will only store the StringPoolElement and ids, making the nodes easier to deallocate.
	std::unordered_map<StringPoolElement, size_t, PooledStringHasher, PooledStringEqual> m_strId;
	// Main index of the strings contained in the pool. The StringPoolElements are pods so
	// they should not incur in any extra allocation costs.
	std::vector<StringPoolElement> m_idStr;
};

inline
auto StringIndex::strings() const
{
	return m_idStr |
		std::views::transform([&](const StringPoolElement & e) -> std::string_view
		{
			return strView(e);
		});
}

inline
auto StringIndex::keys() const
{
	return std::views::iota(size_t{0}, m_idStr.size());
}

inline
auto StringIndex::size() const -> size_t
{
	return m_idStr.size();
}

}
