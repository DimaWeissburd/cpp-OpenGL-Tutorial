#ifndef STATES_HPP
#define STATES_HPP

namespace States {
	template<typename T>
	bool isActive(T* states, int target) {
		return (*states & (T(1) << target)) != 0;
	}

	template<typename T>
	void activate(T* states, int target) {
		*states |= T(1) << target;
	}

	template<typename T>
	void uniquelyActivate(T* states, int target) {
		activate<T>(states, target);
		*states &= T(1) << target;
	}

	template<typename T>
	void deactivate(T* states, int target) {
		*states &= ~(T(1) << target);
	}

	template<typename T>
	void toggle(T* states, int target) {
		*states ^= T(1) << target;
	}
}

#endif