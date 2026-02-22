/* =============================================================================
 * PROJECT:      ABCU Advising Assistant
 * VERSION:      Enhancement 3 – Databases
 *
 * AUTHOR:       Jaden B. Knutson
 * COURSE:       CS 499 – Computer Science Capstone
 *
 * ORIGINAL:     June 2024
 * ENHANCED:     February 2026
 *
 * DESCRIPTION:  Console-based course advising system that loads course data from 
 *               a relational database and supports sorted listing and prerequisite 
 *               lookup.
 *
 * UPGRADE:      This enhancement integrates SQLite database persistence to replace 
 *               file-based input, demonstrating structured query execution, database 
 *               connectivity, resource management, and separation of persistent 
 *               storage from in-memory data structures.
 * =============================================================================
 */




// ============================================================================
// IMPORTS
// ----------------------------------------------------------------------------
#include <iostream>         // Console input/output (UI layer).
#include <fstream>          // File input for CSV loading (logic layer).
#include <sstream>          // String parsing for CSV line processing.
#include <vector>           // Stores course prerequisites and course ordering.
#include <algorithm>        // Sorting course codes for alphanumeric list output.
#include <stdexcept>        // Standard exceptions for safe, consistent error handling.
#include "sqlite3.h"        // Provides SQLite database functionality.
using namespace std;        // Simplifies access to standard library components.
// ============================================================================



// ============================================================================
// CONSTANTS
// ----------------------------------------------------------------------------
//Prime-sized hash table (17) chosen to reduce collisions.
const int HASH_TABLE_SIZE = 17; 
// ============================================================================



// ============================================================================
// DATA LAYER: Course Class
// ----------------------------------------------------------------------------
class Course {

// Private members enforce data integrity.
private:
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
// ----------------------------------------------------------------------------

// Node structure supports linked-list chaining, allowing multiple courses at the same hash index.
struct Node {
    Course course;      // Stores course object at this hash index
    Node* next;         // Pointer to next node in collision chain
    
    // Constructor initializes node and ensures chain starts.
    Node(Course c) : course(c), next(nullptr) {}
};

class HashTable {
private:

    // Manual hash buckets; each index is the head of a collision chain.
    Node* table[HASH_TABLE_SIZE];
    
    // Stores course codes separately to support sorted output.
    vector<string> courseOrder;
    
    // Tracks whether data has been loaded before access.
    bool dataLoaded = false;

    // Polynomial rolling hash (×31) for low-collision key mapping.
    unsigned int hash(const string& key) const {
        unsigned int hashVal = 0;
        for (char ch : key) hashVal = hashVal * 31 + ch;
        return hashVal % HASH_TABLE_SIZE;
    }
    
    // Normalizes input to uppercase for case-insensitive hashing.
    string toUpper(const string& str) const {
        string upperString = str;
        transform(upperString.begin(), upperString.end(), upperString.begin(), ::toupper);
        return upperString;
    }

public:

    // Initializes all hash buckets to nullptr for safe insertion and lookup.
    HashTable() {
        for (int i = 0; i < HASH_TABLE_SIZE; ++i) table[i] = nullptr;
    }

    // Releases all dynamically allocated nodes to prevent memory leaks.
    ~HashTable() {
        for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
            Node* curr = table[i];
            while (curr != nullptr) {
                Node* next = curr->next;
                delete curr;
                curr = next;
            }
        }
    }
    
