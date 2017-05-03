// Copyright (c) YugaByte, Inc.

#ifndef YB_DOCDB_DOC_WRITE_BATCH_CACHE_H_
#define YB_DOCDB_DOC_WRITE_BATCH_CACHE_H_

#include <unordered_map>
#include <string>

#include <boost/optional.hpp>

#include "yb/common/hybrid_time.h"
#include "yb/docdb/key_bytes.h"
#include "yb/docdb/value_type.h"

namespace yb {
namespace docdb {

// A utility used by DocWriteBatch. Caches generation hybrid_times (hybrid_times of full overwrite
// or deletion) for key prefixes that were read from RocksDB or created by previous operations
// performed on the DocWriteBatch.
//
// This class is not thread-safe.
class DocWriteBatchCache {
 public:
  using Entry = std::pair<DocHybridTime, ValueType>;

  // Records the generation hybrid_time corresponding to the given encoded key prefix, which is
  // assumed not to include the hybrid_time at the end.
  void Put(const KeyBytes& encoded_key_prefix, DocHybridTime gen_ht, ValueType value_type);

  // Returns the latest generation hybrid_time for the document/subdocument identified by the given
  // encoded key prefix.
  // TODO: switch to taking a slice as an input to avoid making a copy on lookup.
  boost::optional<Entry> Get(const KeyBytes& encoded_key_prefix);

  std::string ToDebugString();

  static std::string EntryToStr(const Entry& entry);

  void Clear();

 private:
  std::unordered_map<std::string, Entry> prefix_to_gen_ht_;
};


}  // namespace docdb
}  // namespace yb

#endif  // YB_DOCDB_DOC_WRITE_BATCH_CACHE_H_
