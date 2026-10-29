# python3 bench.py build -> если через Cmake
# python3 bench.py cmake-build-release -> если профиля нет то -debug вместо release
import csv
import math
import os
import platform
import random
import shutil
import statistics
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "experiments")
PLOTS = os.path.join(HERE, "plots")
BIN = os.path.join(OUT, "bin")
EXE = ".exe" if os.name == "nt" else ""

VERSIONS = ["v0_baseline",
            "v1_coeffs",
            "v2_incremental",
            "v3_bounds",
            "v3_order",
            "v4_columns",
            "v5_forced",
            "v6_bitmask"]

SHORT = {"v0_baseline": "v0",
         "v1_coeffs": "v1",
         "v2_incremental": "v2",
         "v3_bounds": "v3a",
         "v3_order": "v3b",
         "v4_columns": "v4",
         "v5_forced": "v5",
         "v6_bitmask": "v6"}

# >>>> Палитра <<<<
CAT = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#6250d6", "#e34948"]
BLUE = CAT[0]
INK, MUTED, GRID, NEUTRAL = "#1f1f1e", "#6b6a63", "#e4e3dc", "#9a998f"

QUICK = "--quick" in sys.argv


# >>>> сборка и запуск <<<<

def compiler():
    cc = os.environ.get("CC")
    if cc:
        return cc
    for c in ("cc", "clang", "gcc"):
        if shutil.which(c):
            return c
    sys.exit("Не найден компилятор C. macOS: xcode-select --install; Windows: MinGW (set CC=gcc).")


def build(version, flags, tag):
    out = os.path.join(BIN, f"{version}_{tag}{EXE}")
    cmd = [compiler(), "-std=c11", *flags, "-o", out, os.path.join(HERE, version + ".c")]
    r = subprocess.run(cmd, capture_output=True, text=True)
    return out if r.returncode == 0 else None

def run(exe, puzzles, min_ms):
    """Запуск в режиме бенчмарка; возвращает список (ребус, мс, узлы, ok)"""
    with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False, encoding="utf-8") as f:
        f.write("\n".join(puzzles) + "\n")
        name = f.name
    try:
        r = subprocess.run([exe, "-b", "-t", str(min_ms), "-f", name],
                           capture_output=True, text=True, encoding="utf-8")
    finally:
        os.unlink(name)
    rows = []
    for line in r.stdout.strip().splitlines():
        q, ms, nodes, st = line.split("\t")
        rows.append((q, float(ms), int(nodes), st == "OK"))
    return rows


def build_all(flags=("-O2",), tag="O2"):
    os.makedirs(BIN, exist_ok=True)
    exes = {}
    for v in VERSIONS:
        exe = build(v, list(flags), tag)
        if not exe:
            sys.exit(f"Не удалось собрать {v}")
        exes[v] = exe
    return exes


def standard_tests():
    """Встроенные тесты из rebus.h"""
    src = open(os.path.join(HERE, "rebus.h"), encoding="utf-8").read()
    block = src[src.index("tests[] = {"):src.index("};", src.index("tests[] = {"))]
    return [line.strip().strip('",') for line in block.splitlines()[1:] if '"' in line]


