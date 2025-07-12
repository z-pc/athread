#include "athread/athread.h"
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <chrono>

using namespace at;
using namespace std;

// Simple image class for demonstration
class Image {
public:
    Image(int width = 0, int height = 0) : width(width), height(height) {
        if (width > 0 && height > 0) {
            // Initialize with random pixel values
            pixels.resize(width * height, 0);
        }
    }
    
    void resize(int newWidth, int newHeight) {
        width = newWidth;
        height = newHeight;
        pixels.resize(width * height, 0);
        cout << "Image resized to " << width << "x" << height << endl;
    }
    
    void applyFilter(const string& filterName) {
        cout << "Applying " << filterName << " filter to " << width << "x" << height << " image" << endl;
        // Simulate filter application
        this_thread::sleep_for(chrono::milliseconds(300));
        cout << filterName << " filter applied successfully" << endl;
    }
    
    void save(const string& filename) {
        cout << "Saving image to " << filename << endl;
        // Simulate saving file
        this_thread::sleep_for(chrono::milliseconds(500));
        cout << "Image saved as " << filename << endl;
    }
    
private:
    int width;
    int height;
    vector<int> pixels;
};

int main() {
    // Create thread graph with 3 worker threads
    ThreadGraph graph(3);
    
    // Create a set of images to process
    vector<Image> images;
    vector<string> imageNames = {"photo1.jpg", "photo2.jpg", "photo3.jpg"};
    
    // Random size generator
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> widthDist(800, 1200);
    uniform_int_distribution<> heightDist(600, 900);
    
    cout << "Image Processing Pipeline Example" << endl;
    cout << "--------------------------------" << endl;
    
    // Step 1: Load images (can be done in parallel)
    vector<Task> loadTasks;
    for (int i = 0; i < imageNames.size(); i++) {
        images.emplace_back();
        auto loadTask = graph.push([i, &images, &imageNames, &widthDist, &heightDist, &gen]() {
            cout << "Loading image: " << imageNames[i] << endl;
            // Simulate loading image with random dimensions
            int width = widthDist(gen);
            int height = heightDist(gen);
            images[i] = Image(width, height);
            this_thread::sleep_for(chrono::milliseconds(300)); // Simulate loading time
            cout << "Loaded image " << imageNames[i] << " with dimensions " << width << "x" << height << endl;
        });
        loadTasks.push_back(loadTask);
    }
    
    // Step 2: Resize images (depends on loading)
    vector<Task> resizeTasks;
    for (int i = 0; i < imageNames.size(); i++) {
        auto resizeTask = graph.push([i, &images]() {
            cout << "Resizing image " << i << endl;
            images[i].resize(800, 600); // Standardize size
            this_thread::sleep_for(chrono::milliseconds(200)); // Simulate resize operation
        });
        // Resize depends on load
        resizeTask.depend(loadTasks[i]);
        resizeTasks.push_back(resizeTask);
    }
    
    // Step 3: Apply filters (depends on resize)
    vector<Task> filterTasks;
    vector<string> filters = {"Sharpen", "Contrast", "Saturation"};
    for (int i = 0; i < imageNames.size(); i++) {
        auto filterTask = graph.push([i, &images, &filters]() {
            cout << "Applying filters to image " << i << endl;
            images[i].applyFilter(filters[i % filters.size()]); // Apply different filter based on image index
        });
        // Filter depends on resize
        filterTask.depend(resizeTasks[i]);
        filterTasks.push_back(filterTask);
    }
    
    // Step 4: Save processed images (depends on filter application)
    vector<Task> saveTasks;
    for (int i = 0; i < imageNames.size(); i++) {
        auto saveTask = graph.push([i, &images, &imageNames]() {
            string outputName = "processed_" + imageNames[i];
            images[i].save(outputName);
        });
        // Save depends on filter
        saveTask.depend(filterTasks[i]);
        saveTasks.push_back(saveTask);
    }
    
    // Step 5: Create image catalog (depends on all saves)
    auto catalogTask = graph.push([&imageNames]() {
        cout << "Creating image catalog..." << endl;
        cout << "Catalog includes the following processed images:" << endl;
        for (const auto& name : imageNames) {
            cout << "- processed_" << name << endl;
        }
        this_thread::sleep_for(chrono::milliseconds(400)); // Simulate catalog generation
        cout << "Image catalog created successfully!" << endl;
    });
    
    // Catalog depends on all save operations
    for (const auto& task : saveTasks) {
        catalogTask.depend(task);
    }
    
    // Execute the pipeline
    cout << "\nStarting image processing pipeline...\n" << endl;
    graph.start();
    graph.wait();
    cout << "\nImage processing pipeline completed!" << endl;
    
    return 0;
}
