#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

struct BenchmarkResult {
    std::string stackName;
    int threadCount;
    int operationsPerThread;

    long long totalAttempts;
    long long pushCount;
    long long popSuccessCount;
    long long popFailCount;
    long long finalDrainCount;
    long long expectedFinalCount;

    double elapsedMs;
    double throughputOpsPerSec;
    double averageLatencyNs;
    double cpuUsagePercent;

    long long retryCount;
    bool correct;
};

inline std::string boolToText(bool value) {
    return value ? "true" : "false";
}

template <typename StackType>
BenchmarkResult runMixedBenchmark(
    const std::string& stackName,
    int threadCount,
    int operationsPerThread) {
    
    StackType stack;

    const long long totalAttempts =
        static_cast<long long>(threadCount) * operationsPerThread;

    const long long prefillCount = totalAttempts / 2;

    for (long long i = 0; i < prefillCount; ++i) {
        stack.push(static_cast<int>(-i));
    }

    stack.resetRetryCount();

    std::atomic<long long> pushCount(0);
    std::atomic<long long> popSuccessCount(0);
    std::atomic<long long> popFailCount(0);

    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    std::clock_t cpuStart = std::clock();
    auto wallStart = std::chrono::high_resolution_clock::now();

    for (int t = 0; t < threadCount; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operationsPerThread; ++i) {
                if (((i + t) % 2) == 0) {
                    int value = t * operationsPerThread + i;
                    stack.push(value);
                    pushCount.fetch_add(1, std::memory_order_relaxed);
                } else {
                    int value = 0;

                    if (stack.pop(value)) {
                        popSuccessCount.fetch_add(
                            1,
                            std::memory_order_relaxed);
                    } else {
                        popFailCount.fetch_add(
                            1,
                            std::memory_order_relaxed);
                    }
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto wallEnd = std::chrono::high_resolution_clock::now();
    std::clock_t cpuEnd = std::clock();

    std::chrono::duration<double> elapsedSeconds = wallEnd - wallStart;
    double elapsedMs = elapsedSeconds.count() * 1000.0;

    long long retryCount = stack.getRetryCount();

    long long finalDrainCount = 0;
    int value = 0;

    while (stack.pop(value)) {
        ++finalDrainCount;
    }

    long long expectedFinalCount =
        prefillCount +
        pushCount.load(std::memory_order_relaxed) -
        popSuccessCount.load(std::memory_order_relaxed);

    bool correct = (finalDrainCount == expectedFinalCount);

    double throughput =
        static_cast<double>(totalAttempts) / elapsedSeconds.count();

    double averageLatencyNs =
        elapsedSeconds.count() * 1'000'000'000.0 /
        static_cast<double>(totalAttempts);

    double cpuSeconds =
        static_cast<double>(cpuEnd - cpuStart) / CLOCKS_PER_SEC;

    unsigned int hardwareThreads = std::thread::hardware_concurrency();

    if (hardwareThreads == 0) {
        hardwareThreads = 1;
    }

    double cpuUsagePercent =
        cpuSeconds /
        (elapsedSeconds.count() * hardwareThreads) *
        100.0;

    BenchmarkResult result;

    result.stackName = stackName;
    result.threadCount = threadCount;
    result.operationsPerThread = operationsPerThread;

    result.totalAttempts = totalAttempts;
    result.pushCount = pushCount.load(std::memory_order_relaxed);
    result.popSuccessCount = popSuccessCount.load(std::memory_order_relaxed);
    result.popFailCount = popFailCount.load(std::memory_order_relaxed);
    result.finalDrainCount = finalDrainCount;
    result.expectedFinalCount = expectedFinalCount;

    result.elapsedMs = elapsedMs;
    result.throughputOpsPerSec = throughput;
    result.averageLatencyNs = averageLatencyNs;
    result.cpuUsagePercent = cpuUsagePercent;

    result.retryCount = retryCount;
    result.correct = correct;

    return result;
}

inline void printHeader() {
    std::cout << std::left
              << std::setw(15) << "Stack"
              << std::setw(10) << "Threads"
              << std::setw(15) << "Operations"
              << std::setw(15) << "Time(ms)"
              << std::setw(18) << "Throughput"
              << std::setw(18) << "Latency(ns)"
              << std::setw(15) << "Retries"
              << std::setw(12) << "Correct"
              << '\n';

    std::cout << std::string(118, '-') << '\n';
}

inline void printResult(const BenchmarkResult& result) {
    std::cout << std::left
              << std::setw(15) << result.stackName
              << std::setw(10) << result.threadCount
              << std::setw(15) << result.totalAttempts
              << std::setw(15) << std::fixed << std::setprecision(2)
              << result.elapsedMs
              << std::setw(18) << std::fixed << std::setprecision(2)
              << result.throughputOpsPerSec
              << std::setw(18) << std::fixed << std::setprecision(2)
              << result.averageLatencyNs
              << std::setw(15) << result.retryCount
              << std::setw(12) << boolToText(result.correct)
              << '\n';
}

inline void writeResultsToCsv(
    const std::string& filename,
    const std::vector<BenchmarkResult>& results) {
    
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open CSV file: " << filename << '\n';
        return;
    }

    file << "Stack,"
         << "Threads,"
         << "OperationsPerThread,"
         << "TotalAttempts,"
         << "PushCount,"
         << "PopSuccessCount,"
         << "PopFailCount,"
         << "ExpectedFinalCount,"
         << "FinalDrainCount,"
         << "ElapsedMs,"
         << "ThroughputOpsPerSec,"
         << "AverageLatencyNs,"
         << "CpuUsagePercent,"
         << "RetryCount,"
         << "Correct\n";

    for (const auto& result : results) {
        file << result.stackName << ','
             << result.threadCount << ','
             << result.operationsPerThread << ','
             << result.totalAttempts << ','
             << result.pushCount << ','
             << result.popSuccessCount << ','
             << result.popFailCount << ','
             << result.expectedFinalCount << ','
             << result.finalDrainCount << ','
             << result.elapsedMs << ','
             << result.throughputOpsPerSec << ','
             << result.averageLatencyNs << ','
             << result.cpuUsagePercent << ','
             << result.retryCount << ','
             << boolToText(result.correct) << '\n';
    }

    file.close();
}

#endif