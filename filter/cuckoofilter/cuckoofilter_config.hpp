#pragma once

#include <dtl/dtl.hpp>
#include <dtl/filter/block_addressing_logic.hpp>

namespace dtl {
namespace cuckoofilter {


//===----------------------------------------------------------------------===//
struct config {
  $u32 bits_per_tag;
  $u32 tags_per_bucket;
  dtl::block_addressing addr_mode = dtl::block_addressing::POWER_OF_TWO;

  bool
  operator<(const config &o) const {
    return bits_per_tag < o.bits_per_tag
        || (bits_per_tag == o.bits_per_tag && tags_per_bucket  < o.tags_per_bucket)
        || (bits_per_tag == o.bits_per_tag && tags_per_bucket == o.tags_per_bucket && addr_mode  < o.addr_mode);
  }

  void print(std::ostream& os) const {
    std::stringstream str;
    str << "cf"
        << ",bits_per_tag=" << bits_per_tag
        << ",tags_per_bucket=" << tags_per_bucket
        << ",addr=" << (addr_mode == dtl::block_addressing::POWER_OF_TWO ? "pow2" : "magic");
    os << str.str();
  }

};
//===----------------------------------------------------------------------===//


} // namespace cuckoofilter
} // namespace dtl
