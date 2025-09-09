#ifndef ACTIVITIES_H
#define ACTIVITIES_H

int compare_bgs(const void *a, const void *b);

int verify_activities(AtomicNode *atomic);

int execute_activities(AtomicNode *atomic, BG_process **bg_prcs, int *active_bgs);

#endif