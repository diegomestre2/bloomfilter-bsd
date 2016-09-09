#pragma once

#include "adept.hpp"
#include "math.hpp"

#include <array>
#include <bitset>
#include <functional>

template<typename T, size_t N>
struct vec {
  static_assert(is_power_of_two(N), "Template parameter 'N' must be a power of two.");

  alignas(64) std::array<T, N> data;

  using type = vec<T, N>;
  using mask_t = std::bitset<N>;

  static mask_t make_all_mask() {
    mask_t m;
    return ~m;
  }

  static mask_t make_none_mask() {
    mask_t m;
    return m;
  }

  T& operator[](u64 index) {
    return data[index];
  }

  T operator[](u64 index) const {
    return data[index];
  }

  vec gather(const vec<$u64, N>& index) const {
    vec d;
    for (size_t i = 0; i < N; i++) {
      d.data[i] = data[index[i]];
    }
    return d;
  }

  vec gather(const vec<$u64, N>& index, const mask_t& mask) const {
    vec d;
    for (size_t i = 0; i < N; i++) {
      if (!mask[i]) continue;
      d.data[i] = data[index[i]];
    }
    return d;
  }

  void scatter(const vec<T, N>& what, const vec<$u64, N>& where) {
    for (size_t i = 0; i < N; i++) {
      data[where[i]] = what[i];
    }
  }

  void scatter(const vec<T, N>& what, const vec<$u64, N>& where, const mask_t& mask) {
    for (size_t i = 0; i < N; i++) {
      if (!mask[i]) continue;
      data[where[i]] = what[i];
    }
  }

  // conflict detection
  vec conflict_detection(const vec<T, N>& a) {
    // just a naive translation of the '_mm512_conflict_epi32'
    // intrinsic function.
    vec dst;
    for ($u64 j = 0; j < N; j++) {
      for ($u64 k = 0; k < j; k++) {
        u64 are_equal = a[j] == a[k];
        dst[j] |= are_equal << k;
      }
    }
    return dst;
  }


  // binary operators
  vec binary_operator(auto op, const vec& b) const  noexcept {
    vec d;
    for (size_t i = 0; i < N; i++) {
      d.data[i] = op(data[i], b.data[i]);
    }
    return d;
  }

  vec operator+(const vec& o) const noexcept { return binary_operator(std::plus<T>(), o); }
  vec operator-(const vec& o) const noexcept { return binary_operator(std::minus<T>(), o); }
  vec operator*(const vec& o) const noexcept { return binary_operator(std::multiplies<T>(), o); }
  vec operator/(const vec& o) const noexcept { return binary_operator(std::divides<T>(), o); }
  vec operator|(const vec& o) const noexcept { return binary_operator(std::bit_or<T>(), o); }
  vec operator^(const vec& o) const noexcept { return binary_operator(std::bit_xor<T>(), o); }
  vec operator&(const vec& o) const noexcept { return binary_operator(std::bit_and<T>(), o); }
  vec operator<<(const vec& o) const noexcept { return binary_operator(std::bit_shift_left<T>(), o); }
  vec operator>>(const vec& o) const noexcept { return binary_operator(std::bit_shift_right<T>(), o); }

  vec binary_operator(auto op, const T& b) const noexcept {
    vec d;
    for (size_t i = 0; i < N; i++) {
      d.data[i] = op(data[i], b);
    }
    return d;
  }

  vec operator+(const T& o) const noexcept { return binary_operator(std::plus<T>(), o); }
  vec operator-(const T& o) const noexcept { return binary_operator(std::minus<T>(), o); }
  vec operator*(const T& o) const noexcept { return binary_operator(std::multiplies<T>(), o); }
  vec operator/(const T& o) const noexcept { return binary_operator(std::divides<T>(), o); }
  vec operator|(const T& o) const noexcept { return binary_operator(std::bit_or<T>(), o); }
  vec operator^(const T& o) const noexcept { return binary_operator(std::bit_xor<T>(), o); }
  vec operator&(const T& o) const noexcept { return binary_operator(std::bit_and<T>(), o); }
  vec operator<<(const T& o) const noexcept { return binary_operator(std::bit_shift_left<T>(), o); }
  vec operator>>(const T& o) const noexcept { return binary_operator(std::bit_shift_right<T>(), o); }


  // unary operators
  vec unary_operator(auto op) const noexcept {
    vec d;
    for (size_t i = 0; i < N; i++) {
      d.data[i] = op(data[i]);
    }
    return d;
  }

  vec operator~() const noexcept { return binary_operator(std::bit_not<T>()); }


