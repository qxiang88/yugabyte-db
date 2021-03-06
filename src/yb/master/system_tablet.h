// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//

#ifndef YB_MASTER_SYSTEM_TABLET_H
#define YB_MASTER_SYSTEM_TABLET_H

#include "yb/common/entity_ids.h"
#include "yb/common/hybrid_time.h"
#include "yb/common/schema.h"
#include "yb/master/yql_virtual_table.h"
#include "yb/tablet/abstract_tablet.h"

namespace yb {
namespace master {

// This is a virtual tablet that is used for our virtual tables in the system namespace.
class SystemTablet : public tablet::AbstractTablet {
 public:
  SystemTablet(const Schema& schema, std::unique_ptr<YQLVirtualTable> yql_virtual_table,
               const TabletId& tablet_id);

  const Schema& SchemaRef() const override;

  const common::QLStorageIf& QLStorage() const override;

  TableType table_type() const override;

  const TabletId& tablet_id() const override;

  void RegisterReaderTimestamp(HybridTime read_point) override;
  void UnregisterReader(HybridTime read_point) override;

  CHECKED_STATUS HandleRedisReadRequest(
      const ReadHybridTime& read_time,
      const RedisReadRequestPB& redis_read_request,
      RedisResponsePB* response) override;

  CHECKED_STATUS HandleQLReadRequest(
      const ReadHybridTime& read_time,
      const QLReadRequestPB& ql_read_request,
      const TransactionMetadataPB& transaction_metadata,
      tablet::QLReadRequestResult* result) override;

  CHECKED_STATUS CreatePagingStateForRead(const QLReadRequestPB& ql_read_request,
                                          const size_t row_count,
                                          QLResponsePB* response) const override;

  const TableName& GetTableName() const;
 private:
  HybridTime DoGetSafeHybridTimeToReadAt(
      tablet::RequireLease require_lease, HybridTime min_allowed, MonoTime deadline) const override;

  Schema schema_;
  std::unique_ptr<YQLVirtualTable> yql_virtual_table_;
  TabletId tablet_id_;
};

}  // namespace master
}  // namespace yb
#endif // YB_MASTER_SYSTEM_TABLET_H
