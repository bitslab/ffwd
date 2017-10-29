/**
 *  @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_PADDED_H_
#define WLPDSTM_PADDED_H_

namespace wlpdstm {
	
	union PaddedUnsigned {
		volatile unsigned val;
		char padding[CACHE_LINE_SIZE_BYTES];
	};
	
	union PaddedWord {
		volatile Word val;
		char padding[CACHE_LINE_SIZE_BYTES];
	};	
	
	union PaddedBool {
		volatile Word val;
		char padding[CACHE_LINE_SIZE_BYTES];
	};	
}

#endif /* WLPDSTM_PADDED_H_ */
