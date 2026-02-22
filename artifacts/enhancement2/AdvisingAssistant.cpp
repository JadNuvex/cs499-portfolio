/* =============================================================================
 * PROJECT:      ABCU Advising Assistant
 * VERSION:      Enhancement 2 – Algorithms & Data Structures
 *
 * AUTHOR:       Jaden B. Knutson
 * COURSE:       CS 499 – Computer Science Capstone
 *
 * ORIGINAL:     June 2024
 * ENHANCED:     January 2026
 *
 * DESCRIPTION:  Console-based course advising system that loads course data from 
 *               CSV and supports sorted listing and prerequisite lookup. 
 * 
 * UPGRADE:      This enhancement replaces standard library hashing with a manually 
 *               implemented hash table using a custom hash function and linked-list 
 *               chaining to demonstrate collision handling, controlled traversal, 
 *               and low-level data structure design.
 * =============================================================================
 */


// ============================================================================
// IMPORTS
// ============================================================================
#include <iostream>         // Console input/output (UI layer)
#include <fstream>          // File input for CSV loading (logic layer)
#include <sstream>          // String parsing for CSV line processing
#include <vector>           // Stores course prerequisites and course ordering
#include <algorithm>        // Sorting course codes for alphanumeric list output
#include <stdexcept>        // Standard exceptions for safe, consistent error handling
using namespace std;

// ============================================================================
// DATA LAYER: Course Class
// ============================================================================

// UPGRADE: Uses  prime table size, reduces likelihood of hash collisions.
const int HASH_TABLE_SIZE = 17;

class Course {
private:
    // Private members enforce data integrity.
    string code;
    string title;
    vector<string> prerequisites;

public:
    // Default constructor.
    Course() {}

    // Constructor performs self-validation so invalid objects never enter system.
    Course(string c, string t, vector<string> p) : code(c), title(t), prerequisites(p) {
        validate();
    }

    // Prevents storing empty/invalid course records.
    void validate() const {
        if (code.empty() || title.empty()) {
            throw runtime_error("Invalid Course Data: Code or Title is missing.");
        }
    }

    // Getters expose data safely, read-only.
    string getCode() const { return code; }
    string getTitle() const { return title; }
    const vector<string>& getPrereqs() const { return prerequisites; }
};
// ============================================================================









// ============================================================================
// LOGIC LAYER: Manual HashTable Manager
//
// UPGRADE: Removed unordered_map to instead implement a manual hash table.
// Demonstrates understanding of low-level data structures,
// memory management, and collision resolution strategies.
// ============================================================================

// UPGRADE: Uses node structure for Linked-List Chaining.
// Each node stores single Course and pointer to next node,
// allows multiple courses to share same hash index safely.
struct Node {
    Course course;     // Stores course object at thIShash position
    Node* next;        // Pointer to next node in collision chain

    // Constructor initializes node and ensures chain starts.
    Node(Course c) : course(c), next(nullptr) {}
};

class HashTable {
private:
    // UPGRADE: Uses manual array of pointers acting as hash buckets.
    // Each index represents start of a linked list (collision chain).
    Node* table[HASH_TABLE_SIZE];

    // Stores course codes to support sorted output
    // without requiring traversal of entire hash table.
    vector<string> courseOrder;

    // Tracks whether data has been successfully loaded before access.
    bool dataLoaded = false;

    // UPGRADED CUSTOM HASH FUNCTION:
    // Uses polynomial rolling hash with prime multiplier (31)
    // Reduces collisions and evenly distribute keys.
    // Average-case lookup remains O(1).
    unsigned int hash(const string& key) const {
        unsigned int hashVal = 0;
        for (char ch : key) {
            hashVal = hashVal * 31 + ch; // Polynomial accumulation
        }
        return hashVal % HASH_TABLE_SIZE; // Constrain index to table size
    }

    // Normalizes input to uppercase to ensure consistent hashing
    // Prevents case-sensitive lookup errors.
    string toUpper(const string& str) const {
        string upperString = str;
        transform(upperString.begin(), upperString.end(),
                  upperString.begin(), ::toupper);
        return upperString;
    }

