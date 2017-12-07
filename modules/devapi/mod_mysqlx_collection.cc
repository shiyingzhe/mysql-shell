/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/devapi/mod_mysqlx_collection.h"
#include <memory>
#include <string>
#include <mysqld_error.h>
#include "modules/devapi/mod_mysqlx_schema.h"

#include "modules/devapi/mod_mysqlx_collection_add.h"
#include "modules/devapi/mod_mysqlx_collection_create_index.h"
#include "modules/devapi/mod_mysqlx_collection_find.h"
#include "modules/devapi/mod_mysqlx_collection_modify.h"
#include "modules/devapi/mod_mysqlx_collection_remove.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "shellcore/utils_help.h"

using namespace std::placeholders;
namespace mysqlsh {
namespace mysqlx {
using namespace shcore;

REGISTER_HELP(COLLECTION_BRIEF,
              "A Collection is a container that may be used to store Documents "
              "in a MySQL database.");
REGISTER_HELP(COLLECTION_DETAIL,
              "A Document is a set of key and value pairs, as represented by a "
              "JSON object.");
REGISTER_HELP(COLLECTION_DETAIL1,
              "A Document is represented internally using the MySQL binary "
              "JSON object, through the JSON MySQL datatype.");
REGISTER_HELP(COLLECTION_DETAIL2,
              "The values of fields can contain other documents, arrays, and "
              "lists of documents.");
REGISTER_HELP(COLLECTION_PARENTS, "DatabaseObject");
Collection::Collection(std::shared_ptr<Schema> owner, const std::string &name)
    : DatabaseObject(owner->_session.lock(),
                     std::static_pointer_cast<DatabaseObject>(owner), name) {
  init();
}
void Collection::init() {
  add_method("add", std::bind(&Collection::add_, this, _1), "searchCriteria",
             shcore::String, NULL);
  add_method("modify", std::bind(&Collection::modify_, this, _1),
             "searchCriteria", shcore::String, NULL);
  add_method("find", std::bind(&Collection::find_, this, _1), "searchCriteria",
             shcore::String, NULL);
  add_method("remove", std::bind(&Collection::remove_, this, _1),
             "searchCriteria", shcore::String, NULL);
  add_method("createIndex", std::bind(&Collection::create_index_, this, _1),
             "searchCriteria", shcore::String, NULL);
  add_method("dropIndex", std::bind(&Collection::drop_index_, this, _1),
             "searchCriteria", shcore::String, NULL);
  add_method("replaceOne", std::bind(&Collection::replace_one_, this, _1),
              "id", shcore::String, "doc", shcore::Map, NULL);
  add_method("addOrReplaceOne",
             std::bind(&Collection::add_or_replace_one, this, _1), "id",
             shcore::String, "doc", shcore::Map, NULL);
  add_method("getOne", std::bind(&Collection::get_one, this, _1), "id",
             shcore::String, NULL);
  add_method("removeOne", std::bind(&Collection::remove_one, this, _1),
              "id", shcore::String, NULL);
}

Collection::~Collection() {}

REGISTER_HELP(COLLECTION_ADD_BRIEF,
              "Inserts one or more documents into a collection.");
REGISTER_HELP(COLLECTION_ADD_CHAINED, "CollectionAdd.add.[execute]");

/**
* $(COLLECTION_ADD_BRIEF)
*
* ### Full Syntax
*
* <code>
*   <table border = "0">
*     <tr><td>Collection</td><td>.add(...)</td></tr>
*     <tr><td></td><td>.$(COLLECTIONADD_EXECUTE_SYNTAX)</td></tr>
*   </table>
* </code>
*
* #### .add(...)
*
* ##### Alternatives
*
* @li $(COLLECTIONADD_ADD_SYNTAX)
* @li $(COLLECTIONADD_ADD_SYNTAX1)
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
* #### .execute()
*
* $(COLLECTIONADD_EXECUTE_BRIEF)
*
* ### Examples
*/
#if DOXYGEN_JS
/**
* \snippet mysqlx_collection_add.js CollectionAdd: Chained Calls
*
* $(COLLECTIONADD_ADD_DETAIL9)
*
* $(COLLECTIONADD_ADD_DETAIL10)
* \snippet mysqlx_collection_add.js CollectionAdd: Using an Expression
*
* #### Using a Document List
* Adding document using an existing document list
* \snippet mysqlx_collection_add.js CollectionAdd: Document List
*
* #### Multiple Parameters
* Adding document using a separate parameter for each document on a single call
* to add(...)
* \snippet mysqlx_collection_add.js CollectionAdd: Multiple Parameters
*/
CollectionAdd Collection::add(...) {}
#elif DOXYGEN_PY
/**
* Adding documents using chained calls to add(...)
* \snippet mysqlx_collection_add.py CollectionAdd: Chained Calls
*
* $(COLLECTIONADD_ADD_DETAIL9)
*
* $(COLLECTIONADD_ADD_DETAIL10)
* \snippet mysqlx_collection_add.py CollectionAdd: Using an Expression
*
* #### Using a Document List
* Adding document using an existing document list
* \snippet mysqlx_collection_add.py CollectionAdd: Document List
*
* #### Multiple Parameters
* Adding document using a separate parameter for each document on a single call
* to add(...)
* \snippet mysqlx_collection_add.py CollectionAdd: Multiple Parameters
*/
CollectionAdd Collection::add(...) {}
#endif
shcore::Value Collection::add_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionAdd> collectionAdd(
      new CollectionAdd(shared_from_this()));

