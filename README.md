<div align="center">
    <img src="logo.png" alt="Aestra Logo" height="200"/>
    <h1>Aestra</h1>
</div>

*Note: Aestra contains AI-generated code and content.*

**Aestra** is a UCI-compliant chess engine written in C++17. It communicates via the [Universal Chess Interface (UCI)](https://www.chessprogramming.org/UCI) protocol and is compatible with any GUI that supports UCI — including [Cute Chess](https://cutechess.com/), [En Croissant](https://encroissant.org/), and others.

## Features

### Board Representation

- 64‑bit bitboards with LSB0 square indexing (`a1` = 0, `h8` = 63)
- **Magic bitboards** for sliding pieces, with precomputed magic numbers and a PEXT‑based (BMI2) fast path
- **Zobrist hashing** for position keys
- Compact 16‑bit packed moves (`from`, `to`, move type, promotion piece)
- Incremental piece‑square score maintained in `make`/`unmake` for efficient evaluation
- FEN parsing, `make`/`unmake`, castling, en‑passant, promotions, and draw detection

### Search

- **Iterative deepening** with **aspiration windows** (Δ = 25 cp)
- **PVS** — principal variation search with zero‑window searches on the remaining moves
- **Quiescence search** with stand‑pat, extended to all legal moves when in check
- **Transposition table**: 4 entries per bucket, mate‑distance pruning.
- **Futility pruning**: 150 cp threshold
- **Null‑move pruning** (reduction R = 4)
- **Late Move Reductions (LMR)**, with re‑search on fail‑high
- **Check extensions** (+1 ply for moves giving check)
- **Move ordering**: TT move → promotions → MVV‑LVA captures → killer moves → history heuristic
- **Lazy SMP** multithreading (up to 16 threads) with root move rotation
- Time management

### Evaluation

- Material values and piece‑square tables in middlegame/endgame pairs
- Bishop pair bonus
- Mobility for bishops, rooks, and queens
- Passed pawns, connected pawns, doubled pawns, isolated pawns
- Rook on open files or semi-open files
- Rook on the seventh rank bonus
- Tapered evaluation based on game phase
- Small tempo bonus for the side to move

### UCI Support

#### Options

- `Hash` (1 MB–1 TB)
- `Threads` (1–16)

#### Additional Commands

- `board` - print the current board state
- `eval` - print the current evaluation score
- `bench` - run the benchmarking suite

## Building

Requirements:

- A C++17 compiler (the Makefile defaults to **clang++**)
- GNU Make
- A 64‑bit CPU

Build the native‑optimized binary (CPU features auto‑detected at compile time):

```sh
make basic
```

Or run `make release` to build all hand‑tuned variants at once:

| Target          | Features                                            |
| --------------- | --------------------------------------------------- |
| `none`          | No CPU features enabled                             |
| `pext`          | popcount + BMI2 PEXT                                |
| `popcnt`        | popcount                                            |

Additional target: `make basic` (native, fastest on the build machine).

## Usage

Run the engine directly for an interactive UCI session:

```sh
./Aestra.exe
uci
isready
position startpos moves e2e4 e7e5
go depth 12
```

To play against it, point a UCI‑compatible GUI at the `Aestra.exe` binary as a new engine.

## Credits and Thanks

Aestra has drawn inspiration from the following engines:

- [Stockfish](https://github.com/official-stockfish/Stockfish) by The Stockfish Team
- [Ethereal](https://github.com/AndyGrant/Ethereal) by Andrew Grant
- [Altair](https://github.com/Alex2262/AltairChessEngine) by Alexander Tian

The following tools were used for development:

- [Cute Chess](https://cutechess.com/)
- [Fastchess](https://github.com/Disservin/fastchess)
- [Texel Tuner](https://github.com/GediminasMasaitis/texel-tuner)

Additional thanks to:

- [The Chess Programming Wiki](https://www.chessprogramming.org/)

AI tools have also been used throughout the development process, thanks to modern technological advancements.

## License

Aestra is free software distributed under the terms of the [GNU General Public License, version 3](LICENSE).

## AI Generations

Throughout development, we made regular use of AI tools to assist with debugging and improve code readability. The primary tools we leveraged include:

- [Claude Code](https://anthropic.com/claude-code) from Anthropic
- [Codex](https://openai.com/codex/) from OpenAI
- [DeepSeek Harness](https://github.com/deepseek-ai/deepseek-harness) from DeepSeek

We also worked with the following models during development:

- [DeepSeek](https://deepseek.com/)
- [GPT 5.6](https://openai.com/index/gpt-5-6/)

## Borrowed Data Sources

The following files were from external sources:

- `bench.csv`: Directly from Ethereal's benchmarking data
- `attack.hpp` magics: Copied from Ethereal's attack.hpp
- `types.hpp` PRNG: Stockfish's xorshift implementation
