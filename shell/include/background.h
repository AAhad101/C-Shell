#ifndef BACKGROUND_H
#define BACKGROUND_H

void add_background(pid_t pid, int job_no, char *command, BG_process **bg_prcs, int *active_bgs);

void remove_terminated_bg(BG_process **bg_prcs, int *active_bgs);

void print_terminated_bg(BG_process **bg_prcs, int *active_bgs);

void print_bgs(BG_process *bg_prcs, int active_bgs);

#endif