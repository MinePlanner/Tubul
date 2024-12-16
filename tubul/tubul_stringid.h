#include <string>


namespace TU{

/** 
  * This class provides a string-like identifier that is typed on a tag-type class.
  * This means that you can create classes that are basically strings but can't be 
  * mixed with other StringId classes with different tags.
  * Example:
  *   struct ingredient_tag[{};
  *   struct name_tag[{};
  *   using IngredientName = StringId<ingredient_tag>;
  *   using PersonName = StringId<name_tag>;
  * 
  *   IngredientName c("Basil");
  *   PersonaName p = c ; //Can't be done!
  *
  *   std::map<IngredientName, float> ingredientsForCake;
  *
  */
    template<class TagType>
    struct StringId
    {
        //Not explicit!
        StringId(std::string s): _value(std::move(s)) {}
        StringId(): _value() {}

        const std::string& value() const {
            return _value;
        }

    private:
        std::string _value;

        friend bool operator == (const StringId& l, const StringId& r) {
            return l._value == r._value;
        }
        //This one is not truly needed, but it's nice to have to avoid
        //writing foo.value() == "bar" and you can just use foo = "bar"
        friend bool operator == (const StringId& l, const char* r) {
            return l._value == r;
        }

        //Tt will only compare with the same tag!
        friend bool operator < (const StringId& l, const StringId& r) {
            return l._value < r._value;
        }
        friend bool operator < (const StringId& l, const char* r) {
            return l._value < r;
        }

        //Also some extra functions to keep the illusion of a normal std::string
        friend
        auto to_string(const StringId& r)
        -> const std::string&
        {
            return r._value;
        }

        friend
        auto operator << (std::ostream& os, const StringId& sid)
        -> std::ostream&
        {
            return os << sid.value();
        }

        friend
        std::size_t hash_code(const StringId& sid)
        {
            std::size_t seed = typeid(TagType).hash_code();
            seed ^= std::hash<std::string>()(sid._value);
            return seed;
        }
    };

} //end namespace TU

//Overloading std::hash for ease of use on hash/sets and other shennanigans 
namespace std {
    template<class Tag>
    struct hash<TU::StringId<Tag>>
    {
        using argument_type = TU::StringId<Tag>;
        using result_type = std::size_t;

        result_type operator()(const argument_type& arg) const {
            return hash_code(arg);
        }
    };
}


