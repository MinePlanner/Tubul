//
// Created by Nicolas Loira on 16-03-23.
//


#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include "tubul_exception.h"
#include <sstream>
#include <functional>

////////////
// Strings
///////////

namespace TU {
/** String handling functions.
 * Split will return a vector of string_views with the tokens detected. Do note
 * there are 2 versions. If you don't pass the argument, the delimiter is assumed to be
 * a space and also consecutive spaces are considered one. When the delimiter is given
 * the consecutive delimiters are considered as if they are surrounding an empty string. This
 * can be very important when working with csv files.
 *
 * Join is the opposite to split, where you can pass the delimiters and a set of strings
 * to be merged, and returns the new resulting string. Do note you can pass iterators if
 * you don't want to merge a whole container.
 */
    std::vector<std::string_view> split(std::string const &input);

    std::vector<std::string_view> split(std::string const &input, std::string const &delims);

    std::vector<std::string_view> split(std::string_view const &input);

    std::vector<std::string_view> split(std::string_view const &input, std::string const &delims);

    //only for strings + joiner
    template <typename IteratorType>
    std::string join(IteratorType begin, IteratorType end, std::string const& joiner){
        using StringType = typename IteratorType::value_type;
        std::string result;
        //if we are given nothing to join, that's it.
        if (begin == end)
            return result;
        //Check the total size of the input strings.
        size_t total_size = 0;
        size_t total_items = std::distance(begin, end);
        std::for_each(begin,end,[&]( StringType const& it){ total_size+= it.size();});
        //If the sum is 0, means all strings are empty. Should we really do something?
        //I think it's debatable, but for example for CSV files, we would still want
        // the comma separated empty strings, so I prefer to continue, although some
        // cases are questionable.

        //If we only have one item, there's nothing more to do.
        if (total_items == 1)
        {
            result = *begin;
            return result;
        }
        //If the joiner is empty, we just concatenate the strings on the result
        if (joiner.empty() )
        {
            result.reserve(total_size );
            std::for_each(begin,end,[&]( StringType const& it){ result.append(it);});
            return result;
        }
        //Note that we already stablished there's at least 2 elements!
        result.reserve(total_size + total_items*joiner.size() );
        result.append(*begin);
        ++begin;
        std::for_each(begin,end,[&]( StringType const& it){ result.append(joiner); result.append(it); });

        return result;
    }
    
    //for any other container type, x should return a string.
    template <typename IteratorType, typename funcType>
    std::string join(IteratorType begin, IteratorType end, std::string const& joiner, funcType x){
        std::ostringstream buffer;

        using itemType = typename IteratorType::value_type;

        //if empty container
        if(begin == end) return buffer.str();
        
        buffer << x(*begin);
        begin++;

        std::for_each(begin, end, [&]( itemType const& it){
            buffer << joiner << x(it);
        });

        return buffer.str();
    }

    template <typename ContainerType>
    std::string join(ContainerType const& container, std::string const& joiner)
    {
        using itemType = typename ContainerType::value_type;

        //case string
        if constexpr (std::is_same_v<typename ContainerType::value_type, std::string> || std::is_same_v<typename ContainerType::value_type, std::string_view> )
            return join(std::begin(container), std::end(container), joiner );
        else
            return join(std::begin(container), std::end(container), joiner, [](const itemType& x) {return std::to_string(x);} );
        
    }

    //for any container and to_string lambda
    template <std::ranges::range ContainerType, typename funcType>
	std::string join(ContainerType const &container, std::string const &joiner, funcType x){
        return join(std::begin(container), std::end(container), joiner, x);
    }

    
    /** Commonly used functions to remove the trailing whitespace of strings
     * Do note, these only work on string views so be aware if passing strings!
     */
    namespace details {
        static const std::string WHITESPACE = " \n\r\t\f\v";
    }

    inline
    std::string_view ltrim(const std::string_view s) {
        size_t start = s.find_first_not_of(details::WHITESPACE);
        return (start == std::string::npos) ? std::string_view{} : s.substr(start);
    }

