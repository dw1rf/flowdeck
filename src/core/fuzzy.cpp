#include "core/fuzzy.hpp"

#include <algorithm>
#include <cctype>

namespace flowdeck {

namespace {

char Lower(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

bool IsBoundary(char c) {
    return c == ' ' || c == '_' || c == '-' || c == '/' || c == '.' ||
           c == ':';
}

}  // namespace

FuzzyMatch FuzzyScore(std::string_view query, std::string_view text) {
    FuzzyMatch m;
    if (query.empty()) {
        m.score = 1;
        return m;
    }
    if (text.empty() || query.size() > text.size()) return m;

    int score = 0;
    int run = 0;
    size_t ti = 0;
    bool boundary = true;

    for (char qc : query) {
        const char q = Lower(qc);
        bool matched = false;

        while (ti < text.size()) {
            const char t = Lower(text[ti]);
            if (t == q) {
                m.offsets.push_back(static_cast<int>(ti));
                score += boundary ? 12 : (2 + run * 3);
                ++run;
                boundary = false;
                ++ti;
                matched = true;
                break;
            }
            boundary = IsBoundary(t);
            run = 0;
            ++ti;
        }

        if (!matched) {
            m.score = 0;
            m.offsets.clear();
            return m;
        }
    }

    if (Lower(text[0]) == Lower(query[0])) score += 8;      // prefix bonus
    score -= static_cast<int>(text.size() / 16);            // prefer short

    m.score = std::max(score, 1);
    return m;
}

std::vector<int> FuzzyRank(std::string_view query,
                           const std::vector<std::string>& texts) {
    std::vector<int> out;

    if (query.empty()) {
        out.resize(texts.size());
        for (size_t i = 0; i < texts.size(); ++i) out[i] = static_cast<int>(i);
        return out;
    }

    std::vector<std::pair<int, int>> scored;
    scored.reserve(texts.size());
    for (size_t i = 0; i < texts.size(); ++i) {
        const FuzzyMatch m = FuzzyScore(query, texts[i]);
        if (m.score > 0) scored.emplace_back(m.score, static_cast<int>(i));
    }

    std::stable_sort(scored.begin(), scored.end(),
                     [](const auto& a, const auto& b) {
                         return a.first > b.first;
                     });

    out.reserve(scored.size());
    for (const auto& [s, i] : scored) out.push_back(i);
    return out;
}

}  // namespace flowdeck
