/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#ifdef HAVE_PYTHON
#include "mysqlshdk/include/scripting/python_utils.h"
#endif

#include <mysql.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "shellcore/shell_core_options.h"
#include "shellcore/interrupt_handler.h"
#include "unittest/gtest_clean.h"

#ifdef WIN32
#define putenv _putenv
#endif

extern "C" {
const char *g_argv0 = nullptr;
}
char *g_mppath = nullptr;

static void check_zombie_sandboxes() {
  int port = 3306;
  if (getenv("MYSQL_PORT")) {
    port = atoi(getenv("MYSQL_PORT"));
  }
  int sport1, sport2, sport3;

  const char *sandbox_port1 = getenv("MYSQL_SANDBOX_PORT1");
  if (sandbox_port1) {
    sport1 = atoi(getenv("MYSQL_SANDBOX_PORT1"));
  } else {
    sport1 = port + 10;
  }
  const char *sandbox_port2 = getenv("MYSQL_SANDBOX_PORT2");
  if (sandbox_port2) {
    sport2 = atoi(getenv("MYSQL_SANDBOX_PORT2"));
  } else {
    sport2 = port + 20;
  }
  const char *sandbox_port3 = getenv("MYSQL_SANDBOX_PORT3");
  if (sandbox_port3) {
    sport3 = atoi(getenv("MYSQL_SANDBOX_PORT3"));
  } else {
    sport3 = port + 30;
  }

  bool have_zombies = false;

  MYSQL mysql;
  mysql_init(&mysql);
  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(&mysql, MYSQL_OPT_PROTOCOL, &tcp);
  // if connect succeeds or error is a server error, then there's a server
  if (mysql_real_connect(&mysql, "localhost", "root", "", NULL, sport1, NULL,
                         0) ||
      mysql_errno(&mysql) < 2000 || mysql_errno(&mysql) >= 3000) {
    std::cout << mysql_error(&mysql) << "  " << mysql_errno(&mysql) << "\n";
    std::cout << "Server already running on port " << sport1 << "\n";
    have_zombies = true;
  }
  mysql_init(&mysql);
  mysql_options(&mysql, MYSQL_OPT_PROTOCOL, &tcp);
  if (mysql_real_connect(&mysql, "localhost", "root", "", NULL, sport2, NULL,
                         0) ||
      mysql_errno(&mysql) < 2000 || mysql_errno(&mysql) >= 3000) {
    std::cout << mysql_error(&mysql) << "  " << mysql_errno(&mysql) << "\n";
    std::cout << "Server already running on port " << sport2 << "\n";
    have_zombies = true;
  }
  mysql_init(&mysql);
  mysql_options(&mysql, MYSQL_OPT_PROTOCOL, &tcp);
  if (mysql_real_connect(&mysql, "localhost", "root", "", NULL, sport3, NULL,
                         0) ||
      mysql_errno(&mysql) < 2000 || mysql_errno(&mysql) >= 3000) {
    std::cout << mysql_error(&mysql) << "  " << mysql_errno(&mysql) << "\n";
    std::cout << "Server already running on port " << sport3 << "\n";
    have_zombies = true;
  }

  if (have_zombies) {
    std::cout << "WARNING: mysqld running on port reserved for sandbox tests\n";
    std::cout << "Sandbox ports: " << sport1 << ", " << sport2 << ", " << sport3
              << "\n";
    std::cout << "If they're left from a previous run, terminate them first\n";
    std::cout << "Or setenv TEST_SKIP_ZOMBIE_CHECK to skip this check\n";
    std::cout << "Or setenv MYSQL_SANDBOX_PORT1..3 to pick different ports for "
                 "test sandboxes\n";
    exit(1);
  }

  const char *tmpdir = getenv("TMPDIR");
  if (tmpdir) {
    std::string zombies;
    std::string d;
    d = tmpdir;
    d.append("/").append(std::to_string(sport1));
    if (shcore::file_exists(d)) {
      zombies.append(d).append("\n");
    }
    d = tmpdir;
    d.append("/").append(std::to_string(sport2));
    if (shcore::file_exists(d)) {
      zombies.append(d).append("\n");
    }
    d = tmpdir;
    d.append("/").append(std::to_string(sport3));
    if (shcore::file_exists(d)) {
      zombies.append(d).append("\n");
    }
    if (!zombies.empty()) {
      std::cout << "The following sandbox directories seem to be leftover and "
                   "must be deleted:\n";
      std::cout << zombies;
      exit(1);
    }
  }
}

