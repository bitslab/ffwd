#ifndef WLPDSTM_VERSION_LOCK_H_
#define WLPDSTM_VERSION_LOCK_H_

#include "constants.h"

namespace wlpdstm {

	typedef Word VersionLock;

	inline bool is_locked(VersionLock lock) {
		return (unsigned)lock & LSB;
	}

	inline bool is_read_locked(VersionLock lock) {
		return (unsigned)lock & LSB;
	}

	inline bool is_write_locked(VersionLock lock) {
		return lock != WRITE_LOCK_CLEAR;
	}	

	inline Word get_value(VersionLock lock) {
		return lock >> LOCK_RESERVED_BITS;
	}

	inline VersionLock get_version_lock(Word ts) {
		return ts << LOCK_RESERVED_BITS;
	}

	inline VersionLock get_locked(Word thread) {
		return thread | LSB;
	}

	inline Word get_ptr(VersionLock lock) {
		return lock & ~LSB;
	}
}

#endif // WLPDSTM_VERSION_LOCK_H_
