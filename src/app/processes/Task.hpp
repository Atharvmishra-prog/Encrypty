#ifndef TASK_HPP
#define TASK_HPP

#include "../fileHandling/IO.hpp"
#include <fstream>
#include <string>
#include <sstream>
using namespace std;

enum class Action {
    ENCRYPT,
    DECRYPT
};

struct Task {
    string filePath;
    fstream f_stream;
    Action action;

    //&& is used here-->volatile or temporary buffer--> It allows moving resources instead of copying them.
    //A rvalue reference (T&&) allows us to take ownership of temporary objects (rvalues) and avoid unnecessary copies.
    //Optimize performance in resource-heavy classes

    Task(fstream&& stream, Action act, string filePath) { //constructor 

        f_stream = std::move(stream); //ownership is moved
        action = act; //encrypt ya decrypt
        this->filePath = filePath;
    }
    

    //to serialise and deserialise: 2 methods--> toString() and fromString()


    //SERIALISATION: Definition: Serialization is the process of converting an object 
    // into a format that can be stored or transmitted (e.g., string, JSON, binary).
    //Task object into a comma-separated string.

    string toString() const {
        ostringstream oss;
        //test.txt ENCRYPT f_stream
        //"test.txt, ENCRYPT"
        oss << filePath << "," << (action == Action::ENCRYPT ? "ENCRYPT" : "DECRYPT");
        return oss.str();
    }

    //DESERALISATION: Deserialization is the process of reconstructing an object 
    // from its serialized format (string, JSON, binary).
    //("test.txt,ENCRYPT") back into a Task object


    //The fromString() method is declared as static because 
    //it does not depend on any existing Task object. 
    //Instead, it creates and returns a new Task object from a string.

    //A static method in C++:
    //Belongs to the class, not an instance (doesn’t require an object to be called).
    //Cannot access non-static members of the class directly.
    //Can be called without an object → Task::fromString("file.txt,ENCRYPT").


    static Task fromString(const string& taskData) {
        istringstream iss(taskData);
        string filePath;
        string actionStr;

        if (getline(iss, filePath, ',') && getline(iss, actionStr)) {

            Action action = (actionStr == "ENCRYPT") ? Action::ENCRYPT : Action::DECRYPT;

            IO io(filePath);
            fstream f_stream = io.getFileStream();
            if (f_stream.is_open()) {
                return Task(std::move(f_stream), action, filePath);
            }
            else {
                throw runtime_error("Failed to open file: " + filePath);
            }
        }
        else {
            throw runtime_error("Invalid task data format");
        }
    }
};

#endif

//getline(istream& input, string& str, char delimiter);
/*
istream& input → Input stream (e.g., cin, ifstream, istringstream).

string& str → String where the extracted content is stored.

char delimiter (optional) → Stops reading when this character is found (default is '\n').
*/