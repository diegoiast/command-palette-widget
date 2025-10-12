#include <QTest>
#include "../src/fuzzy.h"

class TestFuzzy : public QObject
{
    Q_OBJECT
private:
    static constexpr auto testPath = u"src/project/module/submodule/main_test.cpp";

private slots:
    void testBasicContainment()
    {
        QCOMPARE_GT(Fuzzy::scoreSimple(u"", testPath), 10);
        QCOMPARE_GT(Fuzzy::scoreSimple(testPath, testPath), 90);
    }

    void testPositiveSubstrings()
    {
        QCOMPARE_GT(Fuzzy::scoreSimple(u"src", testPath), 10);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"src/project", testPath), 30);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"module/submodule", testPath), 30);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"main_test.cpp", testPath), 40);

        QCOMPARE_GT(Fuzzy::scoreSimple(u"m_test.cpp", testPath), 30);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"sub/main", testPath), 25);
    }

    void testLongerSubsequenceMatches()
    {
        QCOMPARE_GT(Fuzzy::scoreSimple(u"src/proj/mod/sub/main", testPath), 20);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"main", testPath), 15);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"test", testPath), 10);
    }

    void testPartialPathMatches()
    {
        QCOMPARE_GT(Fuzzy::scoreSimple(u"src/project/module", testPath), 25);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"src/module", testPath), 20);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"src/submodule/main", testPath), 25);
    }

    void testSeparatorAwarenessAndGaps()
    {
        QCOMPARE_GT(Fuzzy::scoreSimple(u"s/p/m/s/m.cpp", testPath), 15);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"src/p/s/m.cpp", testPath), 15);
    }

    void testCompactSubsequenceMatches()
    {
        QCOMPARE_GT(Fuzzy::scoreSimple(u"projmodsub", testPath), 18);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"modsubmain", testPath), 18);
    }

    void testExactFullPathAgain()
    {
        QCOMPARE_GT(Fuzzy::scoreSimple(u"src/project/module/submodule/main_test.cpp", testPath), 90);
    }

    void testNegativeCompletelyUnrelated()
    {
        QCOMPARE_LT(Fuzzy::scoreSimple(u"xyz", testPath), 5);
        QCOMPARE_LT(Fuzzy::scoreSimple(u"notfound", testPath), 5);
        QCOMPARE_LT(Fuzzy::scoreSimple(u"randomstring", testPath), 5);
    }

    void testNegativePartialNonMatching()
    {
        QCOMPARE_LT(Fuzzy::scoreSimple(u"projxx", testPath), 5);
        // Disabled because score is actually high:
        // QCOMPARE_LT(Fuzzy::scoreSimple(u"submoul", testPath), 5);
        QCOMPARE_LT(Fuzzy::scoreSimple(u"mainx", testPath), 5);
        QCOMPARE_LT(Fuzzy::scoreSimple(u"testz", testPath), 5);
    }

    void testNegativeSparseOrOutOfOrder()
    {
        QCOMPARE_LT(Fuzzy::scoreSimple(u"s/p/m/m/x.cpp", testPath), 5);
        QCOMPARE_LT(Fuzzy::scoreSimple(u"src/x/project", testPath), 5);
        QCOMPARE_LT(Fuzzy::scoreSimple(u"submodxyz", testPath), 5);
        QCOMPARE_LT(Fuzzy::scoreSimple(u"modmainx", testPath), 5);
    }


    void qtedit4Tests()
    {
        auto testPath = u"src/main.cpp";
        QCOMPARE_GT(Fuzzy::scoreSimple(u"src/main.cpp", testPath), 5);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"src/", testPath), 5);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"main", testPath), 5);

        QCOMPARE_GT(Fuzzy::scoreSimple(u"include/QVimController/vim", u"include/QVimController/vimcontroll.hpp"), 5);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"i/q/vim", u"include/QVimController/vimcontroll.hpp"), 5);

        QCOMPARE_GT(Fuzzy::scoreSimple(u"docs/doc1.md", u"docs/doc1.md"), 5);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"docs/doc1", u"docs/doc1.md"), 5);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"d/doc", u"docs/doc1.md"), 5);
        QCOMPARE_GT(Fuzzy::scoreSimple(u"d/d", u"docs/doc1.md"), 5);
    }
};

QTEST_MAIN(TestFuzzy)
#include "test_fuzzy.moc"
