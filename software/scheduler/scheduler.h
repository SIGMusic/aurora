/**
 * SIGMusic Lights 2016
 * Scheduler class
 */

#pragma once

#include <common.h>

class Scheduler {
public:

    /**
     * Initializes the scheduler.
     */
    Scheduler();

    /**
     * Runs the scheduler control loop. Does not return.
     * 
     * @param s The struct of shared memory
     */
    void run(struct shared* s);

private:

    static struct shared* s;
};