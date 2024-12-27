/************************************************************************************
EasySAT: A CDCL SAT Solver
================================================================================
Copyright (c) 2022 SeedSolver Beijing.
https://seedsolver.com
help@seedsolver.com

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
************************************************************************************/
#include "IsingTree.hpp"
#include <fstream>
#include <sstream>  // For std::istringstream
#include <iostream> // For std::ios


namespace py = pybind11;

#define value(lit) (lit > 0 ? value[lit] : -value[-lit])    // Get the value of a literal
#define watch(id) (watches[vars + id])                      // Remapping a literal [-maxvar, +maxvar] to its watcher.

char *read_whitespace(char *p) {                            // Aid function for parser
    while ((*p >= 9 && *p <= 13) || *p == 32) ++p;
    return p;
}

char *read_until_new_line(char *p) {                        // Aid function for parser
    while (*p != '\n') {
        if (*p++ == '\0') exit(1);
    }
    return ++p;
}

char *read_int(char *p, int *i) {                           // Aid function for parser
    bool sym = true; *i = 0;
    p = read_whitespace(p);
    if (*p == '-') sym = false, ++p;
    while (*p >= '0' && *p <= '9') {
        if (*p == '\0') return p;
        *i = *i * 10 + *p - '0', ++p;
    }
    if (!sym) *i = -(*i);
    return p;
}

int Solver::add_clause(std::vector<int> &c) {                   
    clause_DB.push_back(Clause(c.size()));                          // Add a clause c into database.
    int id = clause_DB.size() - 1;                                  // Getting clause index.
    for (int i = 0; i < (int)c.size(); i++) clause_DB[id][i] = c[i];     // Copy literals
    watch(-c[0]).push_back(Watcher(id, c[1]));                      // Watch this clause by literal -c[0]
    watch(-c[1]).push_back(Watcher(id, c[0]));                      // Watch this clause by literal -c[1]
    return id;                                                      
}

int Solver::add_soft_clause(std::vector<int> &c) {                   
    clause_soft_DB.push_back(Clause(c.size()));
    int id = clause_soft_DB.size() - 1;
    for (int i = 0; i < (int)c.size(); i++) clause_soft_DB[id][i] = c[i];
    return id;                                                      
}

int Solver::parse(char *filename) {
    std::ifstream infile(filename);
    if (!infile){
        std::cout << "c the input filename " << filename << " is invalid, please input the correct filename." << std::endl;
        exit(-1);
    }

    std::istringstream iss;
    std::string line;
    char tempstr1[10];
    char tempstr2[10];

    int top_weight;
    int weight;
    int cur_lit;

    while (getline(infile, line)) {
        if (line[0] == 'p') {
            vars = clauses = 0;
            int read_items = sscanf(line.c_str(), "%s %s %d %d %d", tempstr1, tempstr2, &vars, &clauses, &top_weight);

            if (read_items < 5) {
                std::cout << "read item < 5 " << std::endl;
                exit(-1);
            } else 
                alloc_memory();
            break;
        }
    }

    std::vector<int> buffer;                                        // Save the clause that waiting to push

    while (getline(infile, line)) {
        if (line[0] == 'c')
            continue;
        else {
            iss.clear();
            iss.str(line);
            iss.seekg(0, std::ios::beg);
        }

        iss >> weight;
        iss >> cur_lit;
        while (cur_lit != 0) {
            buffer.push_back(cur_lit);
            iss >> cur_lit;
        }
        if (weight == top_weight) {
            add_clause(buffer);
        } else {
            add_soft_clause(buffer);
        }
        buffer.clear();
    }

    infile.close();
    origin_clauses = clause_DB.size();
    return (propagate() == -1 ? 0 : 20);                                                    // Simplify by BCP.
}

void Solver::alloc_memory() {
    value       = new int[vars + 1];
    reason      = new int[vars + 1];
    level       = new int[vars + 1];
    mark        = new int[vars + 1];
    saved       = new int[vars + 1];
    activity    = new double[vars + 1];
    watches     = new std::vector<Watcher>[vars * 2 + 1];
    fast_lbd_sum = lbd_queue_size = lbd_queue_pos = slow_lbd_sum = 0;
    vsids.setComp(GreaterActivity(activity));
    for (int i = 1; i <= vars; i++) 
        value[i] = reason[i] = level[i] = mark[i] = activity[i] = saved[i] = 0, vsids.insert(i);
}

