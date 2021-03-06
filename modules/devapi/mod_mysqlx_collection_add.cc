/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "modules/devapi/mod_mysqlx_collection_add.h"
#include <iomanip>
#include <random>
#include <sstream>

#include "db/mysqlx/expr_parser.h"
#include "db/mysqlx/util/setter_any.h"
#include "modules/devapi/mod_mysqlx_collection.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "shellcore/utils_help.h"
#include "utils/utils_string.h"

namespace mysqlsh {
namespace mysqlx {

using std::placeholders::_1;

REGISTER_HELP_CLASS(CollectionAdd, mysqlx);
REGISTER_HELP(COLLECTIONADD_BRIEF,
              "Operation to insert documents into a Collection.");
REGISTER_HELP(COLLECTIONADD_DETAIL,
              "A CollectionAdd object represents an operation to add documents "
              "into a Collection, it is created through the <b>add</b> "
              "function on the <b>Collection</b> class.");
CollectionAdd::CollectionAdd(std::shared_ptr<Collection> owner)
    : Collection_crud_definition(
          std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::DOCUMENT);

  // Exposes the methods available for chaining
  add_method("add", std::bind(&CollectionAdd::add, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function(F::add, F::_empty | F::add);
  register_dynamic_function(F::execute, F::add);
  register_dynamic_function(F::__shell_hook__, F::add);

  // Initial function update
  update_functions(F::_empty);
}

REGISTER_HELP_FUNCTION(add, CollectionAdd);
REGISTER_HELP(COLLECTIONADD_ADD_BRIEF, "Adds documents into a collection.");
REGISTER_HELP(COLLECTIONADD_ADD_SIGNATURE, "(documentList)");
REGISTER_HELP(COLLECTIONADD_ADD_SIGNATURE1, "(document[, document, ...])");
REGISTER_HELP(COLLECTIONADD_ADD_SIGNATURE2, "(mysqlx.expr(...))");
REGISTER_HELP(COLLECTIONADD_ADD_RETURNS, "@returns This CollectionAdd object.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL,
              "This function receives one or more document definitions to be "
              "added into a collection.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL1,
              "A document definition may be provided in two ways:");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL2,
              "@li Using a dictionary containing the document fields.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL3,
              "@li Using A JSON string as a document expression.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL4,
              "There are three ways to add multiple documents:");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL5,
              "@li Passing several parameters to the function, each parameter "
              "should be a document definition.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL6,
              "@li Passing a list of document definitions.");
REGISTER_HELP(
    COLLECTIONADD_ADD_DETAIL7,
    "@li Calling this function several times before calling execute().");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL8,
              "To be added, every document must have a string property named "
              "'_id' ideally with a universal unique identifier (UUID) as "
              "value. "
              "If the '_id' property is missing, it is automatically set with "
              "an internally generated UUID.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL9, "<b>JSON as Document Expressions</b>");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL10,
              "A document can be represented as a JSON expression as follows:");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL11, "mysqlx.expr(<JSON String>)");

/**
 * $(COLLECTIONADD_ADD_BRIEF)
 *
 * #### Parameters
 *
 * @li \b document The definition of a document to be added.
 * @li \b documents A list of documents to be added.
 *
 * $(COLLECTIONADD_ADD_RETURNS)
 *
 * $(COLLECTIONADD_ADD_DETAIL)
 *
 * $(COLLECTIONADD_ADD_DETAIL1)
 *
 * $(COLLECTIONADD_ADD_DETAIL2)
 * $(COLLECTIONADD_ADD_DETAIL3)
 *
 * $(COLLECTIONADD_ADD_DETAIL4)
 *
 * $(COLLECTIONADD_ADD_DETAIL5)
 * $(COLLECTIONADD_ADD_DETAIL6)
 * $(COLLECTIONADD_ADD_DETAIL7)
 *
 * $(COLLECTIONADD_ADD_DETAIL8)
 *
 * #### Method Chaining
 *
 * This method can be called many times, every time it is called the received
 * document(s) will be cached into an internal list.
 * The actual addition into the collection will occur only when the execute
 * method is called.
 */
//@{
#if DOXYGEN_JS
CollectionAdd CollectionAdd::add(
    DocDefinition document[, DocDefinition document, ...]) {}
CollectionAdd CollectionAdd::add(List documents) {}
#elif DOXYGEN_PY
CollectionAdd CollectionAdd::add(
    DocDefinition document[, DocDefinition document, ...]) {}
