#ifndef TRIE_HPP
#define TRIE_HPP

#include <string>
#include <vector>
#include <stdexcept>

namespace Trie {
	struct Range {
		int lower;
		int upper;

		int calculateRange() {
			return upper - lower + 1;
		}

		bool contains(int i) {
			return i >= lower && i <= upper;
		}
	};

	typedef std::vector<Range> charset;

	const charset ascii_letters = { { 'A', 'Z' },{ 'a', 'z' } };
	const charset ascii_lowercase = { { 'a', 'z' } };
	const charset ascii_uppercase = { { 'A', 'Z' } };
	const charset digits = { { '0', '9' } };
	const charset alpha_numeric = { { '0', '9' },{ 'A', 'Z' },{ 'a', 'z' } };

	template <typename T>
	struct node {
		bool exists;
		T data;
		struct node<T>** children;

		void traverse(void(*itemViewer)(T data), unsigned int noChildren) {
			if (exists) {
				itemViewer(data);
			}

			if (children) {
				for (int i = 0; i < noChildren; i++) {
					if (children[i]) {
						children[i]->traverse(itemViewer, noChildren);
					}
				}
			}
		}
	};

	template <typename T>
	class Trie {
	public:
		Trie(charset chars = alpha_numeric)
			: chars(chars), noChars(0), root(nullptr) {
			for (Range r : chars) {
				noChars += r.calculateRange();
			}

			root = new node<T>;
			root->exists = false;
			root->children = new node<T> *[noChars];
			for (int i = 0; i < noChars; i++) {
				root->children[i] = NULL;
			}
		}

		bool insert(std::string key, T element) {
			int idx;
			node<T>* current = root;

			for (char c : key) {
				idx = getIdx(c);
				if (idx == -1) {
					return false;
				}
				if (!current->children[idx]) {
					current->children[idx] = new node<T>;
					current->children[idx]->exists = false;
					current->children[idx]->children = new node<T> *[noChars];
					for (int i = 0; i < noChars; i++) {
						current->children[idx]->children[i] = NULL;
					}
				}
				current = current->children[idx];
			}

			current->data = element;
			current->exists = true;

			return true;
		}

		bool erase(std::string key) {
			if (!root) {
				return false;
			}

			return findKey<bool>(key, [](node<T>* element) -> bool {
				if (!element) {
					return false;
				}

				element->exists = false;
				return true;
				});
		}

		void cleanup() {
			unloadNode(root);
		}

		bool containsKey(std::string key) {
			return findKey<bool>(key, [](node<T>* element) -> bool {
				if (!element) {
					return false;
				}

				return element->exists;
				});
		}

		T& operator[](std::string key) {
			return findKey<T&>(key, [](node<T>* element) -> T& {
				if (!element || !element->exists) {
					throw std::invalid_argument("key not found");
				}

				return element->data;
				});
		}

		void traverse(void(*itemViewer)(T data)) {
			if (root) {
				root->traverse(itemViewer, noChars);
			}
		}

	private:
		charset chars;
		unsigned int noChars;

		node<T>* root;

		template <typename V>
		V findKey(std::string key, V(*process)(node<T>* element)) {
			int idx;
			node<T>* current = root;
			for (char c : key) {
				idx = getIdx(c);

				if (idx == -1) {
					return process(nullptr);
				}

				current = current->children[idx];
				if (!current) {
					return process(nullptr);
				}
			}
			return process(current);
		}

		int getIdx(char c) {
			int ret = 0;

			for (Range r : chars) {
				if (r.contains((int)c)) {
					ret += (int)c - r.lower;
					break;
				}
				else {
					ret += r.calculateRange();
				}
			}

			return ret == noChars ? -1 : ret;
		}

		void unloadNode(node<T>* top) {
			if (!top) {
				return;
			}

			for (int i = 0; i < noChars; i++) {
				if (top->children[i]) {
					unloadNode(top->children[i]);
				}
			}

			top = nullptr;
		}
	};
}

#endif