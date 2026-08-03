# Aestra

*Disclaimer*: Aestra contains AI-generated code and content.

**Aestra** is a UCI-compliant chess engine written in C++17. It plays by the [Universal Chess Interface (UCI)](https://www.chessprogramming.org/UCI) protocol, so it works with any compatible GUI — [Cute Chess](https://cutechess.com/), [En Croissant](https://encroissant.org/), among others.

## Features

### Board representation

- 64-bit bitboards with LSB0 square indexing (`a1` = 0, `h8` = 63)
- **Magic bitboards** for sliding pieces, with precomputed magic numbers and a
  PEXT-based (BMI2) fast path
- **Zobrist hashing** for position keys
- Compact 16-bit packed moves (`from`, `to`, move type, promotion piece)
- Incremental piece-square score kept in `make`/`unmake` for a cheap evaluation
- FEN parsing, `make`/`unmake`, castling, en-passant, promotions, and draw detection

### Search

- Iterative deepening with **aspiration windows** (Δ = 25 cp)
- **PVS** — principal variation search (zero-window searches on the remaining
  moves)
- **Quiescence search** with stand-pat, extended to all legal moves when in
  check
- **Transposition table**: 4 entries per bucket, 16-bit key verification,
  generation-based replacement, bound types (exact/lower/upper), and
  mate-distance pruning. Configurable from 1 to 1 TB (default 16 MB)
- **Null-move pruning** (reduction R = 4)
- **Late move reductions (LMR)**, with re-search on fail-high
- Check extensions (+1 ply for moves giving check)
- **Move ordering**: en passant → TT move → promotions → MVV-LVA captures →
  killer moves → history heuristic
- Draw score detection
- **Lazy SMP** multithreading (up to 256 threads) with root move rotation
- Time management

### Evaluation

- Material values and **piece-square tables** in middlegame/endgame pairs (tunable-friendly single table layout in `psqt.cpp`)
- Game-phase based **tapering** between middlegame and endgame scores
- Small tempo bonus for the side to move

### UCI support

Commands: `uci`, `isready`, `setoption`, `ucinewgame`, `position`
(`startpos`/`fen` + `moves`), `go`, `stop`, `quit`. Options: `Hash`
(1–1024 MB) and `Threads` (1–256).

## Building

Requirements:

- A C++17 compiler (the Makefile defaults to **clang++**)
- GNU Make
- A 64-bit CPU (SSE2 baseline; popcount / BMI2 / AVX / AVX2 are detected
  or selected per target)

Build the native-optimized binary (CPU features auto-detected at compile time):

```sh
make basic
```

Or `make release` to build all hand-tuned variants at once:

| Target          | Features                                            |
| --------------- | --------------------------------------------------- |
| `ssse3-popcnt`  | SSSE3 + popcount                                    |
| `ssse3-pext`    | SSSE3 + popcount + BMI2 PEXT                        |
| `avx-popcnt`    | AVX + popcount                                      |
| `avx-pext`      | AVX + popcount + BMI2 PEXT                          |
| `avx2-popcnt`   | AVX2 + FMA + popcount                               |
| `avx2-pext`     | AVX2 + FMA + popcount + BMI2 PEXT                   |

Additional targets: `make basic` (native, fastest on the build machine). All builds use `-O3`; the release variants additionally link statically and the binary is written to `Aestra.exe`.

## Usage

Run the engine directly for an interactive UCI session:

```sh
./Aestra.exe
uci
isready
position startpos moves e2e4 e7e5
go depth 12
```

To play against it, point a UCI-compatible GUI at the `Aestra.exe` binary as a
new engine. Example session against itself from the starting position:

```plaintext
uci
isready
ucinewgame
position startpos
go wtime 60000 btime 60000
...
info depth 16 seldepth 30 score cp 21 nodes 123456 nps 123456 time 789 pv e2e4 e7e5 g1f3 b8c6
bestmove e2e4
```

Scores are reported in centipawns (`cp`), or as `mate N` when a forced mate is
found.

## Credits and Thanks

Aestra has taken inspiration from the following engines:

- [Stockfish](https://github.com/official-stockfish/Stockfish) by The Stockfish Team
- [Ethereal](https://github.com/AndyGrant/Ethereal) by Andrew Grant
- [Altair](https://github.com/Alex2262/AltairChessEngine) by Alexander Tian

Also thanks to the following resources:

- [The Chess Programming Wiki](https://www.chessprogramming.org/)

Thanks to modern technological developments, AI tools (especially [Claude Code](https://claude.ai/) and [DeepSeek](https://deepseek.com/)) have also been used during the development process.

## License

Aestra is free software distributed under the terms of the [GNU General Public License, version 3](LICENSE).
