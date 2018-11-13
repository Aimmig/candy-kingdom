/*
 * Propagate.h
 *
 *  Created on: Jul 18, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_PROPAGATE_THREADSAFE_H_
#define SRC_CANDY_CORE_PROPAGATE_THREADSAFE_H_

#include "candy/core/SolverTypes.h"
#include "candy/core/ClauseDatabase.h"
#include "candy/core/Clause.h"
#include "candy/core/Trail.h"
#include "candy/utils/CheckedCast.h"

#include <array>

//#define FUTURE_PROPAGATE

namespace Candy {

struct WatcherTS {
    Clause* cref;
    Lit watch0;
    Lit watch1;

    WatcherTS(Clause* clause, Lit one, Lit two)
     : cref(clause), watch0(one), watch1(two) {}
};

class PropagateThreadSafe {
private:
    ClauseDatabase& clause_db;
    Trail& trail;

    std::vector<std::vector<WatcherTS*>> watchers;

public:
    uint64_t nPropagations;

    PropagateThreadSafe(ClauseDatabase& _clause_db, Trail& _trail)
        : clause_db(_clause_db), trail(_trail), watchers(), nPropagations(0) { }

    void init(size_t maxVars) {
        watchers.resize(mkLit(maxVars, true));
    }

    void attachClause(Clause* clause) {
        assert(clause->size() > 1);
        if (clause->size() > 2) {
            WatcherTS* watcher = new WatcherTS(clause, clause->first(), clause->second());
            watchers[~clause->first()].push_back(watcher);
            watchers[~clause->second()].push_back(watcher);
        }
    }

    void detachClause(const Clause* clause) {
        assert(clause->size() > 1);
        if (clause->size() > 2) {
            WatcherTS* watcher;
            for (Lit lit : *clause) {
                auto it = std::find_if(watchers[~lit].begin(), watchers[~lit].end(), [clause](WatcherTS* w){ return w->cref == clause; });
                if (it != watchers[~lit].end()) {
                    watchers[~lit].erase(std::remove(watchers[~lit].begin(), watchers[~lit].end(), watcher), watchers[~lit].end());
                    watcher = *it;
                }
            }
            delete watcher;
        }
    }

    void attachAll(std::vector<Clause*>& clauses) {
        for (Clause* clause : clauses) {
            attachClause(clause);
        }
    }

    void detachAll() {
        for (unsigned int lit = 0; lit < watchers.size(); lit++) {
            for (WatcherTS* watcher : watchers[lit]) {
                Lit other = watcher->watch0 == ~lit ? watcher->watch1 : watcher->watch0;
                watchers[~other].erase(std::remove(watchers[~other].begin(), watchers[~other].end(), watcher), watchers[~other].end());
                delete watcher;
            }
            watchers[lit].clear();
        }
    }

    void sortWatchers() {
        size_t nVars = watchers.size() / 2;
        for (size_t v = 0; v < nVars; v++) {
            Var vVar = checked_unsignedtosigned_cast<size_t, Var>(v);
            for (Lit l : { mkLit(vVar, false), mkLit(vVar, true) }) {
                sort(watchers[l].begin(), watchers[l].end(), [](WatcherTS* w1, WatcherTS* w2) {
                    Clause& c1 = *w1->cref;
                    Clause& c2 = *w2->cref;
                    return c1.size() < c2.size() || (c1.size() == c2.size() && c1.getActivity() > c2.getActivity());
                });
            }
        }
    }

    inline const Clause* propagate_binary_clauses(Lit p) {
        const std::vector<BinaryWatcher>& list = clause_db.getBinaryWatchers(p);
        for (BinaryWatcher watcher : list) {
            lbool val = trail.value(watcher.other);
            if (val == l_False) {
                return watcher.clause;
            }
            if (val == l_Undef) {
                trail.uncheckedEnqueue(watcher.other, watcher.clause);
            }
        }
        return nullptr;
    }

    /**************************************************************************************************
     *
     *  propagate : [void]  ->  [Clause*]
     *
     *  Description:
     *    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
     *    otherwise CRef_Undef.
     *
     *    Post-conditions:
     *      * the propagation queue is empty, even if there was a conflict.
     **************************************************************************************************/
    inline const Clause* propagate_watched_clauses(Lit p) {
        std::vector<WatcherTS*>& list = watchers[p];

        auto keep = list.begin();
        for (auto iter = list.begin(); iter != list.end(); iter++) {
            WatcherTS* watcher = *iter;
            if (watcher->watch0 != p) {
                std::swap(watcher->watch0, watcher->watch1);
                assert(watcher->watch0 == p);
            }
            lbool val = trail.value(watcher->watch1);
            if (val != l_True) { // Try to avoid inspecting the clause
                const Clause* clause = watcher->cref;
                
                for (Lit lit : *clause) {
                    if (lit != watcher->watch0 && lit != watcher->watch1 && trail.value(lit) != l_False) {
                        watcher->watch1 = lit;
                        watchers[~lit].push_back(watcher);
                        goto propagate_skip;
                    }
                }

                // did not find watch
                if (val == l_False) { // conflict
                    list.erase(keep, iter);
                    return clause;
                }
                else { // unit
                    trail.uncheckedEnqueue(clause->first(), (Clause*)clause);
                }
            }
            *keep = *iter;
            keep++;
            propagate_skip:;
        }
        list.erase(keep, list.end());

        return nullptr;
    }

    const Clause* propagate() {
        const Clause* conflict = nullptr;
        unsigned int old_qhead = trail.qhead;

        while (trail.qhead < trail.trail_size) {
            Lit p = trail[trail.qhead++];
            
            // Propagate binary clauses
            conflict = propagate_binary_clauses(p);
            if (conflict != nullptr) break;

            // Propagate other 2-watched clauses
            conflict = propagate_watched_clauses(p);
            if (conflict != nullptr) break;
        }

        nPropagations += trail.qhead - old_qhead;
        return conflict;
    }
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_PROPAGATE_H_ */
