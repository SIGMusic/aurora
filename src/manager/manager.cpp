/**
 * SIGMusic Lights 2015
 * Raspberry Pi manager
 */

#include <cstdlib>
#include <iostream>
#include <sys/mman.h>
#include <semaphore.h>
#include "server.h"
#include "radio.h"
#include "manager.h"


// The software version number
#define VERSION     0


void printWelcomeMessage(void);


Server server;
Radio radio;


int main(int argc, char** argv) {
    
    printWelcomeMessage();

    // Map a shared array to store the colors
    color_t* colors = (color_t*)mmap(NULL, NUM_IDS * sizeof(color_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (colors == (void*)-1) {
        perror("mmap");
        exit(1);
    }

    // Set up the colors semaphore
    sem_t* colors_sem = (sem_t*)mmap(NULL, sizeof(sem_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (colors_sem == (void*)-1) {
        perror("mmap");
        exit(1);
    }

    if (sem_init(colors_sem, 1, 1)) {
        perror("sem_init");
        exit(1);
    }

    // Map a shared array to store the connected list
    uint32_t* connected = (uint32_t*)mmap(NULL,
        (NUM_IDS  + sizeof(uint32_t) - 1) / sizeof(uint32_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (connected == (void*)-1) {
        perror("mmap");
        exit(1);
    }

    // Set up the connected semaphore
    sem_t* connected_sem = (sem_t*)mmap(NULL, sizeof(sem_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (connected_sem == (void*)-1) {
        perror("mmap");
        exit(1);
    }

    if (sem_init(connected_sem, 1, 1)) {
        perror("sem_init");
        exit(1);
    }

    // Fork off the websocket server
    if (!fork()) {
        server.run(colors, colors_sem, connected, connected_sem);
    }

    // Fork off the radio controller
    if (!fork()) {
        radio.run(colors, colors_sem, connected, connected_sem);
    }

    while(1) {
        sleep(1);
    }

    return 0; // Should not get here
}

/**
 * Prints the welcome message for the serial console.
 */
void printWelcomeMessage(void) {
    printf("-------------------------------------\n");
    printf("ACM@UIUC SIGMusic Lights Base Station\n");
    printf("Version %x\n", VERSION);
    printf("This base station is endpoint %d\n", BASE_STATION_ID);
    printf("-------------------------------------\n");
}
