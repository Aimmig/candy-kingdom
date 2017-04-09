/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

#include <gtest/gtest.h>
#include <candy/rsil/BranchingHeuristics.h>
#include <candy/testutils/TestUtils.h>

namespace Candy {
    
    using TestedRSILBranchingHeuristic = RSILBranchingHeuristic3;
    using TestedRSILVanishingBranchingHeuristic = RSILVanishingBranchingHeuristic3;
    
    template<class Heuristic>
    static void test_uninitializedHeuristicReturnsUndefForMinInput() {
        Heuristic underTest;
        auto result = underTest.getAdvice({mkLit(0)}, {0}, {}, {0});
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, uninitializedHeuristicReturnsUndefForMinInput) {
        test_uninitializedHeuristicReturnsUndefForMinInput<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, uninitializedHeuristicReturnsUndefForMinInput) {
        test_uninitializedHeuristicReturnsUndefForMinInput<TestedRSILVanishingBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_emptyInitializedHeuristicReturnsUndefForMinInput() {
        typename Heuristic::Parameters params{{}};
        Heuristic underTest{params};
        
        auto result = underTest.getAdvice({mkLit(0)}, {0}, {}, {0});
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, emptyInitializedHeuristicReturnsUndefForMinInput) {
        test_emptyInitializedHeuristicReturnsUndefForMinInput<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, emptyInitializedHeuristicReturnsUndefForMinInput) {
        test_emptyInitializedHeuristicReturnsUndefForMinInput<TestedRSILBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesAdviceForSingleEquivalence() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        typename Heuristic::Parameters params{testData};
        Heuristic underTest{params};
        
        typename Heuristic::TrailType testTrail {mkLit(1,0)};
        typename Heuristic::TrailLimType testTrailLim {0};
        typename Heuristic::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        typename Heuristic::AssignsType testAssigns {l_True, l_False, l_Undef, l_Undef, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        ASSERT_NE(result, lit_Undef);
        EXPECT_EQ(result, mkLit(2, 0));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        test_givesAdviceForSingleEquivalence<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        test_givesAdviceForSingleEquivalence<TestedRSILVanishingBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfAssigned() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        typename Heuristic::Parameters params{testData};
        Heuristic underTest{params};
        
        typename Heuristic::TrailType testTrail {mkLit(1,0)};
        typename Heuristic::TrailLimType testTrailLim {0};
        typename Heuristic::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        typename Heuristic::AssignsType testAssigns {l_True, l_False, l_True, l_Undef, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILVanishingBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        typename Heuristic::Parameters params{testData};
        Heuristic underTest{params};
        
        typename Heuristic::TrailType testTrail {mkLit(1,0)};
        typename Heuristic::TrailLimType testTrailLim {0};
        typename Heuristic::DecisionType testDecisionVars {1, 1, 0, 1, 1};
        typename Heuristic::AssignsType testAssigns {l_True, l_False, l_True, l_Undef, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILVanishingBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfIrrelevant() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        typename Heuristic::Parameters params{testData};
        Heuristic underTest{params};
        
        typename Heuristic::TrailType testTrail {mkLit(4,0)};
        typename Heuristic::TrailLimType testTrailLim {0};
        typename Heuristic::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        typename Heuristic::AssignsType testAssigns {l_True, l_True, l_Undef, l_Undef, l_False};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILVanishingBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesAdviceForSingleEquivalenceSize3() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        typename Heuristic::Parameters params{testData};
        Heuristic underTest{params};
        
        typename Heuristic::TrailType testTrail {mkLit(3,0)};
        typename Heuristic::TrailLimType testTrailLim {0};
        typename Heuristic::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        typename Heuristic::AssignsType testAssigns {l_Undef, l_Undef, l_Undef, l_False, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        ASSERT_NE(result, lit_Undef);
        
        EXPECT_TRUE((result == mkLit(1, 0)) || (result == mkLit(2,0)));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILVanishingBranchingHeuristic>();
    }
    
    TEST(RSILBranchingHeuristicsTests, travelsUpTrail) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        TestedRSILBranchingHeuristic::Parameters params{testData};
        TestedRSILBranchingHeuristic underTest{params};
        
        TestedRSILBranchingHeuristic::TrailType testTrail {mkLit(3,0), mkLit(4,0)};
        TestedRSILBranchingHeuristic::TrailLimType testTrailLim {0};
        TestedRSILBranchingHeuristic::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        TestedRSILBranchingHeuristic::AssignsType testAssigns {l_Undef, l_Undef, l_Undef, l_False, l_False};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        ASSERT_NE(result, lit_Undef);
        
        EXPECT_TRUE((result == mkLit(1, 0)) || (result == mkLit(2,0)));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, isFullyActiveInFirstPeriod) {
        Conjectures testData;
        
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        TestedRSILVanishingBranchingHeuristic::Parameters params{{testData}};
        params.probHalfLife = 100ull;
        TestedRSILVanishingBranchingHeuristic underTest{params};
        
        TestedRSILBranchingHeuristic::TrailType testTrail {mkLit(1,0)};
        TestedRSILBranchingHeuristic::TrailLimType testTrailLim {0};
        TestedRSILBranchingHeuristic::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        TestedRSILBranchingHeuristic::AssignsType testAssigns {l_True, l_False, l_Undef, l_Undef, l_Undef};
        
        int calls = 0;
        for (int i = 0; i < 100; ++i) {
            auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
            ++calls;
            EXPECT_NE(result, lit_Undef) << "Unexpected undef at call " << calls;
        }
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, activityMatchesExpectedDistribution) {
        Conjectures testData;
        
        const unsigned long long halfLife = 1000ull;
        const int stages = 20;
        
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        TestedRSILVanishingBranchingHeuristic::Parameters params{{testData}};
        params.probHalfLife = halfLife;
        TestedRSILVanishingBranchingHeuristic underTest{params};
        
        TestedRSILBranchingHeuristic::TrailType testTrail {mkLit(1,0)};
        TestedRSILBranchingHeuristic::TrailLimType testTrailLim {0};
        TestedRSILBranchingHeuristic::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        TestedRSILBranchingHeuristic::AssignsType testAssigns {l_True, l_False, l_Undef, l_Undef, l_Undef};
        
        std::unordered_map<uint8_t, double> distribution;
        std::unordered_map<uint8_t, double> referenceDistribution;
        
        for (int stage = 0; stage < stages; ++stage) {
            uint32_t definedResultCounter = 0;
            for (auto i = decltype(halfLife){0}; i < halfLife; ++i) {
                Lit result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
                definedResultCounter += (result == lit_Undef) ? 0 : 1;
            }
            distribution[stage] = static_cast<double>(definedResultCounter) / static_cast<double>(halfLife);
            referenceDistribution[stage] = 1.0f/static_cast<double>(1 << stage);
        }
        
        // simple Kolomogorov-Smirnov test
        double maxAbsDiff = getMaxAbsDifference(distribution, referenceDistribution);
        EXPECT_LE(maxAbsDiff, 0.05f);
    }
}
