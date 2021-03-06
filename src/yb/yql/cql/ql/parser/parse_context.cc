//--------------------------------------------------------------------------------------------------
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
//--------------------------------------------------------------------------------------------------

#include <stdio.h>

#include "yb/yql/cql/ql/parser/parse_context.h"
#include "yb/util/logging.h"

namespace yb {
namespace ql {

using std::endl;
using std::istream;
using std::min;
using std::shared_ptr;
using std::string;

//--------------------------------------------------------------------------------------------------
// ParseContext
//--------------------------------------------------------------------------------------------------

ParseContext::ParseContext(const char *stmt,
                           size_t stmt_len,
                           const bool reparsed,
                           shared_ptr<MemTracker> mem_tracker)
    : ProcessContext(stmt, stmt_len, ParseTree::UniPtr(new ParseTree(reparsed, mem_tracker))),
      bind_variables_(PTreeMem()),
      stmt_offset_(0),
      trace_scanning_(false),
      trace_parsing_(false) {

  // The MAC version of FLEX requires empty or valid input stream. It does not allow input file to
  // be nullptr.
  ql_file_ = std::unique_ptr<istream>(new istream(nullptr));

  if (VLOG_IS_ON(3)) {
    trace_scanning_ = true;
    trace_parsing_ = true;
  }
}

ParseContext::~ParseContext() {
}

//--------------------------------------------------------------------------------------------------

size_t ParseContext::Read(char* buf, size_t max_size) {
  const size_t copy_size = min<size_t>(stmt_len_ - stmt_offset_, max_size);
  if (copy_size > 0) {
    memcpy(buf, stmt_ + stmt_offset_, copy_size);
    stmt_offset_ += copy_size;
    return copy_size;
  }
  return 0;
}

void ParseContext::GetBindVariables(MCVector<PTBindVar*> *vars) {
  vars->clear();
  int64_t pos = 0;
  for (auto it = bind_variables_.cbegin(); it != bind_variables_.cend(); it++) {
    PTBindVar *var = *it;
    // Set the ordinal position of the bind variable in the statement also.
    if (var->is_unset_pos()) {
      var->set_pos(pos);
    }
    pos++;
    vars->push_back(var);
  }
}

}  // namespace ql
}  // namespace yb
