#pragma once

#include <cstdlib>

#include <dtl/dtl.hpp>
#include <dtl/hash.hpp>
#include <dtl/math.hpp>
#include <dtl/mem.hpp>

#include <dtl/bloomfilter/block_addressing_logic.hpp>
#include <dtl/bloomfilter/blocked_cuckoofilter_block_logic.hpp>
#include <dtl/bloomfilter/blocked_cuckoofilter_multiword_table.hpp>
#include <dtl/bloomfilter/blocked_cuckoofilter_simd.hpp>
#include <dtl/bloomfilter/blocked_cuckoofilter_util.hpp>
#include <dtl/bloomfilter/blocked_cuckoofilter_word_table.hpp>


namespace dtl {
namespace cuckoofilter {

//===----------------------------------------------------------------------===//
// A blocked cuckoo filter template.
//===----------------------------------------------------------------------===//
template<
    typename __key_t = uint32_t,
    typename __block_t = blocked_cuckoofilter_block_logic<__key_t>,
    block_addressing __block_addressing = block_addressing::POWER_OF_TWO
>
struct blocked_cuckoofilter {

  using key_t = __key_t;
  using block_t = __block_t;
  using word_t = typename block_t::word_t;
  using hash_value_t = uint32_t;
  using hasher = dtl::hash::knuth_32_alt<hash_value_t>;
  using addr_t = block_addressing_logic<__block_addressing>;

  static constexpr u32 word_cnt_per_block = block_t::table_t::word_cnt;
  static constexpr u32 word_cnt_per_block_log2 = dtl::ct::log_2<word_cnt_per_block>::value;

  //===----------------------------------------------------------------------===//
  // Members
  //===----------------------------------------------------------------------===//
  const addr_t addr;
//  std::vector<block_t, dtl::mem::numa_allocator<block_t>> blocks;
  //===----------------------------------------------------------------------===//


  //===----------------------------------------------------------------------===//
  explicit
  blocked_cuckoofilter(const std::size_t length) : addr(length, block_t::block_bitlength) { }

  blocked_cuckoofilter(const blocked_cuckoofilter&) noexcept = default;

  blocked_cuckoofilter(blocked_cuckoofilter&&) noexcept = default;

  ~blocked_cuckoofilter() noexcept = default;
  //===----------------------------------------------------------------------===//


  //===----------------------------------------------------------------------===//
  __forceinline__
  void
  insert(word_t* __restrict filter_data, const key_t& key) const {
    const auto hash_val = hasher::hash(key);
    const auto block_idx = addr.get_block_idx(hash_val);
    const auto word_idx = block_idx << word_cnt_per_block_log2;
    auto block_ptr = &filter_data[word_idx];
    if ((addr.get_required_addressing_bits() + block_t::required_hash_bits) <= (sizeof(hash_value_t) * 8)) {
      block_t::insert_hash(block_ptr, hash_val << addr.get_required_addressing_bits());
    }
    else {
      block_t::insert_key(block_ptr, key);
    }
  }
  //===----------------------------------------------------------------------===//


  //===----------------------------------------------------------------------===//
  __forceinline__
  uint64_t
  batch_insert(word_t* __restrict filter_data, const key_t* keys, const uint32_t key_cnt) const {
    for (uint32_t i = 0; i < key_cnt; i++) {
      insert(filter_data, keys[i]);
    }
  }
  //===----------------------------------------------------------------------===//


  //===----------------------------------------------------------------------===//
  __forceinline__
  bool
  contains(const word_t* __restrict filter_data, const key_t& key) const {
    auto hash_val = hasher::hash(key);
    auto block_idx = addr.get_block_idx(hash_val);
    const auto word_idx = block_idx << word_cnt_per_block_log2;
    auto block_ptr = &filter_data[word_idx];
    if ((addr.get_required_addressing_bits() + block_t::required_hash_bits) <= (sizeof(hash_value_t) * 8)) {
      return block_t::contains_hash(block_ptr, hash_val << addr.get_required_addressing_bits());
    }
    else {
      return block_t::contains_key(block_ptr, key);
    }
  }
  //===----------------------------------------------------------------------===//


