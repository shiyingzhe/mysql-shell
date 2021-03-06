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

#include <utility>
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/shell_base_test.h"

namespace testing {

class Group_replication_Test : public tests::Shell_base_test {
 protected:
  void SetUp() {
    tests::Shell_base_test::SetUp();

    // Create instance and Open the session for the tests.
    _connection_options = shcore::get_connection_options(_mysql_uri);
    _session->connect(_connection_options);
    instance = new mysqlshdk::mysql::Instance(_session);
  }

  void TearDown() {
    tests::Shell_base_test::TearDown();

    // Close the session.
    _session->close();
    delete instance;
  }

  std::shared_ptr<mysqlshdk::db::ISession> _session =
      mysqlshdk::db::mysql::Session::create();
  mysqlshdk::mysql::Instance *instance;
  mysqlshdk::db::Connection_options _connection_options;

  bool _gr_req_not_meet = false;
};

TEST_F(Group_replication_Test, plugin_installation) {
  using mysqlshdk::utils::nullable;

  // Check if GR plugin is installed and uninstall it.
  nullable<std::string> init_plugin_state =
      instance->get_plugin_status(mysqlshdk::gr::kPluginName);
  if (!init_plugin_state.is_null()) {
    // Test uninstall the plugin when available.
    bool res = mysqlshdk::gr::uninstall_plugin(*instance);
    EXPECT_TRUE(res);
    nullable<std::string> plugin_state =
        instance->get_plugin_status(mysqlshdk::gr::kPluginName);
    ASSERT_TRUE(plugin_state.is_null());
    // Test trying to uninstall the plugin when not available.
    res = mysqlshdk::gr::uninstall_plugin(*instance);
    EXPECT_FALSE(res);
  }

  // Test installing the plugin (when not installed).
  if (!init_plugin_state.is_null() &&
      (*init_plugin_state).compare(mysqlshdk::gr::kPluginDisabled) == 0) {
    // An exception is expected if the plugin was disabled.
    EXPECT_THROW(mysqlshdk::gr::install_plugin(*instance), std::runtime_error);
  } else {
    // Requirements to install the GR plugin:
    // - server_id != 0
    // - master_info_repository=TABLE
    // - relay_log_info_repository=TABLE
    nullable<int64_t> server_id = instance->get_sysvar_int("server_id");
    if (*server_id == 0) {
      SKIP_TEST("Test server does not meet GR requirements: server_id is 0.");
    }
    nullable<std::string> master_info_repository =
        instance->get_sysvar_string("master_info_repository");
    if ((*master_info_repository).compare("TABLE") != 0) {
      SKIP_TEST(
          "Test server does not meet GR requirements: master_info_repository "
          "must be 'TABLE'.");
    }
    nullable<std::string> relay_log_info_repository =
        instance->get_sysvar_string("relay_log_info_repository");
    if ((*relay_log_info_repository).compare("TABLE") != 0) {
      SKIP_TEST(
          "Test server does not meet GR requirements: "
          "relay_log_info_repository "
          "must be 'TABLE'.");
    }

    // GR plugin is installed and activated (if not previously disabled).
    bool res = mysqlshdk::gr::install_plugin(*instance);
    ASSERT_TRUE(res)
        << "GR plugin was not installed (expected not to be available).";
    nullable<std::string> plugin_state =
        instance->get_plugin_status(mysqlshdk::gr::kPluginName);
    EXPECT_STREQ(mysqlshdk::gr::kPluginActive, (*plugin_state).c_str());

    // Test installing the plugin when already installed.
    res = mysqlshdk::gr::install_plugin(*instance);
    EXPECT_FALSE(res)
        << "GR plugin was installed (expected to be already available).";
    plugin_state = instance->get_plugin_status(mysqlshdk::gr::kPluginName);
    EXPECT_STREQ(mysqlshdk::gr::kPluginActive, (*plugin_state).c_str());
  }

  // Restore initial state (uninstall plugin if needed).
  if (init_plugin_state.is_null()) {
    // Test uninstall the plugin when available.
    bool res = mysqlshdk::gr::uninstall_plugin(*instance);
    EXPECT_TRUE(res);
    nullable<std::string> plugin_state =
        instance->get_plugin_status(mysqlshdk::gr::kPluginName);
    ASSERT_TRUE(plugin_state.is_null());

    // Test trying to uninstall the plugin when not available.
    res = mysqlshdk::gr::uninstall_plugin(*instance);
    EXPECT_FALSE(res);
  }
}

TEST_F(Group_replication_Test, generate_group_name) {
  std::string name1 = mysqlshdk::gr::generate_group_name();
  std::string name2 = mysqlshdk::gr::generate_group_name();
  // Generated group names must be different.
  EXPECT_STRNE(name1.c_str(), name2.c_str());
}

TEST_F(Group_replication_Test, replication_user) {
  // Confirm that there is no replication user.
  auto res =
      mysqlshdk::gr::check_replication_user(*instance, "test_gr_user", "%");
  EXPECT_FALSE(res.user_exists());
  EXPECT_EQ(std::set<std::string>{"REPLICATION SLAVE"},
            res.get_missing_privileges());
  EXPECT_TRUE(res.has_missing_privileges());
  EXPECT_FALSE(res.has_grant_option());

  // Create a replication user.
  std::string passwd;
  mysqlshdk::gr::create_replication_user_random_pass(*instance, "test_gr_user",
                                                     {"%"}, &passwd);
  // Check replication user (now it exist and it has no missing privileges).
  res = mysqlshdk::gr::check_replication_user(*instance, "test_gr_user", "%");

  EXPECT_TRUE(res.user_exists());
  EXPECT_EQ(std::set<std::string>{}, res.get_missing_privileges());
  EXPECT_FALSE(res.has_missing_privileges());
  EXPECT_FALSE(res.has_grant_option());

  // Clean up (remove the create user at the end)
  instance->drop_user("test_gr_user", "%");
}

TEST_F(Group_replication_Test, start_stop_gr) {
  // Beside the start_group_replication() and stop_group_replication()
  // functions, the functions is_member() and get_member_state() are also
  // tested here since the test scenario is the same, in order to avoid
  // additional execution time to run similar test cases.
  // NOTE: START and STOP GROUP_REPLICATION is slow.

  using mysqlshdk::gr::Member_state;
  using mysqlshdk::mysql::Var_qualifier;
  using mysqlshdk::utils::nullable;

  // Check if used server meets the requirements.
  nullable<int64_t> server_id = instance->get_sysvar_int("server_id");
  if (*server_id == 0) {
    SKIP_TEST("Test server does not meet GR requirements: server_id is 0.");
  }
  nullable<bool> log_bin = instance->get_sysvar_bool("log_bin");
  if (*log_bin != true) {
    SKIP_TEST("Test server does not meet GR requirements: log_bin must be ON.");
  }
  nullable<bool> gtid_mode = instance->get_sysvar_bool("gtid_mode");
  if (*gtid_mode != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: gtid_mode must be ON.");
  }
  nullable<bool> enforce_gtid_consistency =
      instance->get_sysvar_bool("enforce_gtid_consistency");
  if (*enforce_gtid_consistency != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: enforce_gtid_consistency "
        "must be ON.");
  }
  nullable<std::string> master_info_repository =
      instance->get_sysvar_string("master_info_repository");
  if ((*master_info_repository).compare("TABLE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: master_info_repository "
        "must be 'TABLE'.");
  }
  nullable<std::string> relay_log_info_repository =
      instance->get_sysvar_string("relay_log_info_repository");
  if ((*relay_log_info_repository).compare("TABLE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: relay_log_info_repository "
        "must be 'TABLE'.");
  }
  nullable<std::string> binlog_checksum =
      instance->get_sysvar_string("binlog_checksum");
  if ((*binlog_checksum).compare("NONE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binlog_checksum must be "
        "'NONE'.");
  }
  nullable<bool> log_slave_updates =
      instance->get_sysvar_bool("log_slave_updates");
  if (*log_slave_updates != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: log_slave_updates must "
        "be ON.");
  }
  nullable<std::string> binlog_format =
      instance->get_sysvar_string("binlog_format");
  if ((*binlog_format).compare("ROW") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binlog_format must be "
        "'ROW'.");
  }

  // Test: member is not part of any group, state must be MISSING.
  bool res = mysqlshdk::gr::is_member(*instance);
  EXPECT_FALSE(res);
  res = mysqlshdk::gr::is_member(*instance, "not_the_group_name");
  EXPECT_FALSE(res);
  Member_state state_res = mysqlshdk::gr::get_member_state(*instance);
  EXPECT_EQ(state_res, Member_state::MISSING);

  // Install GR plugin if needed.
  nullable<std::string> init_plugin_state =
      instance->get_plugin_status(mysqlshdk::gr::kPluginName);
  if (init_plugin_state.is_null()) {
    mysqlshdk::gr::install_plugin(*instance);
  }

  // Get initial value of GR variables (to restore at the end).
  nullable<std::string> gr_group_name =
      instance->get_sysvar_string("group_replication_group_name");
  nullable<std::string> gr_local_address =
      instance->get_sysvar_string("group_replication_local_address");

  // Set GR variable to start GR.
  std::string group_name = mysqlshdk::gr::generate_group_name();
  instance->set_sysvar("group_replication_group_name", group_name,
                       Var_qualifier::GLOBAL);
  std::string local_address = "localhost:13013";
  instance->set_sysvar("group_replication_local_address", local_address,
                       Var_qualifier::GLOBAL);

  // Test: Start Group Replication.
  mysqlshdk::gr::start_group_replication(*instance, true);

  // SUPER READ ONLY must be OFF (verify wait for it to be disable).
  nullable<bool> read_only =
      instance->get_sysvar_bool("super_read_only", Var_qualifier::GLOBAL);
  EXPECT_FALSE(*read_only);

  // Test: member is part of GR group, state must be RECOVERING or ONLINE.
  res = mysqlshdk::gr::is_member(*instance);
  EXPECT_TRUE(res);
  res = mysqlshdk::gr::is_member(*instance, group_name);
  EXPECT_TRUE(res);
  state_res = mysqlshdk::gr::get_member_state(*instance);
  if (state_res == Member_state::ONLINE ||
      state_res == Member_state::RECOVERING)
    SUCCEED();
  else
    ADD_FAILURE() << "Unexpected status after starting GR, member state must "
                     "be ONLINE or RECOVERING";

  // Check GR server status (must be RECOVERING or ONLINE).
  auto session = instance->get_session();
  std::string gr_status_stmt =
      "SELECT MEMBER_STATE "
      "FROM performance_schema.replication_group_members "
      "WHERE MEMBER_ID = @@server_uuid";
  auto resultset = session->query(gr_status_stmt);
  auto row = resultset->fetch_one();
  std::string status = row ? row->get_string(0) : "(empty)";
  if (status.compare("ONLINE") != 0 && status.compare("RECOVERING"))
    ADD_FAILURE() << "Unexpected status after starting GR: " << status;

  // Test: Start Group Replication fails for group already running.
  EXPECT_THROW(mysqlshdk::gr::start_group_replication(*instance, true),
               std::exception);

  // Test: Stop Group Replication.
  mysqlshdk::gr::stop_group_replication(*instance);

  // Starting from MySQL 5.7.20 GR automatically enables super_read_only after
  // stop. Thus, always disable read_only ro consider this situation.
  instance->set_sysvar("super_read_only", false, Var_qualifier::GLOBAL);
  instance->set_sysvar("read_only", false, Var_qualifier::GLOBAL);

  // Test: member is still part of the group, but its state is OFFLINE.
  res = mysqlshdk::gr::is_member(*instance);
  EXPECT_TRUE(res);
  res = mysqlshdk::gr::is_member(*instance, group_name);
  EXPECT_TRUE(res);
  state_res = mysqlshdk::gr::get_member_state(*instance);
  EXPECT_EQ(state_res, Member_state::OFFLINE);

  // Clean up (restore initial server state).
  if (!(*gr_group_name).empty())
    // NOTE: The group_name cannot be set with an empty value.
    instance->set_sysvar("group_replication_group_name", *gr_group_name,
                         Var_qualifier::GLOBAL);
  instance->set_sysvar("group_replication_local_address", *gr_local_address,
                       Var_qualifier::GLOBAL);
  if (init_plugin_state.is_null()) {
    mysqlshdk::gr::uninstall_plugin(*instance);
  }
}

