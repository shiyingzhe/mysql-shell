/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MODULES_DEVAPI_CRUD_DEFINITION_H_
#define MODULES_DEVAPI_CRUD_DEFINITION_H_

#include "modules/devapi/dynamic_object.h"
#include "scripting/common.h"
#include "scripting/types_cpp.h"
#include "mysqlxtest/mysqlx_crud.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#ifdef __GNUC__
#define ATTR_UNUSED __attribute__((unused))
#else
#define ATTR_UNUSED
#endif

namespace mysqlsh {
class DatabaseObject;
namespace mysqlx {
#if DOXYGEN_CPP
/**
* Base class for CRUD operations.
*
* The CRUD operations will use "dynamic" functions to control the method
* chaining.
* A dynamic function is one that will be enabled/disabled based on the method
* chain sequence.
*/
#endif
class Crud_definition : public Dynamic_object {
 public:
  explicit Crud_definition(std::shared_ptr<DatabaseObject> owner);

  // The last step on CRUD operations
  virtual shcore::Value execute(const shcore::Argument_list &args) = 0;
 protected:
  std::shared_ptr<::mysqlx::Result> safe_exec(::mysqlx::Statement &stmt);

  std::weak_ptr<DatabaseObject> _owner;

  void parse_string_list(const shcore::Argument_list &args,
                         std::vector<std::string> &data);
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_CRUD_DEFINITION_H_
