/*
 * GateAnalyzer.cc
 *
 *  Created on: Jul 21, 2016
 *      Author: markus
 */

#include "gates/GateAnalyzer.h"
#include "core/Solver.h"

using namespace std;
using namespace Glucose;

static const char* _cat = "GATE RECOGNITION";
static IntOption opt_gate_tries(_cat, "gate-tries", "Number of heuristic clause selections to enter recursion", 0, IntRange(0, INT32_MAX));
static BoolOption opt_patterns(_cat, "gate-patterns", "Enable Pattern-based Gate Detection", false);
static BoolOption opt_semantic(_cat, "gate-semantic", "Enable Semantic Gate Detection", false);
static BoolOption opt_holistic(_cat, "gate-holistic", "Enable Holistic Gate Detection", false);
static BoolOption opt_decompose(_cat, "gate-decompose", "Enable Local Blocked Decomposition", false);

static const char* _debug = "DEBUGGING";
static BoolOption opt_verbose(_debug, "gate-debug", "Enable Debugging of Gate Analyzer", false);

GateAnalyzer::GateAnalyzer(CNFProblem& dimacs) :
	problem (dimacs),
	solver (),
	maxTries (opt_gate_tries),
	usePatterns (opt_patterns),
	useSemantic (opt_semantic),
    useHolistic (opt_holistic),
    useDecomposition (opt_decompose),
    verbose (opt_verbose) {
  gates = new For(problem.nVars());
  inputs.resize(2 * problem.nVars(), false);
  index = buildIndexFromClauses(problem.getProblem());
  if (useHolistic) solver.insertClauses(problem);
}

// heuristically select clauses
vector<Cl*>& GateAnalyzer::getClausesWithRarestLiteral(vector<For>& index) {
  unsigned int min = INT_MAX;
  int minLit = -1;
  for (int l = 0; l < 2*problem.nVars(); l++) {
    if (index[l].size() > 0 && index[l].size() < min) {
      min = index[l].size();
      minLit = l;
    }
  }
  if (minLit == -1) vector<Cl*>();
  return index[minLit];
}

void GateAnalyzer::analyze() {
  // populate index (except for unit-clauses, they go to roots immediately)
  for (Cl* c : problem.getProblem()) if (c->size() == 1) {
    roots.push_back(c);
    removeFromIndex(index, c);
  }

  // start with unit clauses
  set<Lit> next;
  for (Cl* c : roots) for (Lit l : *c) next.insert(l);
  analyze(next);

  // clause selection loop
  for (int k = 0; k < maxTries; k++) {
    next.clear();
    vector<Cl*>& clauses = getClausesWithRarestLiteral(index);
    for (Cl* c : clauses) {
      next.insert(c->begin(), c->end());
    }
    removeFromIndex(index, clauses);
    analyze(next);
  }
}

bool GateAnalyzer::semanticCheck(vector<Cl*>& fwd, vector<Cl*>& bwd, Var o) {
  if (verbose) {
    printf("semantic check for output %i \n", o+1);
    printf("forward clauses: \n");
    printClauses(fwd);
    printf("backward clauses: \n");
    printClauses(bwd);
  }

  CNFProblem constraint;
  Lit alit = mkLit(problem.nVars(), false);
  Cl clause;
  for (const For& f : { fwd, bwd })
  for (Cl* cl : f) {
    clause.push_back(alit);
    for (Lit l : *cl) {
      if (var(l) != o) {
        clause.push_back(l);
      }
    }
    constraint.readClause(clause);
    clause.clear();
  }
  solver.insertClauses(constraint);
  bool isRightUnique = !solver.solve(~alit);
  solver.addClause(alit);
  return isRightUnique;
}

// clause patterns of full encoding
bool GateAnalyzer::completePattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs) {
  // precondition: fwd and bwd constrain exactly the same inputs (in opposite polarity)
  // and fwd blocks bwd on the output literal
  // given a total of 2^n blocked clauses implies that we have no redundancy in the n inputs
  set<Var> vars;
  for (Lit l : inputs) vars.insert(var(l));
  return fwd.size() == bwd.size() && 2*fwd.size() == pow(2, vars.size()) && 2*vars.size() == inputs.size();
}

// clause patterns of full encoding
bool GateAnalyzer::fullPattern(vector<Cl*>& fwd, vector<Cl*>& bwd, set<Lit>& inputs) {
  // precondition: fwd and bwd constrain exactly the same inputs (in opposite polarity)
  // and fwd blocks bwd on the output literal
  set<Var> vars;
  for (Lit l : inputs) vars.insert(var(l));
  bool fullOr = fwd.size() == 1 && fixedClauseSize(bwd, 2);
  bool fullAnd = bwd.size() == 1 && fixedClauseSize(fwd, 2);
  return fullOr || fullAnd || completePattern(fwd, bwd, inputs);
}

// main analysis routine
void GateAnalyzer::analyze(set<Lit>& roots) {
  vector<Lit> literals(roots.begin(), roots.end());

  for (Lit l : roots) inputs[l]++;

  while (literals.size()) {
    Lit o = literals.back();
    literals.pop_back();

    For& f = index[~o], g = index[o];
    if (f.size() > 0 && (isBlocked(o, f, g) || useDecomposition && isBlockedGreedyDecompose(o, f, g))) {
      bool mono = !inputs[o] || !inputs[~o];
      set<Lit> s, t;
      for (Cl* c : f) for (Lit l : *c) if (l != ~o) s.insert(l);
      if (!mono) for (Cl* c : g) for (Lit l : *c) if (l != o) t.insert(~l);
      bool gate = mono || (usePatterns && s == t && fullPattern(f, g, s)) || ((useSemantic || useHolistic) && semanticCheck(f, g, var(o)));
      if (gate) {
        nGates++;
        (*gates)[var(o)] = new Cl(s.begin(), s.end());
        literals.insert(literals.end(), s.begin(), s.end());
        for (Lit l : s) {
          inputs[l]++;
          if (!mono) inputs[~l]++;
        }
        removeFromIndex(index, f);
        removeFromIndex(index, g);
      }
    }
  }
}