TEST_F(Group_replication_Test, members_state) {
  using mysqlshdk::gr::Member_state;

  // Test to_string() function
  SCOPED_TRACE("to_string() function test");
  std::string str_res = mysqlshdk::gr::to_string(Member_state::ONLINE);
  EXPECT_STREQ("ONLINE", str_res.c_str());
  str_res = mysqlshdk::gr::to_string(Member_state::RECOVERING);
  EXPECT_STREQ("RECOVERING", str_res.c_str());
  str_res = mysqlshdk::gr::to_string(Member_state::OFFLINE);
  EXPECT_STREQ("OFFLINE", str_res.c_str());
  str_res = mysqlshdk::gr::to_string(Member_state::ERROR);
  EXPECT_STREQ("ERROR", str_res.c_str());
  str_res = mysqlshdk::gr::to_string(Member_state::UNREACHABLE);
  EXPECT_STREQ("UNREACHABLE", str_res.c_str());
  str_res = mysqlshdk::gr::to_string(Member_state::MISSING);
  EXPECT_STREQ("(MISSING)", str_res.c_str());

  // Test to_member_state() function
  SCOPED_TRACE("to_member_state() function test");
  Member_state state_res = mysqlshdk::gr::to_member_state("ONLINE");
  EXPECT_EQ(state_res, Member_state::ONLINE);
  state_res = mysqlshdk::gr::to_member_state("RECOVERING");
  EXPECT_EQ(state_res, Member_state::RECOVERING);
  state_res = mysqlshdk::gr::to_member_state("Offline");
  EXPECT_EQ(state_res, Member_state::OFFLINE);
  state_res = mysqlshdk::gr::to_member_state("error");
  EXPECT_EQ(state_res, Member_state::ERROR);
  state_res = mysqlshdk::gr::to_member_state("uNREACHABLE");
  EXPECT_EQ(state_res, Member_state::UNREACHABLE);
  state_res = mysqlshdk::gr::to_member_state("MISSING");
  EXPECT_EQ(state_res, Member_state::MISSING);
  state_res = mysqlshdk::gr::to_member_state("(MISSING)");
  EXPECT_EQ(state_res, Member_state::MISSING);
  EXPECT_THROW(mysqlshdk::gr::to_member_state("invalid"), std::runtime_error);
}

