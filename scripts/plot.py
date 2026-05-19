import csv
import os
from collections import defaultdict

import matplotlib.pyplot as plt


RESULT_FILE = os.path.join("results", "results.csv")
CHART_DIR = "charts"


def read_results(filename):
    rows = []

    with open(filename, "r", encoding="utf-8") as file:
        reader = csv.DictReader(file)

        for row in reader:
            row["Threads"] = int(row["Threads"])
            row["ThroughputOpsPerSec"] = float(row["ThroughputOpsPerSec"])
            row["AverageLatencyNs"] = float(row["AverageLatencyNs"])
            row["RetryCount"] = int(row["RetryCount"])
            row["CpuUsagePercent"] = float(row["CpuUsagePercent"])
            rows.append(row)

    return rows


def group_by_stack(rows):
    grouped = defaultdict(list)

    for row in rows:
        grouped[row["Stack"]].append(row)

    for stack_name in grouped:
        grouped[stack_name].sort(key=lambda item: item["Threads"])

    return grouped


def plot_metric(grouped, metric, ylabel, title, output_filename):
    plt.figure()

    for stack_name, rows in grouped.items():
        threads = [row["Threads"] for row in rows]
        values = [row[metric] for row in rows]

        plt.plot(threads, values, marker="o", label=stack_name)

    plt.xlabel("Thread Count")
    plt.ylabel(ylabel)
    plt.title(title)
    plt.legend()
    plt.grid(True)

    output_path = os.path.join(CHART_DIR, output_filename)
    plt.savefig(output_path, dpi=300, bbox_inches="tight")
    plt.close()

    print(f"Saved chart: {output_path}")


def plot_retry_count(grouped):
    if "LockFreeStack" not in grouped:
        print("No LockFreeStack data found.")
        return

    plt.figure()

    rows = grouped["LockFreeStack"]
    threads = [row["Threads"] for row in rows]
    retries = [row["RetryCount"] for row in rows]

    plt.bar(threads, retries)

    plt.xlabel("Thread Count")
    plt.ylabel("CAS Retry Count")
    plt.title("CAS Retry Count of Lock-Free Stack")
    plt.grid(True)

    output_path = os.path.join(CHART_DIR, "retry_count.png")
    plt.savefig(output_path, dpi=300, bbox_inches="tight")
    plt.close()

    print(f"Saved chart: {output_path}")


def main():
    if not os.path.exists(RESULT_FILE):
        print("Cannot find results/results.csv")
        print("Please run the C++ benchmark first.")
        return

    os.makedirs(CHART_DIR, exist_ok=True)

    rows = read_results(RESULT_FILE)
    grouped = group_by_stack(rows)

    plot_metric(
        grouped,
        "ThroughputOpsPerSec",
        "Operations per Second",
        "Throughput Comparison",
        "throughput.png",
    )

    plot_metric(
        grouped,
        "AverageLatencyNs",
        "Average Latency(ns)",
        "Average Latency Comparison",
        "latency.png",
    )

    plot_metric(
        grouped,
        "CpuUsagePercent",
        "CPU Usage(%)",
        "CPU Usage Comparison",
        "cpu_usage.png",
    )

    plot_retry_count(grouped)


if __name__ == "__main__":
    main()