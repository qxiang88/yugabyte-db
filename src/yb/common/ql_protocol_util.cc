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

#include "yb/common/ql_protocol_util.h"

#include "yb/common/ql_rowblock.h"
#include "yb/common/schema.h"

namespace yb {

QLValuePB* QLPrepareColumn(QLWriteRequestPB* req, int column_id) {
  auto column_value = req->add_column_values();
  column_value->set_column_id(column_id);
  return column_value->mutable_expr()->mutable_value();
}

QLValuePB* QLPrepareCondition(QLConditionPB* condition, int column_id, QLOperator op) {
  condition->add_operands()->set_column_id(column_id);
  condition->set_op(op);
  return condition->add_operands()->mutable_value();
}

#define QL_PROTOCOL_TYPE_DEFINITIONS_IMPL(name, lname, type) \
void PP_CAT3(QLAdd, name, ColumnValue)( \
    QLWriteRequestPB* req, int column_id, type value) { \
  QLPrepareColumn(req, column_id)->PP_CAT3(set_, lname, _value)(value); \
} \
\
void PP_CAT3(QLSet, name, Expression)(QLExpressionPB* expr, type value) { \
  expr->mutable_value()->PP_CAT3(set_, lname, _value)(value); \
} \
\
void PP_CAT3(QLSet, name, Condition)( \
    QLConditionPB* condition, int column_id, QLOperator op, type value) { \
  QLPrepareCondition(condition, column_id, op)->PP_CAT3(set_, lname, _value)(value); \
} \
\
void PP_CAT3(QLAdd, name, Condition)( \
    QLConditionPB* condition, int column_id, QLOperator op, type value) { \
  PP_CAT3(QLSet, name, Condition)( \
    condition->add_operands()->mutable_condition(), column_id, op, value); \
} \

#define QL_PROTOCOL_TYPE_DEFINITIONS(i, data, entry) QL_PROTOCOL_TYPE_DEFINITIONS_IMPL entry

BOOST_PP_SEQ_FOR_EACH(QL_PROTOCOL_TYPE_DEFINITIONS, ~, QL_PROTOCOL_TYPES);

void QLAddNullColumnValue(QLWriteRequestPB* req, int column_id) {
  QLPrepareColumn(req, column_id);
}

void QLAddColumns(const Schema& schema, const std::vector<ColumnId>& columns,
                  QLReadRequestPB* req) {
  if (columns.empty()) {
    QLAddColumns(schema, schema.column_ids(), req);
    return;
  }
  req->clear_selected_exprs();
  req->mutable_column_refs()->Clear();
  QLRSRowDescPB* rsrow_desc = req->mutable_rsrow_desc();
  rsrow_desc->Clear();
  for (const auto& id : columns) {
    auto column = schema.column_by_id(id);
    CHECK_OK(column);
    req->add_selected_exprs()->set_column_id(id);
    req->mutable_column_refs()->add_ids(id);

    QLRSColDescPB* rscol_desc = rsrow_desc->add_rscol_descs();
    rscol_desc->set_name(column->name());
    column->type()->ToQLTypePB(rscol_desc->mutable_ql_type());
  }
}

std::unique_ptr<QLRowBlock> CreateRowBlock(QLClient client, const Schema& schema, Slice data) {
  auto rowblock = std::make_unique<QLRowBlock>(schema);
  if (!data.empty()) {
    // TODO: a better way to handle errors here?
    CHECK_OK(rowblock->Deserialize(client, &data));
  }
  return rowblock;
}

} // namespace yb
