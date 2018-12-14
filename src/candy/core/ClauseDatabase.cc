#include "candy/core/ClauseDatabase.h"

namespace Candy {

ClauseDatabase::ClauseDatabase() : 
    cla_inc(1), clause_decay(ClauseDatabaseOptions::opt_clause_decay),
    persistentLBD(ClauseDatabaseOptions::opt_persistent_lbd),
    reestimationBumpActivity(ClauseDatabaseOptions::opt_reestimation_bump_activity), 
    reestimationReduceLBD(ClauseDatabaseOptions::opt_reestimation_reduce_lbd), 
    track_literal_occurrence(false),
    variableOccurrences(),
    allocator(), 
    clauses() {

}

ClauseDatabase::~ClauseDatabase() {
}

void ClauseDatabase::initOccurrenceTracking(size_t nVars) {
    for (Clause* clause : clauses) {
        for (Lit lit : *clause) {
            variableOccurrences[var(lit)].push_back(clause);
        }
    }
    track_literal_occurrence = true;
}

void ClauseDatabase::stopOccurrenceTracking() {
    variableOccurrences.clear();
    track_literal_occurrence = false;
}

void ClauseDatabase::bumpActivity(Clause* clause) {
    if (clause->incActivity(cla_inc) > 1e20f) {
        for (Clause* clause : clauses) {
            clause->scaleActivity(1e-20f);
        }
        cla_inc *= 1e-20;
    }
}

// DYNAMIC NBLEVEL trick (see competition'09 Glucose companion paper)
void ClauseDatabase::reduceLBD(Trail& trail, Clause* clause) {
    uint_fast16_t nblevels = trail.computeLBD(clause->begin(), clause->end());
    if (nblevels + 1 < clause->getLBD()) {
        clause->setLBD(nblevels); // improve the LBD
        clause->setFrozen(true); // Seems to be interesting, keep it for the next round
    }
}

void ClauseDatabase::reestimateClauseWeights(Trail& trail, std::vector<Clause*>& involved_clauses) {
    for (Clause* clause : involved_clauses) {
        if (clause->isLearnt()) {
            reduceLBD(trail, clause);
            bumpActivity(clause); 
        }
    }
    cla_inc *= (1 / clause_decay);
}

/**
 * In order ot make sure that no clause is locked (reason to an asignment), 
 * do only call this method at decision level 0 and strengthen all clauses first
 **/
void ClauseDatabase::reduce() {
    Statistics::getInstance().solverReduceDBInc();

    std::vector<Clause*> learnts;
    copy_if(clauses.begin(), clauses.end(), std::back_inserter(learnts), [](Clause* clause) { return clause->isLearnt() && clause->size() > 2; });
    std::sort(learnts.begin(), learnts.end(), reduceDB_lt());

    learnts.erase(learnts.begin() + (learnts.size() / 2), learnts.end());
    if (learnts.size() == 0 || learnts.back()->getLBD() <= persistentLBD) {
        return; // We have a lot of "good" clauses, it is difficult to compare them, keep more
    }
    
    size_t count = 0;
    for (Clause* c : learnts) {
        if (c->isFrozen()) { // lbd was reduce during this sequence
            c->setFrozen(false); // reset flag
        } else {
            ++count;
            removeClause(c);
        }
    }

    Statistics::getInstance().solverRemovedClausesInc(count);
}

size_t ClauseDatabase::cleanup() { 
    auto new_end = std::remove_if(clauses.begin(), clauses.end(), [this](Clause* c) { return c->isDeleted(); });
    size_t num = std::distance(new_end, clauses.end());
    clauses.erase(new_end, clauses.end());
    return num;
}

/**
 * Make sure all references are updated after all clauses reside in a new adress space
 */
void ClauseDatabase::defrag() {
    std::vector<Clause*> reallocated = allocator.defrag(clauses);
    clauses.swap(reallocated);
    for (std::vector<BinaryWatcher>& watcher : binaryWatchers) {
        watcher.clear();
    }
    for (Clause* clause : clauses) {
        if (clause->size() == 2) {
            binaryWatchers[~clause->first()].emplace_back(clause, clause->second());
            binaryWatchers[~clause->second()].emplace_back(clause, clause->first());
        }
    }
    if (track_literal_occurrence) {
        variableOccurrences.clear();
        initOccurrenceTracking(variableOccurrences.size());
    }
}

}