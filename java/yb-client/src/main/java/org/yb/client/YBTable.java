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
package org.yb.client;

import static org.yb.Common.TableType;
import org.yb.Schema;
import org.yb.annotations.InterfaceAudience;
import org.yb.annotations.InterfaceStability;

import com.stumbleupon.async.Deferred;

import java.util.List;

/**
 * A YBTable represents a table on a particular cluster. It holds the current
 * schema of the table. Any given YBTable instance belongs to a specific AsyncYBClient
 * instance.
 *
 * Upon construction, the table is looked up in the catalog (or catalog cache),
 * and the schema fetched for introspection. The schema is not kept in sync with the master.
 *
 * This class is thread-safe.
 */
@InterfaceAudience.Public
@InterfaceStability.Evolving
public class YBTable {

  private final Schema schema;
  private final PartitionSchema partitionSchema;
  private final AsyncYBClient client;
  private final String name;
  private final String keyspace;
  private final String tableId;
  private final TableType tableType;

  /**
   * Package-private constructor, use {@link YBClient#openTable(String)} to get an instance.
   * @param client the client this instance belongs to
   * @param name this table's name
   * @param schema this table's schema
   */
  YBTable(AsyncYBClient client, String name, String tableId, Schema schema,
          PartitionSchema partitionSchema, TableType tableType, String keyspace) {
    this.schema = schema;
    this.partitionSchema = partitionSchema;
    this.client = client;
    this.name = name;
    this.tableId = tableId;
    this.tableType = tableType;
    this.keyspace = keyspace;
  }

  YBTable(AsyncYBClient client, String name, String tableId,
          Schema schema, PartitionSchema partitionSchema) {
    this(client, name, tableId, schema, partitionSchema, TableType.YQL_TABLE_TYPE,
         null);
  }

  /**
   * Get this table's schema, as of the moment this instance was created.
   * @return this table's schema
   */
  public Schema getSchema() {
    return this.schema;
  }

  /**
   * Get this table's type.
   */
  public TableType getTableType() {
    return this.tableType;
  }

  /**
   * Gets the table's partition schema.
   *
   * This method is new, and not considered stable or suitable for public use.
   *
   * @return the table's partition schema.
   */
  @InterfaceAudience.LimitedPrivate("Impala")
  @InterfaceStability.Unstable
  public PartitionSchema getPartitionSchema() {
    return partitionSchema;
  }

  /**
   * Get this table's name.
   * @return this table's name
   */
  public String getName() {
    return this.name;
  }

  /**
   * Get this table's keyspace.
   * @return this table's keyspace.
   */
  public String getKeyspace() {
    return this.keyspace;
  }

  /**
   * Get this table's unique identifier.
   * @return this table's tableId
   */
  public String getTableId() {
    return tableId;
  }

  /**
   * Get the async client that created this instance.
   * @return an async yb java client.
   */
  public AsyncYBClient getAsyncClient() {
    return this.client;
  }

  /**
   * Get all the tablets for this table. This may query the master multiple times if there
   * are a lot of tablets.
   * @param deadline deadline in milliseconds for this method to finish
   * @return a list containing the metadata and locations for each of the tablets in the
   *         table
   * @throws Exception
   */
  public List<LocatedTablet> getTabletsLocations(
      long deadline) throws Exception {
    return getTabletsLocations(null, null, deadline);
  }

  /**
   * Asynchronously get all the tablets for this table.
   * @param deadline max time spent in milliseconds for the deferred result of this method to
   *         get called back, if deadline is reached, the deferred result will get erred back
   * @return a {@link Deferred} object that yields a list containing the metadata and
   * locations for each of the tablets in the table
   */
  public Deferred<List<LocatedTablet>> asyncGetTabletsLocations(
      long deadline) throws Exception {
    return asyncGetTabletsLocations(null, null, deadline);
  }

  /**
   * Get all or some tablets for this table. This may query the master multiple times if there
   * are a lot of tablets.
   * This method blocks until it gets all the tablets.
   * @param startKey where to start in the table, pass null to start at the beginning
   * @param endKey where to stop in the table, pass null to get all the tablets until the end of
   *               the table
   * @param deadline deadline in milliseconds for this method to finish
   * @return a list containing the metadata and locations for each of the tablets in the
   *         table
   * @throws Exception
   */
  public List<LocatedTablet> getTabletsLocations(
      byte[] startKey, byte[] endKey, long deadline) throws Exception {
    return client.syncLocateTable(tableId, startKey, endKey, deadline);
  }

  /**
   * Asynchronously get all or some tablets for this table.
   * @param startKey where to start in the table, pass null to start at the beginning
   * @param endKey where to stop in the table, pass null to get all the tablets until the end of
   *               the table
   * @param deadline max time spent in milliseconds for the deferred result of this method to
   *         get called back, if deadline is reached, the deferred result will get erred back
   * @return a {@link Deferred} object that yields a list containing the metadata and locations
   *           for each of the tablets in the table
   */
  public Deferred<List<LocatedTablet>> asyncGetTabletsLocations(
      byte[] startKey, byte[] endKey, long deadline) throws Exception {
    return client.locateTable(tableId, startKey, endKey, deadline);
  }

}
