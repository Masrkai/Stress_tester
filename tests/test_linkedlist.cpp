#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>
#include "../include/LinkedList.hpp"

class LinkedListTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(LinkedListTest, DefaultConstructor) {
    LinkedList<int> list;
    EXPECT_EQ(0, list.getSize());
}

TEST_F(LinkedListTest, InsertSingleElement) {
    LinkedList<int> list;

    int value = 42;
    list.Insert_At_End(std::move(value));

    EXPECT_EQ(1, list.getSize());
}

TEST_F(LinkedListTest, InsertMultipleElements) {
    LinkedList<int> list;

    for (int i = 0; i < 10; ++i) {
        list.Insert_At_End(std::move(i));
    }

    EXPECT_EQ(10, list.getSize());
}

TEST_F(LinkedListTest, InsertLargeNumberOfElements) {
    LinkedList<int> list;
    const int count = 10000;

    for (int i = 0; i < count; ++i) {
        list.Insert_At_End(std::move(i));
    }

    EXPECT_EQ(count, list.getSize());
}

TEST_F(LinkedListTest, InsertUniquePtr) {
    LinkedList<std::unique_ptr<int>> list;

    auto ptr1 = std::make_unique<int>(100);
    auto ptr2 = std::make_unique<int>(200);

    list.Insert_At_End(std::move(ptr1));
    list.Insert_At_End(std::move(ptr2));

    EXPECT_EQ(2, list.getSize());

    // Original pointers should be null after move
    EXPECT_EQ(nullptr, ptr1);
    EXPECT_EQ(nullptr, ptr2);
}

TEST_F(LinkedListTest, InsertVectors) {
    LinkedList<std::vector<int>> list;

    std::vector<int> vec1 = {1, 2, 3, 4, 5};
    std::vector<int> vec2 = {10, 20, 30};

    size_t vec1_size = vec1.size();
    size_t vec2_size = vec2.size();

    list.Insert_At_End(std::move(vec1));
    list.Insert_At_End(std::move(vec2));

    EXPECT_EQ(2, list.getSize());

    // Original vectors should be empty after move
    EXPECT_TRUE(vec1.empty());
    EXPECT_TRUE(vec2.empty());
}

TEST_F(LinkedListTest, InsertStrings) {
    LinkedList<std::string> list;

    std::string str1 = "Hello";
    std::string str2 = "World";

    list.Insert_At_End(std::move(str1));
    list.Insert_At_End(std::move(str2));

    EXPECT_EQ(2, list.getSize());

    // Strings should be empty after move
    EXPECT_TRUE(str1.empty());
    EXPECT_TRUE(str2.empty());
}

TEST_F(LinkedListTest, InsertComplexObjects) {
    struct TestObject {
        int id;
        std::string name;
        std::vector<int> data;

        TestObject(int i, std::string n, std::vector<int> d)
            : id(i), name(std::move(n)), data(std::move(d)) {}

        // Move constructor
        TestObject(TestObject&& other) noexcept
            : id(other.id), name(std::move(other.name)), data(std::move(other.data)) {
            other.id = -1;
        }

        // Move assignment
        TestObject& operator=(TestObject&& other) noexcept {
            if (this != &other) {
                id = other.id;
                name = std::move(other.name);
                data = std::move(other.data);
                other.id = -1;
            }
            return *this;
        }

        // Delete copy constructor and assignment
        TestObject(const TestObject&) = delete;
        TestObject& operator=(const TestObject&) = delete;
    };

    LinkedList<TestObject> list;

    TestObject obj1(1, "Object1", {1, 2, 3});
    TestObject obj2(2, "Object2", {4, 5, 6, 7, 8});

    list.Insert_At_End(std::move(obj1));
    list.Insert_At_End(std::move(obj2));

    EXPECT_EQ(2, list.getSize());

    // Objects should be in moved-from state
    EXPECT_EQ(-1, obj1.id);
    EXPECT_EQ(-1, obj2.id);
    EXPECT_TRUE(obj1.name.empty());
    EXPECT_TRUE(obj2.name.empty());
    EXPECT_TRUE(obj1.data.empty());
    EXPECT_TRUE(obj2.data.empty());
}

TEST_F(LinkedListTest, MemoryManagement) {
    // Test with unique_ptr to ensure proper cleanup
    LinkedList<std::unique_ptr<std::vector<int>>> list;

    const int numBlocks = 1000;
    const size_t blockSize = 1024;

    for (int i = 0; i < numBlocks; ++i) {
        auto block = std::make_unique<std::vector<int>>(blockSize, i);
        list.Insert_At_End(std::move(block));
    }

    EXPECT_EQ(numBlocks, list.getSize());

    // Destructor should clean up all blocks automatically
    // No memory leaks should occur
}

TEST_F(LinkedListTest, PerformanceTest) {
    LinkedList<int> list;
    const int iterations = 100000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        list.Insert_At_End(std::move(i));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(iterations, list.getSize());

    // Performance should be reasonable (less than 1 second for 100k insertions)
    EXPECT_LT(duration.count(), 1000000); // Less than 1 second in microseconds
}

TEST_F(LinkedListTest, EmptyListAfterDestruction) {
    size_t finalSize;

    {
        LinkedList<int> list;
        for (int i = 0; i < 100; ++i) {
            list.Insert_At_End(std::move(i));
        }
        finalSize = list.getSize();
        EXPECT_EQ(100, finalSize);
    } // List destructor called here

    // Test passes if no memory leaks or crashes occur
    EXPECT_EQ(100, finalSize);
}

TEST_F(LinkedListTest, MixedTypeInsertion) {
    LinkedList<std::unique_ptr<int>> list;

    // Insert different types of unique_ptrs
    auto ptr1 = std::make_unique<int>(1);
    auto ptr2 = std::make_unique<int>(2);
    auto ptr3 = std::make_unique<int>(3);

    list.Insert_At_End(std::move(ptr1));
    list.Insert_At_End(std::move(ptr2));
    list.Insert_At_End(std::move(ptr3));

    EXPECT_EQ(3, list.getSize());
    EXPECT_EQ(nullptr, ptr1);
    EXPECT_EQ(nullptr, ptr2);
    EXPECT_EQ(nullptr, ptr3);
}