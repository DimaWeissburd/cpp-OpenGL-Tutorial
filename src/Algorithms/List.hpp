#ifndef LIST_HPP
#define LIST_HPP

#include <vector>
#include <algorithm>

namespace List {
    template<typename T>
    int getIndexOf(const std::vector<T>& v, T x) {
        auto it = std::find(v.begin(), v.end(), x);
        if (it == v.end()) return -1;
        return static_cast<int>(std::distance(v.begin(), it));
    }

    template<typename T>
    bool contains(const std::vector<T>& v, T x) {
        return std::find(v.begin(), v.end(), x) != v.end();
    }
}

#endif