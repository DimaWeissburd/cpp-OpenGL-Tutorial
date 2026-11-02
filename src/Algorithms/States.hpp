#ifndef STATES_HPP
#define STATES_HPP

namespace States {
    template<typename T>
    bool isIndexActive(T* states, int target) {
        return (*states & (1 << target)) == (1 << target);
    }

    template<typename T>
    void activateIndex(T* states, int target) {
        *states |= 1 << target;
    }

    template<typename T>
    void uniquelyActivateIndex(T* states, int target) {
        *states &= 1 << target;
    }

    template<typename T>
    void deactivateIndex(T* states, int target) {
        *states &= ~(1 << target);
    }

    template<typename T>
    void toggleIndex(T* states, int target) {
        *states ^= 1 << target;
    }

    template<typename T>
    bool isActive(T* states, T state) {
        return (*states & state) == state;
    }

    template<typename T>
    void activate(T* states, T state) {
        *states |= state;
    }

    template<typename T>
    void uniquelyActivate(T* states, T state) {
        *states &= state;
    }

    template<typename T>
    void deactivate(T* states, T state) {
        *states &= ~state;
    }

    template<typename T>
    void toggle(T* states, T state) {
        *states ^= state;
    }
}

#endif