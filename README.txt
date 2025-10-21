# Submarine Strike Game

This project is a simple game written in C using the Raylib library. The game involves controlling a submarine and defeating waves of enemies. To compile and run the game, you need to have MinGW and Raylib installed on your system.

## Prerequisites

- **MinGW**: A minimalist development environment for native Microsoft Windows applications.
- **Raylib**: A simple and easy-to-use library for video game programming.

## Installation

1. **Install MinGW**: 
   - Download and install MinGW from [MinGW official website](http://www.mingw.org/).
   - Ensure that the `bin` directory of MinGW is added to your system's PATH.

2. **Install Raylib**:
   - Follow the instructions on the [Raylib official website](https://www.raylib.com/) to download and install Raylib.
   - Make sure to set up the include and lib directories for Raylib in your project.

## Compilation

To compile the program, open a terminal or command prompt and navigate to the directory containing `main.c`. Then, run the following command:
gcc main.c -o main.exe -I -L -lraylib -lopengl32 -lgdi32 -lwinmm

- `-o main.exe`: Specifies the output file name.
- `-O1`: Enables optimization.
- `-Wall`: Enables all compiler's warning messages.
- `-std=c99`: Specifies the C standard to use.
- `-Wno-missing-braces`: Suppresses warnings about missing braces.
- `-I include/`: Specifies the directory for header files.
- `-L lib/`: Specifies the directory for library files.
- `-lraylib`, `-lopengl32`, `-lgdi32`, `-lwinmm`: Links the necessary libraries.

## Running the Game

After successful compilation, run the game in the terminal using the following command:
\main.exe

Enjoy the game!

## Troubleshooting

- Ensure that MinGW and Raylib are correctly installed and configured.
- Check that all paths in the compile command are correct and point to the appropriate directories.

For further assistance, refer to the documentation of MinGW and Raylib.
