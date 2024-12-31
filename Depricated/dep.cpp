
    class Stack {
    private:
        struct Node {
            std::unique_ptr<std::vector<uint32_t>> block;
            Node* next;

            explicit Node(std::unique_ptr<std::vector<uint32_t>>&& blk) : block(std::move(blk)), next(nullptr) {}
        };

        Node* top;
        size_t size;

    public:
        Stack() : top(nullptr), size(0) {}

        ~Stack() {
        }

        void push(std::unique_ptr<std::vector<uint32_t>>&& block) {
            Node* newNode = new Node(std::move(block));
            newNode->next = top;
            top = newNode;
            ++size;
        }

        std::unique_ptr<std::vector<uint32_t>> pop() {
            if (!top) return nullptr;
            Node* temp = top;
            auto block = std::move(top->block);
            top = top->next;
            delete temp;
            --size;
            return block;
        }

        size_t getSize() const {
            return size;
        }
    };

    class Queue {
    private:
        struct Node {
            std::unique_ptr<std::vector<uint32_t>> block;
            Node* next;

            explicit Node(std::unique_ptr<std::vector<uint32_t>>&& blk) : block(std::move(blk)), next(nullptr) {}
        };

        Node *front, *rear;
        size_t size;

    public:
        Queue() : front(nullptr), rear(nullptr), size(0) {}

        ~Queue() {
        }

        void enqueue(std::unique_ptr<std::vector<uint32_t>>&& block) {
            Node* newNode = new Node(std::move(block));
            if (!rear) {
                front = rear = newNode;
            } else {
                rear->next = newNode;
                rear = newNode;
            }
            ++size;
        }

        std::unique_ptr<std::vector<uint32_t>> dequeue() {
            if (!front) return nullptr;
            Node* temp = front;
            auto block = std::move(front->block);
            front = front->next;
            if (!front) rear = nullptr;
            delete temp;
            --size;
            return block;
        }

        size_t getSize() const {
            return size;
        }
    };