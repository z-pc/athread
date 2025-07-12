#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "athread/athread.h"

using namespace at;
using namespace std;

// Document processing pipeline that simulates loading, processing and storing documents
int main()
{
    // Document data structure
    struct Document
    {
        string content;
        bool isProcessed = false;
        unordered_map<string, int> wordFrequency;
        vector<string> keywords;
    };

    vector<shared_ptr<Document>> documents(3);
    for (int i = 0; i < 3; i++)
    {
        documents[i] = make_shared<Document>();
    }

    // Create thread graph with 4 worker threads
    ThreadGraph graph(4);

    // Step 1: Load documents (can run in parallel)
    vector<Task> loadTasks;
    for (int i = 0; i < documents.size(); i++)
    {
        auto loadTask = graph.push(
            [i, &documents]()
            {
                AT_COUT("Loading document " << i << endl);
                documents[i]->content = "This is content of document " + to_string(i) +
                                        " containing some example text for processing" +
                                        " and some more text to analyze word frequency. This text is just a sample.";
                this_thread::sleep_for(chrono::milliseconds(300));  // Simulate I/O delay
                AT_COUT("Document " << i << " loaded successfully" << endl);
            });
        loadTasks.push_back(loadTask);
    }

    // Step 2: Process documents (depends on loading)
    vector<Task> processTasks;
    for (int i = 0; i < documents.size(); i++)
    {
        auto processTask = graph.push(
            [i, &documents]()
            {
                AT_COUT("Processing document " << i << endl);

                // Analyze word frequency
                string& content = documents[i]->content;
                string word;
                size_t pos = 0;
                while ((pos = content.find(" ", pos)) != string::npos)
                {
                    word = content.substr(0, pos);
                    documents[i]->wordFrequency[word]++;
                    content = content.substr(pos + 1);
                    pos = 0;
                }
                if (!content.empty())
                {
                    documents[i]->wordFrequency[content]++;
                }

                documents[i]->isProcessed = true;
                this_thread::sleep_for(chrono::milliseconds(500));  // Simulate processing time
                AT_COUT("Document " << i << " processed successfully" << endl);
            });

        // Processing depends on loading
        processTask.depend(loadTasks[i]);
        processTasks.push_back(processTask);
    }

    // Step 3: Extract keywords (depends on processing)
    vector<Task> keywordTasks;
    for (int i = 0; i < documents.size(); i++)
    {
        auto keywordTask = graph.push(
            [i, &documents]()
            {
                AT_COUT("Extracting keywords from document " << i << endl);

                // Extract top 3 frequent words as keywords
                auto& freq = documents[i]->wordFrequency;
                vector<pair<string, int>> sortedWords(freq.begin(), freq.end());
                std::sort(sortedWords.begin(), sortedWords.end(),
                          [](const auto& a, const auto& b) { return a.second > b.second; });

                for (int j = 0; j < min(3, (int)sortedWords.size()); j++)
                {
                    documents[i]->keywords.push_back(sortedWords[j].first);
                }

                this_thread::sleep_for(chrono::milliseconds(200));  // Simulate processing time
                AT_COUT("Keywords extracted from document " << i << endl);
            });

        // Keyword extraction depends on processing
        keywordTask.depend(processTasks[i]);
        keywordTasks.push_back(keywordTask);
    }

    // Step 4: Generate report (depends on all keyword extractions)
    auto reportTask = graph.push(
        [&documents]()
        {
            AT_COUT("Generating final report..." << endl);
            AT_COUT("Document Processing Summary:" << endl);

            for (int i = 0; i < documents.size(); i++)
            {
                AT_COUT("Document " << i << " keywords: ");
                for (const auto& keyword : documents[i]->keywords)
                {
                    AT_COUT(keyword << " ");
                }
                AT_COUT("" << endl);
            }

            this_thread::sleep_for(chrono::milliseconds(400));  // Simulate report generation
            AT_COUT("Report generated successfully!" << endl);
        });

    // Report depends on all keyword extractions
    for (const auto& task : keywordTasks)
    {
        reportTask.depend(task);
    }

    // Start execution and wait for completion
    AT_COUT("Starting document processing pipeline..." << endl);
    graph.start();
    graph.wait();
    AT_COUT("Document processing pipeline completed" << endl);

    return 0;
}
