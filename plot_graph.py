import subprocess
import matplotlib.pyplot as plt
from datetime import datetime
from collections import defaultdict

def get_git_data():
    cmd = [
        "git",
        "log",
        "--pretty=format:%ad",
        "--date=short",
        "--numstat"
    ]

    result = subprocess.run(cmd, capture_output=True, text=True)
    lines = result.stdout.split("\n")

    data = defaultdict(lambda: {"added": 0, "removed": 0})

    current_date = None

    for line in lines:
        if not line.strip():
            continue

        # linha de data
        try:
            datetime.strptime(line.strip(), "%Y-%m-%d")
            current_date = line.strip()
            continue
        except ValueError:
            pass

        # linha de numstat
        parts = line.split()
        if len(parts) >= 2 and parts[0].isdigit() and parts[1].isdigit():
            added = int(parts[0])
            removed = int(parts[1])

            data[current_date]["added"] += added
            data[current_date]["removed"] += removed

    return data


def plot_data(data):
    dates = sorted(data.keys())
    dates_dt = [datetime.strptime(d, "%Y-%m-%d") for d in dates]

    added = [data[d]["added"] for d in dates]
    removed = [data[d]["removed"] for d in dates]
    net = [a - r for a, r in zip(added, removed)]

    plt.figure(figsize=(12, 6))

    plt.plot(dates_dt, added, label="Linhas adicionadas")
    plt.plot(dates_dt, removed, label="Linhas removidas")
    plt.plot(dates_dt, net, linestyle="--", label="Saldo (added - removed)")

    plt.title("Evolução de Linhas no Repositório")
    plt.xlabel("Data")
    plt.ylabel("Linhas")
    plt.legend()
    plt.grid(True)

    plt.tight_layout()

    plt.savefig("git_lines.png")
    plt.savefig("git_lines.pdf")

    print("Gráficos salvos como git_lines.png e git_lines.pdf")


if __name__ == "__main__":
    data = get_git_data()
    plot_data(data)

