First Come First Serve (FCFS) Implementation:

    - I added a new field 'uint creation_time' to the struct 'proc' in proc.h which is initialised to the value of 'ticks' in the function allocproc() to record the exact time the process was created

    - I modified the scheduler() function in proc.c such that it iterates through the array of processes and find the RUNNABLE process with the earliest (lowest) 'creation_time' and selects it for execution (FCFS logic).


Completely Fair Scheduler (CFS) Implementation:

    - I added new fields to the the struct 'proc' : 'int nice' (a value), 'uint weight' (a value calculated from the 'nice' value), 'uint64 vruntime' (the value on the basis of which the scheduler selects a process for execution), 'uint ticks_run' (to store the number of ticks a process ran for in a time slice for vruntime increment)

    - I created a glabal array 'nice_weights' which stores all the values (rounded off to closest integer) of weights corresponding to 'nice' values from -20 to 19 for direct access and no requirement of calculation based on the approximation provided in the write up.

    - I modified the scheduler() function in proc.c to traverse through the array of processes and select the RUNNABLE process with the least 'vruntime'.

    - A timeslice is calculated using the given formula in the write up. A process is forced to yield control once the number of ticks it has run in this current turn (this is stored in ticks_run) reaches this calculated timeslice.




Performance Comparison:

I added extra metadata in each process to be able to compare the performance of the three different kinds of schedulers available for xv6: Round Robin (RR), First Come First Serve (FCFS), and Completely Fair Scheduler (CFS).

I made 'creation_time' general for all processes (before it was defined only for FCFS scheduler), 'run_time' and 'wait_time' to keep track of each process' running time and waiting time respectively

I initialised 'creation_time' upon the launch of the process the same way as in FCFS (launch time ticks value), and initialised 'run_time' and 'wait_time' to 0 and incremented them each tick (incremented run_time if the process was RUNNING and wait_time if it was RUNNABLE).

After a process terminated, I print the waiting time, running time and turnaround time of the process.

I ran the schedulertests.c for all three schedulers by setting the required SCHEDULER flag and setting CPUS=1 and obtained the scheduling statistics for those tests. They are as follows:

FCFS:

Starting scheduler test: 5 I/O-bound, 5 CPU-bound processes.

--- Process 9 Finished ---
Run Time:      39 ticks
Wait Time:     0 ticks
Turnaround Time: 39 ticks
--------------------------

--- Process 10 Finished ---
Run Time:      35 ticks
Wait Time:     40 ticks
Turnaround Time: 75 ticks
--------------------------

--- Process 11 Finished ---
Run Time:      36 ticks
Wait Time:     75 ticks
Turnaround Time: 111 ticks
--------------------------

--- Process 12 Finished ---
Run Time:      36 ticks
Wait Time:     111 ticks
Turnaround Time: 147 ticks
--------------------------

--- Process 13 Finished ---
Run Time:      36 ticks
Wait Time:     147 ticks
Turnaround Time: 183 ticks
--------------------------

--- Process 4 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 5 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 6 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 7 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 8 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

All child processes finished. Scheduler test complete.


CFS:
Starting scheduler test: 5 I/O-bound, 5 CPU-bound processes.

--- Process 9 Finished ---
Run Time:      39 ticks
Wait Time:     144 ticks
Turnaround Time: 183 ticks
--------------------------

--- Process 10 Finished ---
Run Time:      38 ticks
Wait Time:     147 ticks
Turnaround Time: 185 ticks
--------------------------

--- Process 11 Finished ---
Run Time:      40 ticks
Wait Time:     149 ticks
Turnaround Time: 189 ticks
--------------------------

--- Process 12 Finished ---
Run Time:      36 ticks
Wait Time:     153 ticks
Turnaround Time: 189 ticks
--------------------------

--- Process 13 Finished ---
Run Time:      37 ticks
Wait Time:     153 ticks
Turnaround Time: 190 ticks
--------------------------

--- Process 4 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 5 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 6 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 7 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 8 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

All child processes finished. Scheduler test complete.



RR:

Starting scheduler test: 5 I/O-bound, 5 CPU-bound processes.

--- Process 10 Finished ---
Run Time:      39 ticks
Wait Time:     157 ticks
Turnaround Time: 196 ticks
--------------------------

--- Process 12 Finished ---
Run Time:      39 ticks
Wait Time:     158 ticks
Turnaround Time: 197 ticks
--------------------------

--- Process 9 Finished ---
Run Time:      40 ticks
Wait Time:     158 ticks
Turnaround Time: 198 ticks
--------------------------

--- Process 13 Finished ---
Run Time:      40 ticks
Wait Time:     159 ticks
Turnaround Time: 199 ticks
--------------------------

--- Process 4 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 5 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 6 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 7 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 8 Finished ---
Run Time:      0 ticks
Wait Time:     200 ticks
Turnaround Time: 200 ticks
--------------------------

--- Process 11 Finished ---
Run Time:      42 ticks
Wait Time:     158 ticks
Turnaround Time: 200 ticks
--------------------------

All child processes finished. Scheduler test complete.



FINAL DATA FOR CALCULATION:

FCFS:
Average run time = 18.2 ticks
Average wait time = 137.3 ticks
Average turnaround time = 155.5 ticks


CFS:
Average run time = 19 ticks
Average wait time = 174.6 ticks
Average turnaround time = 193.6 ticks 

RR:
Average run time = 20 ticks
Average wait time = 179 ticks
Average turnaround time = 199 ticks


According to these statistics, CFS and RR seem to be performing more or less equally. This is because we haven't implemented any change in nice values for the processes i.e. they have the same nice values and hence the same weight values all throughout the process, and we have assigned all of them the same nice value of 0 initially which means that the increment in vruntime in all processes running simultaneously will be the same so each process will get get the increment one after the other in subsequent time slices which is pretty much the same as is in RR.

Compared to both of them, FCFS has lower (better) wait and turnaround time.

All have comparable run times which makes sense as the time required to execute a process doesn't depend much on the scheduler rather on the nature of the process itself.


