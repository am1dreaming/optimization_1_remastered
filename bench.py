import csv
import math
import os
import subprocess
import sys

VERSIONS = [
    ("v0_baseline", "v0\nперебор"),
    ("v1_coeffs", "v1\nкоэфф."),
    ("v2_incremental", "v2\nинкрем."),
    ("v3_bounds", "v3a\nграницы"),
    ("v3_order", "v3b\nпорядок"),
    ("v4_columns", "v4\nстолбцы"),
    ("v5_forced", "v5\nвычисл."),
    ("v6_bitmask", "v6\nмаска"),
]

COLOR = "#2a78d6"
INK, MUTED, GRID = "#1f1f1e", "#6b6a63", "#e4e3dc"


def find_exe(build, name):
    exe = name + (".exe" if os.name == "nt" else "")
    for sub in ("", "Release", "RelWithDebInfo", "Debug"):
        p = os.path.join(build, sub, exe)
        if os.path.isfile(p):
            return p
    return None


def run(build):
    rows = []
    for name, _ in VERSIONS:
        exe = find_exe(build, name)
        if not exe:
            print(f"[!] не найден {name} в {build}, пропускаю")
            continue
        print(f"[*] {name} ...", flush=True)
        out = subprocess.run([exe, "-b"], capture_output=True, text=True,
                             encoding="utf-8").stdout
        for line in out.strip().splitlines():
            puzzle, ms, nodes, status = line.split("\t")
            rows.append({"version": name, "puzzle": puzzle, "ms": float(ms),
                         "nodes": int(nodes), "status": status})
    return rows


def save_csv(rows, path):
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["version", "puzzle", "ms", "nodes", "status"])
        w.writeheader()
        w.writerows(rows)


def style(ax):
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(GRID)
    ax.tick_params(colors=MUTED, labelsize=8)
    ax.grid(axis="y", color=GRID, linewidth=0.6)
    ax.set_axisbelow(True)


def bar_chart(plt, names, labels, values, title, ylabel, fmt, path):
    fig, ax = plt.subplots(figsize=(9, 4.5))
    x = range(len(names))
    bars = ax.bar(x, values, width=0.6, color=COLOR)
    ax.set_yscale("log")
    ax.set_xticks(list(x))
    ax.set_xticklabels(labels)
    ax.set_ylabel(ylabel, color=MUTED, fontsize=9)
    ax.set_title(title, color=INK, fontsize=11, loc="left")
    style(ax)
    for b, v in zip(bars, values):
        ax.annotate(fmt(v), (b.get_x() + b.get_width() / 2, v),
                    xytext=(0, 3), textcoords="offset points",
                    ha="center", fontsize=7, color=INK)
    fig.tight_layout()
    fig.savefig(path, dpi=160)
    plt.close(fig)


def plot(rows, outdir):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.ticker import NullFormatter

    os.makedirs(outdir, exist_ok=True)
    names = [n for n, _ in VERSIONS if any(r["version"] == n for r in rows)]
    labels = [l for n, l in VERSIONS if n in names]
    puzzles = list(dict.fromkeys(r["puzzle"] for r in rows))
    get = {(r["version"], r["puzzle"]): r for r in rows}

    total_ms = [sum(get[(n, p)]["ms"] for p in puzzles) for n in names]
    total_nodes = [sum(get[(n, p)]["nodes"] for p in puzzles) for n in names]

    bar_chart(plt, names, labels, total_ms,
              "Суммарное время решения всех тестов", "мс (лог. шкала)",
              lambda v: f"{v:.3g}", os.path.join(outdir, "time_total.png"))
    bar_chart(plt, names, labels, total_nodes,
              "Суммарное число узлов перебора (вызовов search)", "узлы (лог. шкала)",
              lambda v: f"{v:,.0f}".replace(",", " "), os.path.join(outdir, "nodes_total.png"))

    if names[0] == "v0_baseline":
        speedup = [math.exp(sum(math.log(get[("v0_baseline", p)]["ms"] / get[(n, p)]["ms"])
                                for p in puzzles) / len(puzzles)) for n in names]
        bar_chart(plt, names, labels, speedup,
                  "Ускорение относительно v0 (среднее геометрическое по тестам)", "раз (лог. шкала)",
                  lambda v: f"×{v:.3g}", os.path.join(outdir, "speedup.png"))

    cols = 3
    rws = math.ceil(len(puzzles) / cols)
    fig, axes = plt.subplots(rws, cols, figsize=(12, 2.6 * rws), squeeze=False)
    short = [l.split("\n")[0] for l in labels]
    for ax, p in zip(axes.flat, puzzles):
        ys = [get[(n, p)]["ms"] for n in names]
        ax.plot(range(len(names)), ys, color=COLOR, linewidth=2, marker="o", markersize=4)
        ax.set_yscale("log")
        ax.yaxis.set_minor_formatter(NullFormatter())
        ax.set_xticks(range(len(names)))
        ax.set_xticklabels(short)
        ax.set_title(p if len(p) <= 42 else p[:40] + "…", fontsize=8, color=INK, loc="left")
        style(ax)
    for ax in list(axes.flat)[len(puzzles):]:
        ax.axis("off")
    fig.suptitle("Время решения по тестам, мс (лог. шкала)", x=0.01, ha="left",
                 color=INK, fontsize=11)
    fig.tight_layout()
    fig.savefig(os.path.join(outdir, "time_per_test.png"), dpi=160)
    plt.close(fig)


def main():
    build = sys.argv[1] if len(sys.argv) > 1 else "build"
    rows = run(build)
    if not rows:
        sys.exit("Нет результатов: сначала соберите проект (см. README).")
    bad = [r for r in rows if r["status"] != "OK"]
    if bad:
        print("[!] проверка не пройдена:", *(f"{r['version']}: {r['puzzle']}" for r in bad), sep="\n    ")
    save_csv(rows, "results.csv")
    print("[+] results.csv")
    try:
        plot(rows, "plots")
        print("[+] plots/*.png")
    except ImportError:
        print("[!] matplotlib не установлен: pip install matplotlib")


if __name__ == "__main__":
    main()
