#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Benchmark.h"
#include "LockFreeStack.h"
#include "MutexStack.h"

int main(int argc, char* argv[]) {
    try {
        int operationsPerThread = 100000;

        if (argc >= 2) {
            operationsPerThread = std::stoi(argv[1]);
        }

        std::vector<int> threadCounts = {2, 4, 8, 16};
        std::vector<BenchmarkResult> results;

        std::filesystem::create_directories("results");
        std::filesystem::create_directories("charts");

        std::cout << "CAS Lock-Free Stack Benchmark\n";
        std::cout << "Operations per thread: "
                  << operationsPerThread << "\n\n";

        printHeader();

        for (int threadCount : threadCounts) {
            BenchmarkResult mutexResult =
                runMixedBenchmark<MutexStack>(
                    "MutexStack",
                    threadCount,
                    operationsPerThread);

            results.push_back(mutexResult);
            printResult(mutexResult);

            BenchmarkResult lockFreeResult =
                runMixedBenchmark<LockFreeStack>(
                    "LockFreeStack",
                    threadCount,
                    operationsPerThread);

            results.push_back(lockFreeResult);
            printResult(lockFreeResult);
        }

        writeResultsToCsv("results/results.csv", results);

        std::cout << "\nCSV result saved to: results/results.csv\n";
        std::cout << "Run this command to generate charts:\n";
        std::cout << "python3 scripts/plot.py\n";

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}