/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <algorithm>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

#include "DexClass.h"
#include "DexInstruction.h"
#include "DexLoader.h"
#include "Show.h"
#include "VerifyUtil.h"

namespace {
// We use this ugly macro expansion instead of loops for better gtest reporting.
// (Name, Expected # of code-unit reduction)
#define TESTS                                               \
  WORK(test_Coalesce_InitVoid_AppendString, 4)              \
  WORK(test_Coalesce_AppendString_AppendString, 2 * 6)      \
  WORK(test_CompileTime_StringLength, 4 * 4)                \
  WORK(test_Remove_AppendEmptyString, 0 * 5)                \
  WORK(test_Coalesce_Init_AppendChar, 0)                    \
  WORK(test_Coalesce_AppendString_AppendInt, 5 + 3 * 6 + 7) \
  WORK(test_Coalesce_AppendString_AppendChar, 0)            \
  WORK(test_Coalesce_AppendString_AppendBoolean, 2 * 5)     \
  WORK(test_Coalesce_AppendString_AppendLongInt, 9 + 6 + 7) \
  WORK(test_CompileTime_StringCompare, 4 * 7)               \
  WORK(test_Replace_ValueOfBoolean, 2 * 3)                  \
  WORK(test_Replace_ValueOfChar, 0)                         \
  WORK(test_Replace_ValueOfInt, 3 * 3 + 3 * 4 + 2 * 5)      \
  WORK(test_Replace_ValueOfLongInt, 7 + 4 + 5)              \
  WORK(test_Replace_ValueOfFloat, 1 * 5)                    \
  WORK(test_Replace_ValueOfDouble, 7 + 4)

void loadMethodSizes(DexClasses& classes,
                     std::unordered_map<std::string, int>& map) {
  auto cls = find_class_named(classes,
                              "Lcom/facebook/redex/test/instr/SimplifyString;");
  ASSERT_NE(nullptr, cls);

#define WORK(name, ...)                                   \
  {                                                       \
    auto method_##name = find_vmethod_named(*cls, #name); \
    ASSERT_NE(nullptr, method_##name);                    \
    map[#name] = method_##name->get_code()->size();       \
  }
  TESTS
#undef WORK
}
}

struct PrePostVerify : testing::Test {
  std::unordered_map<std::string, int> before_sizes;
  std::unordered_map<std::string, int> after_sizes;

  PrePostVerify() {
    g_redex = new RedexContext;
    DexClasses before_classes(load_classes_from_dex(std::getenv("dex_pre")));
    loadMethodSizes(before_classes, before_sizes);
    delete g_redex;

    g_redex = new RedexContext;
    DexClasses after_classes(load_classes_from_dex(std::getenv("dex_post")));
    loadMethodSizes(after_classes, after_sizes);
    delete g_redex;

    g_redex = nullptr;
  }

  ~PrePostVerify() {}
};

// To verify whether Redex replaced the patterns successfully, we compute the
// differences of the before/after methods.
TEST_F(PrePostVerify, CheckSizes) {
#define WORK(name, saving)                                       \
  {                                                              \
    auto diff_##name = before_sizes[#name] - after_sizes[#name]; \
    constexpr int expected_saving = saving;                      \
    EXPECT_EQ(expected_saving, diff_##name);                     \
  }
  TESTS
#undef WORK
}