void Solver::assign(int lit, int l, int cref) {
    int var = abs(lit);
    value[var]  = lit > 0 ? 1 : -1;
    level[var]  = l, reason[var] = cref;                                         
    trail.push_back(lit);
}

int Solver::propagate() {
    while (propagated < (int)trail.size()) { 
        int p = trail[propagated++];                    // Pick an unpropagated literal in trail.
        std::vector<Watcher> &ws = watch(p);            // Fetch the watcher for this literal.
        int i, j, size = ws.size();                     
        for (i = j = 0; i < size; ) {               
            int blocker = ws[i].blocker;                       
            if (value(blocker) == 1) {                  // Pre-judge whether the clause is already SAT
                ws[j++] = ws[i++]; continue;
            }
            int cref = ws[i].idx_clause, k, sz;
            Clause& c = clause_DB[cref];                // Fetch a clause from watcher
            if (c[0] == -p) c[0] = c[1], c[1] = -p;     // Make sure c[1] is the false literal (-p).
            Watcher w = Watcher(cref, c[0]);            // Prepare a new watcher for c[1]
            i++;
            if (value(c[0]) == 1) {                     // Check whether another lit is SAT.
                ws[j++] = w; continue;
            }
            for (k = 2, sz = c.lit.size(); k < sz && value(c[k]) == -1; k++);    // Find a new watch literal.
            if (k < sz) {                               // Move the watch literal to the second place
                c[1] = c[k], c[k] = -p;
                watch(-c[1]).push_back(w);
            }
            else {                                      // Can not find a new watch literl
                ws[j++] = w;
                if (value(c[0]) == -1) {                // There is a confliction
                    while (i < size) ws[j++] = ws[i++];
                    ws.resize(j);
                    return cref;
                }
                else assign(c[0], level[abs(p)], cref);// Find a new unit clause and assign it.
            }
        }
        ws.resize(j);
    }
    return -1;                                          // Meet a convergence
}

int Solver::analyze(int conflict, int &backtrackLevel, int &lbd) {
    ++time_stamp;
    learnt.clear();
    Clause &c = clause_DB[conflict]; 
    int highestLevel = level[abs(c[0])];
    if (highestLevel == 0) return 20;
    learnt.push_back(0);        // leave a place to save the First-UIP
    int should_visit_ct = 0,    // The number of literals that have not been visited in the higest level of the implication graph.
        resolve_lit = 0,        // The literal to do resolution.
        index = trail.size() - 1;
    do {
        Clause &c = clause_DB[conflict];
        for (int i = (resolve_lit == 0 ? 0 : 1); i < (int)c.lit.size(); i++) {
            int var = abs(c[i]);
            if (mark[var] != time_stamp && level[var] > 0) {
                mark[var] = time_stamp;
                if (level[var] >= highestLevel) should_visit_ct++;
                else learnt.push_back(c[i]);
            }
        }
        do {                                         // Find the last marked literal in the trail to do resolution.
            while (mark[abs(trail[index--])] != time_stamp);
            resolve_lit = trail[index + 1];
        } while (level[abs(resolve_lit)] < highestLevel);
        conflict = reason[abs(resolve_lit)], mark[abs(resolve_lit)] = 0, should_visit_ct--;
    } while (should_visit_ct > 0);                   // Have find the convergence node in the highest level (First UIP)
    learnt[0] = -resolve_lit;
    ++time_stamp, lbd = 0;
    for (int i = 0; i < (int)learnt.size(); i++) {   // Calculate the LBD.
        int l = level[abs(learnt[i])];
        if (l && mark[l] != time_stamp) 
            mark[l] = time_stamp, ++lbd;
    }
    if (lbd_queue_size < 50) lbd_queue_size++;       // update fast-slow.
    else fast_lbd_sum -= lbd_queue[lbd_queue_pos];
    fast_lbd_sum += lbd, lbd_queue[lbd_queue_pos++] = lbd;
    if (lbd_queue_pos == 50) lbd_queue_pos = 0;
    slow_lbd_sum += (lbd > 50 ? 50 : lbd);
    if (learnt.size() == 1) backtrackLevel = 0;
    else {                                           // find the second highest level for backtracking.
        int max_id = 1;
        for (int i = 2; i < (int)learnt.size(); i++)
            if (level[abs(learnt[i])] > level[abs(learnt[max_id])]) max_id = i;
        int p = learnt[max_id];
        learnt[max_id] = learnt[1], learnt[1] = p, backtrackLevel = level[abs(p)];
    }
    return 0;
}