CollectionAdd CollectionAdd::add(list documents) {}
#endif
//@}
shcore::Value CollectionAdd::add(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_at_least(1, get_function_name("add").c_str());

  if (_owner) {
    std::shared_ptr<Collection> collection(
        std::static_pointer_cast<Collection>(_owner));

    if (collection) {
      try {
        if (args.size() == 1 && args[0].type == shcore::Array) {
          // add([doc])
          shcore::Value::Array_type_ref docs = args[0].as_array();
          int i = 0;
          for (auto doc : *docs) {
            add_one_document(doc, "Element #" + std::to_string(++i));
          }
        } else {
          // add(doc, doc, ...)
          // add(mysqlx.expr(), mysqlx.expr(), ...)
          for (size_t i = 0; i < args.size(); i++) {
            add_one_document(args[i], "Argument #" + std::to_string(i + 1));
          }
        }
        // Updates the exposed functions (since a document has been added)
        update_functions(F::add);
      }
      CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("add").c_str());
    }
  }

  return shcore::Value(
      std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#if 0
static std::string extract_id(Mysqlx::Expr::Expr *expr) {
  assert(expr->type() == Mysqlx::Expr::Expr::OBJECT);

  for (auto i = 0, c = expr->object().fld_size(); i < c; i++) {
    if (expr->object().fld(i).key() == "_id") {
      // The document ID is stored as literal-octets
      auto value = expr->object().fld(i).value();
      if (value.has_literal() && value.literal().has_v_octets())
        return value.literal().v_octets().value();
      else if (value.has_literal() && value.literal().has_v_string())
        return value.literal().v_string().value();
      else
        throw shcore::Exception::argument_error(
            "Invalid data type for _id field, should be a string");
    }
  }
  return "";
}
#endif

void CollectionAdd::add_one_document(shcore::Value doc,
                                     const std::string &error_context) {
  if (!(doc.type == shcore::Map ||
        (doc.type == shcore::Object &&
         doc.as_object()->class_name() == "Expression"))) {
    throw shcore::Exception::argument_error(
        error_context +
        " expected to be a document, JSON expression or a list of documents");
  }

  std::unique_ptr<Mysqlx::Expr::Expr> docx;
  if (doc.type == shcore::Map) {
    // add(doc)
    docx = encode_document_expr(doc);
  } else {
    // add(mysqlx.expr(str))
    docx.reset(new Mysqlx::Expr::Expr());
    encode_expression_object(docx.get(), doc);
    if (docx->type() != Mysqlx::Expr::Expr::OBJECT) {
      throw shcore::Exception::argument_error(
          error_context +
          " expected to be a document, JSON expression or a list of documents");
    }
  }

  /*std::string id = extract_id(docx.get());
  if (id.empty()) {
    auto session = std::dynamic_pointer_cast<Session>(_owner->session());
    id = session->get_uuid();
    // inject the id
    auto fld = docx->mutable_object()->add_fld();
    fld->set_key("_id");
    mysqlshdk::db::mysqlx::util::set_scalar(*fld->mutable_value(), id);
  }
  last_document_ids_.push_back(id);*/
  message_.mutable_row()->Add()->mutable_field()->AddAllocated(docx.release());
}

REGISTER_HELP_FUNCTION(execute, CollectionAdd);
REGISTER_HELP(COLLECTIONADD_EXECUTE_BRIEF,
              "Executes the add operation, the documents are added to the "
              "target collection.");
REGISTER_HELP(COLLECTIONADD_EXECUTE_RETURNS, "@returns A Result object.");

/**
 * $(COLLECTIONADD_EXECUTE_BRIEF)
 *
 * $(COLLECTIONADD_EXECUTE_RETURNS)
 *
 * #### Method Chaining
 *
 * This function can be invoked once after:
 *
 * - add(DocDefinition document[, DocDefinition document, ...])
 *
 * ### Examples
 */
//@{
#if DOXYGEN_JS
/**
 * #### Using a Document List
 * Adding document using an existing document list
 * \snippet collection_add.js CollectionAdd: Document List
 *
 * #### Multiple Parameters
 * Adding document using a separate parameter for each document on a single call
 * to add(...)
 * \snippet collection_add.js CollectionAdd: Multiple Parameters
 *
 * #### Chaining Addition
 * Adding documents using chained calls to add(...)
 * \snippet collection_add.js CollectionAdd: Chained Calls
 *
 * $(COLLECTIONADD_ADD_DETAIL9)
 *
 * $(COLLECTIONADD_ADD_DETAIL10)
 * \snippet collection_add.js CollectionAdd: Using an Expression
 */
Result CollectionAdd::execute() {}
#elif DOXYGEN_PY
/**
 * #### Using a Document List
 * Adding document using an existing document list
 * \snippet collection_add.py CollectionAdd: Document List
 *
 * #### Multiple Parameters
 * Adding document using a separate parameter for each document on a single call
 * to add(...)
 * \snippet collection_add.py CollectionAdd: Multiple Parameters
 *
 * #### Chaining Addition
 * Adding documents using chained calls to add(...)
 * \snippet collection_add.py CollectionAdd: Chained Calls
 *
 * $(COLLECTIONADD_ADD_DETAIL9)
 *
 * $(COLLECTIONADD_ADD_DETAIL10)
 * \snippet collection_add.py CollectionAdd: Using an Expression
 */
Result CollectionAdd::execute() {}
#endif
//@}
shcore::Value CollectionAdd::execute(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("execute").c_str());
  shcore::Value ret_val;
  try {
    ret_val = execute(false);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute").c_str());

  return ret_val;
}

shcore::Value CollectionAdd::execute(bool upsert) {
  std::unique_ptr<mysqlsh::mysqlx::Result> result;

  mysqlshdk::utils::Profile_timer timer;
  insert_bound_values(message_.mutable_args());
  timer.stage_begin("CollectionAdd::execute");
  if (upsert) message_.set_upsert(upsert);
  if (message_.mutable_row()->size()) {
    result.reset(new mysqlx::Result(safe_exec(
        [this]() { return session()->session()->execute_crud(message_); })));
  } else {
    result.reset(new mysqlsh::mysqlx::Result({}));
  }
  timer.stage_end();
  result->set_execution_time(timer.total_seconds_ellapsed());

  return result ? shcore::Value::wrap(result.release()) : shcore::Value::Null();
}

}  // namespace mysqlx
}  // namespace mysqlsh