  return collectionAdd->add(args);
}

REGISTER_HELP(COLLECTION_MODIFY_BRIEF, "Creates a collection update handler.");
REGISTER_HELP(COLLECTION_MODIFY_CHAINED,
              "CollectionModify.modify.[set].[unset].[merge].[patch].["
              "arrayInsert].[arrayAppend].[arrayDelete].[sort].[limit].[bind].["
              "execute]");

/**
* $(COLLECTION_ADD_BRIEF)
*
* ### Full Syntax
*
* <code>
*   <table border = "0">
*     <tr><td>Collection</td><td>.modify(...)</td></tr>
*     <tr><td></td><td>[.set(...)]</td></tr>
*     <tr><td></td><td>[.$(COLLECTIONMODIFY_UNSET_SYNTAX)]</td></tr>
*     <tr><td></td><td>[.merge(...)]</td></tr>
*     <tr><td></td><td>[.patch(...)]</td></tr>
*/
#if DOXYGEN_JS
/**
*     <tr><td></td><td>[.arrayInsert(...)]</td></tr>
*     <tr><td></td><td>[.arrayAppend(...)]</td></tr>
*     <tr><td></td><td>[.arrayDelete(...)]</td></tr>
*/
#elif DOXYGEN_PY
/**
*     <tr><td></td><td>[.array_insert(...)]</td></tr>
*     <tr><td></td><td>[.array_append(...)]</td></tr>
*     <tr><td></td><td>[.array_delete(...)]</td></tr>
*/
#endif
/**
*     <tr><td></td><td>[.sort(...)]</td></tr>
*     <tr><td></td><td>[.limit(...)]</td></tr>
*     <tr><td></td><td>[.bind(...)]</td></tr>
*     <tr><td></td><td>[.execute()]</td></tr>
*   </table>
* </code>
*
* #### .modify()
*
* $(COLLECTIONMODIFY_MODIFY_PARAM)
*
* $(COLLECTIONMODIFY_MODIFY_DETAIL)
*
* $(COLLECTIONMODIFY_MODIFY_DETAIL1)
*
* $(COLLECTIONMODIFY_MODIFY_DETAIL2)
*
* #### .set()
*
* $(COLLECTIONMODIFY_SET_DETAIL)
* $(COLLECTIONMODIFY_SET_DETAIL1)
* $(COLLECTIONMODIFY_SET_DETAIL2)
*
* $(COLLECTIONMODIFY_SET_DETAIL3)
*
* $(COLLECTIONMODIFY_SET_DETAIL4)
*
* $(COLLECTIONMODIFY_SET_DETAIL5)
*
* To define an expression use:
* \code{.py}
* mysqlx.expr(expression)
* \endcode
*
* The expression also can be used for \a [Parameter
* Binding](param_binding.html).
*
* The attribute addition will be done on the collection's documents once the
* execute method is called.
*
* #### .unset()
*
* ##### Alternatives
*
* @li $(COLLECTIONMODIFY_UNSET_SYNTAX)
* @li $(COLLECTIONMODIFY_UNSET_SYNTAX1)
*
* $(COLLECTIONMODIFY_UNSET_BRIEF)
*
* $(COLLECTIONMODIFY_UNSET_DETAIL)
*
* #### .merge()
*
* $(COLLECTIONMODIFY_MERGE_DETAIL)
*
* $(COLLECTIONMODIFY_MERGE_DETAIL1)
*
* $(COLLECTIONMODIFY_MERGE_DETAIL2)
*
* #### .patch()
*
* $(COLLECTIONMODIFY_PATCH_BRIEF)
*
* $(COLLECTIONMODIFY_PATCH_DETAIL)
*
* $(COLLECTIONMODIFY_PATCH_DETAIL1)
*
* $(COLLECTIONMODIFY_PATCH_DETAIL2)
* $(COLLECTIONMODIFY_PATCH_DETAIL3)
* $(COLLECTIONMODIFY_PATCH_DETAIL4)
* $(COLLECTIONMODIFY_PATCH_DETAIL5)
*
* $(COLLECTIONMODIFY_PATCH_DETAIL6)
* $(COLLECTIONMODIFY_PATCH_DETAIL7)
* $(COLLECTIONMODIFY_PATCH_DETAIL8)
*
* $(COLLECTIONMODIFY_PATCH_DETAIL9)
*
* #### .arrayInsert()
*
* $(COLLECTIONMODIFY_ARRAYINSERT_DETAIL)
*
* $(COLLECTIONMODIFY_ARRAYINSERT_DETAIL1)
*
* #### .arrayAppend()
*
* $(COLLECTIONMODIFY_ARRAYAPPEND_DETAIL)
*
* #### .arrayDelete()
*
* $(COLLECTIONMODIFY_ARRAYDELETE_DETAIL)
*
* $(COLLECTIONMODIFY_ARRAYDELETE_DETAIL1)
*
* #### .sort()
*
* $(COLLECTIONMODIFY_SORT_DETAIL)
* $(COLLECTIONMODIFY_SORT_DETAIL1)
*
* $(COLLECTIONMODIFY_SORT_DETAIL2)
*
* #### .limit()
*
* $(COLLECTIONMODIFY_LIMIT_DETAIL)
*
* #### .bind()
*
* $(COLLECTIONMODIFY_BIND_BRIEF)
*
* #### .execute()
*
* $(COLLECTIONMODIFY_EXECUTE_BRIEF)
*/
#if DOXYGEN_JS
/**
*
* #### Examples
* \dontinclude "mysqlx_collection_modify.js"
* \skip //@# CollectionModify: Set Execution
* \until //@ CollectionModify: sorting and limit Execution - 4
* \until print(dir(doc));
*/
#elif DOXYGEN_PY
/**
*
* #### Examples
* \dontinclude "mysqlx_collection_modify.py"
* \skip #@# CollectionModify: Set Execution
* \until #@ CollectionModify: sorting and limit Execution - 4
* \until print dir(doc)
*/
#endif
#if DOXYGEN_JS
CollectionModify Collection::modify(String searchCondition) {}
#elif DOXYGEN_PY
CollectionModify Collection::modify(str searchCondition) {}
#endif
shcore::Value Collection::modify_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionModify> collectionModify(
      new CollectionModify(shared_from_this()));

