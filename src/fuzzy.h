#pragma once
#include <QString>
#include <QVector>
#include <qanystringview.h>

namespace Fuzzy {

// --- tunable constants -------------------------------------------------------
constexpr auto BASE_SCORE    = 1.0;
constexpr auto CASE_BONUS    = 0.1;
constexpr auto CONSEC_BONUS  = 1.0;
constexpr auto WORD_BONUS    = 2.0;
constexpr auto START_BONUS   = 1.5;
constexpr auto GAP_PENALTY   = 0.15;
constexpr auto TRAIL_PENALTY = 0.05;

struct MatchResult {
    double score{};
    QVector<int> indices{};
};


auto levenshteinDistance(const QStringView s1, const QStringView s2) -> int;

auto score(const QStringView query, const QStringView target) -> MatchResult;
auto scoreSimple(const QStringView query, const QStringView target) -> double;

}
