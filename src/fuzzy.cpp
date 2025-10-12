#include "fuzzy.h"
#include <QDebug>

namespace Fuzzy {

inline auto isWordBoundary(QChar prev, QChar curr) -> bool
{
    if (prev.isNull()) return true;
    if (prev == '/' || prev == '\\'  || prev == '_' || prev == '-' || prev == '.' || prev.isSpace())
        return true;
    if (prev.isLower() && curr.isUpper())
        return true;
    return false;
}


auto levenshteinDistance(const QStringView s1, const QStringView s2) -> int
{
    auto len1 = s1.length();
    auto len2 = s2.length();
    QVector<int> col(len2 + 1);
    QVector<int> prevCol(len2 + 1);

    for (auto i = 0; i < prevCol.size(); i++) {
        prevCol[i] = i;
    }

    for (auto  i = 0; i < len1; i++) {
        col[0] = i + 1;
        for (auto j = 0; j < len2; j++) {
            auto cost = (s1.at(i).toLower() == s2.at(j).toLower()) ? 0 : 1;
            col[j + 1] = std::min({prevCol[j + 1] + 1, col[j] + 1, prevCol[j] + cost});
        }
        col.swap(prevCol);
    }
    return prevCol[len2];
}


auto score(const QStringView query, const QStringView target) -> MatchResult
{
    if (target.isEmpty()) {
        return {0.0, {}};
    }

    if (query.isEmpty()) {
        return {100.0, {}}; 
    }

    auto score = 0.0;
    auto consecutive = 0;
    auto qi = 0;
    auto ti = 0;
    QVector<int> indices;
    indices.reserve(query.size());

    while (qi < query.size() && ti < target.size()) {
        auto qc = query[qi].toCaseFolded();
        auto tc = target[ti].toCaseFolded();

        if (qc == tc) {
            auto charScore = BASE_SCORE;

            if (query[qi] == target[ti])
                charScore += CASE_BONUS;

            if (ti == 0 && qi == 0)
                charScore += START_BONUS;

            if (isWordBoundary(ti > 0 ? target[ti - 1] : QChar(), target[ti]))
                charScore += WORD_BONUS;

            if (!indices.isEmpty() && ti == indices.last() + 1)
                consecutive++;
            else
                consecutive = 0;

            charScore += consecutive * CONSEC_BONUS;
            score += charScore;
            indices.append(static_cast<int>(ti));
            qi++;
        }
        ti++;
    }

    if (qi < query.size())
        return {0.0, {}};

    for (auto i = 1; i < indices.size(); ++i) {
        int gap = indices[i] - indices[i - 1] - 1;
        score -= gap * GAP_PENALTY;
    }

    auto trailing = target.size() - indices.last() - 1;
    score -= trailing * TRAIL_PENALTY;
    score = (score / target.size()) * 100.0;
    score = std::max(0.0, score);

    return {score, indices};
}

auto scoreSimple(const QStringView query, const QStringView target) -> double {
    auto s = score(query, target);
    return s.score;
}

} // namespace Fuzzy
