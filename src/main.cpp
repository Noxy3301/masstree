#include <iostream>

#include "include/masstree.h"

void worker() {
    return;
}

int main() {
    Masstree masstree;
    GarbageCollector gc;

    std::vector<Key*> k{};

    Key k0{{0}, 1};
    masstree.put(k0, new Value{0}, gc);


    for (size_t i = 1; i < 100; i++) {
        Key *test = new Key({i}, 1);
        masstree.put(*test, new Value{2442}, gc);
        Value *value = masstree.get(*test);
        if (value->getBody() == 2442) std::cout << "unchi!" << std::endl;
    }

    return 0;
}