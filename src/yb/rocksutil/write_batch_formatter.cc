// Copyright (c) YugaByte, Inc.

#include <yb/gutil/stringprintf.h>
#include "yb/rocksutil/write_batch_formatter.h"
#include "yb/util/bytes_formatter.h"

using std::endl;
using rocksdb::Status;
using rocksdb::Slice;
using rocksdb::SequenceNumber;
using yb::util::FormatBytesAsStr;
using yb::util::QuotesType;

namespace yb {

WriteBatchFormatter::WriteBatchFormatter()
    : need_separator_(false),
      user_sequence_number_(0),
      update_index_(0) {
}

Status WriteBatchFormatter::PutCF(uint32_t column_family_id,
                                  const Slice& key,
                                  const Slice& value) {
  StartOutputLine(__FUNCTION__);
  OutputField(key);
  OutputField(value);
  FinishOutputLine();
  return Status::OK();
}

Status WriteBatchFormatter::DeleteCF(uint32_t column_family_id, const Slice& key) {
  StartOutputLine(__FUNCTION__);
  OutputField(key);
  FinishOutputLine();
  return Status::OK();
}

Status WriteBatchFormatter::SingleDeleteCF(uint32_t column_family_id, const Slice& key) {
  StartOutputLine(__FUNCTION__);
  OutputField(key);
  FinishOutputLine();
  return Status::OK();
}

Status WriteBatchFormatter::MergeCF(uint32_t column_family_id,
                                    const Slice& key,
                                    const Slice& value) {
  StartOutputLine(__FUNCTION__);
  OutputField(key);
  OutputField(value);
  FinishOutputLine();
  return Status::OK();
}

void WriteBatchFormatter::SetUserSequenceNumber(SequenceNumber user_sequence_number) {
  user_sequence_number_ = user_sequence_number;
}

void WriteBatchFormatter::StartOutputLine(const char* name) {
  ++update_index_;
  out_ << update_index_ << ". ";
  out_ << name << "(";
  need_separator_ = false;
}

void WriteBatchFormatter::OutputField(const Slice& value) {
  if (need_separator_) {
    out_ << ", ";
  }
  need_separator_ = true,
  out_ << FormatBytesAsStr(value.data(), value.size(), QuotesType::kSingleQuotes);
}

void WriteBatchFormatter::FinishOutputLine() {
  out_ << ")" << endl;
}


}