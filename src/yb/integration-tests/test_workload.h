// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// The following only applies to changes made to this file as part of YugaByte development.
//
// Portions Copyright (c) YugaByte, Inc.
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
#ifndef YB_INTEGRATION_TESTS_TEST_WORKLOAD_H_
#define YB_INTEGRATION_TESTS_TEST_WORKLOAD_H_

#include <string>
#include <vector>

#include "yb/client/client.h"
#include "yb/gutil/macros.h"
#include "yb/gutil/ref_counted.h"
#include "yb/util/atomic.h"
#include "yb/util/countdown_latch.h"
#include "yb/util/monotime.h"

namespace yb {

class MiniClusterBase;
class Thread;

// Utility class for generating a workload against a test cluster.
//
// The actual data inserted is random, and thus can't be verified for
// integrity. However, this is still useful in conjunction with ClusterVerifier
// to verify that replicas do not diverge.
class TestWorkload {
 public:
  static const client::YBTableName kDefaultTableName;

  explicit TestWorkload(MiniClusterBase* cluster);
  ~TestWorkload();

  void set_payload_bytes(int n) {
    payload_bytes_ = n;
  }

  void set_num_write_threads(int n) {
    num_write_threads_ = n;
  }

  void set_write_batch_size(int s) {
    write_batch_size_ = s;
  }

  void set_client_default_rpc_timeout_millis(int t) {
    client_builder_.default_rpc_timeout(MonoDelta::FromMilliseconds(t));
  }

  void set_write_timeout_millis(int t) {
    write_timeout_millis_ = t;
  }

  // Set whether to fail if we see a TimedOut() error inserting a row.
  // By default, this triggers a CHECK failure.
  void set_timeout_allowed(bool allowed) {
    timeout_allowed_ = allowed;
  }

  // Set whether to fail if we see a NotFound() error inserting a row.
  // This sort of error is triggered if the table is deleted while the workload
  // is running.
  // By default, this triggers a CHECK failure.
  void set_not_found_allowed(bool allowed) {
    not_found_allowed_ = allowed;
  }

  void set_num_replicas(int r) {
    num_replicas_ = r;
  }

  // Set the number of tablets for the table created by this workload.
  // The split points are evenly distributed through positive int32s.
  void set_num_tablets(int tablets) {
    CHECK_GT(tablets, 1);
    num_tablets_ = tablets;
  }

  void set_table_name(const client::YBTableName& table_name) {
    table_name_ = table_name;
  }

  const client::YBTableName& table_name() const {
    return table_name_;
  }

  void set_pathological_one_row_enabled(bool enabled) {
    pathological_one_row_enabled_ = enabled;
  }

  // Sets up the internal client and creates the table which will be used for
  // writing, if it doesn't already exist.
  void Setup(client::YBTableType table_type = client::YBTableType::YQL_TABLE_TYPE);

  // Start the write workload.
  void Start();

  // Stop the writers and wait for them to exit.
  void StopAndJoin();

  // Return the number of rows inserted so far. This may be called either
  // during or after the write workload.
  int64_t rows_inserted() const {
    return rows_inserted_.Load();
  }

  // Return the number of batches in which we have successfully inserted at
  // least one row.
  int64_t batches_completed() const {
    return batches_completed_.Load();
  }

 private:
  void WriteThread();

  MiniClusterBase* cluster_;
  client::YBClientBuilder client_builder_;
  std::shared_ptr<client::YBClient> client_;

  int payload_bytes_;
  int num_write_threads_;
  int write_batch_size_;
  int write_timeout_millis_;
  bool timeout_allowed_;
  bool not_found_allowed_;
  bool pathological_one_row_enabled_;

  int num_replicas_;
  int num_tablets_;
  client::YBTableName table_name_;

  CountDownLatch start_latch_;
  AtomicBool should_run_;
  std::atomic<int64_t> pathological_one_row_counter_{0};
  std::atomic<bool> pathological_one_row_inserted_{false};
  AtomicInt<int64_t> rows_inserted_;
  AtomicInt<int64_t> batches_completed_;

  std::vector<scoped_refptr<Thread> > threads_;

  DISALLOW_COPY_AND_ASSIGN(TestWorkload);
};

}  // namespace yb
#endif  // YB_INTEGRATION_TESTS_TEST_WORKLOAD_H_
