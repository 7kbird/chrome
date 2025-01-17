// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/config.h"
#include "tools/gn/config_values_extractors.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scope.h"

namespace {

struct FlagWriter {
  void operator()(const std::string& dir, std::ostream& out) const {
    out << dir << " ";
  }
};

struct IncludeWriter {
  void operator()(const SourceDir& dir, std::ostream& out) const {
    out << dir.value() << " ";
  }
};

}  // namespace

TEST(ConfigValuesExtractors, IncludeOrdering) {
  TestWithScope setup;

  // Construct a chain of dependencies: target -> dep1 -> dep2
  // Add representative values: cflags (opaque, always copied) and include_dirs
  // (uniquified) to each one so we can check what comes out the other end.

  // Set up dep2, direct and all dependent configs.
  Config dep2_all(setup.settings(), Label(SourceDir("//dep2/"), "all"));
  dep2_all.config_values().cflags().push_back("--dep2-all");
  dep2_all.config_values().include_dirs().push_back(SourceDir("//dep2/all/"));

  Config dep2_direct(setup.settings(), Label(SourceDir("//dep2/"), "direct"));
  dep2_direct.config_values().cflags().push_back("--dep2-direct");
  dep2_direct.config_values().include_dirs().push_back(
      SourceDir("//dep2/direct/"));

  Target dep2(setup.settings(), Label(SourceDir("//dep2/"), "dep2"));
  dep2.set_output_type(Target::SOURCE_SET);
  dep2.SetToolchain(setup.toolchain());
  dep2.all_dependent_configs().push_back(LabelConfigPair(&dep2_all));
  dep2.direct_dependent_configs().push_back(LabelConfigPair(&dep2_direct));

  // Set up dep1, direct and all dependent configs.
  Config dep1_all(setup.settings(), Label(SourceDir("//dep1/"), "all"));
  dep1_all.config_values().cflags().push_back("--dep1-all");
  dep1_all.config_values().include_dirs().push_back(SourceDir("//dep1/all/"));

  Config dep1_direct(setup.settings(), Label(SourceDir("//dep1/"), "direct"));
  dep1_direct.config_values().cflags().push_back("--dep1-direct");
  dep1_direct.config_values().include_dirs().push_back(
      SourceDir("//dep1/direct/"));

  Target dep1(setup.settings(), Label(SourceDir("//dep1/"), "dep1"));
  dep1.set_output_type(Target::SOURCE_SET);
  dep1.SetToolchain(setup.toolchain());
  dep1.all_dependent_configs().push_back(LabelConfigPair(&dep1_all));
  dep1.direct_dependent_configs().push_back(LabelConfigPair(&dep1_direct));
  dep1.deps().push_back(LabelTargetPair(&dep2));

  // Set up target, direct and all dependent configs.
  Config target_all(setup.settings(), Label(SourceDir("//target/"), "all"));
  target_all.config_values().cflags().push_back("--target-all");
  target_all.config_values().include_dirs().push_back(
      SourceDir("//target/all/"));

  Config target_direct(setup.settings(),
                       Label(SourceDir("//target/"), "direct"));
  target_direct.config_values().cflags().push_back("--target-direct");
  target_direct.config_values().include_dirs().push_back(
      SourceDir("//target/direct/"));

  // This config is applied directly to target.
  Config target_config(setup.settings(),
                       Label(SourceDir("//target/"), "config"));
  target_config.config_values().cflags().push_back("--target-config");
  target_config.config_values().include_dirs().push_back(
      SourceDir("//target/config/"));

  Target target(setup.settings(), Label(SourceDir("//target/"), "target"));
  target.set_output_type(Target::SOURCE_SET);
  target.SetToolchain(setup.toolchain());
  target.all_dependent_configs().push_back(LabelConfigPair(&target_all));
  target.direct_dependent_configs().push_back(LabelConfigPair(&target_direct));
  target.configs().push_back(LabelConfigPair(&target_config));
  target.deps().push_back(LabelTargetPair(&dep1));


  // Additionally add some values directly on "target".
  target.config_values().cflags().push_back("--target");
  target.config_values().include_dirs().push_back(
      SourceDir("//target/"));

  // Mark targets resolved. This should push dependent configs.
  dep2.OnResolved();
  dep1.OnResolved();
  target.OnResolved();

  // Verify cflags by serializing.
  std::ostringstream flag_out;
  FlagWriter flag_writer;
  RecursiveTargetConfigToStream<std::string, FlagWriter>(
      &target, &ConfigValues::cflags, flag_writer, flag_out);
  EXPECT_EQ(flag_out.str(),
            "--target --target-config --target-all --target-direct "
            "--dep1-all --dep2-all --dep1-direct ");

  // Verify include dirs by serializing.
  std::ostringstream include_out;
  IncludeWriter include_writer;
  RecursiveTargetConfigToStream<SourceDir, IncludeWriter>(
      &target, &ConfigValues::include_dirs, include_writer, include_out);
  EXPECT_EQ(include_out.str(),
            "//target/ //target/config/ //target/all/ //target/direct/ "
            "//dep1/all/ //dep2/all/ //dep1/direct/ ");
}
