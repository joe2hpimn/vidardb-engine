//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once
#include <vector>
#include "db/write_thread.h"
#include "vidardb/types.h"
#include "vidardb/write_batch.h"
#include "vidardb/db.h"
#include "vidardb/options.h"

namespace vidardb {

class MemTable;
class FlushScheduler;
class ColumnFamilyData;

class ColumnFamilyMemTables {
 public:
  virtual ~ColumnFamilyMemTables() {}
  virtual bool Seek(uint32_t column_family_id) = 0;
  // returns true if the update to memtable should be ignored
  // (useful when recovering from log whose updates have already
  // been processed)
  virtual uint64_t GetLogNumber() const = 0;
  virtual MemTable* GetMemTable() const = 0;
  virtual ColumnFamilyHandle* GetColumnFamilyHandle() = 0;
  virtual ColumnFamilyData* current() { return nullptr; }
};

class ColumnFamilyMemTablesDefault : public ColumnFamilyMemTables {
 public:
  explicit ColumnFamilyMemTablesDefault(MemTable* mem)
      : ok_(false), mem_(mem) {}

  bool Seek(uint32_t column_family_id) override {
    ok_ = (column_family_id == 0);
    return ok_;
  }

  uint64_t GetLogNumber() const override { return 0; }

  MemTable* GetMemTable() const override {
    assert(ok_);
    return mem_;
  }

  ColumnFamilyHandle* GetColumnFamilyHandle() override { return nullptr; }

 private:
  bool ok_;
  MemTable* mem_;
};

// WriteBatchInternal provides static methods for manipulating a
// WriteBatch that we don't want in the public WriteBatch interface.
class WriteBatchInternal {
 public:

  // WriteBatch header has an 8-byte sequence number followed by a 4-byte count.
  static const size_t kHeader = 12;

  // WriteBatch methods with column_family_id instead of ColumnFamilyHandle*
  static void Put(WriteBatch* batch, uint32_t column_family_id,
                  const Slice& key, const Slice& value);

  static void Put(WriteBatch* batch, uint32_t column_family_id,
                  const SliceParts& key, const SliceParts& value);

  static void Delete(WriteBatch* batch, uint32_t column_family_id,
                     const SliceParts& key);

  static void Delete(WriteBatch* batch, uint32_t column_family_id,
                     const Slice& key);

  static void MarkEndPrepare(WriteBatch* batch, const Slice& xid);

  static void MarkRollback(WriteBatch* batch, const Slice& xid);

  static void MarkCommit(WriteBatch* batch, const Slice& xid);

  static void InsertNoop(WriteBatch* batch);

  // Return the number of entries in the batch.
  static int Count(const WriteBatch* batch);

  // Set the count for the number of entries in the batch.
  static void SetCount(WriteBatch* batch, int n);

  // Return the sequence number for the start of this batch.
  static SequenceNumber Sequence(const WriteBatch* batch);

  // Store the specified number as the sequence number for the start of
  // this batch.
  static void SetSequence(WriteBatch* batch, SequenceNumber seq);

  // Returns the offset of the first entry in the batch.
  // This offset is only valid if the batch is not empty.
  static size_t GetFirstOffset(WriteBatch* batch);

  static Slice Contents(const WriteBatch* batch) {
    return Slice(batch->rep_);
  }

  static size_t ByteSize(const WriteBatch* batch) {
    return batch->rep_.size();
  }

  static void SetContents(WriteBatch* batch, const Slice& contents);

  // Inserts batches[i] into memtable, for i in 0..num_batches-1 inclusive.
  //
  // If dont_filter_deletes is false AND options.filter_deletes is true
  // AND db->KeyMayExist is false, then a Delete won't modify the memtable.
  //
  // If ignore_missing_column_families == true. WriteBatch
  // referencing non-existing column family will be ignored.
  // If ignore_missing_column_families == false, processing of the
  // batches will be stopped if a reference is found to a non-existing
  // column family and InvalidArgument() will be returned. The writes
  // in batches may be only partially applied at that point.
  //
  // If log_number is non-zero, the memtable will be updated only if
  // memtables->GetLogNumber() >= log_number.
  //
  // If flush_scheduler is non-null, it will be invoked if the memtable
  // should be flushed.
  //
  // Under concurrent use, the caller is responsible for making sure that
  // the memtables object itself is thread-local.
  static Status InsertInto(const std::vector<WriteThread::Writer*>& batches,
                           SequenceNumber sequence,
                           ColumnFamilyMemTables* memtables,
                           FlushScheduler* flush_scheduler,
                           bool ignore_missing_column_families = false,
                           uint64_t log_number = 0, DB* db = nullptr,
                           bool concurrent_memtable_writes = false);

  // Convenience form of InsertInto when you have only one batch
  // last_seq_used returns the last sequence number used in a MemTable insert
  static Status InsertInto(const WriteBatch* batch,
                           ColumnFamilyMemTables* memtables,
                           FlushScheduler* flush_scheduler,
                           bool ignore_missing_column_families = false,
                           uint64_t log_number = 0, DB* db = nullptr,
                           bool concurrent_memtable_writes = false,
                           SequenceNumber* last_seq_used = nullptr);

  static Status InsertInto(WriteThread::Writer* writer,
                           ColumnFamilyMemTables* memtables,
                           FlushScheduler* flush_scheduler,
                           bool ignore_missing_column_families = false,
                           uint64_t log_number = 0, DB* db = nullptr,
                           bool concurrent_memtable_writes = false);

  static void Append(WriteBatch* dst, const WriteBatch* src);

  // Returns the byte size of appending a WriteBatch with ByteSize
  // leftByteSize and a WriteBatch with ByteSize rightByteSize
  static size_t AppendedByteSize(size_t leftByteSize, size_t rightByteSize);
};

}  // namespace vidardb