  //===----------------------------------------------------------------------===//
  /// Performs a batch-probe
  __forceinline__ __unroll_loops__ __host__
  std::size_t
  batch_contains(const word_t* __restrict filter_data,
                 const key_t* __restrict keys, u32 key_cnt,
                 $u32* __restrict match_positions, u32 match_offset) const {
    constexpr u32 mini_batch_size = 16;
    const u32 mini_batch_cnt = key_cnt / mini_batch_size;

    $u32* match_writer = match_positions;
    if ((addr.get_required_addressing_bits() + block_t::required_hash_bits) <= (sizeof(hash_value_t) * 8)) {

      for ($u32 mb = 0; mb < mini_batch_cnt; mb++) {
        for (uint32_t j = mb * mini_batch_size; j < ((mb + 1) * mini_batch_size); j++) {
          auto h = hasher::hash(keys[j]);
          auto i = addr.get_block_idx(h);
          auto w = i << word_cnt_per_block_log2;
          auto p = &filter_data[w];
          auto is_contained = block_t::contains_hash(p, h << addr.get_required_addressing_bits());
          *match_writer = j + match_offset;
          match_writer += is_contained;
        }
      }
      for (uint32_t j = (mini_batch_cnt * mini_batch_size); j < key_cnt; j++) {
        auto h = hasher::hash(keys[j]);
        auto i = addr.get_block_idx(h);
        auto w = i << word_cnt_per_block_log2;
        auto p = &filter_data[w];
        auto is_contained = block_t::contains_hash(p, h << addr.get_required_addressing_bits());
        *match_writer = j + match_offset;
        match_writer += is_contained;
      }
    }

    else {
      for ($u32 mb = 0; mb < mini_batch_cnt; mb++) {
        for (uint32_t j = mb * mini_batch_size; j < ((mb + 1) * mini_batch_size); j++) {
          auto k = keys[j];
          auto h = hasher::hash(k);
          auto i = addr.get_block_idx(h);
          auto w = i << word_cnt_per_block_log2;
          auto p = &filter_data[w];
          auto is_contained = block_t::contains_key(p, k);
          *match_writer = j + match_offset;
          match_writer += is_contained;
        }
      }
      for (uint32_t j = (mini_batch_cnt * mini_batch_size); j < key_cnt; j++) {
        auto k = keys[j];
        auto h = hasher::hash(k);
        auto i = addr.get_block_idx(h);
        auto w = i << word_cnt_per_block_log2;
        auto p = &filter_data[w];
        auto is_contained = block_t::contains_key(p, k);
        *match_writer = j + match_offset;
        match_writer += is_contained;
      }
    }
    return match_writer - match_positions;
  }
  //===----------------------------------------------------------------------===//


};
//===----------------------------------------------------------------------===//

} // namespace cuckoofilter


// TODO move somewhere else
static constexpr uint64_t cache_line_size = 64;


//===----------------------------------------------------------------------===//
// Cuckoo filter base class.
// using static polymorphism (CRTP)
//===----------------------------------------------------------------------===//
template<typename __key_t, typename __word_t, typename __derived>
struct blocked_cuckoofilter_base {

  using key_t = __key_t;
  using word_t = __word_t;

  //===----------------------------------------------------------------------===//
  __forceinline__ __host__ __device__
  void
  insert(word_t* __restrict filter_data, const key_t& key) {
    return static_cast<__derived*>(this)->filter.insert(filter_data, key);
  }
  //===----------------------------------------------------------------------===//


  //===----------------------------------------------------------------------===//
  __forceinline__ __host__
  uint64_t
  batch_insert(word_t* __restrict filter_data, const key_t* keys, const uint32_t key_cnt) {
    return static_cast<__derived*>(this)->filter.batch_insert(filter_data, keys, key_cnt);
  }
  //===----------------------------------------------------------------------===//


  //===----------------------------------------------------------------------===//
  __forceinline__ __host__ __device__
  bool
  contains(const word_t* __restrict filter_data, const key_t& key) const {
    return static_cast<const __derived*>(this)->filter.contains(filter_data, key);
  }
  //===----------------------------------------------------------------------===//


