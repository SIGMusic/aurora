/**
 * SIGMusic Lights 2016
 * Raspberry Pi manager
 */

#include <cstdlib>
#include <iostream>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

#include <common.h>
#include "radio.h"


int main(int argc, char** argv) {

    Radio radio;

    // Map a shared array to store the colors
    struct shared* s = (struct shared*)mmap(NULL, sizeof(struct shared),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (s == (void*)-1) {
        perror("mmap");
        exit(EX_OSERR);
    }

    // Set up the colors semaphore
    if (sem_init(&s->colors_sem, 1, 1)) {
        perror("sem_init");
        exit(EX_OSERR);
    }

    // Fork off the radio controller
    int radio_pid = fork();
    if (radio_pid == -1) {
        perror("fork");
        exit(EX_OSERR);
    } else if (radio_pid == 0) {
        radio.run(s);
        exit(EXIT_FAILURE); // Should not get here
    }

    // Wait on all the children
    errno = 0;
    while (wait(NULL)) {
        if (errno == ECHILD) {
            break;
        }
    }

    exit(EXIT_FAILURE); // Should not get here

    // // Orphan the children so that they are adopted by init
    // exit(EXIT_SUCCESS);
}