  // comparison operators / relational operators
  mask_t comparison_operator(auto op, const vec& b) const noexcept {
    mask_t mask;
    for (size_t i = 0; i < N; i++) {
      mask[i] = op(data[i], b.data[i]);
    }
    return mask;
  }
  mask_t operator<(const vec& o) const noexcept { return comparison_operator(std::less<T>(), o); }
  mask_t operator<=(const vec& o) const noexcept { return comparison_operator(std::less_equal<T>(), o); }
  mask_t operator==(const vec& o) const noexcept { return comparison_operator(std::equal_to<T>(), o); }
  mask_t operator!=(const vec& o) const noexcept { return comparison_operator(std::not_equal_to<T>(), o); }
  mask_t operator>=(const vec& o) const noexcept { return comparison_operator(std::greater_equal<T>(), o); }
  mask_t operator>(const vec& o) const noexcept { return comparison_operator(std::greater<T>(), o); }

  mask_t comparison_operator(auto op, const T& b) const noexcept {
    mask_t mask;
    for (size_t i = 0; i < N; i++) {
      mask[i] = op(data[i], b);
    }
    return mask;
  }
  mask_t operator<(const T& o) const noexcept { return comparison_operator(std::less<T>(), o); }
  mask_t operator<=(const T& o) const noexcept { return comparison_operator(std::less_equal<T>(), o); }
  mask_t operator==(const T& o) const noexcept { return comparison_operator(std::equal_to<T>(), o); }
  mask_t operator!=(const T& o) const noexcept { return comparison_operator(std::not_equal_to<T>(), o); }
  mask_t operator>=(const T& o) const noexcept { return comparison_operator(std::greater_equal<T>(), o); }
  mask_t operator>(const T& o) const noexcept { return comparison_operator(std::greater<T>(), o); }



  template<typename S>
  vec<S, N> cast() const noexcept {
    vec<S, N> d;
    for (size_t i = 0; i < N; i++) {
      d.data[i] = data[i];
    }
    return d;
  }

  template<size_t W>
  vec<T, W>* begin()  {
    return reinterpret_cast<vec<T, W>*>(data);
  }

  template<size_t W>
  vec<T, W>* end() const {
    return begin() + (N / W);
  }

  template<u64... Idxs>
  static constexpr vec<T, N> make_index_vector(integer_sequence<Idxs...>) {
    return {{ Idxs...}};
  }

  static constexpr vec<T, N> make_index_vector() {
    return make_index_vector(make_integer_sequence<N>());
  };


  // compound assignment operators
  vec& compound_assignment_operator(auto op, const vec& b) noexcept {
    for (size_t i = 0; i < N; i++) {
      data[i] = op(data[i], b.data[i]);
    }
    return *this;
  }

  vec& compound_assignment_operator(auto op, const vec& b, const mask_t& m) noexcept {
    for (size_t i = 0; i < N; i++) {
      data[i] = m[i] ? op(data[i], b.data[i]) : data[i];
    }
    return *this;
  }

  vec& operator+=(const vec& o) noexcept { return compound_assignment_operator(std::plus<T>(), o); }
  vec& assignment_plus(const vec& o, const mask_t& m) noexcept { return compound_assignment_operator(std::plus<T>(), o, m); }
  vec& operator-=(const vec& o) noexcept { return compound_assignment_operator(std::minus<T>(), o); }
  vec& operator|=(const vec& o) noexcept { return compound_assignment_operator(std::bit_or<T>(), o); }
  vec& operator^=(const vec& o) noexcept { return compound_assignment_operator(std::bit_xor<T>(), o); }
  vec& operator&=(const vec& o) noexcept { return compound_assignment_operator(std::bit_and<T>(), o); }


  vec& compound_assignment_operator(auto op, const T& b) noexcept {
    for (size_t i = 0; i < N; i++) {
      data[i] = op(data[i], b);
    }
    return *this;
  }

  vec& compound_assignment_operator(auto op, const T& b, const mask_t& m) noexcept {
    for (size_t i = 0; i < N; i++) {
      data[i] = m[i] ? op(data[i], b) : data[i];
    }
    return *this;
  }

  vec& operator+=(const T& o) noexcept { return compound_assignment_operator(std::plus<T>(), o); }
  vec& assignment_plus(const T& o, const mask_t& m) noexcept { return compound_assignment_operator(std::plus<T>(), o, m); }
  vec& operator-=(const T& o) noexcept { return compound_assignment_operator(std::minus<T>(), o); }
  vec& operator|=(const T& o) noexcept { return compound_assignment_operator(std::bit_or<T>(), o); }
  vec& operator^=(const T& o) noexcept { return compound_assignment_operator(std::bit_xor<T>(), o); }
  vec& operator&=(const T& o) noexcept { return compound_assignment_operator(std::bit_and<T>(), o); }
  vec& assignment_bit_and(const T& o, const mask_t& m) noexcept { return compound_assignment_operator(std::bit_and<T>(), o, m); }

};
