#pragma once

#include <cstddef>
#include <utility>

// Template class for a LinkedList
template <typename T>
class LinkedList {
private:
    // Internal structure representing a node in the LinkedList
    struct Node {
        T data;       // Data stored in the node
        Node *next;   // Pointer to the next node

        // Constructor to initialize a node with given data (move semantics used for efficiency)
        explicit Node(T &&data) : data(std::move(data)), next(nullptr) {}
    };

    Node *head;     // Pointer to the first node in the LinkedList
    Node *tail;     // Pointer to the last node in the LinkedList
    size_t size;    // Tracks the number of nodes in the LinkedList

public:
    // Constructor: Initializes an empty LinkedList
    LinkedList() : head(nullptr), tail(nullptr), size(0) {}

    // Destructor: Clean up resources
    ~LinkedList() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }

    // Adds a new element to the end of the LinkedList using move semantics
    void push_back(T &&value) {
        Node *newNode = new Node(std::move(value)); // Create a new node with the given value
        if (!head) { // If the list is empty, initialize both head and tail to the new node
            head = tail = newNode;
        } else { // Otherwise, append the new node to the end of the list
            tail->next = newNode;
            tail = newNode;
        }
        ++size; // Increment the size of the list
    }

    // Returns the current size of the LinkedList
    size_t getSize() const { return size; }
};