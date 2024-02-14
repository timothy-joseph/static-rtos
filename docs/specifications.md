# The specifications of static-rtos

## Creating threads and using the scheduler

All memory for static-rtos should be allocated by the user, either statically or
dynamically (keep in mind that not all malloc implementations are thread-safe)

These are the steps to make the rtos work

1. The user must first provide a array in which information about threads will
be stored
2. The user will allocate memory for each task's stack and call a function in
which he will specify the priority of the task (higher number, higher priority)
3. The user will call the start scheduling function with his desired
configuration

## The inner workings of the scheduler

The scheduler will arange the the threads in the threads array in a priority
queue format with strict priority comparisings (this will ensure a round robin
scheduling system for threads with the same priority that are ready to be
executed).

If dynamic priority is enabled, then the scheduler will increase the priority
of all ready non-running threads by one every n units of time (n is specified
by the user)

The max priority of a thread is 254

Read threads will be placed into the array starting at the start of the array\
The running thread will be not be placed into the threads array and the priority
queue will be re-heapified in order to create a round-robin effect for threads
with the same priority\
Suspended threads are placed into the array starting at its end

### Before starting scheduling

All threads are placed like in a normal array upon creating them\
Upon creation, there will also be context created at that function\
If a thread is suspended, then it will be moved towards the end of the array\
If a thread is unsuspended, then it will be moved back towards the start of the
array

### On starting scheduling

Upon starting the scheduler, the part of the array with the ready threads will
be heapified and the thread with the highest priority will start running.

### While the scheduler is running

If the current running thread is suspended, then the context will switch\
If a thread with a higher priority is ready/unsuspended the context will
switch\

Switching happens either immedietly or at the next tick

### When to reheapify

1. A new thread is inserted while the scheduler is running
2. Right after any operation involving the priority queue
3. A thread is readied
4. A thread is suspended

## The inner workings of the context switching

## Using notifications

## The inner workings of notifications

## Using mutexes

## The inner workings of mutexes
