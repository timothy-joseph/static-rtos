How the queue works:
---

Priority queue ordered by priority
The end of the queue contains suspended threads

The end grows to the right, but the suspended end grows to the left


1 2 3 | 4 5

Porting
---

all port functions/variables/structs/macros names follow the following naming scheme:
1. functions or macros with capital letters begin with PORT_
2. function, variable names, structs, etc with lower care letters being with port_

the exception to this rule is struct mcu\_context

The following must be implemented (in any way possible), but they must follow the description given

The user must also provide either a standard library or basic implementations of

stdint.h -> all int types
stddef.h -> NULL
string.h -> memset

Functionality
---

1. Create thread with static memory allocation
2. Suspend and resume thread execution
3. Sleep function for a number of ticks
4. Yield execution back to the scheduler
5. Atomic blocks of code
