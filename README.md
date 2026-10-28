<div align="center">

# 🧩 Cryptarithm Solver — Optimization Lab

### `SEND + MORE = MONEY` → `9567 + 1085 = 10652`, and then 600× faster.

A verbal-arithmetic solver in plain C11, taken from brute force to branch-and-bound
through 8 measurable versions — every optimization isolated, profiled and plotted.

<br/>


<img src="https://img.shields.io/badge/C-C11-00599C?logo=c&logoColor=white" />
<img src="https://img.shields.io/badge/CMake-3.10%2B-064F8C?logo=cmake&logoColor=white" />
<img src="https://img.shields.io/badge/macOS-arm64%20%7C%20x86__64-000000?logo=apple&logoColor=white" />
<img src="https://img.shields.io/badge/Windows-MSVC%20%7C%20MinGW-0078D6?logo=windows&logoColor=white" />
<img src="https://img.shields.io/badge/Linux-gcc%20%7C%20clang-FCC624?logo=linux&logoColor=black" />
<br>

<br/>
<img src="plots/speedup.png" width="80%" alt="Speedup of every version relative to brute force" />
<br/>
<sub>Speedup over the brute-force baseline (geometric mean over 15 puzzles, log scale).</sub>
<br/><br/>

<sub>

[✨ Features](#-features) | [🪜 Optimization ladder](#-optimization-ladder) | [📊 Results](#-results) | [📈 Complexity](#-complexity) | [🔧 Build](#-build) | [🧪 Experiments](#-experiments)

</sub>

</div>

---



## ✨ Features

<table>
<tr>
<td width="50%" valign="top">

**🧮 Solves any additive cryptarithm**
- 2 to 7 addends: `ELEVEN + NINE + FIVE + FIVE = THIRTY`
- One digit per letter, no leading zeros
- Strict parser — rejects `SEND MORE = MONEY`, `A + + B = C`
- Every answer re-checked by an independent `verify()`

</td>
<td width="50%" valign="top">

**🪜 8 versions, one idea each**
- 2 machine-independent optimizations
- 3 algorithmic optimizations
- 1 heuristic
- 1 machine-dependent optimization
- Each version is a standalone file — diff any two

</td>
</tr>
<tr>
<td width="50%" valign="top">

**📊 Measured, not guessed**
- Built-in benchmark mode with a node counter
- Empirical complexity vs. theoretical bound
- 300 random puzzles with guaranteed solutions
- Median of repeated runs + compiler-flag study

</td>
<td width="50%" valign="top">

**💻 Runs**
- macOS (Apple Silicon & Intel), Windows, Linux
- MSVC · MinGW · gcc · clang
- Portable timer, `ctz` intrinsic, UTF-8 console
- One CMake file, no dependencies

</td>
</tr>
</table>

---

## 🪜 Optimization ladder

Each step builds on the previous one, so the effect of every single idea is visible.

| Version | Idea | Type |
|---------|------|------|
| **`v0_baseline`** | Try every digit assignment; at each leaf rebuild every number from its letters and check the sum | *baseline* |
| **`v1_coeffs`** | Precompute a weight per letter (`±10^position`) — a leaf becomes `Σ weight·digit` | machine-independent |
| **`v2_incremental`** | Carry the partial sum down the recursion — a leaf is checked in O(1) | machine-independent |
| **`v3_bounds`** | Branch and bound: cut a branch when the remaining letters can no longer bring the sum to 0 | algorithmic |
| **`v3_order`** | Assign letters with the largest \|weight\| first, so the bound bites from the very first levels | heuristic |
| **`v4_columns`** | Column-wise search with carry: check a column as soon as all its letters are known | algorithmic |
| **`v5_forced`** | A result letter is never guessed — its digit is computed as `(column sum + carry) mod 10` | algorithmic |
| **`v6_bitmask`** | Used digits as a 10-bit mask; iterate free digits with count-trailing-zeros | machine-dependent |

---

## 📊 Results

<div align="center">

<img src="plots/repeats.png" width="80%" alt="Total time per version, median of 7 runs" />

</div>

| | v0 | v1 | v2 | v3 bounds | v3 order | v4 | v5 | v6 |
|---|---|---|---|---|---|---|---|---|
| **Total, ms** (15 puzzles) | 369 | 245 | 243 | 30.4 | 0.51 | 0.96 | 0.92 | 0.61 |
| **Speedup** | ×1 | ×1.5 | ×1.5 | ×12 | ×724 | ×385 | ×401 | ×605 |

<sub>Median of 7 runs, `-O2`. Machine and compiler are recorded in `experiments/system.txt`.</sub>

> 💡 **The interesting part:** the fastest version is not the last one. Ordering letters by weight (`v3_order`) lets the bound prune from the most significant digits down, while column search (`v4`–`v6`) has to go right to left. Columns win on puzzles like `CROSS + ROADS = DANGER` and on long 5–7-addend sums; ordering wins where one or two leading digits decide everything.

---

## 📈 Complexity

A puzzle with *n* distinct letters has at most A(10, *n*) = 10!/(10−*n*)! digit assignments — 3.6 million for 10 letters. Every version walks the same search tree; they differ in **how early a dead branch is cut** and **how much one node costs**.

| Version | Worst case | In practice |
|---------|------------|-------------|
| v0 | O(A(10, n) · L), L — expression length | full tree |
| v1 | O(A(10, n) · n) | full tree, cheaper leaves |
| v2 | O(A(10, n)) | full tree, O(1) leaves |
| v3–v6 | O(A(10, n)) | 2–4 orders of magnitude fewer nodes |

<div align="center">
<img src="plots/complexity.png" width="80%" alt="Search nodes vs number of letters" />
<br/>
<sub>Measured nodes vs. the theoretical tree size Σ A(10, k), 15 random puzzles per point.</sub>
</div>

---

## 🔧 Build

<details open>
<summary><b>🍎 macOS</b></summary>

```bash
xcode-select --install # compiler, once
brew install cmake # or use the cmake bundled with CLion
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

No CMake? Plain `cc` works too:

```bash
mkdir -p build
for v in v0_baseline v1_coeffs v2_incremental v3_bounds v3_order v4_columns v5_forced v6_bitmask; do
  cc -std=c11 -O2 -Wall -Wextra -o build/$v $v.c
done
```

</details>

<details>
<summary><b>🪟 Windows — Visual Studio</b></summary>

Visual Studio 2019 16.8+ with the *Desktop development with C++* workload. In **Developer PowerShell for VS**:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Binaries land in `build\Release\`.

</details>

<details>
<summary><b>🪟 Windows - MinGW / MSYS2</b></summary>

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

</details>

<details>
<summary><b>🐧 Linux</b></summary>

```bash
cmake -S . -B build && cmake --build build
```

</details>

### Run

```bash
./build/v6_bitmask # solve the 15 built-in puzzles
./build/v6_bitmask "BIG + CAT = LION" # your own puzzle
./build/v6_bitmask -b # benchmark
./build/v6_bitmask -b -t 50 -f puzzles.txt # 50 ms per puzzle, puzzles from a file
```

Benchmark output: `puzzle ⇥ ms per solve ⇥ search nodes ⇥ OK/FAIL`. The exit code is non-zero if any answer fails verification.

---

## 🧪 Experiments

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install matplotlib

python3 bench.py build 
python3 experiments.py # 4 extra experiments, ~2–5 min
```

| # | Experiment | Chart |
|---|------------|-------|
| **1** | Search nodes vs. number of letters *n* = 3…10, against Σ A(10, k) | `complexity.png` |
| **2** | 300 random puzzles with a guaranteed solution — all versions must agree | `random_boxplot.png` |
| **3** | 7 repeated runs: median, min…max, machine info | `repeats.png` · `system.txt` |
| **4** | `-O0 … -O3 -march=native` for v0, v3 order and v6 | `flags.png` |

<details>
<summary><b>Correctness on random puzzles</b></summary>

Puzzles are generated backwards: random numbers without leading zeros are added up, then every digit is replaced by its own letter — so a solution always exists. All 8 versions solve all 300 of them, each answer checked by `verify()`.

<img src="plots/random_boxplot.png" width="100%" alt="Time distribution on 300 random puzzles" />

</details>

<details>
<summary><b>Compiler flags vs. algorithm</b></summary>

The compiler buys ~3×. The algorithm buys ~600×.

<img src="plots/flags.png" width="100%" alt="Effect of compiler flags" />

</details>

<details>
<summary><b>🔬 Profiling</b></summary>

- **macOS:** Instruments → Time Profiler, or `xctrace record --template "Time Profiler" --launch -- ./build/v0_baseline -b`
- **Linux / MinGW:** `cmake -S . -B build-prof -DREBUS_PROFILE=ON && cmake --build build-prof`, run once, then `gprof build-prof/v0_baseline gmon.out`
- **Windows:** Visual Studio → Debug → Performance Profiler → CPU Usage

Every version also counts search nodes — a machine-independent measure of how much work an optimization removes.

</details>

---

## 📁 Project structure

```
optimization_1_remastered/
├── rebus.h                    parser · output · verify() · tests · timer · main()
├── v0_baseline.c              brute force
├── v1_coeffs.c                letter weights
├── v2_incremental.c           incremental sum
├── v3_bounds.c                branch and bound
├── v3_order.c                 + letters ordered by |weight|
├── v4_columns.c               column search with carry
├── v5_forced.c                + computed result digits
├── v6_bitmask.c               + bitmask & ctz
├── CMakeLists.txt             one target per version
├── bench.py                   benchmark → results.csv + plots/
├── experiments.py             4 extra experiments → experiments/ + plots/
├── experiments/               raw CSV data · system.txt
└── plots/                     charts shown in this README
```

Each `vN_*.c` includes `rebus.h` and implements a single function, `solve_core()`.

## 📦 Requirements

- A C11 compiler: Apple clang, gcc, clang, MSVC 2019 16.8+ or MinGW.
- CMake 3.10+ (optional — `cc` alone is enough).
- Python 3.8+ with `matplotlib` for charts.

---

