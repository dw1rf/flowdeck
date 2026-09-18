#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace flowdeck {

struct FuzzyMatch {
    int score = 0;             // 0 = no match, higher is better
    std::vector<int> offsets;  // matched character indices in the haystack
};

// Case-insensitive subsequence match with bonuses for word boundaries,
// consecutive runs and prefix matches.
FuzzyMatch FuzzyScore(std::string_view query, std::string_view text);

// Rank `texts` against `query`; returns indices sorted best-first,
// dropping non-matches. An empty query keeps the original order.
std::vector<int> FuzzyRank(std::string_view query,
                           const std::vector<std::string>& texts);

}  // namespace flowdeck
