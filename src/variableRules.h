#pragma once

#include "rule.h"

// No variable names can be the same as keywords in c++
void rule_7_1_a(Rule rule, RuleContext context);

// No variable names can be the same as c standard library names
void rule_7_1_b(Rule rule, RuleContext context);
