/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_PRINT_UTILS_H_
#define WLPDSTM_PRINT_UTILS_H_

#include <inttypes.h>
#include <stdio.h>

#ifndef PRIu64

#ifdef WLPDSTM_32
#define PRIu64 "llu"
#elif defined WLPDSTM_64
#define PRIu64 "lu"
#endif /* ARCH */

#endif /* PRIu64 */

namespace wlpdstm {

	void print_indent(FILE *out_file, unsigned indent);
}

inline void wlpdstm::print_indent(FILE *out_file, unsigned indent) {
	for(unsigned u = 0;u < indent;u++) {
		fprintf(out_file, "\t");
	}
}

#endif /* WLPDSTM_PRINT_UTILS_H_ */
