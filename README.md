# Hot Dog Manager - Producer-Consumer Concurrency Program

A multi-threaded C program that simulates a factory with making machines (producers) and packing machines (consumers) using POSIX threads.

## Prerequisites

- GCC compiler
- POSIX threads library (pthread)
- Linux/Unix environment (or WSL on Windows)

## Compilation

To compile the program, use:

```bash
gcc -o hotdog_manager hotdog_manager.c -lpthread
```

This will create an executable named `hotdog_manager`.

**Alternative compilation with debugging flags:**
```bash
gcc -o hotdog_manager hotdog_manager.c -lpthread -Wall -g
```

## Running the Program

Run the program with the following command:

```bash
./hotdog_manager N S M P
```

### Arguments

- **N** - Number of hot dogs to make (must be > 0)
- **S** - Number of slots in the hot dog pool (must be < N)
- **M** - Number of making machines (1-30)
- **P** - Number of packing machines (1-30)

### Example

```bash
./hotdog_manager 10 3 2 2
```

This will:
- Make 10 hot dogs
- Use a pool with 3 slots
- Create 2 making machines (m1, m2)
- Create 2 packing machines (p1, p2)

## Output

The program creates a `log.txt` file containing:

1. **Header**: Order, capacity, number of making machines, and packing machines
2. **Production Log**: Records of each hot dog being made and packed
3. **Summary**: Statistics showing how many hot dogs each machine made/packed

### Viewing the Output

After running the program, view the log file:

```bash
cat log.txt
```

Or on Windows:
```bash
type log.txt
```

## Sample Output

```
order:10
capacity:3
making machines:2
packing machines:2
-----
m1 puts 1
m2 puts 2
p1 gets 1 from m1
...
-----
summary:
m1 made 5
m2 made 5
p1 packed 5
p2 packed 5
```

## Notes

- The program uses pthread synchronization primitives (mutexes and condition variables)
- All file operations are thread-safe
- The program will wait for all threads to complete before generating the summary

