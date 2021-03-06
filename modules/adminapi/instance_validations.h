/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_INSTANCE_VALIDATIONS_H_
#define MODULES_ADMINAPI_INSTANCE_VALIDATIONS_H_

#include <memory>
#include <string>
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlsh {
namespace dba {

class ProvisioningInterface;  // NOLINT

namespace checks {

bool validate_host_address(mysqlshdk::mysql::IInstance *instance, bool verbose);

bool validate_schemas(std::shared_ptr<mysqlshdk::db::ISession> session);

void validate_innodb_page_size(mysqlshdk::mysql::IInstance *instance);

bool validate_configuration(mysqlshdk::mysql::IInstance *instance,
                            const std::string &mycnf_path,
                            std::shared_ptr<ProvisioningInterface> mp,
                            bool *restart_needed, bool *mycnf_change_needed,
                            bool *sysvar_change_needed, bool *fatal_errors,
                            shcore::Value *ret_val = nullptr);

void ensure_instance_configuration_valid(
    mysqlshdk::mysql::IInstance *target_instance, const std::string &mycnf_path,
    std::shared_ptr<ProvisioningInterface> mp);

void check_required_actions(const shcore::Dictionary_t &result, bool *restart,
                            bool *dynamic_sysvar_change,
                            bool *config_file_change);
}  // namespace checks
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_INSTANCE_VALIDATIONS_H_
