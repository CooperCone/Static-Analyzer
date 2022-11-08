#pragma once

#include "rule.h"

// Procedures cannot have a name that is a keyword in c or c++
// TODO: Find more names that should be restricted
void rule_6_1_a(Rule rule, RuleContext context);

// Procedures cannot have a name that is a function in c stdlib
// TODO: Find more names that should be restricted
void rule_6_1_b(Rule rule, RuleContext context);

// Procedures cannot have a name that starts with an _
void rule_6_1_c(Rule rule, RuleContext context);
