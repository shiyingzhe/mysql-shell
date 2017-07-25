/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <memory>
#include <string>
#include <vector>
#include "proj_parser.h"

#include "modules/devapi/base_database_object.h"
#include "modules/devapi/crud_definition.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/mod_mysql_session.h"
#include "shellcore/interrupt_handler.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::mysqlx;
using namespace shcore;

Crud_definition::Crud_definition(std::shared_ptr<DatabaseObject> owner)
    : _owner(owner) {
  try {
    add_method("__shell_hook__", std::bind(&Crud_definition::execute, this, _1),
               "data");
    add_method("execute", std::bind(&Crud_definition::execute, this, _1),
               "data");
  } catch (shcore::Exception &e) {
    // Invalid typecast exception is the only option
    // The exception is recreated with a more explicit message
    throw shcore::Exception::argument_error(
        "Invalid connection used on CRUD operation.");
  }
}

shcore::Value Crud_definition::execute(
    const shcore::Argument_list &UNUSED(args)) {
  // TODO: Callback handling logic
  shcore::Value ret_val;
  /*std::shared_ptr<mysqlsh::X_connection> connection(_conn.lock());

  if (connection)
  ret_val = connection->crud_execute(class_name(), _data);*/

  return ret_val;
}

void Crud_definition::parse_string_list(const shcore::Argument_list &args,
                                        std::vector<std::string> &data) {
  // When there is 1 argument, it must be either an array of strings or a string
  if (args.size() == 1 && args[0].type != Array && args[0].type != String)
    throw shcore::Exception::argument_error(
        "Argument #1 is expected to be a string or an array of strings");

  if (args.size() == 1 && args[0].type == Array) {
    Value::Array_type_ref shell_fields = args.array_at(0);
    Value::Array_type::const_iterator index, end = shell_fields->end();

    int count = 0;
    for (index = shell_fields->begin(); index != end; index++) {
      count++;
      if (index->type != shcore::String)
        throw shcore::Exception::argument_error(
            str_format("Element #%d is expected to be a string", count));
      else
        data.push_back(index->as_string());
    }
  } else {
    for (size_t index = 0; index < args.size(); index++)
      data.push_back(args.string_at(index));
  }
}


std::shared_ptr<::mysqlx::Result> Crud_definition::safe_exec(
    ::mysqlx::Statement &stmt) {

  bool interrupted = false;

  std::shared_ptr<DatabaseObject> owner(_owner.lock());
  std::shared_ptr<ShellBaseSession> session(owner->get_session());

  shcore::Interrupt_handler intrl([session, &interrupted]() {
    try {
      if (session) {
        session->kill_query();
        interrupted = true;
      }
    } catch (std::exception &e) {
      log_warning("Exception trying to kill query: %s", e.what());
    }
    // don't propagate
    return false;
  });
  auto result = std::shared_ptr<::mysqlx::Result>(stmt.execute());
  if (result && interrupted) {
    // If the query was interrupted but it didn't throw an exception
    // from "Error 1317 Query execution was interrupted", it means the
    // interruption happened at a time the query was not active. But we
    // still need to take action, because for the caller the query will look
    // like it was interrupted and no results will be expected. That will
    // leave the result data waiting on the wire, messing up the protocol
    // ordering.
    log_warning("Flushing resultset data from interrupted query...");
    while (result && result->nextDataSet()) {}
    result.reset();
    throw shcore::Exception::runtime_error(
        "Query interrupted. Results where flushed");
  }
  return result;
}