TEST_F(Group_replication_Test, get_replication_user) {
  using mysqlshdk::utils::nullable;

  // Check if used server meets the requirements.
  nullable<int64_t> server_id = instance->get_sysvar_int("server_id");
  if (*server_id == 0) {
    SKIP_TEST("Test server does not meet GR requirements: server_id is 0.");
  }
  nullable<bool> log_bin = instance->get_sysvar_bool("log_bin");
  if (*log_bin != true) {
    SKIP_TEST("Test server does not meet GR requirements: log_bin must be ON.");
  }
  nullable<bool> gtid_mode = instance->get_sysvar_bool("gtid_mode");
  if (*gtid_mode != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: gtid_mode must be ON.");
  }
  nullable<bool> enforce_gtid_consistency =
      instance->get_sysvar_bool("enforce_gtid_consistency");
  if (*enforce_gtid_consistency != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: enforce_gtid_consistency "
        "must be ON.");
  }
  nullable<std::string> master_info_repository =
      instance->get_sysvar_string("master_info_repository");
  if ((*master_info_repository).compare("TABLE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: master_info_repository "
        "must be 'TABLE'.");
  }
  nullable<std::string> relay_log_info_repository =
      instance->get_sysvar_string("relay_log_info_repository");
  if ((*relay_log_info_repository).compare("TABLE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: relay_log_info_repository "
        "must be 'TABLE'.");
  }
  nullable<std::string> binlog_checksum =
      instance->get_sysvar_string("binlog_checksum");
  if ((*binlog_checksum).compare("NONE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binlog_checksum must be "
        "'NONE'.");
  }
  nullable<bool> log_slave_updates =
      instance->get_sysvar_bool("log_slave_updates");
  if (*log_slave_updates != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: log_slave_updates "
        "must be ON.");
  }
  nullable<std::string> binlog_format =
      instance->get_sysvar_string("binlog_format");
  if ((*binlog_format).compare("ROW") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binlog_format must be "
        "'ROW'.");
  }

  // Install GR plugin if needed.
  nullable<std::string> init_plugin_state =
      instance->get_plugin_status(mysqlshdk::gr::kPluginName);
  if (init_plugin_state.is_null()) {
    mysqlshdk::gr::install_plugin(*instance);
  }

  // Test: empty string returned if no replication user was defined (or empty).
  std::string res = mysqlshdk::gr::get_recovery_user(*instance);
  EXPECT_TRUE(res.empty());

  // Set replication user
  auto session = instance->get_session();
  std::string change_master_stmt =
      "CHANGE MASTER TO MASTER_USER = 'test_user' "
      "FOR CHANNEL 'group_replication_recovery'";
  session->execute(change_master_stmt);

  // Test: correct replication user is returned.
  res = mysqlshdk::gr::get_recovery_user(*instance);
  EXPECT_STREQ("test_user", res.c_str());

  // Test: Start Group Replication fails for group already running.
  EXPECT_THROW(mysqlshdk::gr::start_group_replication(*instance, true),
               std::exception);

  // Clean up (restore initial server state).
  if (session)
    // Set user to empty value.
    session->execute(
        "CHANGE MASTER TO MASTER_USER = '' "
        "FOR CHANNEL 'group_replication_recovery'");
  if (init_plugin_state.is_null()) {
    mysqlshdk::gr::uninstall_plugin(*instance);
  }
}

TEST_F(Group_replication_Test, is_group_replication_delayed_starting) {
  using mysqlshdk::db::Type;

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  mock_session
      ->expect_query(
          "SELECT COUNT(*) FROM performance_schema.threads WHERE NAME = "
          "'thread/group_rpl/THD_delayed_initialization'")
      .then_return({{"", {"COUNT(*)"}, {Type::UInteger}, {{"1"}}}});
  EXPECT_TRUE(mysqlshdk::gr::is_group_replication_delayed_starting(instance));

  mock_session
      ->expect_query(
          "SELECT COUNT(*) FROM performance_schema.threads WHERE NAME = "
          "'thread/group_rpl/THD_delayed_initialization'")
      .then_return({{"", {"COUNT(*)"}, {Type::UInteger}, {{"0"}}}});
  EXPECT_FALSE(mysqlshdk::gr::is_group_replication_delayed_starting(instance));
}

}  // namespace testing
