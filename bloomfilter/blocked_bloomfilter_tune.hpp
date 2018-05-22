#pragma once

#include <chrono>
#include <map>
#include <random>

#include <dtl/dtl.hpp>
#include <dtl/bloomfilter/block_addressing_logic.hpp>
#include <dtl/bloomfilter/blocked_bloomfilter_config.hpp>

#include "immintrin.h"

namespace dtl {


namespace {

//===----------------------------------------------------------------------===//
struct tuning_params {
  $u32 unroll_factor = 1;

  tuning_params() = default;
  ~tuning_params() = default;
  tuning_params(const tuning_params& other) = default;
  tuning_params(tuning_params&& other) = default;

  tuning_params& operator=(const tuning_params& rhs) = default;
  tuning_params& operator=(tuning_params&& rhs) = default;
};
//===----------------------------------------------------------------------===//

} // anonymous namespace


//===----------------------------------------------------------------------===//
/// Provides tuning parameters to the Bloom filter instance.
struct blocked_bloomfilter_tune {

  /// Sets the SIMD unrolling factor for the given blocked Bloom filter config.
  /// Note: unrolling by 0 means -> scalar code (no SIMD)
  virtual void
  set_unroll_factor(const blocked_bloomfilter_config& config,
                    u32 unroll_factor) {
    throw std::runtime_error("Not supported");
  }


  /// Returns the SIMD unrolling factor for the given blocked Bloom filter config.
  /// Note: unrolling by 0 means -> scalar code (no SIMD)
  virtual $u32
  get_unroll_factor(const blocked_bloomfilter_config& config) const {
    return 1; // default
  }


  /// Determines the best performing SIMD unrolling factor for the given
  /// blocked Bloom filter config.
  virtual $u32
  tune_unroll_factor(const blocked_bloomfilter_config& config) {
    throw std::runtime_error("Not supported");
  }


  /// Determines the best performing SIMD unrolling factor for all valid
  /// blocked Bloom filter configs.
  virtual void
  tune_unroll_factor() {
    throw std::runtime_error("Not supported");
  }

};
//===----------------------------------------------------------------------===//

} // namespace dtl