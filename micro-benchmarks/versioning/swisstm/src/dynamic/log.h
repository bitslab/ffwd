/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.h
 *
 */

#ifndef WLPDSTM_DYNAMIC_LOG_H_
#define WLPDSTM_DYNAMIC_LOG_H_

#include "../common/word.h"
#include "../common/log.h"

#include "version_lock.h"

namespace wlpdstm {

	struct ReadLogEntry {
		VersionLock *read_lock;
		VersionLock version;
	};
	
	// mask is needed here to avoid overwriting non-transactional
	// data with their stale values upon commit 
	struct WriteWordLogEntry {
		Word *address;
		Word value;
		Word mask;
		WriteWordLogEntry *next;

		Word MaskWord();
	};

	typedef Log<WriteWordLogEntry> WriteWordLogMemPool;

	class TransactionDynamic;

	struct WriteLogEntry {
		VersionLock *read_lock;
		WriteLock *write_lock;
		
		VersionLock old_version;
		
		TransactionDynamic *owner;

		WriteWordLogEntry *head;

		// this is needed only for lazy hashtable
		WriteLogEntry *next;
		
		// methods
		void InsertWordLogEntry(WriteWordLogMemPool &write_word_log_mem_pool, Word *address, Word value, Word mask);
		
		WriteWordLogEntry *FindWordLogEntry(Word *address);
		
		void ClearWordLogEntries();
	};

	Word MaskWord(Word old, Word val, Word mask);

	typedef Log<ReadLogEntry> ReadLog;

	struct WriteLog : public Log<WriteLogEntry> {
		WriteWordLogMemPool write_word_log_mem_pool;
		void clear();
	};
}

inline void wlpdstm::WriteLogEntry::InsertWordLogEntry(WriteWordLogMemPool &write_word_log_mem_pool,
													   Word *address, Word value, Word mask) {
	WriteWordLogEntry *entry = FindWordLogEntry(address);
	
	// new entry
	if(entry == NULL) {
		entry = write_word_log_mem_pool.get_next();
		entry->address = address;
		entry->next = head;
		entry->value = value;
		entry->mask = mask;
		head = entry;
	} else {
		entry->value = MaskWord(entry->value, value, mask);
		entry->mask |= mask;
	}
}

inline wlpdstm::WriteWordLogEntry *wlpdstm::WriteLogEntry::FindWordLogEntry(Word *address) {
	WriteWordLogEntry *curr = head;
	
	while(curr != NULL) {
		if(curr->address == address) {
			break;
		}
		
		curr = curr->next;
	}
	
	return curr;
}

inline void wlpdstm::WriteLogEntry::ClearWordLogEntries() {
	head = NULL;
}

inline Word wlpdstm::MaskWord(Word old, Word val, Word mask) {
	if(mask == LOG_ENTRY_UNMASKED) {
		return val;
	}
	
	return (old & ~mask) | (val & mask);
}

inline Word wlpdstm::WriteWordLogEntry::MaskWord() {
	return wlpdstm::MaskWord(*address, value, mask);
}

inline void wlpdstm::WriteLog::clear() {
	Log<WriteLogEntry>::clear();
	write_word_log_mem_pool.clear();
}

#endif /* WLPDSTM_DYNAMIC_LOG_H_ */
