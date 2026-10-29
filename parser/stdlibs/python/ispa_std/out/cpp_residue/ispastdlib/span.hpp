/**
 * ispastdlib/span.hpp - Span<T> (C++17 span) and container operator<<
 * Part of the split ISPA std lib; include <ispastdlib.hpp> for everything.
 */
#pragma once
#include <ispastdlib/core.hpp>

namespace ISPA_STD {
    // prior C++20 span type
    template<typename T>
    class Span {
    public:
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;

        Span() : data_(nullptr), size_(0) {}

        Span(pointer data, size_type size)
            : data_(data), size_(size) {}

        template<std::size_t N>
        Span(T (&arr)[N]) // from raw array
            : data_(arr), size_(N) {}
        // Robust converting constructor (like std::span)
        template<typename U,
                 typename = std::enable_if_t<
                     std::is_convertible_v<U (*)[], T (*)[]>
                 >
        >
        Span(const Span<U>& other) noexcept
            : data_(other.data()), size_(other.size()) {}
        [[nodiscard]] pointer data() const { return data_; }
        [[nodiscard]] size_type size() const { return size_; }
        [[nodiscard]] bool empty() const { return size_ == 0; }
        reference operator[](size_type index) {
            return data_[index];
        }
        const_reference operator[](size_type index) const {
            return data_[index];
        }
        reference at(size_type index) const {
            if (index >= size_) throw std::out_of_range("Span::at");
            return data_[index];
        }

        pointer begin() const { return data_; }
        pointer end() const { return data_ + size_; }
    private:
        pointer data_;
        size_type size_;
    };
    template<typename T>
    std::ostream& operator<<(std::ostream& os, const Span<T>& span) {
        os << "[";
        for (auto i = 0; i < span.size(); ++i) {
            if (i != 0) os << ", ";
            os << span[i];
        }
        os << "]";
        return os;
    }
    template<typename T, std::size_t N>
    std::ostream& operator<<(std::ostream& os, const std::array<T, N>& arr) {
        os << "[";
        for (auto i = 0; i < arr.size(); ++i) {
            if (i != 0) os << ", ";
            os << arr[i];
        }
        os << "]";
        return os;
    }
    template<typename T>
    std::ostream& operator<<(std::ostream& os, const std::vector<T>& arr) {
        os << "[";
        for (auto i = 0; i < arr.size(); ++i) {
            if (i != 0) os << ", ";
            os << arr[i];
        }
        os << "]";
        return os;
    }
} // namespace ISPA_STD
