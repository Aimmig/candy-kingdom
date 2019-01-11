/*
 * ClauseAllocator.h
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CLAUSEALLOCATOR_H_
#define SRC_CANDY_CORE_CLAUSEALLOCATOR_H_

#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <numeric>

#include <candy/core/clauses/Clause.h>
#include <candy/core/Statistics.h>

namespace Candy {

class ClauseAllocator {
public:
    ClauseAllocator() : pages(), cursor(0) {
        newPage();
    }

    ~ClauseAllocator() {
        for (unsigned char* page : pages) {
            std::free((void*)page);
        }
        pages.clear();
    }

    inline void* allocate(unsigned int length) {
        unsigned int size = clauseBytes(length);
        if (cursor + size > PAGE_SIZE) {
            newPage();
        }
        void* clause = memory + cursor;
        cursor += size;
        return clause;
    }

    std::vector<Clause*> reallocate(bool free_old_pages = true) {
        std::vector<Clause*> clauses {};
        std::vector<unsigned char*> old_pages;
        old_pages.swap(pages);
        newPage();
        for (unsigned char* old_page : old_pages) {
            unsigned int num_literals = 0;
            unsigned int num_bytes = 0;
            for (unsigned char *p = old_page, *end = old_page + PAGE_SIZE; p < end; p += num_bytes) {
                Clause* old_clause = (Clause*)p;
                num_literals = old_clause->size();
                num_bytes = clauseBytes(num_literals);
                if (num_literals == 0) {
                    break;
                }
                else if (!old_clause->isDeleted()) {
                    void* clause = allocate(num_literals);
                    memcpy(clause, (void*)old_clause, num_bytes);
                    clauses.push_back((Clause*)clause);
                }
            }
        }
        if (free_old_pages) {
            for (unsigned char* page : old_pages) {
                std::free((void*)page);
            }
        }
        return clauses;
    }

    std::vector<Clause*> collect() {
        std::vector<Clause*> clauses {};
        for (unsigned char* page : pages) {
            unsigned int num_bytes = 0;
            for (unsigned char *p = page, *end = page + PAGE_SIZE; p < end; p += num_bytes) {
                Clause* clause = (Clause*)p;
                if (clause->size() == 0) {
                    break;
                }
                else if (!clause->isDeleted()) {
                    clauses.push_back(clause);
                }
                num_bytes = clauseBytes(clause->size());
            }
        }
        return clauses;
    }

    void reset(bool free = true) {
        if (free) this->free();
        else pages.clear();
        newPage();
    }

    void free() {
        for (unsigned char* page : pages) {
            std::free((void*)page);
        }
        pages.clear();
        cursor = 0;
        memory = nullptr;
    }

    void import(ClauseAllocator& other) {
        pages.insert(pages.end(), other.pages.begin(), other.pages.end());
    }

private:
    static constexpr unsigned int PAGE_SIZE = 128*1024*1024;

    std::vector<unsigned char*> pages;

    size_t cursor;
    unsigned char* memory;

    ClauseAllocator(ClauseAllocator const&) = delete;
    void operator=(ClauseAllocator const&) = delete;

    inline void newPage() {
        memory = (unsigned char*)std::malloc(PAGE_SIZE);
        std::memset(memory, 0, PAGE_SIZE);
        pages.push_back(memory);
        cursor = 0;
    }

    inline unsigned int clauseBytes(unsigned int length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