  return collectionModify->modify(args);
}

REGISTER_HELP(COLLECTION_REMOVE_BRIEF, "Creates a document deletion handler.");
REGISTER_HELP(COLLECTION_REMOVE_CHAINED,
              "CollectionRemove.remove.[sort].[limit].[bind].[execute]");

/**
* $(COLLECTION_REMOVE_BRIEF)
*
* ### Full Syntax
*
* <code>
*   <table border = "0">
*     <tr><td>Collection</td><td>.remove(...)</td></tr>
*     <tr><td></td><td>[.sort(...)]</td></tr>
*     <tr><td></td><td>[.limit(...)]</td></tr>
*     <tr><td></td><td>[.bind(...)]</td></tr>
*     <tr><td></td><td>[.execute(...)]</td></tr>
*   </table>
* </code>
*
* #### .remove()
*
* $(COLLECTIONREMOVE_REMOVE_DETAIL)
*
* $(COLLECTIONREMOVE_REMOVE_DETAIL1)
*
* $(COLLECTIONREMOVE_REMOVE_DETAIL2)
*
* $(COLLECTIONREMOVE_REMOVE_DETAIL3)
*
* #### .sort()
*
* $(COLLECTIONREMOVE_SORT_DETAIL)
*
* $(COLLECTIONREMOVE_SORT_DETAIL1)
*
* $(COLLECTIONREMOVE_SORT_DETAIL2)
*
* #### .limit()
*
* $(COLLECTIONREMOVE_LIMIT_BRIEF)
*
* $(COLLECTIONREMOVE_LIMIT_DETAIL)
*
* #### .bind()
*
* $(COLLECTIONREMOVE_BIND_DETAIL)
*
* $(COLLECTIONREMOVE_BIND_DETAIL1)
*
* #### .execute()
*
* $(COLLECTIONREMOVE_EXECUTE_BRIEF)
*
* \sa CollectionRemove
*
* #### Examples
*/
#if DOXYGEN_JS
/**
* #### Remove under condition
*
* \snippet mysqlx_collection_remove.js CollectionRemove: remove under condition
*
* #### Remove with binding
*
* \snippet mysqlx_collection_remove.js CollectionRemove: remove with binding
*
* #### Full remove
*
* \snippet mysqlx_collection_remove.js CollectionRemove: full remove
*
*/
#elif DOXYGEN_PY
/**
* #### Remove under condition
*
* \snippet mysqlx_collection_remove.py CollectionRemove: remove under condition
*
* #### Remove with binding
*
* \snippet mysqlx_collection_remove.py CollectionRemove: remove with binding
*
* #### Full remove
*
* \snippet mysqlx_collection_remove.py CollectionRemove: full remove
*
*/
#endif
#if DOXYGEN_JS
CollectionRemove Collection::remove(String searchCondition) {}
#elif DOXYGEN_PY
CollectionRemove Collection::remove(str searchCondition) {}
#endif
shcore::Value Collection::remove_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionRemove> collectionRemove(
      new CollectionRemove(shared_from_this()));

  return collectionRemove->remove(args);
}

