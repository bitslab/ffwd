/**
 * An efficient implementation of container for logging read and
 * write sets (and possibly other things).
 * 
 * This log can be used as a memory pool for logging word values too.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_ARRAY_H_
#define WLPDSTM_ARRAY_H_

#include <string.h>

#include "alloc.h"

#ifdef CHUNKED_LOG

namespace wlpdstm {

	template<typename T, int CHUNK_LENGTH = 2048, bool INIT = false>
	class Log : public WlpdstmAlloced {
		protected:
			struct LogChunk : public WlpdstmAlloced {
				LogChunk() : next(NULL), prev(NULL) { }

				T elements[CHUNK_LENGTH];
				LogChunk *next;
				LogChunk *prev;
			};

		public:
			class iterator : public WlpdstmAlloced {
				public:
					iterator(LogChunk *c = NULL, unsigned el = 0,
							LogChunk *lc = NULL, unsigned lel = 0)
						: chunk(c), element(el), lastChunk(lc),
						lastElement(lel) { }

				public:
					bool operator==(const iterator& rhs);
					bool operator!=(const iterator& rhs);
					void next();
					bool hasNext();
					void prev();
					bool hasPrev();

					iterator& operator++();
					iterator operator++(int);
					T& operator*() const;

				protected:
					LogChunk *chunk;
					unsigned element;
					LogChunk *lastChunk;
					unsigned lastElement;
			};

		class rev_iterator : public WlpdstmAlloced {
			public:
				rev_iterator(LogChunk *hc = NULL, LogChunk *c = NULL, unsigned el = 0)
					: headChunk(hc), chunk(c), element(el) { }
			
			public:
				void prev();
				bool hasPrev();
				T& operator*() const;
				
			protected:
				LogChunk *headChunk;
				LogChunk *chunk;
				unsigned element;
		};

		public:
			Log() : head(new LogChunk()), lastChunk(head),
				nextElement(0) { }

			iterator begin() const;
			rev_iterator rbegin() const;
			iterator end() const;

			bool empty() const;
			void insert(const T& element);
			T remove();
			void delete_last();
			T *get_next();
			void clear();

			bool contains(const T *el);

            unsigned get_size() const;

		protected:
			LogChunk *head;
			// logically last chunk
			LogChunk *lastChunk;
			unsigned nextElement;
	};
}

namespace wlpdstm {
	
	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline typename Log<T, CHUNK_LENGTH, INIT>::iterator Log<T, CHUNK_LENGTH, INIT>::begin() const {
		return iterator(head, 0, lastChunk, nextElement);
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline typename Log<T, CHUNK_LENGTH, INIT>::rev_iterator Log<T, CHUNK_LENGTH, INIT>::rbegin() const {
		return rev_iterator(head, lastChunk, nextElement);
	}
	
	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline typename Log<T, CHUNK_LENGTH, INIT>::iterator Log<T, CHUNK_LENGTH, INIT>::end() const {
		return iterator(lastChunk, nextElement, lastChunk, nextElement);
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline bool Log<T, CHUNK_LENGTH, INIT>::empty() const {
		return (nextElement == 0 && lastChunk == head);
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline void Log<T, CHUNK_LENGTH, INIT>::insert(const T& el) {
		lastChunk->elements[nextElement++] = el;

		if(nextElement == CHUNK_LENGTH) {
			LogChunk *chunk = lastChunk->next;

			if(chunk == NULL) {
				chunk = new LogChunk();
				lastChunk->next = chunk;
				chunk->prev = lastChunk;
			}

			lastChunk = chunk;
			nextElement = 0;
		}
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline T *Log<T, CHUNK_LENGTH, INIT>::get_next() {
		T *ret = lastChunk->elements + nextElement++;
		//lastChunk->elements[nextElement++] = el;

		if(nextElement == CHUNK_LENGTH) {
			LogChunk *chunk = lastChunk->next;

			if(chunk == NULL) {
				chunk = new LogChunk();
				lastChunk->next = chunk;
				chunk->prev = lastChunk;
			}

			lastChunk = chunk;
			nextElement = 0;
		}

		return ret;
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline bool Log<T, CHUNK_LENGTH, INIT>::contains(const T *el) {
		LogChunk *curr = head;

		while(curr != NULL) {
			const T *chunk_start = curr->elements;
			const T *chunk_end = chunk_start + CHUNK_LENGTH;

			if(el >= chunk_start && el < chunk_end) {			
				return true;
			}

			if(curr == lastChunk) {
				return false;
			}

			curr = curr->next;
		}

		return false;
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline T Log<T, CHUNK_LENGTH, INIT>::remove() {
		if(nextElement == 0) {
			nextElement = CHUNK_LENGTH - 1;
			lastChunk = lastChunk->prev;
			return lastChunk->elements[CHUNK_LENGTH - 1];
		}

		return lastChunk->elements[--nextElement];
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline void Log<T, CHUNK_LENGTH, INIT>::delete_last() {
		if(nextElement == 0) {
			nextElement = CHUNK_LENGTH - 1;
			lastChunk = lastChunk->prev;
		} else {
			--nextElement;
		}
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline void Log<T, CHUNK_LENGTH, INIT>::clear() {
		lastChunk = head;
		nextElement = 0;
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline bool Log<T, CHUNK_LENGTH, INIT>::iterator::operator==(
			const Log<T, CHUNK_LENGTH, INIT>::iterator& rhs) {
		return chunk == rhs.chunk && element == rhs.element;
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline bool Log<T, CHUNK_LENGTH, INIT>::iterator::operator!=(
			const Log<T, CHUNK_LENGTH, INIT>::iterator& rhs) {
		return !(chunk == rhs.chunk && element == rhs.element);
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline void Log<T, CHUNK_LENGTH, INIT>::iterator::next() {
		element++;

		if(element == CHUNK_LENGTH) {
			element = 0;
			chunk = chunk->next;
		}
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline bool Log<T, CHUNK_LENGTH, INIT>::iterator::hasNext() {
		return !(element == lastElement && chunk == lastChunk);
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline typename Log<T, CHUNK_LENGTH, INIT>::iterator& Log<T, CHUNK_LENGTH, INIT>::iterator::operator++() {
		next();
		return this;
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline typename Log<T, CHUNK_LENGTH, INIT>::iterator Log<T, CHUNK_LENGTH, INIT>::iterator::operator++(int dummy) {
		typename Log<T, CHUNK_LENGTH, INIT>::iterator ret = *this;
		next();
		return ret;
	}

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline T& Log<T, CHUNK_LENGTH, INIT>::iterator::operator*() const {
		return chunk->elements[element];
	}

    // return truncated size
	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline unsigned Log<T, CHUNK_LENGTH, INIT>::get_size() const {
		return head == lastChunk ? nextElement : CHUNK_LENGTH;
	}

	//////////////////
	// rev iterator //
	//////////////////

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline T& Log<T, CHUNK_LENGTH, INIT>::rev_iterator::operator*() const {
		return chunk->elements[element - 1];
	}	

	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline void Log<T, CHUNK_LENGTH, INIT>::rev_iterator::prev() {
		element--;

		if(element == 0 && chunk != headChunk) {
			element = CHUNK_LENGTH;
			chunk = chunk->prev;
		}
	}
	
	template<typename T, int CHUNK_LENGTH, bool INIT>
	inline bool Log<T, CHUNK_LENGTH, INIT>::rev_iterator::hasPrev() {
		return !(element == 0 && chunk == headChunk);
	}	
}

#elif defined VECTOR_LOG

namespace wlpdstm {

	template<typename T, int INITIAL_SIZE = 256>
	class Log : public WlpdstmAlloced {
		public:
			class iterator : public WlpdstmAlloced {
				public:
					iterator(T *ar = NULL, unsigned s = 0,
							unsigned c = 0)
						: array(ar), size(s), current(c) {  }

				public:
					bool operator==(const iterator& rhs);
					bool operator!=(const iterator& rhs);
					void next();
					bool hasNext();
					iterator& operator++();
					iterator operator++(int);
					T& operator*() const;

				protected:
					T *array;
					unsigned size;
					unsigned current;
			};

		public:
			Log() : array((T *)wlpdstm::malloc(INITIAL_SIZE * sizeof(T))),
				size(INITIAL_SIZE), next(0) { }

			iterator begin() const;
			iterator end() const;

			bool empty() const;
			void insert(const T& element);
			void clear();
			T *get_next();

		protected:
			T *array;
			unsigned size;
			unsigned next;
	};
}

namespace wlpdstm {

	template<typename T, int INITIAL_SIZE>
	inline typename Log<T, INITIAL_SIZE>::iterator Log<T, INITIAL_SIZE>::begin() const {
		return iterator(array, next);
	}

	template<typename T, int INITIAL_SIZE>
	inline typename Log<T, INITIAL_SIZE>::iterator Log<T, INITIAL_SIZE>::end() const {
		return iterator(array, next, next);
	}

	template<typename T, int INITIAL_SIZE>
	inline bool Log<T, INITIAL_SIZE>::empty() const {
		return (next == 0);
	}

	template<typename T, int INITIAL_SIZE>
	inline void Log<T, INITIAL_SIZE>::insert(const T& el) {
		if(next == size) {
			size *= 2;
			T *oldArray = array;
			array = (T *)wlpdstm::malloc(size * sizeof(T));
			memcpy(array, oldArray, next);
			wlpdstm::free(oldArray);
		}

		array[next++] = el;
	}

	template<typename T, int INITIAL_SIZE>
	inline void Log<T, INITIAL_SIZE>::clear() {
		next = 0;
	}

	template<typename T, int INITIAL_SIZE>
	inline bool Log<T, INITIAL_SIZE>::iterator::operator==(
			const Log<T, INITIAL_SIZE>::iterator::iterator& rhs) {
		return array == rhs.array && current == rhs.current;
	}

	template<typename T, int INITIAL_SIZE>
	inline bool Log<T, INITIAL_SIZE>::iterator::operator!=(
			const Log<T, INITIAL_SIZE>::iterator::iterator& rhs) {
		return !(array == rhs.array && current == rhs.current);
	}

	template<typename T, int INITIAL_SIZE>
	inline void Log<T, INITIAL_SIZE>::iterator::next() {
		current++;
	}

	template<typename T, int INITIAL_SIZE>
	inline bool Log<T, INITIAL_SIZE>::iterator::hasNext() {
		return (current < size) && (size != 0);
	}

	template<typename T, int INITIAL_SIZE>
	inline typename Log<T, INITIAL_SIZE>::iterator& Log<T, INITIAL_SIZE>::iterator::operator++() {
		next();
		return this;
	}

	template<typename T, int INITIAL_SIZE>
	inline typename Log<T, INITIAL_SIZE>::iterator Log<T, INITIAL_SIZE>::iterator::operator++(int dummy) {
		typename Log<T, INITIAL_SIZE>::iterator ret = *this;
		next();
		return ret;
	}

	template<typename T, int INITIAL_SIZE>
	inline T& Log<T, INITIAL_SIZE>::iterator::operator*() const {
		return array[current];
	}

	template<typename T, int INITIAL_SIZE>
	inline T *Log<T, INITIAL_SIZE>::get_next() {
		return array + next++;
	}

	template<typename T, int CHUNK_LENGTH>
	inline bool Log<T, CHUNK_LENGTH>::contains(const T *el) {
		return el >= array && el <= array + size - 1;
	}
}

#elif defined LINKED_LOG

//
// Linked log is the most flexible variant of the log. It allows splitting and merging of logs easily:
//    -One log can be created from another and take over its free elements.
//    -Two logs can be merged. Free elements are merged. A special (slightly faster) version of the merge
//     doesn't merge live areas of the log.
//
// The flexibility should be very useful when implementing various types of nested transactions.
//
// Nodes in the free list don't need to have prev field initialized.
//

namespace wlpdstm {
	
	template<typename T, int INITIAL_SIZE = 2048, int GROWTH_FACTOR = 2048>
	class Log : public WlpdstmAlloced {
		protected:
			struct Node : public WlpdstmAlloced {
				T data;
				Node *next;
				Node *prev;
			};

		public:
			class iterator : public WlpdstmAlloced {
				public:
					iterator(Node *n = NULL) : current(n)  {  }
	
				public:
					bool operator==(const iterator& rhs);
					bool operator!=(const iterator& rhs);
					void next();
					bool hasNext() const;
					iterator& operator++();
					iterator operator++(int);
					T& operator*() const;
	
				protected:
					Node *current;
			};

		public:
			Log();

			iterator begin() const;
			iterator end() const;

			bool empty() const;
			void insert(const T& element);
			T remove();
			void delete_last();
			T *get_next();
			void clear();

//			bool contains(const T *el);

//			unsigned get_size() const;

		protected:
			void allocate_free(int size);

		protected:
			Node *head;
			Node *tail;
			Node *free;
	};
}


namespace wlpdstm {

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline Log<T, INITIAL_SIZE, GROWTH_FACTOR>::Log() : head(NULL), tail(NULL) {
		allocate_free(INITIAL_SIZE);
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline typename Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator Log<T, INITIAL_SIZE, GROWTH_FACTOR>::begin() const {
		return iterator(head);
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline typename Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator Log<T, INITIAL_SIZE, GROWTH_FACTOR>::end() const {
		return iterator();
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline bool Log<T, INITIAL_SIZE, GROWTH_FACTOR>::empty() const {
		return tail == NULL;
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline void Log<T, INITIAL_SIZE, GROWTH_FACTOR>::insert(const T& element) {
		T *next = get_next();
		*next = element;
	}

	// I assume that free is empty
	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline void Log<T, INITIAL_SIZE, GROWTH_FACTOR>::allocate_free(int size) {
		Node *curr = new Node[size];
		Node *next = curr + 1;
		free = curr;

		for(int i = 0;i < size - 1;i++) {
			curr->next = next;
			++curr;
			++next;
		}

		// initialize the last
		curr->next = NULL;
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline T Log<T, INITIAL_SIZE, GROWTH_FACTOR>::remove() {
		Node *last = tail;
		delete_last();
		return last->data;
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline void Log<T, INITIAL_SIZE, GROWTH_FACTOR>::delete_last() {
		Node *last = tail;

		// remove from the used list
		tail = last->prev;

		if(tail == NULL) {
			head = NULL;
		} else {
			tail->next = NULL;
		}
		
		// put into the free list
		last->next = free;
		free = last;
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline T *Log<T, INITIAL_SIZE, GROWTH_FACTOR>::get_next() {
		// get the free element
		Node *new_node = free;
		
		if(new_node == NULL) {
			allocate_free(GROWTH_FACTOR);
			new_node = free;
		}
		
		// take the free element out of the free list
		free = free->next;

		new_node->prev = tail;
		new_node->next = NULL;
		
		if(tail == NULL) {
			head = new_node;
		} else {
			tail->next = new_node;
		}
		
		tail = new_node;
		return &(new_node->data);
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline void Log<T, INITIAL_SIZE, GROWTH_FACTOR>::clear() {
		if(tail != NULL) {
			tail->next = free;
			free = head;
			tail = head = NULL;
		}
	}

	// iterator
	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline bool Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator::operator==(const iterator& rhs) {
		return rhs.current == current;
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline bool Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator::operator!=(const iterator& rhs) {
		return rhs.current != current;
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline void Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator::next() {
		current = current->next;
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline bool Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator::hasNext() const {
		return current != NULL;
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline typename Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator& Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator::operator++() {
		next();
		return *this;
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline typename Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator::operator++(int) {
		iterator ret = *this;
		next();
		return ret;
	}

	template<typename T, int INITIAL_SIZE, int GROWTH_FACTOR>
	inline T& Log<T, INITIAL_SIZE, GROWTH_FACTOR>::iterator::operator*() const {
		return current->data;
	}

}

#endif // linked log

#endif // WLPDSTM_ARRAY_H_
