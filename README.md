# OS Process Scheduler

## Project Description

This project is a simulation of an operating system process scheduler, designed to demonstrate how different scheduling algorithms and memory management techniques work in practice. The system consists of multiple cooperating processes that communicate using inter-process communication (IPC) mechanisms such as message queues and shared memory. It supports several classic scheduling algorithms (SJF, HPF, RR, Multilevel Feedback Queue) and implements a buddy system for dynamic memory allocation. The project is intended for educational purposes, providing insight into the inner workings of OS-level process management and scheduling.

## Features

- **Process Scheduling Algorithms:**
  - Shortest Job First (SJF)
  - Highest Priority First (HPF)
  - Round Robin (RR)
  - Multilevel Feedback Queue

- **Memory Management:**
  - Buddy system allocation and deallocation

- **Inter-Process Communication:**
  - Message queues for process transfer between generator and scheduler
  - Shared memory for clock emulation

## Project Structure

- `process_generator.c` — Reads process data and sends processes to the scheduler at the correct time.
- `scheduler.c` — Implements the scheduling algorithms and manages process execution and memory.
- `clk.c` — Emulates a system clock using shared memory.
- `process.c` — Represents the process to be scheduled and executed.
- `test_generator.c` — Generates random process data for testing.
- `DataStructures.h` — Contains data structures for processes, queues, priority queues, and memory blocks.
- `headers.h` — Contains clock and IPC helper functions.
- `processes.txt` — Input file with process data.
- `scheduler.log` — Log file for process events.
- `scheduler.perf` — Performance metrics output.
- `memory.log` — Memory allocation/deallocation log.
- `Makefile` — Build and run automation.

## How to Build

To compile the project, run:
```sh
make
```

To clean build artifacts:
```sh
make clean
```

## How to Run

1. Generate process data (optional):
    ```sh
    ./test_generator.out
    ```
    This will create or overwrite `processes.txt`.

2. Run the scheduler system:
    ```sh
    make run
    ```
    This will start the process generator, which forks the clock and scheduler.

3. You can also run the components manually if needed.

## Notes

- Make sure to add any new source files to the `Makefile`.
- The clock must be running before any process tries to access it.
- All logs and output files are generated in the project root.