REGISTER_HELP(
    COLLECTION_FIND_BRIEF,
    "Retrieves documents from a collection, matching a specified criteria.");
REGISTER_HELP(
    COLLECTION_FIND_CHAINED,
    "CollectionFind.find.[fields].[groupBy->[having]].[sort].[limit->[skip]].[bind].[execute]");

/**
* $(COLLECTION_FIND_BRIEF)
*
* ### Full Syntax
*
* <code>
*   <table border = "0">
*     <tr><td>Collection</td><td>.find(...)</td></tr>
*     <tr><td></td><td>[.fields(...)]</td></tr>
*/

#if DOXYGEN_JS
/**
 * <tr><td></td><td>[.groupBy(...)[.$(COLLECTIONFIND_HAVING_SYNTAX)]]</td></tr>*/
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.group_by(...)[.$(COLLECTIONFIND_HAVING_SYNTAX)]]</td></tr>*/
#endif
/**
*     <tr><td></td><td>[.sort(...)]</td></tr>
*     <tr><td></td><td>[.$(COLLECTIONFIND_LIMIT_SYNTAX)[.$(COLLECTIONFIND_SKIP_SYNTAX)]]</td></tr>
*     <tr><td></td><td>[.$(COLLECTIONFIND_BIND_SYNTAX)]</td></tr>
*     <tr><td></td><td>.$(COLLECTIONFIND_EXECUTE_SYNTAX)</td></tr>
*   </table>
* </code>
*
* #### .find(...)
*
* ##### Alternatives
*
* @li $(COLLECTIONFIND_FIND_SYNTAX)
* @li $(COLLECTIONFIND_FIND_SYNTAX1)
*
* $(COLLECTIONFIND_FIND_DETAIL)
*
* $(COLLECTIONFIND_FIND_DETAIL1)
*
* #### .fields(...)
*
* ##### Alternatives
*
* @li $(COLLECTIONFIND_FIELDS_SYNTAX)
* @li $(COLLECTIONFIND_FIELDS_SYNTAX1)
* @li $(COLLECTIONFIND_FIELDS_SYNTAX2)
*
* $(COLLECTIONFIND_FIELDS_DETAIL)
*ALLBUTDBA
* $(COLLECTIONFIND_FIELDS_DETAIL1)
*
* $(COLLECTIONFIND_FIELDS_DETAIL2)
*
* $(COLLECTIONFIND_FIELDS_DETAIL3)
* $(COLLECTIONFIND_FIELDS_DETAIL4)
* $(COLLECTIONFIND_FIELDS_DETAIL5)
*
*/