def write_csv(name, header, rows):
    with open(os.path.join(OUT, name), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


# генератор ребусов

LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"


def make_puzzle(rng, n_letters=None, addends=(2, 7), max_len=7):
    for _ in range(200000):
        k = rng.randint(*addends)
        top = rng.randint(1, max_len)
        nums = []
        for _ in range(k):
            ln = rng.randint(1, top)
            nums.append(rng.randint(10 ** (ln - 1) if ln > 1 else 1, 10 ** ln - 1))
        total = sum(nums)
        if len(str(total)) > 16:
            continue
        digits = set("".join(map(str, nums + [total])))
        if n_letters is not None and len(digits) != n_letters:
            continue
        letters = rng.sample(LETTERS, len(digits))
        m = dict(zip(sorted(digits), letters))
        word = lambda x: "".join(m[c] for c in str(x))
        return " + ".join(map(word, nums)) + " = " + word(total)
    raise RuntimeError(f"не удалось сгенерировать ребус с {n_letters} буквами")


# графики

def plt_setup():
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.ticker import NullFormatter
    os.makedirs(PLOTS, exist_ok=True)
    return plt, NullFormatter


def style(ax, grid_axis="y"):
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(GRID)
    ax.tick_params(colors=MUTED, labelsize=8)
    ax.grid(axis=grid_axis, color=GRID, linewidth=0.6)
    ax.set_axisbelow(True)


def title(ax, text):
    ax.set_title(text, color=INK, fontsize=11, loc="left")


def legend(ax, **kw):
    lg = ax.legend(frameon=False, fontsize=8, **kw)
    for t in lg.get_texts():
        t.set_color(INK)


def save(fig, name):
    fig.tight_layout()
    fig.savefig(os.path.join(PLOTS, name), dpi=160)
    print(f"    [+] plots/{name}")


# >>>> 1. сложность <<<<

def tree_size(n):
    """Число узлов полного дерева перебора, именно сумма размещений A(10, k), k = 0..n"""
    return sum(math.perm(10, k) for k in range(n + 1))


def exp_complexity(exes):
    print("[1] Эмпирическая сложность: узлы перебора от числа букв")
    rng = random.Random(1)
    per_n = 5 if QUICK else 15
    ns = list(range(3, 11))
    puzzles = {n: [make_puzzle(rng, n, addends=(2, 4), max_len=6) for _ in range(per_n)] for n in ns}

    # v1 и v2 обходят то же дерево, что и v0; v6 - то же, что v5 (меняется только цена узла)
    shown = [("v0_baseline", "v0–v2: полный перебор"),
             ("v3_bounds", "v3a: границы"),
             ("v3_order", "v3b: + порядок букв"),
             ("v4_columns", "v4: столбцы"),
             ("v5_forced", "v5–v6: вычисление цифры")]
    rows, mean_nodes = [], {}
    for v, _ in shown:
        mean_nodes[v] = []
        for n in ns:
            res = run(exes[v], puzzles[n], 0)
            assert all(ok for *_, ok in res), f"{v}: неверный ответ"
            nodes = [r[2] for r in res]
            mean_nodes[v].append(statistics.geometric_mean(nodes))
            rows += [(v, n, q, nd) for (q, _, nd, _) in res]
    write_csv("complexity.csv", ["version", "letters", "puzzle", "nodes"], rows)

    plt, NullF = plt_setup()
    fig, ax = plt.subplots(figsize=(9, 5))
    ax.plot(ns, [tree_size(n) for n in ns], color=NEUTRAL, linewidth=2, linestyle="--",
            label="теория: полное дерево Σ A(10,k)")
    for i, (v, label) in enumerate(shown):
        ax.plot(ns, mean_nodes[v], color=CAT[i], linewidth=2, marker="o", markersize=5, label=label)
    ax.set_yscale("log")
    ax.yaxis.set_minor_formatter(NullF())
    ax.set_xticks(ns)
    ax.set_xlabel("число разных букв n", color=MUTED, fontsize=9)
    ax.set_ylabel("узлы перебора (ср. геометрическое, лог. шкала)", color=MUTED, fontsize=9)
    title(ax, f"Сложность: рост перебора с числом букв ({per_n} случайных ребусов на точку)")
    style(ax, "both")
    legend(ax, loc="upper left")
    save(fig, "complexity.png")
    plt.close(fig)

    v0 = mean_nodes["v0_baseline"]
    print("    n:", ns)
    print("    v0 / теория:", [f"{a / tree_size(n):.2f}" for a, n in zip(v0, ns)])


# >>>> 2. случайные ребусы <<<<

def exp_random(exes):
    count = 60 if QUICK else 300
    print(f"[2] Корректность и распределение времени: {count} случайных ребусов")
    rng = random.Random(2)
    puzzles = [make_puzzle(rng, None, addends=(2, 7), max_len=6) for _ in range(count)]
    puzzles = [p for p in puzzles if len(set(c for c in p if c.isalpha())) <= 10]

    times, fails, rows = {}, {}, []
    for v in VERSIONS:
        t0 = time.time()
        res = run(exes[v], puzzles, 0 if v in ("v0_baseline", "v1_coeffs", "v2_incremental") else 2)
        fails[v] = sum(not ok for *_, ok in res) + (len(puzzles) - len(res))
        times[v] = [r[1] for r in res]
        rows += [(v, q, ms, nd, int(ok)) for (q, ms, nd, ok) in res]
        print(f"    {SHORT[v]:>4}: {len(res)} решено, ошибок {fails[v]}  ({time.time() - t0:.1f} с)")
    write_csv("random.csv", ["version", "puzzle", "ms", "nodes", "ok"], rows)

    plt, NullF = plt_setup()
    fig, ax = plt.subplots(figsize=(9, 4.8))
    data = [[max(t, 1e-5) for t in times[v]] for v in VERSIONS]
    bp = ax.boxplot(data, widths=0.55, patch_artist=True, showfliers=True,
                    medianprops=dict(color=INK, linewidth=1.5),
                    whiskerprops=dict(color=MUTED), capprops=dict(color=MUTED),
                    flierprops=dict(marker="o", markersize=2.5, markerfacecolor=NEUTRAL,
                                    markeredgecolor="none", alpha=0.6))
    for b in bp["boxes"]:
        b.set_facecolor(BLUE)
        b.set_edgecolor(BLUE)
        b.set_alpha(0.85)
    ax.set_yscale("log")
    ax.yaxis.set_minor_formatter(NullF())
    ax.set_xticks(range(1, len(VERSIONS) + 1))
    ax.set_xticklabels([SHORT[v] for v in VERSIONS])
    ax.set_ylabel("мс на ребус (лог. шкала)", color=MUTED, fontsize=9)
    ok_all = all(f == 0 for f in fails.values())
    title(ax, f"Время на {len(puzzles)} случайных ребусах (2–7 слагаемых)"
              + (" — все ответы верны" if ok_all else " — ЕСТЬ ОШИБКИ!!!!"))
    style(ax)
    save(fig, "random_boxplot.png")
    plt.close(fig)


# >>>> 3. повторные замеры <<<<

def system_info():
    cc = compiler()
    try:
        ver = subprocess.run([cc, "--version"], capture_output=True, text=True).stdout.splitlines()[0]
    except Exception:
        ver = "?"
    cpu = platform.processor() or platform.machine()
    if sys.platform == "darwin":
        try:
            cpu = subprocess.run(["sysctl", "-n", "machdep.cpu.brand_string"],
                                 capture_output=True, text=True).stdout.strip() or cpu
        except Exception:
            pass
    elif sys.platform.startswith("linux"):
        try:
            for line in open("/proc/cpuinfo"):
                if line.startswith("model name"):
                    cpu = line.split(":", 1)[1].strip()
                    break
        except Exception:
            pass
    return {
        "ОС": platform.platform(),
        "Процессор": cpu,
        "Ядер": os.cpu_count(),
        "Компилятор": f"{cc}: {ver}",
        "Флаги": "-std=c11 -O2",
        "Python": platform.python_version(),
        "Дата": time.strftime("%Y-%m-%d %H:%M"),
    }


def exp_repeats(exes):
    reps = 3 if QUICK else 7
    min_ms = 20 if QUICK else 50
    print(f"[3] Повторные замеры: {reps} прогонов, не меньше {min_ms} мс на ребус")
    tests = standard_tests()
    totals = {v: [] for v in VERSIONS}
    per_test = {(v, q): [] for v in VERSIONS for q in tests}
    for r in range(reps):
        for v in VERSIONS:
            res = run(exes[v], tests, min_ms)
            totals[v].append(sum(x[1] for x in res))
            for q, ms, _, _ in res:
                per_test[(v, q)].append(ms)
        print(f"    прогон {r + 1}/{reps}")

    def q(xs, p):
        xs = sorted(xs)
        return xs[min(len(xs) - 1, max(0, round(p * (len(xs) - 1))))]

    rows = []
    for v in VERSIONS:
        for t in tests:
            xs = per_test[(v, t)]
            rows.append((v, t, statistics.median(xs), q(xs, 0.25), q(xs, 0.75), min(xs), max(xs),
                         statistics.stdev(xs) / statistics.mean(xs) * 100 if len(xs) > 1 else 0))
    write_csv("repeats.csv", ["version", "puzzle", "median_ms", "q25_ms", "q75_ms", "min_ms", "max_ms",
                              "cv_percent"], rows)

    info = system_info()
    with open(os.path.join(OUT, "system.txt"), "w", encoding="utf-8") as f:
        for k, val in info.items():
            f.write(f"{k}: {val}\n")
        f.write(f"\nПовторов: {reps}, минимум {min_ms} мс замера на ребус\n\n")
        f.write(f"{'версия':<8}{'медиана, мс':>14}{'мин':>12}{'макс':>12}{'разброс':>10}\n")
        for v in VERSIONS:
            xs = totals[v]
            med = statistics.median(xs)
            f.write(f"{SHORT[v]:<8}{med:>14.4f}{min(xs):>12.4f}{max(xs):>12.4f}"
                    f"{(max(xs) - min(xs)) / med * 100:>9.1f}%\n")
    print("    [+] experiments/system.txt")

    plt, NullF = plt_setup()
    fig, ax = plt.subplots(figsize=(9, 4.8))
    x = range(len(VERSIONS))
    med = [statistics.median(totals[v]) for v in VERSIONS]
    lo = [m - min(totals[v]) for m, v in zip(med, VERSIONS)]
    hi = [max(totals[v]) - m for m, v in zip(med, VERSIONS)]
    ax.bar(x, med, width=0.6, color=BLUE)
    ax.errorbar(x, med, yerr=[lo, hi], fmt="none", ecolor=INK, elinewidth=1.2, capsize=4)
    for i, m in enumerate(med):
        ax.annotate(f"{m:.3g}", (i, m + hi[i]), xytext=(0, 4), textcoords="offset points",
                    ha="center", fontsize=7, color=INK)
    ax.set_yscale("log")
    ax.yaxis.set_minor_formatter(NullF())
    ax.set_xticks(list(x))
    ax.set_xticklabels([SHORT[v] for v in VERSIONS])
    ax.set_ylabel("мс на все 15 тестов (лог. шкала)", color=MUTED, fontsize=9)
    title(ax, f"Суммарное время: медиана {reps} прогонов, усы — мин…макс")
    ax.text(0.99, 0.97, f"{info['Процессор']}\n{info['Компилятор'].split(':')[0]} -O2",
            transform=ax.transAxes, ha="right", va="top", fontsize=7, color=MUTED)
    style(ax)
    save(fig, "repeats.png")
    plt.close(fig)


# >>>> 4. флаги компилятора <<<<

def native_flag():
    """-march=native там, где он поддерживается (Apple clang на arm64 понимает -mcpu=native)"""
    probe = os.path.join(BIN, "probe" + EXE)
    src = os.path.join(HERE, "v0_baseline.c")
    for f in (["-march=native"], ["-mcpu=native"]):
        r = subprocess.run([compiler(), "-std=c11", "-O3", *f, "-o", probe, src],
                           capture_output=True, text=True)
        if r.returncode == 0:
            return f
    return None


def exp_flags():
    print("[4] Флаги компилятора")
    os.makedirs(BIN, exist_ok=True)
    configs = [("-O0", ["-O0"]), ("-O1", ["-O1"]), ("-O2", ["-O2"]), ("-O3", ["-O3"])]
    nf = native_flag()
    if nf:
        configs.append(("-O3 " + nf[0], ["-O3", *nf]))
    versions = [("v0_baseline", "v0 полный перебор"),
                ("v3_order", "v3b порядок букв"),
                ("v6_bitmask", "v6 итоговая")]
    tests = standard_tests()
    reps = 2 if QUICK else 5
    min_ms = 20 if QUICK else 50
    result, rows = {}, []
    for v, _ in versions:
        for name, flags in configs:
            exe = build(v, flags, name.replace(" ", "").replace("=", "").replace("-", ""))
            totals = [sum(r[1] for r in run(exe, tests, min_ms)) for _ in range(reps)]
            result[(v, name)] = statistics.median(totals)
            rows.append((v, name, result[(v, name)]))
            print(f"    {SHORT[v]:>4} {name:<22} {result[(v, name)]:.4f} мс")
    write_csv("flags.csv", ["version", "flags", "median_total_ms"], rows)

    plt, NullF = plt_setup()
    fig, ax = plt.subplots(figsize=(9, 4.8))
    nv, nc = len(versions), len(configs)
    w = 0.8 / nv
    for i, (v, label) in enumerate(versions):
        xs = [c + (i - (nv - 1) / 2) * w for c in range(nc)]
        ys = [result[(v, name)] for name, _ in configs]
        ax.bar(xs, ys, width=w * 0.92, color=CAT[i], label=label)
        base = result[(v, "-O0")]
        for xx, yy in zip(xs, ys):
            ax.annotate(f"×{base / yy:.1f}", (xx, yy), xytext=(0, 3), textcoords="offset points",
                        ha="center", fontsize=6.5, color=INK)
    ax.set_yscale("log")
    ax.yaxis.set_minor_formatter(NullF())
    ax.set_xticks(range(nc))
    ax.set_xticklabels([name for name, _ in configs])
    ax.set_ylabel("мс на все 15 тестов (лог. шкала)", color=MUTED, fontsize=9)
    title(ax, "Влияние флагов компилятора (подписи — ускорение относительно -O0)")
    style(ax)
    legend(ax, loc="upper center", bbox_to_anchor=(0.5, -0.08), ncol=3)
    save(fig, "flags.png")
    plt.close(fig)


# >>>> main 0_0 <<<<

def main():
    chosen = {a for a in sys.argv[1:] if a.isdigit()} or {"1", "2", "3", "4"}
    os.makedirs(OUT, exist_ok=True)
    try:
        import matplotlib
    except ImportError:
        sys.exit("Нужен matplotlib: pip install matplotlib")
    print(f"Компилятор: {compiler()}" + ("   (быстрый режим)" if QUICK else ""))
    exes = build_all() if chosen & {"1", "2", "3"} else {}
    t0 = time.time()
    if "1" in chosen:
        exp_complexity(exes)
    if "2" in chosen:
        exp_random(exes)
    if "3" in chosen:
        exp_repeats(exes)
    if "4" in chosen:
        exp_flags()
    print(f"Готово за {time.time() - t0:.0f} с. Данные: experiments/, графики: plots/")


if __name__ == "__main__":
    main()