  //===----------------------------------------------------------------------===//
  __forceinline__  __host__
  uint64_t
  batch_contains(const word_t* __restrict filter_data,
                 const key_t* __restrict keys, const uint32_t key_cnt,
                 uint32_t* __restrict match_positions, const uint32_t match_offset) const {
    return static_cast<const __derived*>(this)->filter.batch_contains(filter_data, keys, key_cnt, match_positions, match_offset);
  };
  //===----------------------------------------------------------------------===//

};
//===----------------------------------------------------------------------===//


//===----------------------------------------------------------------------===//
// Instantiations of some reasonable cuckoo filters.
// Note, that not all instantiations are suitable for SIMD.
//===----------------------------------------------------------------------===//
template<uint32_t block_size_bytes, uint32_t bits_per_element, uint32_t associativity, block_addressing addressing>
struct blocked_cuckoofilter {};


template<uint32_t block_size_bytes, block_addressing addressing>
struct blocked_cuckoofilter<block_size_bytes, 16, 4, addressing>
    : blocked_cuckoofilter_base<uint32_t, uint64_t, blocked_cuckoofilter<block_size_bytes, 16, 4, addressing>> {

  using key_t = uint32_t;
  using word_t = uint64_t;
  using table_t = cuckoofilter::blocked_cuckoofilter_multiword_table<word_t, block_size_bytes, 16, 4>;
  using block_t = cuckoofilter::blocked_cuckoofilter_block_logic<key_t, table_t>;
  using filter_t = cuckoofilter::blocked_cuckoofilter<uint32_t, block_t, addressing>;

  filter_t filter; // the actual filter instance

  explicit blocked_cuckoofilter(const std::size_t length) : filter(length) { }

  // use SIMD implementation
  __forceinline__ uint64_t
  batch_contains(const key_t* __restrict keys, const uint32_t key_cnt,
                 uint32_t* __restrict match_positions, const uint32_t match_offset) const {
    return dtl::cuckoofilter::internal::simd_batch_contains_16_4(*this, keys, key_cnt, match_positions, match_offset);
  };

};


template<uint32_t block_size_bytes, block_addressing addressing>
struct blocked_cuckoofilter<block_size_bytes, 16, 2, addressing>
    : blocked_cuckoofilter_base<uint32_t, uint64_t, blocked_cuckoofilter<block_size_bytes, 16, 2, addressing>> {

  using key_t = uint32_t;
  using word_t = uint64_t;
  using table_t = cuckoofilter::blocked_cuckoofilter_multiword_table<word_t, block_size_bytes, 16, 2>;
  using block_t = cuckoofilter::blocked_cuckoofilter_block_logic<key_t, table_t>;
  using filter_t = cuckoofilter::blocked_cuckoofilter<uint32_t, block_t, addressing>;

  filter_t filter; // the actual filter instance

  explicit blocked_cuckoofilter(const std::size_t length) : filter(length) { }

};


template<uint32_t block_size_bytes, block_addressing addressing>
struct blocked_cuckoofilter<block_size_bytes, 12, 4, addressing>
    : blocked_cuckoofilter_base<uint32_t, uint64_t, blocked_cuckoofilter<block_size_bytes, 12, 4, addressing>> {

  using key_t = uint32_t;
  using word_t = uint64_t;
  using table_t = cuckoofilter::blocked_cuckoofilter_multiword_table<word_t, block_size_bytes, 12, 4>;
  using block_t = cuckoofilter::blocked_cuckoofilter_block_logic<key_t, table_t>;
  using filter_t = cuckoofilter::blocked_cuckoofilter<uint32_t, block_t, addressing>;

  filter_t filter; // the actual filter instance

  explicit blocked_cuckoofilter(const std::size_t length) : filter(length) { }

};


template<uint32_t block_size_bytes, block_addressing addressing>
struct blocked_cuckoofilter<block_size_bytes, 10, 6, addressing>
    : blocked_cuckoofilter_base<uint32_t, uint64_t, blocked_cuckoofilter<block_size_bytes, 10, 6, addressing>> {

  using key_t = uint32_t;
  using word_t = uint64_t;
  using table_t = cuckoofilter::blocked_cuckoofilter_multiword_table<word_t, block_size_bytes, 10, 6>;
  using block_t = cuckoofilter::blocked_cuckoofilter_block_logic<key_t, table_t>;
  using filter_t = cuckoofilter::blocked_cuckoofilter<uint32_t, block_t, addressing>;

  filter_t filter; // the actual filter instance

  explicit blocked_cuckoofilter(const std::size_t length) : filter(length) { }

};


template<uint32_t block_size_bytes, block_addressing addressing>
struct blocked_cuckoofilter<block_size_bytes, 8, 8, addressing>
    : blocked_cuckoofilter_base<uint32_t, uint64_t, blocked_cuckoofilter<block_size_bytes, 8, 8, addressing>> {

  using key_t = uint32_t;
  using word_t = uint64_t;
  using table_t = cuckoofilter::blocked_cuckoofilter_multiword_table<word_t, block_size_bytes, 8, 8>;
  using block_t = cuckoofilter::blocked_cuckoofilter_block_logic<key_t, table_t>;
  using filter_t = cuckoofilter::blocked_cuckoofilter<uint32_t, block_t, addressing>;

  filter_t filter; // the actual filter instance

  explicit blocked_cuckoofilter(const std::size_t length) : filter(length) { }

};


template<uint32_t block_size_bytes, block_addressing addressing>
struct blocked_cuckoofilter<block_size_bytes, 8, 4, addressing>
    : blocked_cuckoofilter_base<uint32_t, uint32_t, blocked_cuckoofilter<block_size_bytes, 8, 4, addressing>> {

  using key_t = uint32_t;
  using word_t = uint32_t;
  using table_t = cuckoofilter::blocked_cuckoofilter_multiword_table<word_t, block_size_bytes, 8, 4>;
  using block_t = cuckoofilter::blocked_cuckoofilter_block_logic<key_t, table_t>;
  using filter_t = cuckoofilter::blocked_cuckoofilter<uint32_t, block_t, addressing>;

  filter_t filter; // the actual filter instance

  explicit blocked_cuckoofilter(const std::size_t length) : filter(length) { }

  // use SIMD implementation
  __forceinline__ uint64_t
  batch_contains(const word_t* __restrict filter_data,
                 const key_t* __restrict keys, const uint32_t key_cnt,
                 uint32_t* __restrict match_positions, const uint32_t match_offset) const {
    return dtl::cuckoofilter::internal::simd_batch_contains_8_4(*this, keys, key_cnt, match_positions, match_offset);
  };

};
//===----------------------------------------------------------------------===//


} // namespace dtl