#if DOXYGEN_JS
/**
*
* #### .groupBy(...)
*
*/
#elif DOXYGEN_PY
/**
*
* #### .group_by(...)
*
*/
#endif

/**
*
* $(COLLECTIONFIND_GROUPBY_DETAIL)
*
* #### .$(COLLECTIONFIND_HAVING_SYNTAX)
*
* $(COLLECTIONFIND_HAVING_DETAIL)
*
* #### .sort(...)
*
* ##### Alternatives
*
* @li $(COLLECTIONFIND_SORT_SYNTAX)
* @li $(COLLECTIONFIND_SORT_SYNTAX1)
*
* $(COLLECTIONFIND_SORT_DETAIL)
*
* $(COLLECTIONFIND_SORT_DETAIL1)
*
* $(COLLECTIONFIND_SORT_DETAIL2)
*
* $(COLLECTIONFIND_SORT_DETAIL3)
*
* #### .$(COLLECTIONFIND_LIMIT_SYNTAX)
*
* $(COLLECTIONFIND_LIMIT_DETAIL)
*
* #### .$(COLLECTIONFIND_SKIP_SYNTAX)
*
* $(COLLECTIONFIND_SKIP_DETAIL)
*
* #### .$(COLLECTIONFIND_BIND_SYNTAX)
*
* $(COLLECTIONFIND_BIND_DETAIL)
*
* $(COLLECTIONFIND_BIND_DETAIL1)
*
* $(COLLECTIONFIND_BIND_DETAIL2)
*
* #### .$(COLLECTIONFIND_EXECUTE_SYNTAX)
*
* $(COLLECTIONFIND_EXECUTE_BRIEF)
*
* ### Examples
*/
#if DOXYGEN_JS
/**
* #### Retrieving All Documents
* \snippet mysqlx_collection_find.js CollectionFind: All Records
*
* #### Filtering
* \snippet mysqlx_collection_find.js CollectionFind: Filtering
*
* #### Field Selection
* Using a field selection list
* \snippet mysqlx_collection_find.js CollectionFind: Field Selection List
*
* Using separate field selection parameters
* \snippet mysqlx_collection_find.js CollectionFind: Field Selection Parameters
*
* Using a projection expression
* \snippet mysqlx_collection_find.js CollectionFind: Field Selection Projection
*
* #### Sorting
* \snippet mysqlx_collection_find.js CollectionFind: Sorting
*
* #### Using Limit and Skip
* \snippet mysqlx_collection_find.js CollectionFind: Limit and Skip
*
* #### Parameter Binding
* \snippet mysqlx_collection_find.js CollectionFind: Parameter Binding
*/
CollectionFind Collection::find(...) {}
#elif DOXYGEN_PY
/**
* #### Retrieving All Documents
* \snippet mysqlx_collection_find.py CollectionFind: All Records
*
* #### Filtering
* \snippet mysqlx_collection_find.py CollectionFind: Filtering
*
* #### Field Selection
* Using a field selection list
* \snippet mysqlx_collection_find.py CollectionFind: Field Selection List
*
* Using separate field selection parameters
* \snippet mysqlx_collection_find.py CollectionFind: Field Selection Parameters
*
* Using a projection expression
* \snippet mysqlx_collection_find.py CollectionFind: Field Selection Projection
*
* #### Sorting
* \snippet mysqlx_collection_find.py CollectionFind: Sorting
*
* #### Using Limit and Skip
* \snippet mysqlx_collection_find.py CollectionFind: Limit and Skip
*
* #### Parameter Binding
* \snippet mysqlx_collection_find.py CollectionFind: Parameter Binding
*/
CollectionFind Collection::find(...) {}
#endif
shcore::Value Collection::find_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionFind> collectionFind(
      new CollectionFind(shared_from_this()));

  return collectionFind->find(args);
}

