/*
 * Clause.cc
 *
 *  Created on: Feb 15, 2017
 *      Author: markus
 */

#include <candy/core/Clause.h>
#include <candy/core/ClauseAllocator.h>

namespace Candy {

ClauseAllocator* Clause::allocator = new ClauseAllocator(50, 100);

Clause::Clause(const std::vector<Lit>& ps, bool learnt) {
    header.deleted = 0;
    header.learnt = learnt;
    header.frozen = 0;
    setLBD(0);

    std::copy(ps.begin(), ps.end(), literals);
    length = ps.size();

    if (learnt) {
        data.act = 0;
    } else {
        calcAbstraction();
    }
}

Clause::Clause(std::initializer_list<Lit> list) {
    header.deleted = 0;
    header.learnt = false;
    header.frozen = 0;
    setLBD(0);

    std::copy(list.begin(), list.end(), literals);
    length = list.size();

    if (header.learnt) {
        data.act = 0;
    } else {
        calcAbstraction();
    }
}

Clause::~Clause() { }

void* Clause::operator new (std::size_t size, uint16_t length) {
    return allocate(length);
}

void Clause::operator delete (void* p) {
    deallocate((Clause*)p);
}

void* Clause::allocate(uint16_t length) {
    return allocator->allocate(length);
}

void Clause::deallocate(Clause* clause) {
    allocator->deallocate(clause);
}

Lit& Clause::operator [](int i) {
    return literals[i];
}

Lit Clause::operator [](int i) const {
    return literals[i];
}

Clause::const_iterator Clause::begin() const {
    return literals;
}

Clause::const_iterator Clause::end() const {
    return literals + length;
}

Clause::iterator Clause::begin() {
    return literals;
}

Clause::iterator Clause::end() {
    return literals + length;
}

uint16_t Clause::size() const {
    return length;
}

bool Clause::contains(const Lit lit) const {
    return std::find(begin(), end(), lit) != end();
}

bool Clause::contains(const Var v) const {
    return std::find_if(begin(), end(), [v](Lit lit) { return var(lit) == v; }) != end();
}

bool Clause::isLearnt() const {
    return header.learnt;
}

bool Clause::isDeleted() const {
    return header.deleted;
}

void Clause::setDeleted() {
    header.deleted = 1;
}

bool Clause::isFrozen() const {
    return header.frozen;
}

void Clause::setFrozen(bool flag) {
    header.frozen = flag;
}

const Lit Clause::back() const {
    return *(this->end()-1);
}

float& Clause::activity() {
    return data.act;
}

uint32_t Clause::abstraction() const {
    return data.abs;
}

void Clause::setLBD(uint16_t i) {
    uint16_t lbd_max = (1 << BITS_LBD) - 1;
    header.lbd = std::min(i, lbd_max);
}

uint16_t Clause::getLBD() const {
    return header.lbd;
}

void Clause::calcAbstraction() {
    uint32_t abstraction = 0;
    for (Lit lit : *this) {
        abstraction |= 1 << (var(lit) & 31);
    }
    data.abs = abstraction;
}

} /* namespace Candy */
