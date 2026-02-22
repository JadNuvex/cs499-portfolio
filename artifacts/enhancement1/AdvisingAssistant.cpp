/* =============================================================================
 * PROJECT:      ABCU Advising Assistant
 * VERSION:      Enhancement 1 – Software Engineering
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
 * UPGRADE:      This enhancement implements layered architecture, encapsulation, 
 *               and structured exception handling to improve modularity and maintainability.
 *               exception handling to improve modularity and maintainability.
 * =============================================================================
 */


// ============================================================================
// IMPORTS
// ============================================================================
#include <iostream>         // Console input/output (UI layer)
#include <fstream>          // File input for CSV loading (logic layer)
#include <sstream>          // String parsing for CSV line processing
#include <unordered_map>    // Hash table storage: O(1) average lookups by course code
#include <vector>           // Stores course prerequisites and course ordering
#include <algorithm>        // Sorting course codes for alphanumeric list output
#include <stdexcept>        // Standard exceptions for safe, consistent error handling
using namespace std;

// ============================================================================
// DATA LAYER: Course Class - Represents single course (code, title, prerequisites).
//
// UPGRADE: Converted from struct to a class to enforce encapsulation, protect internal
// course data, and ensure validation blocks invalid records while providing
// safe, read-only access for the UI.
// ============================================================================

// Course Class - stores details of each course.
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
// LOGIC LAYER: HashTable Manager - Load course data, stores in hash table, and provides retrieval methods.
//
// UPGRADE: Changed to be UI-agnostic by removing console output and using
// exceptions for error handling, improving modularity, testability, and
// long-term maintainability.
// ============================================================================
class HashTable {
private:
    unordered_map<string, Course> courseMap;  // Key: uppercased course code, Value: Course object
    vector<string> courseOrder;               // Stores course codes for sorting/display
    bool dataLoaded = false;                  // Tracks system state to prevent invalid access

    // Converts user input to uppercase so searches are case-insensitive. 
    string toUpper(const string& str) const {
        string upperString = str;
        transform(upperString.begin(), upperString.end(), upperString.begin(), ::toupper);
        return upperString;
    }

    // Parses one CSV line into a Course object, validating required fields (code and
    // title) and reading remaining comma-separated values as optional prereqs.
    Course parseCourse(const string& line) {
        stringstream ss(line);
        string code, title, prereq;
        vector<string> prereqs;

        // Rejects CSV rows with missing required fields, preventing invalid or partial data from entering system.
        if (!getline(ss, code, ',') || !getline(ss, title, ',')) {
            throw runtime_error("Malformed line in file.");
        }

        // Collect remaining columns as prereqs.
        while (getline(ss, prereq, ',')) {
            if (!prereq.empty()) prereqs.push_back(prereq);
        }

        // Creating Course automatically triggers validation.
        return Course(code, title, prereqs);
    }

public:
    // Loads course data from CSV file into the hash table, using exceptions instead
    // of return codes so UI controls error display and detailed messages aid
    // debugging and data maintenance.
    void loadData(const string& filename) {
        ifstream file(filename);

        // Fail fast with a meaningful error.
        if (!file.is_open()) {
            throw runtime_error("Could not open file: " + filename);
        }

        // Clears existing data before loading new data preventing duplicates or stale
        // entries for when multiple files are loaded.
        courseMap.clear();
        courseOrder.clear();

        string line;
        int lineNum = 0;

        // Reads file line-by-line and then builds internal data structure.
        while (getline(file, line)) {
            lineNum++;
            try {
                Course course = parseCourse(line);

                // Store using uppercase key so user input searches are case-insensitive.
                string upperCode = toUpper(course.getCode());
                courseMap[upperCode] = course;
                courseOrder.push_back(upperCode);
            }
            catch (const exception& e) {
                // Includes line numbers in error reporting to identify which CSV row caused failure.
                throw runtime_error("Error on line " + to_string(lineNum) + ": " + e.what());
            }
        }

        // Once loaded successfully, unlock other features.
        dataLoaded = true;
    }

    // Returns a Course for the given code, with state checking to prevent access
    // before data is loaded and avoid crashes or confusing behavior.

    Course getCourse(string code) const {
        if (!dataLoaded) throw runtime_error("No data loaded.");

        // Standardize code so searches work with different user capitalization.
        string upperCode = toUpper(code);

        // Explicit "not found" error instead of silent failure.
        if (courseMap.find(upperCode) == courseMap.end()) {
            throw runtime_error("Course not found.");
        }

        // Use .at() for safe access (will not create new blank entry).
        return courseMap.at(upperCode);
    }

    // Returns sorted course codes for display, sorting only the code list while
    // UI retrieves full course details as needed.
    vector<string> getSortedCourseCodes() {
        if (!dataLoaded) throw runtime_error("No data loaded.");
        sort(courseOrder.begin(), courseOrder.end());
        return courseOrder;
    }
};

// ============================================================================
// PRESENTATION LAYER: UI Logic - Handles console input/output and user interactions.
//
// UPGRADE: Centralizes all console input/output and catches exceptions from the logic
// layer, converting them into clear, user-friendly messages while keeping
// the HashTable reusable.
// ============================================================================
void displayMenu() {
    cout << "=============================\n";
    cout << "ABCU Advising Assistant (Enhancement 1)\n";
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

        // Global try/catch block that prevents crashes on invalid input or data errors
        // and ensures the user always receives a clear explanation of what went wrong.
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
