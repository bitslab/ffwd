/* ++ EDIT */

/* #define DEFAULT_NUM_REDUCE_TASKS    256 */
/* #define EXTENDED_NUM_REDUCE_TASKS   (DEFAULT_NUM_REDUCE_TASKS * 128) */
/* #define DEFAULT_CACHE_SIZE        (8 * 1024) */
/* #define DEFAULT_KEYVAL_ARR_LEN      10 */
/* #define DEFAULT_VALS_ARR_LEN        10 */
/* #define L2_CACHE_LINE_SIZE          64 */

#define DEFAULT_NUM_REDUCE_TASKS    256
#define EXTENDED_NUM_REDUCE_TASKS   (DEFAULT_NUM_REDUCE_TASKS * 128)
#define DEFAULT_CACHE_SIZE        (64 * 1024) // AMD48 L1 data cache size = 64 Ko
#define DEFAULT_KEYVAL_ARR_LEN      10
#define DEFAULT_VALS_ARR_LEN        10
#define L2_CACHE_LINE_SIZE          64 // AMD48 L2 cache line size = 64 o

/* -- EDIT */
