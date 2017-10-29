#ifndef WLPDSTM_SWISSTM_CONSTANTS_H_
#define WLPDSTM_SWISSTM_CONSTANTS_H_

#include "../common/constants.h"

namespace wlpdstm {

	// this constant is useful for defining various data structures, but it should be possible
	// to replace it with a higher number with no problems
	const unsigned MAX_THREADS = 128;

	// the smallest timestamp used
	const unsigned MINIMUM_TS = 0;

	const unsigned WORD_LOG_SIZE = 22;

	const unsigned LOCK_RESERVED_BITS = 2;

	// -1 to get the maximum number
	const Word MAXIMUM_TS = (1l << (ADDRESS_SPACE_SIZE - LOCK_RESERVED_BITS)) - 1 - MAX_THREADS;

	// top level transaction
	const int TX_TOP_LEVEL = 0;
	const int TX_NO_LEVEL = (TX_TOP_LEVEL - 1);

	enum OperationType {
		NO_OP = 0,
		READ_OP = 1,
		WRITE_OP = 2,
		DELETE_OP = 3
	};

	inline const char *getOperationName(OperationType optype) {
		const char *names[] = {
			"noop",
			"read",
			"write",
			"delete"
		};

		return names[optype];
	}
}

#define LSB 1u

#define MINIMUM_VERSION ((Word *)0)

#define WRITE_LOCK_CLEAR ((Word)0)

#define READ_LOCK_SET ((Word)LSB)

#endif /* WLPDSTM_SWISSTM_CONSTANTS_H_ */
