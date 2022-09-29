#pragma once

#include "rule.h"

// Verifies that lines not longer than 80 characters
void rule_1_2_a(Rule rule, RuleContext context);

// Verifies braces on own line and closing brace in same column
void rule_1_3_b(Rule rule, RuleContext);
