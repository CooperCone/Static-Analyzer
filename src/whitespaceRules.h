#pragma once

#include "rule.h"

// Ensures 1 space after if, while, for, switch, and return
void rule_3_1_a(Rule rule, RuleContext context);

// Ensures 1 space before and after =, +=, -=, *=, /=, %=, &=, |=, and ^=
void rule_3_1_b(Rule rule, RuleContext context);

// Ensures 1 space before and after +, -, *, /, %, <, <=, >, >=, ==,
// !=, <<, >>, &, |, ^, &&, ||
void rule_3_1_c(Rule rule, RuleContext context);
