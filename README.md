# myls — Custom Linux Directory Lister

A POSIX-compliant implementation of the standard `ls` utility, written in **C++11** for Linux systems. This project was developed as a technical assessment.

## Features

The utility implements the most frequently used parameters of the original `ls`:

**Detailed Output (`-l`)**: Displays a detailed table of files, including permissions, hard link count, owner, group, size, last modification time, and filename.

**Reverse Sort (`-r`)**: Reverses the order of the output (default is alphabetical).

**Human-Readable Sizes (`-h`)**: Converts byte sizes into readable formats, e.g., 1024 = 1K.

**Color Support**: Automatically highlights directories, executables, and symlinks when outputting to a terminal.

## Installation & Usage

### Prerequisites

* OS: Linux 

* Compiler: GCC (with C++11 support) 

### Build

To compile the utility using the provided `Makefile`:

```bash
make
```

### Run

```bash
./myls [-l] [-r] [-h] [directory]
```

If no directory is specified, the current directory is used by default.

## Notes on Requirements

The technical specification mentioned a `-1` parameter in text but displayed `-l` (long format) logic in examples. This utility implements the **long format (`-l`)** as it matches the provided visual samples and standard usage patterns.