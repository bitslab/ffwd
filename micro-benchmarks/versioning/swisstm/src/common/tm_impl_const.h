/**
 * Constants used to refer to various tm implementations.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_TM_IMPL_CONST_H_
#define WLPDSTM_TM_IMPL_CONST_H_

enum TmImplementation {
	TM_MIXED = 0,
	TM_LAZY,
	TM_EAGER,
	TM_IMPLEMENTATION_COUNT
};

#endif /* WLPDSTM_TM_IMPL_CONST_H_ */