void Solver::backtrack(int backtrackLevel) {
    if ((int)pos_in_trail.size() <= backtrackLevel) return;
    for (int i = trail.size() - 1; i >= pos_in_trail[backtrackLevel]; i--) {
        int v = abs(trail[i]);
        value[v] = 0, saved[v] = trail[i] > 0 ? 1 : -1; // phase saving 
        if (!vsids.inHeap(v)) vsids.insert(v);          // update heap
    }
    propagated = pos_in_trail[backtrackLevel];
    trail.resize(propagated);
    pos_in_trail.resize(backtrackLevel);
}

int Solver::decide() {      
    int next = -1;
    while (next == -1 || value(next) != 0) {    // Picking a variable according to VSIDS
        if (vsids.empty()) return 10;
        else next = vsids.pop();
    }
    if (activity[next] > threshold) {
        pos_in_trail.push_back(trail.size());
        if (saved[next]) next *= saved[next];       // Pick the polarity of the varible
        assign(next, pos_in_trail.size(), -1);
        return 0;
    } else return 1;
}

int Solver::solve_limited() {
    int res = 0;
    while (!res) {
        int cref = propagate();                         // Boolean Constraint Propagation (BCP)
        if (cref != -1) {                               // Find a conflict
            int backtrackLevel = 0, lbd = 0;
            res = analyze(cref, backtrackLevel, lbd);   // Conflict analyze
            if (res == 20) break;                       // Find a conflict in 0-level
            backtrack(backtrackLevel);                  // backtracking         
            if (learnt.size() == 1) assign(learnt[0], 0, -1);   // Learnt a unit clause.
            else {                     
                int cref = add_clause(learnt);                  // Add a clause to data base.
                clause_DB[cref].lbd = lbd;              
                assign(learnt[0], backtrackLevel, cref);        // The learnt clause implies the assignment of the UIP variable.
            }
        }
        else res = decide();
    }
    return res;
}

void Solver::setThreshold(double aNumberFrom0To1){
    threshold = aNumberFrom0To1;
}

py::list Solver::getModel() {
    py::list result;
    for (size_t i = 1; i < vars + 1; ++i) {
        result.append(value[i]);
    }
    return result;
}

py::list Solver::getActivity() {
    py::list result;
    for (size_t i = 1; i < vars + 1; ++i) {
        result.append(activity[i]);
    }
    return result;
}

py::list Solver::getHardClause() {
    py::list pyClauseDB;
    py::list temp;
    for (size_t i = 0; i < origin_clauses; ++i) {
        temp = py::cast(clause_DB[i].lit);
        pyClauseDB.append(temp);
    }
    return pyClauseDB;
}

py::list Solver::getSoftClause() {
    py::list pyClauseDB;
    py::list temp;
    for (size_t i = 0; i < clause_soft_DB.size(); ++i) {
        temp = py::cast(clause_soft_DB[i].lit);
        pyClauseDB.append(temp);
    }
    return pyClauseDB;
}

void Solver::fromIsing(py::list continuousValues) {
    double temp;
    for (size_t i=1; i<=vars; ++i) {
        temp = py::cast<double>(continuousValues[i-1]);
        activity[i] = fabs(temp);
        if (!vsids.inHeap(i)) vsids.insert(i);
        vsids.update(i);
        if (temp < 0) saved[i] = 1;
        else saved[i] = -1;
    }
    /*
    When continuousValues is x0
    x0 < 0, means the phase should be True
    x0 > 0, means the phase should be False
     */
    /*
    When continuousValues is grad(x0)
    grad(x0) < 0, means the phase should be False
    grad(x0) > 0, means the phase should be True
     */
}