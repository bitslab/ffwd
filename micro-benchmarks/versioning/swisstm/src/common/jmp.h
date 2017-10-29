/**
 * Mask differences related to long_jmp functions on different platforms.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_JMP_H_
#define WLPDSTM_JMP_H_

#ifdef WLPDSTM_ICC

#include "../../intel/jmp.h"
#define LONG_JMP_BUF begin_transaction_jmpbuf

#else

#include <setjmp.h>

#ifdef WLPDSTM_MACOS
#define LONG_JMP_BUF sigjmp_buf
#elif defined WLPDSTM_LINUXOS || defined WLPDSTM_SOLARIS
#define LONG_JMP_BUF jmp_buf
#endif /* MACOS */

#endif /* WLPDSTM_ICC */

#define LONG_JMP_FIRST_FLAG 0
#define LONG_JMP_RESTART_FLAG 1
#define LONG_JMP_ABORT_FLAG 2

#endif /* WLPDSTM_JMP_H_ */