    inline
    std::string_view rtrim(const std::string_view s) {
        size_t end = s.find_last_not_of(details::WHITESPACE);
        return (end == std::string::npos) ? std::string_view{} : s.substr(0, end + 1);
    }

    inline
    std::string_view trim(const std::string_view &s) {
        std::string_view toTrim(s);
        return rtrim(ltrim(toTrim));
    }
    inline
    std::string_view ltrim(std::string&& s) { throw Exception("trim functions can't be used with lvalue strings");}

    inline
    std::string_view rtrim(std::string&& s) { throw Exception("trim functions can't be used with lvalue strings");}

    inline
    std::string_view trim(std::string&& s) { throw Exception("trim functions can't be used with lvalue strings");}


    /** String manipulating functions
     * It's very common to need the "tolower"/"toupper" functions to strings, so let's
     * write them just once.
     */
    inline
    auto tolower(std::string_view word) -> std::string {
        std::string data(word);
        std::transform(data.begin(), data.end(), data.begin(),
                       [](const unsigned char c) { return std::tolower(c); });
        return data;
    }
    inline
    auto tolower(const std::string& word) -> std::string {
        return tolower(std::string_view(word) );
    }

    inline
    auto toupper(std::string_view word) -> std::string {
        std::string data(word);
        std::transform(data.begin(), data.end(), data.begin(),
                       [](const unsigned char c) { return std::toupper(c); });
        return data;
    }

    inline
    auto toupper(const std::string& word) -> std::string {
        return toupper(std::string_view(word) );
    }

    namespace details {
        class string_line_range {
        private:
            class iter {
            public:
                explicit iter(std::string_view s) :
                        source_(s) {
                    if (s.empty()) {
                        start_ = finish_ = std::string::npos;
                    } else {
                        start_ = 0;
                        finish_ = source_.find_first_of('\n', start_);
                        if (finish_ >= 1 && source_[finish_-1] == '\r')
                            finish_ -= 1;
                    }

                }

                explicit iter(std::string_view s, std::string::size_type) :
                        source_(s),
                        start_(std::string::npos),
                        finish_(std::string::npos) {
                }

                bool operator!=(iter const &other) const {
                    return source_.data() != other.source_.data() ||
                           source_.size() != other.source_.size() ||
                           start_ != other.start_ ||
                           finish_ != other.finish_;
                }

                std::string_view operator*() const {
                    auto real_start = source_.data() + start_;
                    return std::string_view{real_start, finish_ - start_ };
                }

                iter &operator++() {
                    find_next_line();
                    return *this;
                }

            private:
                void find_next_line() {
                    //If we were already past the end of the source in the current
                    //iteration, there's no next line possible.
                    if (finish_ == std::string::npos) {
                        start_ = std::string::npos;
                        return;
                    }

                    //If we had a finish_ pointing to a \r or \n, we move past it
                    //for the next iteration, but we have to also check if the \r and
                    //or \n were the last characters from the source.
                    size_t fwd = 1;
                    if ( finish_ < source_.size() and source_[finish_] == '\r' )
                        ++fwd;
                    start_ = finish_ + fwd;
                    if ( start_ >= source_.size() ) {
                        start_ = std::string::npos;
                        finish_ = std::string::npos;
                        return;
                    }

                    //We know there are more characters after the start_ index, so we look
                    //for the next \n available (and check if it is preceded by a \r just in
                    // case)
                    finish_ = source_.find_first_of('\n', start_);
                    if (finish_ == std::string::npos)
                        finish_ = source_.size();
                    else if ( source_[finish_-1] == '\r' )
                        finish_ -= 1;
                }

                std::string_view source_;
                std::string::size_type start_;
                std::string::size_type finish_;
            };

        public:
            explicit string_line_range(const std::string &s) :
                    source_(s) {}

            explicit string_line_range(const std::string_view s) :
                    source_(s) {}

            iter begin() { return iter(source_); }

            iter end() { return iter(source_, std::string::npos); }

            const std::string_view source_;
        };
    }//namespace details

    details::string_line_range slinerange(const std::string& s);
    details::string_line_range slinerange(const std::string_view s);
    using StringLineRange = details::string_line_range;
}
