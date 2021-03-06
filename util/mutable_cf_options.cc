//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "util/mutable_cf_options.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <limits>
#include <cassert>
#include <string>
#include "port/port.h"
#include "vidardb/env.h"
#include "vidardb/options.h"
#include "vidardb/immutable_options.h"

namespace vidardb {

// Multiple two operands. If they overflow, return op1.
uint64_t MultiplyCheckOverflow(uint64_t op1, int op2) {
  if (op1 == 0) {
    return 0;
  }
  if (op2 <= 0) {
    return op1;
  }
  uint64_t casted_op2 = (uint64_t) op2;
  if (std::numeric_limits<uint64_t>::max() / op1 < casted_op2) {
    return op1;
  }
  return op1 * casted_op2;
}

void MutableCFOptions::RefreshDerivedOptions(
    const ImmutableCFOptions& ioptions) {
  max_file_size.resize(ioptions.num_levels);
  for (int i = 0; i < ioptions.num_levels; ++i) {
    if (i == 0 && ioptions.compaction_style == kCompactionStyleUniversal) {
      max_file_size[i] = ULLONG_MAX;
    } else if (i > 1) {
      max_file_size[i] = MultiplyCheckOverflow(max_file_size[i - 1],
                                               target_file_size_multiplier);
    } else {
      max_file_size[i] = target_file_size_base;
    }
  }
}

uint64_t MutableCFOptions::MaxFileSizeForLevel(int level) const {
  assert(level >= 0);
  assert(level < (int)max_file_size.size());
  return max_file_size[level];
}
uint64_t MutableCFOptions::MaxGrandParentOverlapBytes(int level) const {
  return MaxFileSizeForLevel(level) * max_grandparent_overlap_factor;
}
uint64_t MutableCFOptions::ExpandedCompactionByteSizeLimit(int level) const {
  return MaxFileSizeForLevel(level) * expanded_compaction_factor;
}

void MutableCFOptions::Dump(Logger* log) const {
  // Memtable related options
  Log(log, "                        write_buffer_size: %" VIDARDB_PRIszt,
      write_buffer_size);
  Log(log, "                  max_write_buffer_number: %d",
      max_write_buffer_number);
  Log(log, "                         arena_block_size: %" VIDARDB_PRIszt,
      arena_block_size);
  Log(log, "                 disable_auto_compactions: %d",
      disable_auto_compactions);
  Log(log, "       level0_file_num_compaction_trigger: %d",
      level0_file_num_compaction_trigger);
  Log(log, "           max_grandparent_overlap_factor: %d",
      max_grandparent_overlap_factor);
  Log(log, "               expanded_compaction_factor: %d",
      expanded_compaction_factor);
  Log(log, "                 source_compaction_factor: %d",
      source_compaction_factor);
  Log(log, "                    target_file_size_base: %" PRIu64,
      target_file_size_base);
  Log(log, "              target_file_size_multiplier: %d",
      target_file_size_multiplier);
  Log(log, "                 max_bytes_for_level_base: %" PRIu64,
      max_bytes_for_level_base);
  Log(log, "           max_bytes_for_level_multiplier: %d",
      max_bytes_for_level_multiplier);
  std::string result;
  char buf[10];
  for (const auto m : max_bytes_for_level_multiplier_additional) {
    snprintf(buf, sizeof(buf), "%d, ", m);
    result += buf;
  }
  result.resize(result.size() - 2);
  Log(log, "max_bytes_for_level_multiplier_additional: %s", result.c_str());
  Log(log, "           verify_checksums_in_compaction: %d",
      verify_checksums_in_compaction);
}

}  // namespace vidardb
