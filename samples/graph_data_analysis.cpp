#include "athread/athread.h"
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <unordered_map>
#include <numeric>
#include <functional>
#include <chrono>

using namespace at;
using namespace std;

// Data analysis workflow that demonstrates multiple dependency patterns
int main() {
    // Create a thread graph with automatic thread count optimization
    ThreadGraph graph(4, true);
    
    // Data structures for our analysis
    vector<vector<double>> rawData;
    vector<vector<double>> cleanedData;
    vector<double> aggregatedResults;
    unordered_map<string, double> analysisResults;
    vector<string> reportSections;
    bool reportComplete = false;
    
    cout << "Data Analysis Workflow Example" << endl;
    cout << "-----------------------------" << endl;
    
    // Step 1: Configuration and initialization
    auto configTask = graph.push([&]() {
        cout << "Initializing analysis configuration..." << endl;
        // Simulate configuration loading
        this_thread::sleep_for(chrono::milliseconds(200));
        
        // Initialize with 4 datasets
        rawData.resize(4);
        cleanedData.resize(4);
        cout << "Configuration initialized for 4 datasets" << endl;
    });
    
    // Step 2: Data loading (depends on configuration)
    vector<Task> loadTasks;
    for (int i = 0; i < 4; i++) {
        auto loadTask = graph.push([i, &rawData]() {
            cout << "Loading dataset " << i << endl;
            
            // Generate random data
            random_device rd;
            mt19937 gen(rd());
            normal_distribution<> dist(100.0 + i*10, 20.0);
            
            // Create dataset with 1000 points
            rawData[i].resize(1000);
            for (auto& val : rawData[i]) {
                val = dist(gen);
            }
            
            this_thread::sleep_for(chrono::milliseconds(300)); // Simulate loading time
            cout << "Dataset " << i << " loaded with " << rawData[i].size() << " data points" << endl;
        });
        
        // Loading depends on configuration
        loadTask.depend(configTask);
        loadTasks.push_back(loadTask);
    }
    
    // Step 3: Data cleaning (depends on loading)
    vector<Task> cleaningTasks;
    for (int i = 0; i < 4; i++) {
        auto cleanTask = graph.push([i, &rawData, &cleanedData]() {
            cout << "Cleaning dataset " << i << endl;
            
            // Copy valid data points (simulating data cleaning)
            cleanedData[i].reserve(rawData[i].size());
            for (const auto& val : rawData[i]) {
                // Filter out values based on some criteria
                if (val >= 50.0 && val <= 200.0) {
                    cleanedData[i].push_back(val);
                }
            }
            
            this_thread::sleep_for(chrono::milliseconds(250)); // Simulate cleaning time
            cout << "Dataset " << i << " cleaned: " << cleanedData[i].size() 
                 << " valid points out of " << rawData[i].size() << endl;
        });
        
        // Cleaning depends on loading
        cleanTask.depend(loadTasks[i]);
        cleaningTasks.push_back(cleanTask);
    }
    
    // Step 4: Data aggregation (depends on all cleaning tasks)
    auto aggregateTask = graph.push([&cleanedData, &aggregatedResults]() {
        cout << "Aggregating data from all datasets..." << endl;
        
        // Merge all cleaned datasets
        for (const auto& dataset : cleanedData) {
            aggregatedResults.insert(aggregatedResults.end(), dataset.begin(), dataset.end());
        }
        
        this_thread::sleep_for(chrono::milliseconds(350)); // Simulate aggregation time
        cout << "Data aggregated: " << aggregatedResults.size() << " total data points" << endl;
    });
    
    // Aggregation depends on all cleaning tasks
    for (const auto& task : cleaningTasks) {
        aggregateTask.depend(task);
    }
    
    // Step 5: Parallel analysis tasks (depend on aggregation)
    // These analysis tasks can run in parallel after aggregation
    
    // 5.1 - Statistical analysis
    auto statsTask = graph.push([&aggregatedResults, &analysisResults]() {
        cout << "Performing statistical analysis..." << endl;
        
        // Calculate basic statistics
        double sum = accumulate(aggregatedResults.begin(), aggregatedResults.end(), 0.0);
        double mean = sum / aggregatedResults.size();
        
        double sq_sum = inner_product(
            aggregatedResults.begin(), aggregatedResults.end(),
            aggregatedResults.begin(), 0.0,
            std::plus<>(), [mean](double a, double b) { return (a - mean) * (b - mean); }
        );
        double stddev = sqrt(sq_sum / aggregatedResults.size());
        
        // Store results
        analysisResults["mean"] = mean;
        analysisResults["stddev"] = stddev;
        
        this_thread::sleep_for(chrono::milliseconds(400)); // Simulate analysis time
        cout << "Statistical analysis complete: mean = " << mean << ", stddev = " << stddev << endl;
    });
    statsTask.depend(aggregateTask);
    
    // 5.2 - Outlier detection
    auto outlierTask = graph.push([&aggregatedResults, &analysisResults]() {
        cout << "Detecting outliers..." << endl;
        
        // Simple outlier detection (values more than 2 standard deviations from mean)
        double sum = accumulate(aggregatedResults.begin(), aggregatedResults.end(), 0.0);
        double mean = sum / aggregatedResults.size();
        
        double sq_sum = inner_product(
            aggregatedResults.begin(), aggregatedResults.end(),
            aggregatedResults.begin(), 0.0,
            std::plus<>(), [mean](double a, double b) { return (a - mean) * (b - mean); }
        );
        double stddev = sqrt(sq_sum / aggregatedResults.size());
        
        int outliers = 0;
        for (const auto& val : aggregatedResults) {
            if (abs(val - mean) > 2 * stddev) {
                outliers++;
            }
        }
        
        analysisResults["outliers"] = outliers;
        
        this_thread::sleep_for(chrono::milliseconds(350)); // Simulate analysis time
        cout << "Outlier analysis complete: found " << outliers << " outliers" << endl;
    });
    outlierTask.depend(aggregateTask);
    
    // 5.3 - Trend analysis
    auto trendTask = graph.push([&aggregatedResults, &analysisResults]() {
        cout << "Analyzing trends in data..." << endl;
        
        // Simple trend analysis (simulated)
        this_thread::sleep_for(chrono::milliseconds(450)); // Simulate analysis time
        
        // Simulated trend detection
        analysisResults["trend_slope"] = 0.05;
        analysisResults["trend_confidence"] = 0.92;
        
        cout << "Trend analysis complete: detected upward trend with 92% confidence" << endl;
    });
    trendTask.depend(aggregateTask);
    
    // Step 6: Report generation (depends on all analysis tasks)
    // These report section tasks can run in parallel after their respective analysis completes
    
    // 6.1 - Statistics report section
    auto statsReportTask = graph.push([&analysisResults, &reportSections]() {
        cout << "Generating statistics report section..." << endl;
        this_thread::sleep_for(chrono::milliseconds(250)); // Simulate report generation
        
        string report = "STATISTICAL SUMMARY\n"
                        "-------------------\n"
                        "Mean value: " + to_string(analysisResults["mean"]) + "\n"
                        "Standard deviation: " + to_string(analysisResults["stddev"]) + "\n";
        
        reportSections.push_back(report);
        cout << "Statistics report section complete" << endl;
    });
    statsReportTask.depend(statsTask);
    
    // 6.2 - Outliers report section
    auto outlierReportTask = graph.push([&analysisResults, &reportSections]() {
        cout << "Generating outliers report section..." << endl;
        this_thread::sleep_for(chrono::milliseconds(200)); // Simulate report generation
        
        string report = "OUTLIER ANALYSIS\n"
                        "----------------\n"
                        "Detected outliers: " + to_string(int(analysisResults["outliers"])) + "\n"
                        "Outlier ratio: " + to_string(analysisResults["outliers"] / 4000.0 * 100) + "%\n";
        
        reportSections.push_back(report);
        cout << "Outliers report section complete" << endl;
    });
    outlierReportTask.depend(outlierTask);
    
    // 6.3 - Trends report section
    auto trendReportTask = graph.push([&analysisResults, &reportSections]() {
        cout << "Generating trends report section..." << endl;
        this_thread::sleep_for(chrono::milliseconds(230)); // Simulate report generation
        
        string report = "TREND ANALYSIS\n"
                        "--------------\n"
                        "Trend slope: " + to_string(analysisResults["trend_slope"]) + "\n"
                        "Confidence level: " + to_string(analysisResults["trend_confidence"] * 100) + "%\n";
        
        reportSections.push_back(report);
        cout << "Trends report section complete" << endl;
    });
    trendReportTask.depend(trendTask);
    
    // Step 7: Final report assembly (depends on all report sections)
    auto finalReportTask = graph.push([&reportSections, &reportComplete]() {
        cout << "Assembling final report..." << endl;
        this_thread::sleep_for(chrono::milliseconds(300)); // Simulate report assembly
        
        cout << "\n===== FINAL ANALYSIS REPORT =====\n\n";
        for (const auto& section : reportSections) {
            cout << section << endl;
        }
        cout << "===============================" << endl;
        
        reportComplete = true;
        cout << "Final report assembly complete!" << endl;
    });
    
    // Final report depends on all report sections
    finalReportTask.depend(statsReportTask);
    finalReportTask.depend(outlierReportTask);
    finalReportTask.depend(trendReportTask);
    
    // Execute the workflow
    cout << "\nStarting data analysis workflow...\n" << endl;
    auto startTime = chrono::high_resolution_clock::now();
    
    graph.start();
    graph.wait();
    
    auto endTime = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    
    cout << "\nData analysis workflow completed in " << duration.count() << "ms" << endl;
    cout << "Report status: " << (reportComplete ? "Complete" : "Incomplete") << endl;
    
    return 0;
}
