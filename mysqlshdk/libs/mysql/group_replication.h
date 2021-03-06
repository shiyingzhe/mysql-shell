/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_GR_GROUP_REPLICATION_H_
#define MYSQLSHDK_LIBS_GR_GROUP_REPLICATION_H_

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "mysql/instance.h"

#ifdef _WIN32
#undef ERROR
#endif

namespace mysqlshdk {
namespace gr {

static constexpr char kPluginName[] = "group_replication";
static constexpr char kPluginActive[] = "ACTIVE";
static constexpr char kPluginDisabled[] = "DISABLED";

/**
 * Enumeration of the supported states for Group Replication members.
 */
enum class Member_state {
  ONLINE,
  RECOVERING,
  OFFLINE,
  ERROR,
  UNREACHABLE,
  MISSING
};

enum class Member_role { PRIMARY, SECONDARY };

std::string to_string(const Member_state state);
Member_state to_member_state(const std::string &state);

std::string to_string(const Member_role role);
Member_role to_member_role(const std::string &role);

/**
 * Data structure representing a Group Replication member.
 */
struct Member {
  // Address of the member
  std::string host;
  // GR port of the member
  int gr_port;
  // member_id aka server_uuid of the member
  std::string uuid;
  // State of the member
  Member_state state;
  // Role of the member (primary vs secondary)
  Member_role role;
};

// Function to check membership and state.
bool is_member(const mysqlshdk::mysql::IInstance &instance);
bool is_member(const mysqlshdk::mysql::IInstance &instance,
               const std::string &group_name);
Member_state get_member_state(const mysqlshdk::mysql::IInstance &instance);
std::vector<Member> get_members(const mysqlshdk::mysql::IInstance &instance);

bool is_primary(const mysqlshdk::mysql::IInstance &instance);

bool has_quorum(const mysqlshdk::mysql::IInstance &instance,
                int *out_unreachable, int *out_total);

// Fetch various basic info bits from the group the given instance is member of
bool get_group_information(const mysqlshdk::mysql::IInstance &instance,
                           Member_state *out_member_state,
                           std::string *out_member_id,
                           std::string *out_group_name,
                           bool *out_single_primary_mode);

std::string get_group_primary_uuid(const std::shared_ptr<db::ISession> &session,
                                   bool *out_single_primary_mode);

// GR setup specific code should be moved to a separated file

// Functions to manage the configuration of a GR instance.
// NOTE: Requires the Configuration library.
// Configuration get_configurations(
//      const mysqlshdk::mysql::IInstance &instance);
// void update_configurations(const mysqlshdk::mysql::IInstance &instance,
//                           const Configuration &configs,
//                           const bool persist);

// Function to do a change master (set the GR recovery user)
void do_change_master(const mysqlshdk::mysql::IInstance &instance,
                      const std::string &rpl_user, const std::string &rpl_pwd);

// Functions to manage the GR plugin
bool install_plugin(const mysqlshdk::mysql::IInstance &instance);
bool uninstall_plugin(const mysqlshdk::mysql::IInstance &instance);
void start_group_replication(const mysqlshdk::mysql::IInstance &instance,
                             const bool bootstrap,
                             const uint16_t read_only_timeout = 900);
void stop_group_replication(const mysqlshdk::mysql::IInstance &instance);

std::string generate_group_name();

// Function to manage the replication (recovery) user for GR.
mysql::User_privileges_result check_replication_user(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::string &host);

void create_replication_random_user_pass(
    const mysqlshdk::mysql::IInstance &instance, std::string *out_user,
    const std::vector<std::string> &hosts, std::string *out_pwd);

void create_replication_user_random_pass(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::vector<std::string> &hosts, std::string *out_pwd);

std::string get_recovery_user(const mysqlshdk::mysql::IInstance &instance);

// Function to check compliance to use GR.
std::map<std::string, std::string> check_data_compliance(
    const mysqlshdk::mysql::IInstance &instance, const uint16_t max_errors = 0);
std::map<std::string, std::string> check_server_variables(
    const mysqlshdk::mysql::IInstance &instance);

/**
 * Checks if the thread for a delayed initialization of the group replication is
 * currently running on the given instance.
 *
 * @param instance Instance to be checked.
 *
 * @return True if group replication is currently being initialized.
 */
bool is_group_replication_delayed_starting(
    const mysqlshdk::mysql::IInstance &instance);

/**
 * Wait until the given GTID set is applied on the target instance.
 *
 * @param instance target instance to wait for GTIDs to be applied.
 * @param gtid_set string with the GTID set to wait to be applied.
 * @param timeout positive integer with the maximum time in seconds to wait for
 *                all GTIDs to be applied on the instance.
 * @return Return true if the operation succeeded and false if the timeout was
 *         reached.
 * @throws an error if some issue occured when waiting for transaction to be
 *         applied.
 */
bool wait_for_gtid_set(const mysqlshdk::mysql::IInstance &instance,
                       const std::string gtid_set, int timeout);

}  // namespace gr
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_GR_GROUP_REPLICATION_H_
