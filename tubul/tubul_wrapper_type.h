#pragma once

namespace TU::detail {

// Similar trick to StringId to wrap a fundamental type.
template <typename T, typename TagType>
struct Wrapper {
    // Just in case we need the wrapped type from the outside
    using WrappedType = T;

    explicit Wrapper(
        WrappedType val = WrappedType {})
        : m_value(val)
    {
    }

    operator WrappedType&() { return m_value; }
    operator const WrappedType&() const { return m_value; }

    constexpr auto value() const -> WrappedType { return m_value; }

    /* Define spaceship operator as default. This defines ordering and equality as that of the wrapped type */
    constexpr auto operator<=>(
        const Wrapper& other) const
        -> auto
        = default;

    WrappedType m_value;
};

template <typename T, typename TagType>
void swap(
    Wrapper<T, TagType>& lhs,
    Wrapper<T, TagType>& rhs) noexcept
{
    std::swap(lhs.m_value, rhs.m_value);
}
}


// Specialization for hash, so we can use ids directly on containers as if they were the
// wrapped type.
namespace std {
template <typename T, typename TagType>
struct hash<TU::detail::Wrapper<T, TagType>> {
    std::size_t operator()(
        const TU::detail::Wrapper<T, TagType>& val) const
    {
        return std::hash<T>()(val);
    }
};
}

// This macro can be used to easily create a wrapper around a basic type,
// just giving the name you want to use and the base type to wrap around.
// For example,
//       CREATE_WRAPPER(CatId, size_t);
//       CREATE_WRAPPER(DogId, size_t);
// will create two wrappers for size_t, which are essentially the same thing,
// but the type system will disallow nilly-willy conversions from one type to
// the other. This is particularly useful if you use a lot of elements by an
// id like a size_t index, and you are working with ids of different types and
// want to ensure the different ids don't get mixed up.
// The types can be cast back to size_t, or you can ask the value if you want
// to compare, but has to be done explicitly. I.e., you can't blame
// the language if you use a DogId as a CatId somewhere, because you actually have
// to write the code to do so.
#define CREATE_WRAPPER(NameType, BaseType) \
    using NameType = TU::detail::Wrapper<BaseType, struct NameType##_wrapper_tag>;