REGISTER_HELP(COLLECTION_CREATEINDEX_BRIEF,
              "Creates a non unique/unique index on a collection.");
REGISTER_HELP(COLLECTION_CREATEINDEX_CHAINED,
              "CollectionCreateIndex.createIndex.[field].[execute]");

/**
* $(COLLECTION_CREATEINDEX_BRIEF)
*
* <code>
*   <table border = "0">
*/
#if DOXYGEN_JS
/**
*     <tr><td>Collection</td><td>.createIndex(...)</td></tr>
*/
#elif DOXYGEN_PY
/**
*     <tr><td>Collection</td><td>.create_index(...)</td></tr>
*/
#endif
/**
*     <tr><td></td><td>[.field(...)]</td></tr>
*     <tr><td></td><td>[.execute(...)]</td></tr>
*   </table>
* </code>
*
*/
#if DOXYGEN_JS
/**
* #### .createIndex(...)
*/
#elif DOXYGEN_PY
/**
* #### .create_index(...)
*/
#endif
/**
* ##### Alternatives
*
* @li $(COLLECTIONCREATEINDEX_CREATEINDEX_SYNTAX)
* @li $(COLLECTIONCREATEINDEX_CREATEINDEX_SYNTAX1)
*
* $(COLLECTIONCREATEINDEX_CREATEINDEX_BRIEF)
* $(COLLECTIONCREATEINDEX_CREATEINDEX_BRIEF1)
*
* #### .field(...)
*
* $(COLLECTIONCREATEINDEX_FIELD_BRIEF)
*
* #### .execute(...)
*
* $(COLLECTIONCREATEINDEX_EXECUTE_BRIEF)
*
* \sa CollectionCreateIndex
*/
//@{
#if DOXYGEN_JS
CollectionCreateIndex Collection::createIndex(String name) {}
CollectionCreateIndex Collection::createIndex(String name, IndexType type) {}
#elif DOXYGEN_PY
CollectionCreateIndex Collection::create_index(str name) {}
CollectionCreateIndex Collection::create_index(str name, IndexType type) {}
#endif
//@}

shcore::Value Collection::create_index_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionCreateIndex> createIndex(
      new CollectionCreateIndex(shared_from_this()));

  auto ss = createIndex->set_scoped_naming_style(naming_style);

  return createIndex->create_index(args);
}

REGISTER_HELP(COLLECTION_DROPINDEX_BRIEF, "Drops an index from a collection.");

