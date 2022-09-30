#pragma once

#include "rule.h"

// Verifies that lines not longer than 80 characters
void rule_1_2_a(Rule rule, RuleContext context);

// Verifies braces on own line and closing brace in same column
void rule_1_3_b(Rule rule, RuleContext context);

// Verifies no single letter variable names
// NOTE: Doesn't catch all abbreviations!!!
void rule_1_5_a(Rule rule, RuleContext context);

// Verifies there is no use of auto keyword
void rule_1_7_a(Rule rule, RuleContext context);
