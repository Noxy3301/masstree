#include <iostream>

#include "include/masstree.h"

void worker() {
    return;
}

int main() {
    Masstree masstree;
    GarbageCollector gc;

    Key k0{{0}, 1};
    Status status = masstree.insert_value(k0, new Value{5454}, gc);
    if (status == Status::WARN_ALREADY_EXISTS) std::cout << "key {0} already exists" << std::endl;
    if (status == Status::OK) std::cout << "inserted key {0}" << std::endl;

    for (size_t i = 1; i < 100; i++) {
        Key *test = new Key({i}, 1);
        Status status = masstree.insert_value(*test, new Value{i*10}, gc);
        if (status == Status::WARN_ALREADY_EXISTS) std::cout << "key { " << i << " } already exists" << std::endl;
        Value *value = masstree.get_value(*test);
        if (value == nullptr) {
            std::cout << "key { " << i << " } not found" << std::endl;
        } else {
            std::cout << "key { " << i << " } found, value: " << value->getBody() << std::endl;
        }
    }

    return 0;
}