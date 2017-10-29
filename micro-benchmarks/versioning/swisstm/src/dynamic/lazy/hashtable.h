/**
 * This is a hashtable used for lookup of values in lazy SwissTM variant. It assumes that
 * keys are pointers, but the type of the value and the size of the first level table
 * can be specified as template arguments.
 *
 * Value type should have the field next of type VAL * and field KEY_FIELD of type uintptr_t.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef WLPDSTM_DYNAMIC_LAZY_HASHTABLE_H_
#define WLPDSTM_DYNAMIC_LAZY_HASHTABLE_H_

#include <stdint.h>

#include "../log.h"

#define KEY_FIELD read_lock
#define USE_HASHTABLE_MARKERS
#define USE_HASHTABLE_EPOCH_MARKERS

namespace wlpdstm {

	template<class VAL, unsigned SIZE = 256>
	class Hashtable {
		static const unsigned HASH_MASK = SIZE - 1;
#ifdef USE_HASHTABLE_EPOCH_MARKERS
		// this is a maximum number that can be stored in 8 bits
		static const uint8_t MAX_EPOCH = 255;
#endif /* USE_HASHTABLE_EPOCH_MARKERS */	
	
		public:
			void ThreadInit();

			VAL *find(uintptr_t key);
			void insert(uintptr_t key, VAL *val);
			void clear();

		protected:
			static uintptr_t hash_key(uintptr_t key);

		protected:
			VAL *table[SIZE];
#ifdef USE_HASHTABLE_MARKERS
			uint8_t markers[SIZE];
#ifdef USE_HASHTABLE_EPOCH_MARKERS
			uint8_t current_epoch;
#endif /* USE_HASHTABLE_EPOCH_MARKERS */	
#endif /* USE_HASHTABLE_MARKERS */
	};

	typedef Hashtable<WriteLogEntry> WriteLogHashtable;
}

template<class VAL, unsigned SIZE>
inline void wlpdstm::Hashtable<VAL, SIZE>::ThreadInit() {
	memset(table, 0, sizeof(VAL *) * SIZE);
#ifdef USE_HASHTABLE_MARKERS
	memset(markers, 0, sizeof(uint8_t) * SIZE);
#ifdef USE_HASHTABLE_EPOCH_MARKERS
	current_epoch = 1;
#endif /* USE_HASHTABLE_EPOCH_MARKERS */	
#endif /* USE_HASHTABLE_MARKERS */	
}

template<class VAL, unsigned SIZE>
inline uintptr_t wlpdstm::Hashtable<VAL, SIZE>::hash_key(uintptr_t key) {
	return key & HASH_MASK;
}

template<class VAL, unsigned SIZE>
inline VAL *wlpdstm::Hashtable<VAL, SIZE>::find(uintptr_t key) {
	uintptr_t table_idx = hash_key(key);
#ifdef USE_HASHTABLE_MARKERS
#ifdef USE_HASHTABLE_EPOCH_MARKERS
	if(markers[table_idx] != current_epoch) {
		return NULL;
	}
#else
	if(!markers[table_idx]) {
		return NULL;
	}
#endif /* USE_HASHTABLE_EPOCH_MARKERS */
#endif /* USE_HASHTABLE_MARKERS */
	VAL *curr = table[table_idx];

	while(curr != NULL) {
		if((uintptr_t)curr->KEY_FIELD == key) {
			break;
		}

		curr = curr->next;
	}

	return curr;
}

template<class VAL, unsigned SIZE>
inline void wlpdstm::Hashtable<VAL, SIZE>::insert(uintptr_t key, VAL *val) {
	uintptr_t table_idx = hash_key(key);

#ifdef USE_HASHTABLE_MARKERS
#ifdef USE_HASHTABLE_EPOCH_MARKERS
	if(markers[table_idx] != current_epoch) {
		markers[table_idx] = current_epoch;
		table[table_idx] = val;
		val->next = NULL;
		return;
	}
#else
	if(!markers[table_idx]) {
		markers[table_idx] = 1;
		table[table_idx] = val;
		val->next = NULL;
		return;
	}
#endif /* USE_HASHTABLE_EPOCH_MARKERS */
#endif /* USE_HASHTABLE_MARKERS */

	val->next = table[table_idx];
	table[table_idx] = val;
}

template<class VAL, unsigned SIZE>
inline void wlpdstm::Hashtable<VAL, SIZE>::clear() {
#ifdef USE_HASHTABLE_MARKERS
#ifdef USE_HASHTABLE_EPOCH_MARKERS
	if(current_epoch < MAX_EPOCH) {
		current_epoch++;
	} else {
		current_epoch = 1;
		memset(markers, 0, sizeof(uint8_t) * SIZE);		
	}
#else
	memset(markers, 0, sizeof(uint8_t) * SIZE);
#endif /* USE_HASHTABLE_EPOCH_MARKERS */
#else
	memset(table, 0, sizeof(VAL *) * SIZE);
#endif /* USE_HASHTABLE_MARKERS */		
}

#endif /* WLPDSTM_DYNAMIC_LAZY_HASHTABLE_H_ */
