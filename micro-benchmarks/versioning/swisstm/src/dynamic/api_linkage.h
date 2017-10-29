/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_DYNAMIC_API_LINKAGE_H_
#define WLPDSTM_DYNAMIC_API_LINKAGE_H_

#ifdef DYNAMIC_INLINE
#define API_LINKAGE inline
#elif defined DYNAMIC_STATIC
#define API_LINKAGE
#endif /* DYNAMIC_INLINE */

#endif /* WLPDSTM_DYNAMIC_API_LINKAGE_H_ */