/**
* $(COLLECTION_DROPINDEX_BRIEF)
*/
#if DOXYGEN_JS
Undefined Collection::dropIndex(String name) {}
#elif DOXYGEN_PY
None Collection::drop_index(str name) {}
#endif
shcore::Value Collection::drop_index_(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("dropIndex").c_str());

  try {
    args.string_at(0);

    shcore::Dictionary_t drop_index_args = shcore::make_dict();
    Value schema = this->get_member("schema");
    (*drop_index_args)["schema"] = schema.as_object()->get_member("name");
    (*drop_index_args)["collection"] = this->get_member("name");
    (*drop_index_args)["name"] = args[0];

    Value session = this->get_member("session");
    auto session_obj =
          std::static_pointer_cast<Session>(session.as_object());
    try {
    session_obj->_execute_mysqlx_stmt("drop_collection_index",
                                       drop_index_args);
    }
    catch (const mysqlshdk::db::Error e) {
      if (e.code() != ER_CANT_DROP_FIELD_OR_KEY)
        throw;
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropIndex"));
  return shcore::Value();
}

REGISTER_HELP(COLLECTION_REPLACEONE_BRIEF,
  "Replaces an existing document with a new document.");
REGISTER_HELP(COLLECTION_REPLACEONE_PARAM,
  "@param id identifier of the document to be replaced.");
REGISTER_HELP(COLLECTION_REPLACEONE_PARAM1,
  "@param doc the new document.");
REGISTER_HELP(COLLECTION_REPLACEONE_RETURNS,
  "@returns A Result object containing the number of affected rows.");
REGISTER_HELP(COLLECTION_REPLACEONE_DETAIL,
  "Replaces the document identified with the given id. If no document is found "
  "matching the given id the returned Result will indicate 0 affected items.");
REGISTER_HELP(COLLECTION_REPLACEONE_DETAIL1,
  "Only one document will be affected by this operation.");
REGISTER_HELP(COLLECTION_REPLACEONE_DETAIL2,
  "The id of the document remain inmutable, if the new document contains a "
  "different id, it will be ignored.");
REGISTER_HELP(COLLECTION_REPLACEONE_DETAIL3,
  "Any constraint (unique key) defined on the collection is applicable:");
REGISTER_HELP(COLLECTION_REPLACEONE_DETAIL4,
  "The operation will fail if the new document contains a unique key which is "
  "already defined for any document in the collection except the one being "
  "replaced.");

/**
* $(COLLECTION_REPLACEONE_BRIEF)
*
* $(COLLECTION_REPLACEONE_PARAM)
* $(COLLECTION_REPLACEONE_PARAM1)
*
* $(COLLECTION_REPLACEONE_RETURNS)
*
* $(COLLECTION_REPLACEONE_DETAIL)
*
* $(COLLECTION_REPLACEONE_DETAIL1)
*
* $(COLLECTION_REPLACEONE_DETAIL2)
*
* $(COLLECTION_REPLACEONE_DETAIL3)
*
* $(COLLECTION_REPLACEONE_DETAIL4)
*/
#if DOXYGEN_JS
Result Collection::replaceOne(String id, Document doc) {}
#elif DOXYGEN_PY
Result Collection::replace_one(str id, document doc) {}
#endif
shcore::Value Collection::replace_one_(const Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(2, get_function_name("replaceOne").c_str());
  try {
    auto id = args.string_at(0);
    auto document = args.map_at(1);

    CollectionModify modify_op(shared_from_this());
    modify_op.set_filter("_id = :id").bind("id", args[0]);
    modify_op.set_operation(Mysqlx::Crud::UpdateOperation::ITEM_SET, "",
                  args[1]);
    ret_val = modify_op.execute();
  }CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("replaceOne"));

  return ret_val;
}

REGISTER_HELP(COLLECTION_ADDORREPLACEONE_BRIEF,
              "Replaces or adds a document in a collection.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_PARAM,
              "@param id the identifier of the document to be replaced.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_PARAM1,
              "@param doc the new document.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_RETURNS,
    "@returns A Result object containing the number of affected rows.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL,
              "Replaces the document identified with the given id. If no "
              "document is found matching the given id the given document will "
              "be added to the collection.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL1,
              "Only one document will be affected by this operation.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL2,
              "The id of the document remains inmutable, if the new document "
              "contains a different id, it will be ignored.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL3,
              "Any constraint (unique key) defined on the collection is "
              "applicable on both the replace and add operations:");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL4,
              "@li The replace operation will fail if the new document "
              "contains a unique key which is already defined for any document "
              "in the collection except the one being replaced.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL5,
              "@li The add operation will fail if the new document contains a "
              "unique key which is already defined for any document in the "
              "collection.");
