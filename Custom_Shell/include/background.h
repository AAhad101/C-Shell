#ifndef BACKGROUND_H
#define BACKGROUND_H

void add_background(pid_t pid, char *command, Status status);

void remove_terminated_bg();

void print_terminated_bg();

void print_bgs();

#endif