    // Parses single CSV line into Course object.
    // Throws exception if required fields are missing,
    // Prevemts invalid objects from entering system.
    Course parseCourse(const string& line) {
        stringstream ss(line);
        string code, title, prereq;
        vector<string> prereqs;

        // Course code / title are required fields.
        if (!getline(ss, code, ',') || !getline(ss, title, ',')) {
            throw runtime_error("Malformed line in file.");
        }

        // Remaining fields are treated as prereqs.
        while (getline(ss, prereq, ',')) {
            if (!prereq.empty()) prereqs.push_back(prereq);
        }

        return Course(code, title, prereqs);
    }

public:
    // UPGRADE yses constructor to initialize hash buckets to nullptr.
    // Prevents undefined behavior during insertion or lookup.
    HashTable() {
        for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
            table[i] = nullptr;
        }
    }

    // UPGRADE INSERTION ALGORITHM: 
    // Calculates hash index and appends course the end
    // of chain, preservs existing entries, prevents data loss.
    void insert(Course course) {
        string key = toUpper(course.getCode());
        unsigned int index = hash(key);
        Node* newNode = new Node(course);

        // If no collision, insert directly.
        if (table[index] == nullptr) {
            table[index] = newNode;
        }
        // Otherwise, traverse chain and append.
        else {
            Node* curr = table[index];
            while (curr->next != nullptr) {
                curr = curr->next;
            }
            curr->next = newNode;
        }

        // Track insertion order separately for sorted output.
        courseOrder.push_back(key);
    }

    // Loads course data from CSV file, populates hash table.
    // Existing data cleared to prevent duplication or stale entries.
    void loadData(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Could not open file: " + filename);
        }

        // Reset state before loading new data.
        courseOrder.clear();
        for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
            table[i] = nullptr;
        }

        string line;
        int lineNum = 0;

        // Read and process each line of file.
        while (getline(file, line)) {
            lineNum++;
            try {
                Course course = parseCourse(line);
                insert(course); // Uses manual chaining insertion
            }
            catch (const exception& e) {
                // Adds context to parsing errors for easier debugging.
                throw runtime_error(
                    "Error on line " + to_string(lineNum) + ": " + e.what()
                );
            }
        }
        dataLoaded = true;
    }

    // UPGRADE SEARCH ALGORITHM:
    // Computes hash index and traverses only local chain.
    // Preserves O(1) average lookup time, even with collisions.
    Course getCourse(string code) const {
        if (!dataLoaded) {
            throw runtime_error("No data loaded.");
        }

        string key = toUpper(code);
        unsigned int index = hash(key);

        Node* curr = table[index];
        while (curr != nullptr) {
            if (toUpper(curr->course.getCode()) == key) {
                return curr->course;
            }
            curr = curr->next;
        }
        throw runtime_error("Course not found.");
    }

    // Returns sorted list of course codes for display.
    // Sorting is performed separately from the hash table.
    // Keeps lookup performance independent.
    vector<string> getSortedCourseCodes() {
        if (!dataLoaded) {
            throw runtime_error("No data loaded.");
        }

        sort(courseOrder.begin(), courseOrder.end());
        return courseOrder;
    }
};
// ============================================================================









// ============================================================================
// PRESENTATION LAYER: UI Logic
// ============================================================================

void displayMenu() {
    cout << "=============================\n";
    cout << "ABCU Advising Assistant (Enhancement 2)\n";
    cout << "1. Load Data Structure\n";
    cout << "2. Print Course List\n";
    cout << "3. Print Course\n";
    cout << "4. Enter Custom File Name\n";
    cout << "9. Exit\n";
    cout << "=============================\n";
    cout << "Selection: ";
}

int main() {
    HashTable hashTable;
    string userInput;

    // Main menu loop continues until the user selects Exit.
    while (true) {
        displayMenu();
        getline(cin, userInput);

        // Global try/catch block that prevents crashes on invalid input or data errors.
        // Ensures user always receives a clear explanation of what went wrong.
        try {
            
            // Option 1 and 4 both load data; 4 allows a custom file name.
            if (userInput == "1" || userInput == "4") {
                string filename = (userInput == "1") ? "Program_Input.csv" : "";

                // Option 4: user supplies custom file name.
                if (userInput == "4") {
                    cout << "Enter filename: ";
                    getline(cin, filename);
                }

                // Logic layer loads data; any errors throw exceptions handled below.
                hashTable.loadData(filename);
                cout << "Success: Data loaded from " << filename << endl;
            }
            // Print sorted course list.
            else if (userInput == "2") {
                auto codes = hashTable.getSortedCourseCodes();

                // Retrieves sorted course codes from logic layer, then fetches and prints
                // each course’s details.
                for (const auto& c : codes) {
                    Course course = hashTable.getCourse(c);
                    cout << course.getCode() << ": " << course.getTitle() << endl;
                }
            }
            // Print single course with prereqs.
            else if (userInput == "3") {
                cout << "What course code? ";
                getline(cin, userInput);

                // Logic layer handles lookup and throws if course is missing.
                Course course = hashTable.getCourse(userInput);

                cout << course.getCode() << ", " << course.getTitle() << endl;
                cout << "Prerequisites: ";

                // Output prereqs cleanly, comma-separated, instead of space-separated.
                auto prereqs = course.getPrereqs();
                if (prereqs.empty()) cout << "None";
                else {
                    for (size_t i = 0; i < prereqs.size(); ++i) {
                        cout << prereqs[i] << (i < prereqs.size() - 1 ? ", " : "");
                    }
                }
                cout << "\n";
            }
            // Exit
            else if (userInput == "9") {
                break;
            }
            // Any other input is invalid.
            else {
                cout << "Invalid selection." << endl;
            }
        }
        catch (const exception& e) {
            // Displays detailed errors from logic layer in a user-friendly way
            // without application crashing.
            cout << "SYSTEM ERROR: " << e.what() << endl;
        }
        cout << endl;
    }
    cout << "Goodbye." << endl;
    return 0;
}
// ============================================================================