    // Deletes all nodes and resets hash table state.
    void clearTable() {
        for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
            Node* curr = table[i];
            while (curr != nullptr) {
                Node* next = curr->next;
                delete curr;
                curr = next;
            }
            table[i] = nullptr;
        }
    }

    // Loads course data from SQLite and rebuilds the hash table.
    void loadData() {
        sqlite3* db;
        sqlite3_stmt* stmt;

        // Opens database connection for persistent course data.
        if (sqlite3_open("ABCU.db", &db) != SQLITE_OK) {
            throw runtime_error("Could not open database: ABCU.db");
        }

        // Clears existing data to prevent stale or duplicate entries.
        courseOrder.clear();
        clearTable();

        // SQL query to retrieve course records.
        const char* sql = "SELECT code, title, prerequisites FROM courses;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            sqlite3_close(db);
            throw runtime_error("Failed to query database.");
        }

        // Iterates through query results and inserts courses into hash table.
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            string code = (const char*)sqlite3_column_text(stmt, 0);
            string title = (const char*)sqlite3_column_text(stmt, 1);
            string pStr = (const char*)sqlite3_column_text(stmt, 2);

            // Parses comma-separated prerequisites into a vector.
            vector<string> prereqs;
            stringstream ss(pStr);
            string p;
            while (getline(ss, p, ',')) if (!p.empty()) prereqs.push_back(p);

            // Inserts course using manual hash chaining.
            insert(Course(code, title, prereqs));
        }

        // Releases database resources after processing.
        sqlite3_finalize(stmt);
        sqlite3_close(db);

        // Marks data as loaded for safe access.
        dataLoaded = true;
    }

    // Inserts a course using linked-list chaining to preserve entries on collisions.
    void insert(Course course) {
        string key = toUpper(course.getCode());
        unsigned int index = hash(key);
        Node* newNode = new Node(course);
        
        if (table[index] == nullptr) {
            table[index] = newNode;
        } else {
            Node* curr = table[index];
            while (curr->next != nullptr) curr = curr->next;
            curr->next = newNode;
        }
        courseOrder.push_back(key);
    }

    // Retrieves a course by traversing the target bucket chain.
    Course getCourse(string code) const {
        if (!dataLoaded) throw runtime_error("No data loaded.");
        string key = toUpper(code);
        unsigned int index = hash(key);

        Node* curr = table[index];
        while (curr != nullptr) {
            if (toUpper(curr->course.getCode()) == key) return curr->course;
            curr = curr->next;
        }
        throw runtime_error("Course not found.");
    }

    // Returns course codes sorted independently of hash table structure.
    vector<string> getSortedCourseCodes() {
        if (!dataLoaded) throw runtime_error("No data loaded.");
        sort(courseOrder.begin(), courseOrder.end());
        return courseOrder;
    }
};
// ============================================================================



// ============================================================================
// PRESENTATION LAYER: UI Logic
// ----------------------------------------------------------------------------

// Displays the main user menu and available actions.
void displayMenu() {
    cout << "=============================\n";
    cout << "ABCU Advising Assistant (Enhancement 3)\n";
    cout << "1. Load Data from SQL Database\n";
    cout << "2. Print Course List\n";
    cout << "3. Print Course Details\n";
    cout << "9. Exit\n";
    cout << "=============================\n";
    cout << "Selection: ";
}

int main() {

    // Initializes the hash table used for course storage and retrieval.
    HashTable hashTable;
    string userInput;

    // Main application loop for menu-driven interaction.
    while (true) {
        displayMenu();
        getline(cin, userInput);

        // Centralized exception handling to prevent program termination.
        try {
            if (userInput == "1") {
                // Loads persistent course data from the database.
                hashTable.loadData();
                cout << "SUCCESS: Data loaded from ABCU.db" << endl;
            } 
            else if (userInput == "2") {
                // Displays all courses in sorted order.
                auto codes = hashTable.getSortedCourseCodes();
                for (const auto& c : codes) {
                    Course course = hashTable.getCourse(c);
                    cout << course.getCode() << ": " << course.getTitle() << endl;
                }
            } 
            else if (userInput == "3") {
                // Retrieves and displays details for a specific course.
                cout << "What course code? ";
                getline(cin, userInput);
                Course course = hashTable.getCourse(userInput);
                cout << "\n" << course.getCode() << ": " << course.getTitle() << endl;
                cout << "Prerequisites: ";
                auto prereqs = course.getPrereqs();
                if (prereqs.empty()) cout << "None";
                else {
                    for (size_t i = 0; i < prereqs.size(); ++i)
                        cout << prereqs[i] << (i < prereqs.size() - 1 ? ", " : "");
                }
                cout << "\n";
            } 
            else if (userInput == "9") break;
            else cout << "Invalid selection." << endl;
        } 
        catch (const exception& e) {
            // Reports runtime errors without exiting the application.
            cout << "SYSTEM ERROR: " << e.what() << endl;
        }
        cout << endl;
    }
    return 0;
}
// ============================================================================
