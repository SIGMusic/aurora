/**
 * SIGMusic Lights 2016
 * Raspberry Pi manager
 */

#include <cstdlib>
#include <iostream>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sysexits.h>
#include "server.h"
#include "radio.h"
#include "common.h"


int main(int argc, char** argv) {

    Server server;
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

    // Fork off the websocket server
    int server_pid = fork();
    if (server_pid == -1) {
        perror("fork");
        exit(EX_OSERR);
    } else if (server_pid == 0) {
        server.run(s);
        exit(EXIT_FAILURE); // Should not get here
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
    while (wait(NULL) != -1);

    exit(EXIT_FAILURE); // Should not get here
}
