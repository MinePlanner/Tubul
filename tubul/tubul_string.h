//
// Created by Nicolas Loira on 16-03-23.
//


#pragma once
#include <vector>
#include <string_view>
#include <algorithm>

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

    template<typename ContainerType>
    std::string join(ContainerType const &container, std::string const &joiner);

    template<typename IteratorType>
    std::string join(IteratorType begin, IteratorType end, std::string const &joiner);


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
                    if (finish_ == std::string::npos) {
                        start_ = std::string::npos;
                        return;
                    }

                    start_ = finish_ + 1;
                    if ( start_ >= source_.size() ) {
                        start_ = std::string::npos;
                        finish_ = std::string::npos;
                        return;
                    }
                    finish_ = source_.find_first_of('\n', start_);
                    if (finish_ == std::string::npos)
                        finish_ = source_.size();
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
    using StringLineRange = details::string_line_range;
}