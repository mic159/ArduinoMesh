#ifndef LINKED_LIST_H
#define LINKED_LIST_H

template<typename T>
class LinkedList {
public:
	LinkedList():last(NULL),first(NULL),length(0){}

	struct Node {
		T* item;
		Node* next;
		Node* prev;
		Node(T* item):item(item),next(NULL),prev(NULL){}
	};

	void Add(T* item) {
		Node* n = new Node(item);
		if (last == NULL) {
			last = n;
			first = n;
		} else {
			last->next = n;
			n->prev = last;
			last = n;
		}
		length += 1;
	}

	Node* GetItem(T* item) {
		Node* n = first;
		while(n != NULL) {
			if (n->item == item)
				return n;
			n = n->next;
		}
		return NULL;
	}

	void Remove(T* item) {
		Node* n = GetItem(item);
		Remove(n);
	}

	Node* Remove(Node* n) {
		if (!n)
			return NULL;
		if (first == n)
			first = n->next;
		if (last == n)
			last = n->prev;
		if (n->prev)
			n->prev->next = n->next;
		if (n->next)
			n->next->prev = n->prev;
		Node* ret = n->next;
		delete n;
		length -= 1;
		return n->next;
	}

	Node* last;
	Node* first;
	uint16_t length;
};

#endif