int main(int argc, char **argv) {
  g_argv0 = argv[0];
#ifdef HAVE_V8
  extern void JScript_context_init();

  JScript_context_init();
#endif
  // init the ^C handler, so it knows what's the main thread
  shcore::Interrupts::init(nullptr);


  bool show_help = false;
  if (const char *uri = getenv("MYSQL_URI")) {
    if (strcmp(uri, "root@localhost") != 0 &&
        strcmp(uri, "root@127.0.0.1") != 0) {
      std::cerr << "MYSQL_URI is set to " << getenv("MYSQL_URI") << "\n";
      std::cerr << "MYSQL_URI environment variable is no longer supported.\n";
      std::cerr << "Tests must run against local server using root user.\n";
      show_help = true;
    }
  }

  if (show_help) {
    std::cerr
        << "The following environment variables are available:\n"
        << "MYSQL_PORT classic protocol port for local MySQL (default 3306)\n"
        << "MYSQLX_PORT X protocol port for local MySQL (default 33060)\n"
        << "MYSQL_PWD root password for local MySQL server (default "
           ")\n"
        // << "MYSQL_REMOTE_HOST for tests against remove MySQL (default not
        // set)\n"
        // << "MYSQL_REMOTE_PWD root password for remote MySQL server (default
        // "")\n"
        // << "MYSQL_REMOTE_PORT classic port for remote MySQL (default 3306)\n"
        // << "MYSQLX_REMOTE_PORT X port for remote MySQL (default 33060)\n\n"
        << "MYSQL_SANDBOX_PORT1, MYSQL_SANDBOX_PORT2, MYSQL_SANDBOX_PORT3\n"
        << "    ports to use for test sandbox instances. X protocol will use\n"
        << "    MYSQL_SANDBOX_PORT1 * 10\n";
    exit(1);
  }

  if (!getenv("MYSQL_PORT")) {
    if (putenv(const_cast<char *>("MYSQL_PORT=3306")) != 0) {
      std::cerr << "MYSQL_PORT was not set and putenv failed to set it\n";
      exit(1);
    }
  }

  if (!getenv("MYSQLX_PORT")) {
    if (putenv(const_cast<char *>("MYSQLX_PORT=33060")) != 0) {
      std::cerr << "MYSQLX_PORT was not set and putenv failed to set it\n";
      exit(1);
    }
  }

  if (!getenv("MYSQL_REMOTE_HOST")) {
    static char hostname[1024] = "MYSQL_REMOTE_HOST=";
    if (gethostname(hostname + strlen(hostname),
                    sizeof(hostname) - strlen(hostname)) != 0) {
      std::cerr << "gethostname() returned error: " << strerror(errno) << "\n";
      std::cerr << "Set MYSQL_REMOTE_HOST\n";
      // exit(1); this option is not used for now, so no need to fail
    }
    if (putenv(hostname) != 0) {
      std::cerr
          << "MYSQL_REMOTE_HOST was not set and putenv failed to set it\n";
      // exit(1);
    }
    std::cout << "Set default " << hostname << "\n";
  }

  if (!getenv("MYSQL_REMOTE_PORT")) {
    if (putenv(const_cast<char *>("MYSQL_REMOTE_PORT=3306")) != 0) {
      std::cerr
          << "MYSQL_REMOTE_PORT was not set and putenv failed to set it\n";
      // exit(1);
    }
  }

  if (!getenv("MYSQLX_REMOTE_PORT")) {
    if (putenv(const_cast<char *>("MYSQLX_REMOTE_PORT=33060")) != 0) {
      std::cerr
          << "MYSQLX_REMOTE_PORT was not set and putenv failed to set it\n";
      // exit(1);
    }
  }

// Override the configuration home for tests, to not mess with custom data
#ifdef WIN32
  _putenv_s("MYSQLSH_USER_CONFIG_HOME", ".");
#else
  setenv("MYSQLSH_USER_CONFIG_HOME", ".", 1);
#endif
  ::testing::InitGoogleTest(&argc, argv);

  // Helper code for DBA specific groups of tests;
  std::string flags = ::testing::GTEST_FLAG(filter);
  //::testing::FLAGS_gtest_break_on_failure = true;
  if (!flags.empty()) {
    std::string new_flags;

    if (flags == "DBA")
      new_flags = "Shell_py_dba_tests.*:Shell_js_dba_tests.*";
    else if (flags == "DBAJS")
      new_flags = "Shell_js_dba_tests.*";
    else if (flags == "DBAPY")
      new_flags = "Shell_py_dba_tests.*";
    else if (flags == "DBAJSNIG")
      new_flags =
          "Shell_js_dba_tests.no_interactive_deploy*:Shell_js_dba_tests.no_"
          "interactive_classic_global*";
    else if (flags == "DBAPYNIG")
      new_flags =
          "Shell_py_dba_tests.no_interactive_deploy*:Shell_py_dba_tests.no_"
          "interactive_classic_global*";
    else if (flags == "DBAJSNIGDBA")
      new_flags =
          "Shell_js_dba_tests.no_interactive_deploy*:Shell_js_dba_tests.no_"
          "interactive_classic_global_dba";
    else if (flags == "DBAPYNIGDBA")
      new_flags =
          "Shell_py_dba_tests.no_interactive_deploy*:Shell_py_dba_tests.no_"
          "interactive_classic_global_dba";
    else if (flags == "DBAJSNIC")
      new_flags =
          "Shell_js_dba_tests.no_interactive_deploy*:Shell_js_dba_tests.no_"
          "interactive_classic_custom*";
    else if (flags == "DBAPYNIC")
      new_flags =
          "Shell_py_dba_tests.no_interactive_deploy*:Shell_py_dba_tests.no_"
          "interactive_classic_custom*";
    else if (flags == "DBAJSNICDBA")
      new_flags =
          "Shell_js_dba_tests.no_interactive_deploy*:Shell_js_dba_tests.no_"
          "interactive_classic_custom_dba";
    else if (flags == "DBAPYNICDBA")
      new_flags =
          "Shell_py_dba_tests.no_interactive_deploy*:Shell_py_dba_tests.no_"
          "interactive_classic_custom_dba";
    else if (flags == "DBAJSNI")
      new_flags =
          "Shell_js_dba_tests.no_interactive_deploy*:Shell_js_dba_tests.no_"
          "interactive_classic_*";
    else if (flags == "DBAPYNI")
      new_flags =
          "Shell_py_dba_tests.no_interactive_deploy*:Shell_py_dba_tests.no_"
          "interactive_classic_*";
    else if (flags == "DBAJSIG")
      new_flags =
          "Shell_js_dba_tests.no_interactive_deploy*:Shell_js_dba_tests."
          "interactive_classic_global*";
    else if (flags == "DBAPYIG")
      new_flags =
          "Shell_py_dba_tests.no_interactive_deploy*:Shell_py_dba_tests."
          "interactive_classic_global*";
    else if (flags == "DBAJSIGDBA")
      new_flags =
          "Shell_js_dba_tests.no_interactive_deploy*:Shell_js_dba_tests."
          "interactive_classic_global_dba";
    else if (flags == "DBAPYIGDBA")
      new_flags =
          "Shell_py_dba_tests.no_interactive_deploy*:Shell_py_dba_tests."
          "interactive_classic_global_dba";
    else if (flags == "DBAJSIC")
      new_flags =
          "Shell_js_dba_tests.no_interactive_deploy*:Shell_js_dba_tests."
          "interactive_classic_custom*";
    else if (flags == "DBAPYIC")
      new_flags =
          "Shell_py_dba_tests.no_interactive_deploy*:Shell_py_dba_tests."
          "interactive_classic_custom*";
    else if (flags == "DBAJSICDBA")
      new_flags =
          "Shell_js_dba_tests.no_interactive_deploy*:Shell_js_dba_tests."
          "interactive_classic_custom_dba";
    else if (flags == "DBAPYICDBA")
      new_flags =
          "Shell_py_dba_tests.no_interactive_deploy*:Shell_py_dba_tests."
          "interactive_classic_custom_dba";
    else if (flags == "DBAJSI")
      new_flags =
          "Shell_js_dba_tests.no_interactive_deploy*:Shell_js_dba_tests."
          "interactive_classic_*";
    else if (flags == "DBAPYI")
      new_flags =
          "Shell_py_dba_tests.no_interactive_deploy*:Shell_py_dba_tests."
          "interactive_classic_*";
    else if (flags == "ALLBUTDBA")
      new_flags = "*:-Shell_py_dba_tests.*:Shell_js_dba_tests.*";

    if (!new_flags.empty())
      ::testing::GTEST_FLAG(filter) = new_flags.c_str();
  }

  const char *generate_option = "--generate_test_groups=";
  if (argc > 1 &&
      strncmp(argv[1], generate_option, strlen(generate_option)) == 0) {
    const char *path = strchr(argv[1], '=') + 1;
    std::ofstream f(path);

    std::cout << "Updating " << path << "...\n";
    f << "# Automatically generated, use make testgroups to update\n";

    ::testing::UnitTest *ut = ::testing::UnitTest::GetInstance();
    for (int i = 0; i < ut->total_test_case_count(); i++) {
      const char *name = ut->GetTestCase(i)->name();
      f << "add_test(" << name << " run_unit_tests --gtest_filter=" << name
        << ".*)\n";
    }
    return 0;
  }

  // Check for leftover sandbox servers
  if (!getenv("TEST_SKIP_ZOMBIE_CHECK")) {
    check_zombie_sandboxes();
  }
  std::string mppath;
  char *p = strrchr(argv[0], '/');
  if (p) {
    mppath = std::string(argv[0], p - argv[0]);
  } else {
    p = strrchr(argv[0], '\\');
    mppath = std::string(argv[0], p - argv[0]);
  }
#ifndef _WIN32
  // On linux, we need to tell the UTs where the mysqlprovision executable is
  mppath.append("/../mysqlprovision");
  (*shcore::Shell_core_options::get())[SHCORE_GADGETS_PATH] =
      shcore::Value(mppath);
#endif
  g_mppath = strdup(mppath.c_str());

  int ret_val = RUN_ALL_TESTS();

  return ret_val;
}