/**
* $(COLLECTION_ADDORREPLACEONE_BRIEF)
*
* $(COLLECTION_ADDORREPLACEONE_PARAM)
* $(COLLECTION_ADDORREPLACEONE_PARAM1)
*
* $(COLLECTION_ADDORREPLACEONE_RETURNS)
*
* $(COLLECTION_ADDORREPLACEONE_DETAIL)
*
* $(COLLECTION_ADDORREPLACEONE_DETAIL1)
*
* $(COLLECTION_ADDORREPLACEONE_DETAIL2)
*
* $(COLLECTION_ADDORREPLACEONE_DETAIL3)
*
* $(COLLECTION_ADDORREPLACEONE_DETAIL4)
*
* $(COLLECTION_ADDORREPLACEONE_DETAIL5)
*/
#if DOXYGEN_JS
Result Collection::addOrReplaceOne(String id, Document doc) {}
#elif DOXYGEN_PY
Result Collection::add_or_replace_one(str id, document doc) {}
#endif
shcore::Value Collection::add_or_replace_one(
    const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(2, get_function_name("addOrReplaceOne").c_str());
  try {
    auto id = args.string_at(0);
    auto document = args.map_at(1);

    // The document gets updated with given id
    (*document)["_id"] = shcore::Value(id);

    CollectionAdd add_op(shared_from_this());
    add_op.add_one_document(shcore::Value(document), "Parameter #1");
    ret_val = add_op.execute(true);
  }CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addOrReplaceOne"));

  return ret_val;
}

REGISTER_HELP(COLLECTION_GETONE_BRIEF,
              "Fetches the document with the given _id from the collection.");
REGISTER_HELP(COLLECTION_GETONE_PARAM,
              "@param id The identifier of the document to be retrieved.");
REGISTER_HELP(COLLECTION_GETONE_RETURNS,
              "@returns The Document object matching the given id or NULL if "
              "no match is found.");
/**
* $(COLLECTION_GETONE_BRIEF)
*
* $(COLLECTION_GETONE_PARAM)
*
* $(COLLECTION_GETONE_RETURNS)
*/
#if DOXYGEN_JS
Document Collection::getOne(String id) {}
#elif DOXYGEN_PY
document Collection::get_one(str id) {}
#endif
shcore::Value Collection::get_one(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("getOne").c_str());
  shcore::Value ret_val = shcore::Value::Null();
  try {
    auto id = args.string_at(0);

    CollectionFind find_op(shared_from_this());
    find_op.set_filter("_id = :id").bind("id", args[0]);
    auto result = find_op.execute();
    if (result)
      ret_val = result->fetch_one({});
  }CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getOne"));

  return ret_val;
}

REGISTER_HELP(COLLECTION_REMOVEONE_BRIEF,
              "Removes document with the given _id value.");
REGISTER_HELP(COLLECTION_REMOVEONE_PARAM,
              "@param id The id of the document to be removed.");
REGISTER_HELP(COLLECTION_REMOVEONE_RETURNS,
    "@returns A Result object containing the number of affected rows.");
REGISTER_HELP(COLLECTION_REMOVEONE_DETAIL,
              "If no document is found matching the given id, the Result "
              "object will indicate 0 as the number of affected rows.");
/**
* $(COLLECTION_REMOVEONE_BRIEF)
*
* $(COLLECTION_REMOVEONE_PARAM)
*
* $(COLLECTION_REMOVEONE_RETURNS)
*
* $(COLLECTION_REMOVEONE_DETAIL)
*/
#if DOXYGEN_JS
Result Collection::removeOne(String id) {}
#elif DOXYGEN_PY
Result Collection::remove_one(str id) {}
#endif
shcore::Value Collection::remove_one(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("removeOne").c_str());
  shcore::Value ret_val;
  try {
    auto id = args.string_at(0);

    CollectionRemove remove_op(shared_from_this());
    remove_op.set_filter("_id = :id").bind("id", args[0]);
    ret_val = remove_op.execute();
  }CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("removeOne"));

  return ret_val;
}

}  // namespace mysqlx
}  // namespace mysqlsh
