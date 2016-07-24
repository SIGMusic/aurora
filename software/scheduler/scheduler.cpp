/**
 * SIGMusic Lights 2016
 * Scheduler class
 */

#include "scheduler.h"

struct shared* Scheduler::s;

void Scheduler::run(struct shared* s) {
    
    Scheduler::s = s;
}