#!/usr/bin/env python3
"""Plot gradient scaling and penalty history from gradientsOpt.log.
"""

from __future__ import annotations

import argparse
import textwrap
from pathlib import Path
from typing import List, Sequence, Tuple

EXCLUDED_PLOT_COLUMNS = {"PD0Sug", "Mode"}

# Define groups for 2-column layout
COLUMN_GROUPS = {
    "left": ["avgObj", "avgVol", "avgPow", "avgMaxF"],
    "right": ["V", "PVal", "VEroded", "lsmL", "lsmLP", "lsmStep"]
}


def parse_history(path: Path) -> Tuple[List[str], List[List[float]]]:
    """Parse gradientsOpt.log, handling comments and repeated headers."""
    if not path.exists():
        return [], []

    header: List[str] = []
    rows_by_iter = {}

    with path.open("r", encoding="utf-8", errors="ignore") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue

            parts = line.split()
            if not parts:
                continue

            if parts[0] == "Iter":
                # If schema changes inside one file, keep only the latest schema block.
                if header and header != parts:
                    rows_by_iter.clear()
                header = parts
                continue

            if not header:
                # Defer parsing until header appears.
                continue

            expected = len(header)
            if len(parts) < expected:
                # Likely an in-progress write; skip partial line.
                continue
            if len(parts) > expected:
                parts = parts[:expected]

            try:
                it = int(float(parts[0]))
                # Collect float values for data columns, using placeholder for text cols like 'Mode'
                row = [float(it)]
                for idx in range(1, expected):
                    try:
                        row.append(float(parts[idx]))
                    except ValueError:
                        # Non-float column (e.g. 'Mode'); push 0.0 placeholder
                        row.append(0.0)
            except ValueError:
                continue

            rows_by_iter[it] = row

    if not header or not rows_by_iter:
        return header, []

    ordered = [rows_by_iter[k] for k in sorted(rows_by_iter)]
    return header, ordered


def parse_column_selection(header: Sequence[str], columns_arg: str | None) -> List[int]:
    if not header:
        return []

    default_indices = [
        idx for idx in range(1, len(header)) if header[idx] not in EXCLUDED_PLOT_COLUMNS
    ]
    if not columns_arg:
        return default_indices

    wanted = [c.strip() for c in columns_arg.split(",") if c.strip()]
    if not wanted or wanted == ["all"]:
        return default_indices

    name_to_idx = {name: idx for idx, name in enumerate(header)}
    indices: List[int] = []
    for name in wanted:
        if name not in name_to_idx:
            raise ValueError(
                f"Unknown column '{name}'. Available: {', '.join(header[1:])}"
            )
        if name == "Iter":
            continue
        if name in EXCLUDED_PLOT_COLUMNS:
            continue
        indices.append(name_to_idx[name])

    if not indices:
        raise ValueError("No plottable columns selected.")

    return sorted(set(indices))


def build_figure(left_indices: List[int], right_indices: List[int]):
    import matplotlib.pyplot as plt

    n_rows = max(len(left_indices), len(right_indices))
    if n_rows == 0:
        return None, None

    fig, axes = plt.subplots(
        n_rows, 2, 
        sharex=True, 
        figsize=(16, 3.0 * n_rows),
        squeeze=False
    )
    return fig, axes


def render(
    fig,
    axes,
    header: Sequence[str],
    rows: Sequence[Sequence[float]],
    left_indices: List[int],
    right_indices: List[int],
    src_path: Path,
) -> None:
    import matplotlib.pyplot as plt

    if not rows:
        return

    x = [r[0] for r in rows]

    def plot_group(indices: List[int], col_idx: int):
        for row_idx, data_idx in enumerate(indices):
            ax = axes[row_idx, col_idx]
            y = [r[data_idx] for r in rows]
            ax.clear()
            ax.plot(x, y, "-", linewidth=1.6)
            ax.scatter([x[-1]], [y[-1]], s=18)
            ax.set_ylabel(header[data_idx], fontweight="bold")
            ax.grid(True, alpha=0.25)
            if row_idx == len(indices) - 1:
                ax.set_xlabel("Iteration")
        
        # Hide unused axes in this column
        for row_idx in range(len(indices), axes.shape[0]):
            axes[row_idx, col_idx].axis("off")

    plot_group(left_indices, 0)
    plot_group(right_indices, 1)

    fig.suptitle(f"Gradient scaling and penalty history: {src_path}", fontsize=14, y=1.02)
    fig.tight_layout()


def main() -> int:
    epilog = textwrap.dedent(
        """\
        Examples:
          plotGradientsHistory.py
          plotGradientsHistory.py -c avgObj,avgVol,avgPow
          plotGradientsHistory.py --no-show --save solverLogs/gradients_plot.png
          plotGradientsHistory.py -f solverLogs/gradientsOpt.log
        """
    )
    parser = argparse.ArgumentParser(
        description="Plot gradientsOpt.log parameters vs iteration.",
        epilog=epilog,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "-f",
        "--file",
        default="solverLogs/gradientsOpt.log",
        help="Path to gradientsOpt.log (default: solverLogs/gradientsOpt.log)",
    )
    parser.add_argument(
        "-c",
        "--columns",
        default=None,
        help="Comma-separated columns to plot (default: all, excluding Iter, PD0Sug and Mode)",
    )
    parser.add_argument(
        "--save",
        default=None,
        help="Optional output image path, e.g. solverLogs/gradients.png",
    )
    parser.add_argument(
        "--no-show",
        action="store_true",
        help="Run without opening a GUI window.",
    )

    args = parser.parse_args()
    src = Path(args.file)

    if args.no_show:
        import matplotlib
        matplotlib.use("Agg")

    try:
        import matplotlib.pyplot as plt
    except Exception as exc:
        print(f"Failed to import matplotlib: {exc}")
        print("Install matplotlib, e.g. `pip install matplotlib`.")
        return 1

    header, rows = parse_history(src)
    if not header:
        print(f"No header found in {src}.")
        return 1

    try:
        col_indices = parse_column_selection(header, args.columns)
    except ValueError as exc:
        print(str(exc))
        return 1
    
    if not col_indices:
        print("No columns selected to plot.")
        return 1

    if not rows:
        print(f"No data rows parsed from {src}.")
        return 1

    # Group indices based on nature
    left_indices = []
    right_indices = []
    
    name_to_idx = {name: idx for idx, name in enumerate(header)}
    
    # Sort selected indices into groups
    for idx in col_indices:
        name = header[idx]
        if name in COLUMN_GROUPS["left"]:
            left_indices.append(idx)
        elif name in COLUMN_GROUPS["right"]:
            right_indices.append(idx)
        else:
            # Fallback for unexpected columns
            if len(left_indices) <= len(right_indices):
                left_indices.append(idx)
            else:
                right_indices.append(idx)

    fig, axes = build_figure(left_indices, right_indices)
    if fig:
        render(fig, axes, header, rows, left_indices, right_indices, src)
    
    if args.save:
        Path(args.save).parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(args.save, dpi=170, bbox_inches="tight")
        print(f"Saved plot: {args.save}")

    if not args.no_show:
        plt.show()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
