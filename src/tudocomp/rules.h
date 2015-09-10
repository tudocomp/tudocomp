#ifndef RULES_H
#define RULE_H

template<class T>
class GrowableSdslVec {
    T vec;
    size_t size;

    GrowableSdslVec(): size(0) {
    }

    void push_back(T value) {
        if (size == vec.size()) {
            auto cap = size + (size - 1) / 2 + 1;
            vec.resize(cap);
        }
        vec[size] = std::move(value);
        size++;
    }

};

#endif
