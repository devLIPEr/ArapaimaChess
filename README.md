<p align="center" style="font-weight:bold; font-size:32px">
  ArapaimaChess
</p>

<p align="center">
  <img src="./assets/icon.png" width="200" alt="ArapaimaChess logo" />
</p>

<p align="center" style="font-size:16px">
  A simple chess engine made in C++
</p

### Overview

ArapaimaChess is a simple chess engine made from scratch in C++. It does not include a graphical interface, only a CLI. You can interact with it via Universal Chess Interface (UCI) commands.

### Features

This engine implements a few techniques for modularity, you can configure them editing [config.h](https://github.com/devLIPEr/ArapaimaChess/blob/main/src/config.h), comment or remove the line you want to disable the feature and them uncomment or add the line you want to enable.

The [opening_book.txt](https://github.com/devLIPEr/ArapaimaChess/blob/master/opening_book.txt) was built using [Lichess/chess-openings](https://huggingface.co/datasets/Lichess/chess-openings) dataset, limiting to openings with 10 or less moves.

The neural network is a MLP with sizes 781 x 16 x 8 x 1, trained to predict the position evaluation based on the [Lichess/chess-position-evaluations](https://huggingface.co/datasets/Lichess/chess-position-evaluations) dataset.

Syzygy Table probing support using [Fathom](https://github.com/jdart1/Fathom).

Pruning techniques to search fewer positions at each depth (more at [search.cpp](https://github.com/devLIPEr/ArapaimaChess/blob/main/src/search.cpp)):

* Null Move Pruning
* Futility Pruning
* Late Move Pruning
* Razoring
* Delta Pruning

UCI support and a few extra commands:

* uci, to show information about the engine, and the configurable options.
* ucinewgame, to start a new game, clearing previous search results.
* position, to set a position.
* go, to start searching.
* setoption, to change engine options.
* quit/stop, to quit the program and to stop the search
* d/display/print, to show the position and some extra info about it.
* move, make a move directly, without having to pass the move sequence through the position command.

### Files

This repository consists of this files:

* [src](https://github.com/devLIPEr/ArapaimaChess/tree/main/src), a subdirectory that contains the source code.
* chess.nn, a simple neural network used for evaluation.
* [CMakeLists.txt](https://github.com/devLIPEr/ArapaimaChess/blob/master/CMakeLists.txt), a file used by CMake to compile the program.
* [opening_book.txt](https://github.com/devLIPEr/ArapaimaChess/blob/master/opening_book.txt), a file containing openings for the program to use.

### Compiling the program

##### Windows

If you are using Windows and have mingw installed, you can compile the engine using the following commands:

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

##### Linux

If you are using a linux distribution, check if you have cmake installed then run the following commands:

```bash
mkdir build
cd build
cmake ..
make
```
