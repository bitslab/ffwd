/**
 * Declarations and definitions of inlined functions for reading and writing
 * data of different sizes on different machines. These functions are NOT
 * used outside of the STM and are used to make a central place for implementing
 * this kind of code.
 *
 * This code assumes that the smalles addressable memory region is one byte.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

//TODO: This is becoming a bit of a mess. I should clean it up a bit.

#include <stdlib.h>
#include <stdint.h>

#ifdef WLPDSTM_ICC
#include <xmmintrin.h>
#endif /* WLPDSTM_ICC */

#include "transaction.h"

// 64 bit word size
#define SHIFT_8  0
#define SHIFT_16 1
#define SHIFT_32 2

#define MASK_64 0x7l
#define MASK_32 0x3l

#ifdef WLPDSTM_32
#	define MASK MASK_32
#	define READ_WORD(addr, var, tx)	(var).b32[0] = (tx)->ReadWord(addr)
#	define WORD_FIELD_NAME			b32
#elif defined WLPDSTM_64
#	define MASK MASK_64
#	define READ_WORD(addr, var, tx)	(var).b64[0] = (tx)->ReadWord(addr)
#	define WORD_FIELD_NAME			b64
#endif /* word_size */

#define WORD64_ADDRESS(addr)		((Word *)((uintptr_t)addr & ~MASK_64))
#define WORD32_ADDRESS(addr)		((Word *)((uintptr_t)addr & ~MASK_32))

#define WORD64_32_POS(addr)			(((uintptr_t)addr & MASK_64) >> SHIFT_32)
#define WORD64_16_POS(addr)			(((uintptr_t)addr & MASK_64) >> SHIFT_16)
#define WORD64_8_POS(addr)			(((uintptr_t)addr & MASK_64) >> SHIFT_8)

#define WORD32_16_POS(addr)			(((uintptr_t)addr & MASK_32) >> SHIFT_16)
#define WORD32_8_POS(addr)			(((uintptr_t)addr & MASK_32) >> SHIFT_8)

namespace wlpdstm {

	typedef union read_write_conv_u {
		// basic sizes
		uint8_t  b8[16];
		uint16_t b16[8];
		uint32_t b32[4];
		uint64_t b64[2];

		// other types
		float f[4];
		double d[2];

#ifdef WLPDSTM_ICC
		__m128 m128[1];
#endif /* WLPDSTM_ICC */

		////////////////////
		// helper structs //
		////////////////////

		// read 16 bits
		struct {
			uint8_t pad8_1;
			uint16_t b16[4];
			uint8_t pad8_2[7];
		} mask_16;

		// read 32 bit word
		struct {
			uint8_t pad8_1;
			uint32_t b32[2];
			uint8_t pad8_2[7];
		} mask_32_1;

		struct {
			uint8_t pad8_1[2];
			uint32_t b32[2];
			uint8_t pad8_2[6];
		} mask_32_2;

		struct {
			uint8_t pad8_1[3];
			uint32_t b32[2];
			uint8_t pad8_2[5];
		} mask_32_3;

		// read 64 bit word
		struct {
			uint8_t  pad8_1;
			uint64_t b64;
			uint8_t  pad8_2[7];
		} mask_64_1;

		struct {
			uint8_t  pad8_1[2];
			uint64_t b64;
			uint8_t  pad8_2[6];
		} mask_64_2;

		struct {
			uint8_t  pad8_1[3];
			uint64_t b64;
			uint8_t  pad8_2[5];
		} mask_64_3;

		struct {
			uint8_t  pad8_1[4];
			uint64_t b64;
			uint8_t  pad8_2[4];
		} mask_64_4;

		struct {
			uint8_t  pad8_1[5];
			uint64_t b64;
			uint8_t  pad8_2[3];
		} mask_64_5;

		struct {
			uint8_t  pad8_1[6];
			uint64_t b64;
			uint8_t  pad8_2[2];
		} mask_64_6;

		struct {
			uint8_t  pad8_1[7];
			uint64_t b64;
			uint8_t  pad8_2;
		} mask_64_7;
		
	} read_write_conv_t;
	
	// unaligned
	uint8_t read8(Transaction *tx, uint8_t *addr);
	uint16_t read16(Transaction *tx, uint16_t *addr);
	uint32_t read32(Transaction *tx, uint32_t *addr);
	uint64_t read64(Transaction *tx, uint64_t *addr);

	float read_float(Transaction *tx, float *addr);
	double read_double(Transaction *tx, double *addr);

#ifdef WLPDSTM_ICC
	__m128 read_m128(Transaction *tx, __m128 *addr);
#endif /* WLPDSTM_ICC */

	// aligned
	uint8_t read8aligned(Transaction *tx, uint8_t *addr);
	uint16_t read16aligned(Transaction *tx, uint16_t *addr);
	uint32_t read32aligned(Transaction *tx, uint32_t *addr);
	uint64_t read64aligned(Transaction *tx, uint64_t *addr);

	float read_float_aligned(Transaction *tx, float *addr);
	double read_double_aligned(Transaction *tx, double *addr);

	// region
	void read_region(Transaction *tx, uint8_t *src, uintptr_t bytes, uint8_t *dest);

	// unaligned
	void write8(Transaction *tx, uint8_t *addr, uint8_t val);
	void write16(Transaction *tx, uint16_t *addr, uint16_t val);
	void write32(Transaction *tx, uint32_t *addr, uint32_t val);
	void write64(Transaction *tx, uint64_t *addr, uint64_t val);

	void write_float(Transaction *tx, float *addr, float val);
	void write_double(Transaction *tx, double *addr, double val);

#ifdef WLPDSTM_ICC
	void write_m128(Transaction *tx, __m128 *addr, __m128 val);
#endif /* WLPDSTM_ICC */

	// aligned
	void write8aligned(Transaction *tx, uint8_t *addr, uint8_t val);
	void write16aligned(Transaction *tx, uint16_t *addr, uint16_t val);
	void write32aligned(Transaction *tx, uint32_t *addr, uint32_t val);
	void write64aligned(Transaction *tx, uint64_t *addr, uint64_t val);

	void write_float_aligned(Transaction *tx, float *addr, float val);
	void write_double_aligned(Transaction *tx, double *addr, double val);

	// region
	void write_region(Transaction *tx, uint8_t *src, uintptr_t bytes, uint8_t *dest);

	// standard functions
	void *memset_tx(Transaction *tx, void *dest, int c, size_t n);
	void *memcpy_tx(Transaction *tx, void *dest, const void *src, size_t n);
	void *memmove_tx(Transaction *tx, void *dest, const void *src, size_t n);
	int strcmp_tx(Transaction *tx, const char *str_1, const char *str_2);
	int strncmp_tx(Transaction *tx, const char *str_1, const char *str_2, size_t n);
	void qsort_tx(Transaction *tx, void *base, size_t nel, size_t width,
				  int (*compar)(const void *, const void *));
}

////////////////////////////////
// read implementations start //
////////////////////////////////

inline uint8_t wlpdstm::read8(Transaction *tx, uint8_t *addr) {
	// this is fine as there are no unaligned read8s
	return read8aligned(tx, addr);
}

inline uint16_t wlpdstm::read16(Transaction *tx, uint16_t *addr) {
#ifdef WLPDSTM_32
	uintptr_t pos = (uintptr_t)addr & MASK_32;
	Word *first_word_addr = WORD32_ADDRESS(addr);
	read_write_conv_t value;
	value.b32[0] = (uint32_t)(tx->ReadWord(first_word_addr));

	if(pos == 0x0 || pos == 0x2) {
		return value.b16[pos >> SHIFT_16];
	} else if(pos == 0x1) {
		return value.mask_16.b16[0];
	} else {
		value.b32[1] = (uint32_t)(tx->ReadWord(first_word_addr + 1));
		return value.mask_16.b16[1];
	}
#elif defined WLPDSTM_64
	uintptr_t pos = (uintptr_t)addr & MASK_64;
	Word *first_word_addr = WORD64_ADDRESS(addr);
	read_write_conv_t value;
	value.b64[0] = (uint64_t)(tx->ReadWord(first_word_addr));
	
	if(pos == 0x0 || pos == 0x2 || pos == 0x4 || pos == 0x6) {
		return value.b16[pos >> SHIFT_16];
	} else if(pos == 0x1) {
		return value.mask_16.b16[0];
	} else if(pos == 0x3) {
		return value.mask_16.b16[1];
	} else if(pos == 0x5) {
		return value.mask_16.b16[2];
	} else {
		value.b64[1] = (uint64_t)(tx->ReadWord(first_word_addr + 1));
		return value.mask_16.b16[3];
	}
#endif /* word_size */
}

inline uint32_t wlpdstm::read32(Transaction *tx, uint32_t *addr) {
#ifdef WLPDSTM_32
	uintptr_t pos = (uintptr_t)addr & MASK_32;
	Word *first_word_addr = WORD32_ADDRESS(addr);
	read_write_conv_t value;
	value.b32[0] = (uint32_t)(tx->ReadWord(first_word_addr));

	if(pos == 0x0) {
		return value.b32[0];
	}

	value.b32[1] = (uint32_t)(tx->ReadWord(first_word_addr + 1));

	if(pos == 0x1) {
		return value.mask_32_1.b32[0];
	} else if(pos == 0x2) {
		return value.mask_32_2.b32[0];
	} else {
		return value.mask_32_3.b32[0];
	}

#elif defined WLPDSTM_64

	uintptr_t pos = (uintptr_t)addr & MASK_64;
	Word *first_word_addr = WORD64_ADDRESS(addr);
	read_write_conv_t value;
	value.b64[0] = (uint64_t)(tx->ReadWord(first_word_addr));

	if(pos == 0x0) {
		return value.b32[0];
	} else if(pos == 0x4) {
		return value.b32[1];
	} else if(pos == 0x1) {
		return value.mask_32_1.b32[0];
	} else if(pos == 0x2) {
		return value.mask_32_2.b32[0];
	} else if(pos == 0x3) {
		return value.mask_32_3.b32[0];
	} else {
		value.b64[1] = (uint64_t)(tx->ReadWord(first_word_addr + 1));

		if(pos == 0x5) {
			return value.mask_32_1.b32[1];
		} else if(pos == 0x6) {
			return value.mask_32_2.b32[1];
		} else {
			return value.mask_32_3.b32[1];
		}
	}
#endif /* word_size */
}

inline uint64_t wlpdstm::read64(Transaction *tx, uint64_t *addr) {
#ifdef WLPDSTM_32
	uintptr_t pos = (uintptr_t)addr & MASK_32;
	Word *first_word_addr = WORD32_ADDRESS(addr);
	read_write_conv_t value;
	value.b32[0] = (uint32_t)(tx->ReadWord(first_word_addr));
	value.b32[1] = (uint32_t)(tx->ReadWord(first_word_addr + 1));

	if(pos == 0x0) {
		return value.b64[0];
	} else {
		value.b32[2] = (uint32_t)(tx->ReadWord(first_word_addr + 2));

		if(pos == 0x1) {
			return value.mask_64_1.b64;
		} else if(pos == 0x2) {
			return value.mask_64_2.b64;
		} else {
			return value.mask_64_3.b64;
		}
	}
#elif defined WLPDSTM_64
	uintptr_t pos = (uintptr_t)addr & MASK_64;
	Word *first_word_addr = WORD64_ADDRESS(addr);
	read_write_conv_t value;
	value.b64[0] = (uint64_t)(tx->ReadWord(first_word_addr));

	if(pos == 0x0) {
		return value.b64[0];
	} else {
		value.b64[1] = (uint64_t)(tx->ReadWord(first_word_addr + 1));

		if(pos == 0x1) {
			return value.mask_64_1.b64;
		} else if(pos == 0x2) {
			return value.mask_64_2.b64;
		} else if(pos == 0x3) {
			return value.mask_64_3.b64;
		} else if(pos == 0x4) {
			return value.mask_64_4.b64;
		} else if(pos == 0x5) {
			return value.mask_64_5.b64;
		} else if(pos == 0x6) {
			return value.mask_64_6.b64;
		} else {
			return value.mask_64_7.b64;
		}
	}
#endif /* word_size */
}

// TODO: check whether it would be better to repeat the code from read_32
inline float wlpdstm::read_float(Transaction *tx, float *addr) {
	read_write_conv_t value;
	value.b32[0] = read32(tx, (uint32_t *)addr);
	return value.f[0];
}

inline double wlpdstm::read_double(Transaction *tx, double *addr) {
	read_write_conv_t value;
	value.b64[0] = read64(tx, (uint64_t *)addr);
	return value.d[0];	
}

#ifdef WLPDSTM_ICC
inline __m128 wlpdstm::read_m128(Transaction *tx, __m128 *addr) {
	read_write_conv_t value;
	uint64_t *addr64 = (uint64_t *)addr;
	value.b64[0] = read64(tx, addr64);
	value.b64[1] = read64(tx, addr64 + 1);
	return value.m128[0];
}
#endif /* WLPDSTM_ICC */

inline uint8_t wlpdstm::read8aligned(Transaction *tx, uint8_t *addr) {
#ifdef WLPDSTM_32
	read_write_conv_t value;
	value.b32[0] = (uint32_t)(tx->ReadWord(WORD32_ADDRESS(addr)));
	return value.b8[WORD32_8_POS(addr)];
#elif defined WLPDSTM_64
	read_write_conv_t value;
	value.b64[0] = (uint64_t)(tx->ReadWord(WORD64_ADDRESS(addr)));
	return value.b8[WORD64_8_POS(addr)];
#endif /* word_size */
}

inline uint16_t wlpdstm::read16aligned(Transaction *tx, uint16_t *addr) {
#ifdef WLPDSTM_32
	read_write_conv_t value;
	value.b32[0] = (uint32_t)(tx->ReadWord(WORD32_ADDRESS(addr)));
	return value.b16[WORD32_16_POS(addr)];
#elif defined WLPDSTM_64
	read_write_conv_t value;
	value.b64[0] = (uint64_t)(tx->ReadWord(WORD64_ADDRESS(addr)));
	return value.b16[WORD64_16_POS(addr)];
#endif /* word_size */	
}

inline uint32_t wlpdstm::read32aligned(Transaction *tx, uint32_t *addr) {
#ifdef WLPDSTM_32
	return tx->ReadWord((Word *)addr);
#elif defined WLPDSTM_64
	read_write_conv_t value;
	value.b64[0] = (uint64_t)(tx->ReadWord(WORD64_ADDRESS(addr)));
	return value.b32[WORD64_32_POS(addr)];
#endif /* word_size */	
}

inline uint64_t wlpdstm::read64aligned(Transaction *tx, uint64_t *addr) {
#ifdef WLPDSTM_32
	read_write_conv_t val;
	val.b32[0] = tx->ReadWord((Word *)addr);
	val.b32[1] = tx->ReadWord(((Word *)addr) + 1);
	return val.b64[0];		
#elif defined WLPDSTM_64
	return tx->ReadWord((Word *)addr);
#endif /* word_size */	
}

inline float wlpdstm::read_float_aligned(Transaction *tx, float *addr) {
#ifdef WLPDSTM_32
	read_write_conv_t value;
	value.b32[0] = tx->ReadWord((Word *)addr);
	return value.f[0];
#elif defined WLPDSTM_64
	read_write_conv_t value;
	value.b64[0] = (uint64_t)(tx->ReadWord(WORD64_ADDRESS(addr)));
	return value.f[WORD64_32_POS(addr)];
#endif /* word_size */	
}
		
inline double wlpdstm::read_double_aligned(Transaction *tx, double *addr) {
#ifdef WLPDSTM_32
	read_write_conv_t val;
	val.b32[0] = tx->ReadWord((Word *)addr);
	val.b32[1] = tx->ReadWord(((Word *)addr) + 1);
	return val.d[0];		
#elif defined WLPDSTM_64
	read_write_conv_t val;
	val.b64[0] = tx->ReadWord((Word *)addr);
	return val.d[0]; 
#endif /* word_size */
}

inline void wlpdstm::read_region(Transaction *tx, uint8_t *src, uintptr_t bytes, uint8_t *dest) {
	size_t bytes_read = 0;

	read_write_conv_t val;
	Word *next_word_addr = (Word *)((uintptr_t)src & ~MASK);
	uintptr_t first_word_start = (uintptr_t)src & MASK;
	Word *last_word_addr = (Word *)(((uintptr_t)src + bytes - 1) & ~MASK);
	uintptr_t last_word_end = ((uintptr_t)src + bytes - 1) & MASK;

	// if src is unaligned, read the first word
	if(first_word_start != 0x0 || next_word_addr == last_word_addr) {
		READ_WORD(next_word_addr, val, tx);
		uintptr_t count = 0;

		while(first_word_start < sizeof(Word) && count++ < bytes) {
			*dest++ = val.b8[first_word_start++];
			++bytes_read;
		}

		if(next_word_addr == last_word_addr) {
			return;
		}

		++next_word_addr;
	}

	// now read the aligned words
#ifdef ALLOW_UNALIGNED_ACCESSES
	Word *word_dest = (Word *)dest;
#endif /* ALLOW_UNALIGNED_ACCESSES */

	while(next_word_addr < last_word_addr) {
#ifdef ALLOW_UNALIGNED_ACCESSES
		*word_dest++ = tx->ReadWord(next_word_addr);
		bytes_read += sizeof(Word);
#else
		READ_WORD(next_word_addr, val, tx);

		for(unsigned i = 0;i < sizeof(Word);i++) {
			*dest++ = val.b8[i];
		}
#endif /* ALLOW_UNALIGNED_ACCESSES */
		++next_word_addr;
	}

	// deal with the last word
	// if not the whole word
	if((last_word_end & MASK) != MASK) {
#ifdef ALLOW_UNALIGNED_ACCESSES
		dest = (uint8_t *)word_dest;
#endif /* ALLOW_UNALIGNED_ACCESSES */
		READ_WORD(next_word_addr, val, tx);

		for(unsigned i = 0;i <= last_word_end;i++) {
			*dest++ = val.b8[i];
			++bytes_read;
		}
	} else {
#ifdef ALLOW_UNALIGNED_ACCESSES
		*word_dest = tx->ReadWord(next_word_addr);
		bytes_read += sizeof(Word);
#else
		READ_WORD(next_word_addr, val, tx);

		for(unsigned i = 0;i < sizeof(uintptr_t);i++) {
			*dest++ = val.b8[i];
		}		
#endif /* ALLOW_UNALIGNED_ACCESSES */
	}
}

//////////////////////////////
// read implementations end //
//////////////////////////////

/////////////////////////////////
// write implementations start //
/////////////////////////////////

inline void wlpdstm::write8(Transaction *tx, uint8_t *addr, uint8_t val8) {
	// byte writes are always aligned
	write8aligned(tx, addr, val8);
}

inline void wlpdstm::write16(Transaction *tx, uint16_t *addr, uint16_t val16) {
#ifdef WLPDSTM_32
	unsigned pos = (uintptr_t)addr & MASK_32;
	Word *first_word_addr = (Word *)((uintptr_t)addr & ~MASK_32);
	read_write_conv_t mask;
	read_write_conv_t val;
	mask.b32[0] = 0;

	if(pos == 0x0) {
		mask.b16[0] = ~0;
		val.b16[0] = val16;
		tx->WriteWord(first_word_addr, (Word)val.b32[0], (Word)mask.b32[0]);
	} else if(pos == 0x1) {
		mask.mask_16.b16[0] = ~0;
		val.mask_16.b16[0] = val16;
		tx->WriteWord(first_word_addr, (Word)val.b32[0], (Word)mask.b32[0]);
	} else if(pos == 0x2) {
		mask.b16[1] = ~0;
		val.b16[1] = val16;
		tx->WriteWord(first_word_addr, (Word)val.b32[0], (Word)mask.b32[0]);
	} else {
		mask.b32[1] = 0;
		mask.mask_16.b16[1] = ~0;
		val.mask_16.b16[1] = val16;
		tx->WriteWord(first_word_addr, (Word)val.b32[0], (Word)mask.b32[0]);
		tx->WriteWord(first_word_addr + 1, (Word)val.b32[1], (Word)mask.b32[1]);
	}
#elif defined WLPDSTM_64
	unsigned pos = (uintptr_t)addr & MASK_64;
	Word *first_word_addr = (Word *)((uintptr_t)addr & ~MASK_64);
	read_write_conv_t mask;
	read_write_conv_t val;
	mask.b64[0] = 0;

	if(pos == 0x0 || pos == 0x2 || pos == 0x4 || pos == 0x6) {
		mask.b16[pos >> SHIFT_16] = ~1;
		val.b16[pos >> SHIFT_16] = val16;
		tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
	} else if(pos == 0x1) {
		mask.mask_16.b16[0] = ~0;
		val.mask_16.b16[0] = val16;
		tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
	} else if(pos == 0x3) {
		mask.mask_16.b16[1] = ~0;
		val.mask_16.b16[1] = val16;
		tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
	} else if(pos == 0x5) {
		mask.mask_16.b16[2] = ~0;
		val.mask_16.b16[2] = val16;
		tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
	} else {
		mask.b64[1] = 0;
		mask.mask_16.b16[3] = ~0;
		val.mask_16.b16[3] = val16;
		tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
		tx->WriteWord(first_word_addr + 1, (Word)val.b64[1], (Word)mask.b64[1]);
	}
#endif /* word_size */		
}
	
inline void wlpdstm::write32(Transaction *tx, uint32_t *addr, uint32_t val32) {
#ifdef WLPDSTM_32
	unsigned pos = (uintptr_t)addr & MASK_32;
	Word *first_word_addr = (Word *)((uintptr_t)addr & ~MASK_32);

	if(pos == 0x0) {
		tx->WriteWord(first_word_addr, (Word)val32);
	} else {
		read_write_conv_t mask;
		read_write_conv_t val;
		mask.b32[0] = 0;
		mask.b32[1] = 0;

		if(pos == 0x1) {
			mask.mask_32_1.b32[0] = ~0;
			val.mask_32_1.b32[0] = val32;
		} else if(pos == 0x2) {
			mask.mask_32_2.b32[0] = ~0;
			val.mask_32_2.b32[0] = val32;
		} else {
			mask.mask_32_3.b32[0] = ~0;
			val.mask_32_3.b32[0] = val32;
		}

		tx->WriteWord(first_word_addr, (Word)val.b32[0], (Word)mask.b32[0]);
		tx->WriteWord(first_word_addr + 1, (Word)val.b32[1], (Word)mask.b32[1]);
	}
#elif defined WLPDSTM_64
	unsigned pos = (uintptr_t)addr & MASK_64;
	Word *first_word_addr = (Word *)((uintptr_t)addr & ~MASK_64);
	read_write_conv_t mask;
	read_write_conv_t val;
	mask.b64[0] = 0;

	if(pos == 0x0 || pos == 0x4) {
		mask.b32[pos >> SHIFT_32] = ~1;
		val.b32[pos >> SHIFT_32] = val32;
		tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
	} else if(pos == 0x1) {
		mask.mask_32_1.b32[0] = ~1;
		val.mask_32_1.b32[0] = val32;
		tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
	} else if(pos == 0x2) {
		mask.mask_32_2.b32[0] = ~1;
		val.mask_32_2.b32[0] = val32;
		tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
	} else if(pos == 0x3) {
		mask.mask_32_3.b32[0] = ~1;
		val.mask_32_3.b32[0] = val32;
		tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
	} else {
		mask.b64[1] = 0;

		if(pos == 0x5) {
			mask.mask_32_1.b32[1] = ~1;
			val.mask_32_1.b32[1] = val32;
			tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
			tx->WriteWord(first_word_addr + 1, (Word)val.b64[1], (Word)mask.b64[1]);
		} else if(pos == 0x6) {
			mask.mask_32_2.b32[1] = ~1;
			val.mask_32_2.b32[1] = val32;
			tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
			tx->WriteWord(first_word_addr + 1, (Word)val.b64[1], (Word)mask.b64[1]);
		} else {
			mask.mask_32_3.b32[1] = ~1;
			val.mask_32_3.b32[1] = val32;
			tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
			tx->WriteWord(first_word_addr + 1, (Word)val.b64[1], (Word)mask.b64[1]);			
		}
	}
#endif /* word_size */
}

inline void wlpdstm::write64(Transaction *tx, uint64_t *addr, uint64_t val64) {
#ifdef WLPDSTM_32
	unsigned pos = (uintptr_t)addr & MASK_32;
	Word *first_word_addr = (Word *)((uintptr_t)addr & ~MASK_32);
	read_write_conv_t val;
		
	if(pos == 0x0) {
		val.b64[0] = val64;
		tx->WriteWord(first_word_addr, (Word)val.b32[0]);
		tx->WriteWord(first_word_addr + 1, (Word)val.b32[1]);
	} else {
		read_write_conv_t mask;
		mask.b32[0] = 0;
		mask.b32[1] = 0;
		mask.b32[2] = 0;

		if(pos == 0x1) {
			mask.mask_64_1.b64 = ~0;
			val.mask_64_1.b64 = val64;
		} else if(pos == 0x2) {
			mask.mask_64_2.b64 = ~0;
			val.mask_64_2.b64 = val64;
		} else {
			mask.mask_64_3.b64 = ~0;
			val.mask_64_3.b64 = val64;
		}

		tx->WriteWord(first_word_addr, (Word)val.b32[0], (Word)mask.b32[0]);
		tx->WriteWord(first_word_addr + 1, (Word)val.b32[1], (Word)mask.b32[1]);
		tx->WriteWord(first_word_addr + 2, (Word)val.b32[2], (Word)mask.b32[2]);
	}
#elif defined WLPDSTM_64
	unsigned pos = (uintptr_t)addr & MASK_64;
		
	if(pos == 0x0) {
		tx->WriteWord((Word *)addr, (Word)val64);
	} else {
		Word *first_word_addr = (Word *)((uintptr_t)addr & ~MASK_64);
		read_write_conv_t val;
		read_write_conv_t mask;
		mask.b64[0] = 0;
		mask.b64[1] = 0;

		if(pos == 0x1) {
			mask.mask_64_1.b64 = ~0;
			val.mask_64_1.b64 = val64;
		} else if(pos == 0x2) {
			mask.mask_64_2.b64 = ~0;
			val.mask_64_2.b64 = val64;
		} else if(pos == 0x3) {
			mask.mask_64_3.b64 = ~0;
			val.mask_64_3.b64 = val64;
		} else if(pos == 0x4) {
			mask.mask_64_4.b64 = ~0;
			val.mask_64_4.b64 = val64;
		} else if(pos == 0x5) {
			mask.mask_64_5.b64 = ~0;
			val.mask_64_5.b64 = val64;
		} else if(pos == 0x6) {
			mask.mask_64_6.b64 = ~0;
			val.mask_64_6.b64 = val64;
		} else {
			mask.mask_64_7.b64 = ~0;
			val.mask_64_7.b64 = val64;
		}

		tx->WriteWord(first_word_addr, (Word)val.b64[0], (Word)mask.b64[0]);
		tx->WriteWord(first_word_addr + 1, (Word)val.b64[1], (Word)mask.b64[1]);
	}
#endif /* word_size */
}

// TODO: check if this should be decomposed maybe
inline void wlpdstm::write_float(Transaction *tx, float *addr, float valf) {
	read_write_conv_t val;
	val.f[0] = valf;
	write32(tx, (uint32_t *)addr, val.b32[0]);
}

inline void wlpdstm::write_double(Transaction *tx, double *addr, double vald) {
	read_write_conv_t val;
	val.d[0] = vald;
	write64(tx, (uint64_t *)addr, val.b64[0]);	
}

#ifdef WLPDSTM_ICC
inline void wlpdstm::write_m128(Transaction *tx, __m128 *addr, __m128 val) {
	read_write_conv_t value;
	value.m128[0] = val;
	uint64_t *addr64 = (uint64_t *)addr;
	write64(tx, addr64, value.b64[0]);
	write64(tx, addr64 + 1, value.b64[1]);
}
#endif /* WLPDSTM_ICC */

inline void wlpdstm::write8aligned(Transaction *tx, uint8_t *addr, uint8_t val8) {
#ifdef WLPDSTM_32
	unsigned pos = ((uintptr_t)addr & MASK_32) >> SHIFT_8;
	read_write_conv_t mask;
	mask.b32[0] = 0;
	mask.b8[pos] = ~0;
	read_write_conv_t val;
	val.b8[pos] = val8;
	tx->WriteWord((Word *)(((uintptr_t)addr) & ~MASK_32), (Word)val.b32[0], (Word)mask.b32[0]);
#elif defined WLPDSTM_64
	unsigned pos = ((uintptr_t)addr & MASK_64) >> SHIFT_8;
	read_write_conv_t mask;
	mask.b64[0] = 0;
	mask.b8[pos] = ~0;
	read_write_conv_t val;
	val.b8[pos] = val8;
	tx->WriteWord((Word *)(((uintptr_t)addr) & ~MASK_64), (Word)val.b64[0], (Word)mask.b64[0]);
#endif /* word_size */
}
	
inline void wlpdstm::write16aligned(Transaction *tx, uint16_t *addr, uint16_t val16) {
#ifdef WLPDSTM_32
	unsigned pos = ((uintptr_t)addr & MASK_32) >> SHIFT_16;
	read_write_conv_t mask;
	mask.b32[0] = 0;
	mask.b16[pos] = ~0;
	read_write_conv_t val;
	val.b16[pos] = val16;
	tx->WriteWord((Word *)(((uintptr_t)addr) & ~MASK_32), (Word)val.b32[0], (Word)mask.b32[0]);
#elif defined WLPDSTM_64
	unsigned pos = ((uintptr_t)addr & MASK_64) >> SHIFT_16;
	read_write_conv_t mask;
	mask.b64[0] = 0;
	mask.b16[pos] = ~0;
	read_write_conv_t val;
	val.b16[pos] = val16;
	tx->WriteWord((Word *)(((uintptr_t)addr) & ~MASK_64), (Word)val.b64[0], (Word)mask.b64[0]);
#endif /* word_size */		
}

inline void wlpdstm::write32aligned(Transaction *tx, uint32_t *addr, uint32_t val32) {
#ifdef WLPDSTM_32
	tx->WriteWord((Word *)addr, (Word)val32);
#elif defined WLPDSTM_64
	unsigned pos = ((uintptr_t)addr & MASK_64) >> SHIFT_32;
	read_write_conv_t mask;
	mask.b64[0] = 0;
	mask.b32[pos] = ~0;
	read_write_conv_t val;
	val.b32[pos] = val32;
	tx->WriteWord((Word *)(((uintptr_t)addr) & ~MASK_64), val.b64[0], mask.b64[0]);
#endif /* word_size */
}

inline void wlpdstm::write64aligned(Transaction *tx, uint64_t *addr, uint64_t val64) {
#ifdef WLPDSTM_32
	read_write_conv_t val;
	val.b64[0] = val64;
	tx->WriteWord((Word *)addr, val.b32[0]);
	tx->WriteWord(((Word *)addr) + 1, val.b32[1]);		
#elif defined WLPDSTM_64
	tx->WriteWord((Word *)addr, (Word)val64);
#endif /* word_size */
}
	
inline void wlpdstm::write_float_aligned(Transaction *tx, float *addr, float valf) {
#ifdef WLPDSTM_32
	read_write_conv_t val;
	val.f[0] = valf;
	tx->WriteWord((Word *)addr, (Word)val.b32[0]);
#elif defined WLPDSTM_64
	unsigned pos = ((uintptr_t)addr & MASK_64) >> SHIFT_32;
	read_write_conv_t mask;
	mask.b64[0] = 0;
	mask.b32[pos] = ~0;
	read_write_conv_t val;
	val.f[pos] = valf;
	tx->WriteWord((Word *)(((uintptr_t)addr) & ~MASK_64), val.b64[0], mask.b64[0]);
#endif /* word_size */
}

inline void wlpdstm::write_double_aligned(Transaction *tx, double *addr, double vald) {
#ifdef WLPDSTM_32
	read_write_conv_t val;
	val.d[0] = vald;
	tx->WriteWord((Word *)addr, val.b32[0]);
	tx->WriteWord(((Word *)addr) + 1, val.b32[1]);
#elif defined WLPDSTM_64
	read_write_conv_t val;
	val.d[0] = vald;
	tx->WriteWord((Word *)addr, (Word)val.b64[0]);
#endif /* word_size */
}

inline void wlpdstm::write_region(Transaction *tx, uint8_t *src, uintptr_t bytes, uint8_t *dest) {
	unsigned bytes_written = 0;

	read_write_conv_t val;
	read_write_conv_t mask;
	Word *next_word_addr = (Word *)((uintptr_t)dest & ~MASK);
	uintptr_t first_word_start = (uintptr_t)dest & MASK;
	Word *last_word_addr = (Word *)(((uintptr_t)dest + bytes - 1) & ~MASK);
	uintptr_t last_word_end = ((uintptr_t)dest + bytes - 1) & MASK;

	// write the first unaligned word
	if(first_word_start != 0x0 || next_word_addr == last_word_addr) {
		mask.WORD_FIELD_NAME[0] = 0;
		unsigned count = 0;

		while(first_word_start < sizeof(Word) && count++ < bytes) {
			mask.b8[first_word_start] = ~0;
			val.b8[first_word_start] = *src++;
			++first_word_start;
			++bytes_written;
		}

		tx->WriteWord(next_word_addr, (Word)val.WORD_FIELD_NAME[0], (Word)mask.WORD_FIELD_NAME[0]);

		if(next_word_addr == last_word_addr) {
			return;
		}

		++next_word_addr;
	}

#ifdef ALLOW_UNALIGNED_ACCESSES
	Word *next_src_addr = (Word *)src;
#endif /* ALLOW_UNALIGNED_ACCESSES */

	// write aligned words
	while(next_word_addr < last_word_addr) {
#ifdef ALLOW_UNALIGNED_ACCESSES
		tx->WriteWord(next_word_addr, *next_src_addr);
		++next_src_addr;
		bytes_written += sizeof(Word);
#else
		for(uintptr_t i = 0;i < sizeof(Word);i++) {
			val.b8[i] = *src++;
		}

		tx->WriteWord(next_word_addr, val.WORD_FIELD_NAME[0]);
#endif /* ALLOW_UNALIGNED_ACCESSES */
		++next_word_addr;
	}

	// write the last word
	// unaligned
	if((last_word_end & MASK) != MASK) {
#ifdef ALLOW_UNALIGNED_ACCESSES
		src = (uint8_t *)next_src_addr;
#endif /* ALLOW_UNALIGNED_ACCESSES */
		mask.WORD_FIELD_NAME[0] = 0;

		for(uintptr_t i = 0;i <= last_word_end;i++) {
			mask.b8[i] = ~0;
			val.b8[i] = *src++;
			++bytes_written;
		}

		tx->WriteWord(next_word_addr, (Word)val.WORD_FIELD_NAME[0], (Word)mask.WORD_FIELD_NAME[0]);
	} else {
#ifdef ALLOW_UNALIGNED_ACCESSES
		tx->WriteWord(next_word_addr, (Word)*next_src_addr);
		bytes_written += sizeof(Word);
#else
		for(uintptr_t i = 0;i < sizeof(Word);i++) {
			val.b8[i] = *src++;
			++bytes_written;
		}

		tx->WriteWord(next_word_addr, (Word)val.WORD_FIELD_NAME[0]);
#endif /* ALLOW_UNALIGNED_ACCESSES */
	}
}

///////////////////////////////
// write implementations end //
///////////////////////////////

/////////////////////////////////////////////
// tx versions of standard functions start //
/////////////////////////////////////////////

inline void *wlpdstm::memset_tx(Transaction *tx, void *dest, int c, size_t n) {
	read_write_conv_t val;
	read_write_conv_t mask;
	Word *next_word_addr = (Word *)((uintptr_t)dest & ~MASK);
	uintptr_t first_word_start = (uintptr_t)dest & MASK;
	Word *last_word_addr = (Word *)(((uintptr_t)dest + n - 1) & ~MASK);
	uintptr_t last_word_end = ((uintptr_t)dest + n - 1) & MASK;
	uint8_t uch = (uint8_t)c;
	unsigned i;

	// prepare word for writing
	for(i = 0;i < sizeof(Word);i++) {
		val.b8[i] = uch;
	}
	
	// deal with the first word
	if(first_word_start != 0x0) {
		mask.WORD_FIELD_NAME[0] = 0;

		for(i = first_word_start;i < sizeof(Word);i++) {
			mask.b8[i] = ~0;
		}

		tx->WriteWord(next_word_addr, (Word)val.WORD_FIELD_NAME[0], (Word)mask.WORD_FIELD_NAME[0]);

		if(next_word_addr == last_word_addr) {
			return dest;
		}

		next_word_addr++;
	}
	
	// write words before the last
	while(next_word_addr < last_word_addr) {
		tx->WriteWord(next_word_addr, (Word)val.WORD_FIELD_NAME[0]);
		next_word_addr++;
	}

	// write the last word
	// unaligned
	if((last_word_end & MASK) != MASK) {
		mask.WORD_FIELD_NAME[0] = 0;

		for(i = 0;i <= last_word_end;i++) {
			mask.b8[i] = ~0;
		}

		tx->WriteWord(next_word_addr, (Word)val.WORD_FIELD_NAME[0], (Word)mask.WORD_FIELD_NAME[0]);
	} else {
		tx->WriteWord(next_word_addr, (Word)val.WORD_FIELD_NAME[0]);
	}

	return dest;
}

// TODO: think about implementing memcpy and memmove differently
inline void *wlpdstm::memcpy_tx(Transaction *tx, void *dest, const void *src, size_t n) {
	return memmove_tx(tx, dest, src, n);
}

inline void *wlpdstm::memmove_tx(Transaction *tx, void *dest, const void *src, size_t n) {
	uint8_t *buf = (uint8_t *)MemoryManager::Malloc(n);
	read_region(tx, (uint8_t *)src, n, buf);
	write_region(tx, buf, n, (uint8_t *)dest);
	MemoryManager::Free(buf);
	return dest;
}

inline int wlpdstm::strcmp_tx(Transaction *tx, const char *str_1, const char *str_2) {
	Word *next_word_addr_1 = (Word *)((uintptr_t)str_1 & ~MASK);
	Word *next_word_addr_2 = (Word *)((uintptr_t)str_2 & ~MASK);
	uintptr_t i_1 = (uintptr_t)str_1 & MASK;
	uintptr_t i_2 = (uintptr_t)str_2 & MASK;
	read_write_conv_t val_1, val_2;
	int ret;

	val_1.WORD_FIELD_NAME[0] = tx->ReadWord(next_word_addr_1++);
	val_2.WORD_FIELD_NAME[0] = tx->ReadWord(next_word_addr_2++);

	while(true) {
		while(i_1 < sizeof(Word) && i_2 < sizeof(Word)) {
			if(val_1.b8[i_1] != val_2.b8[i_2]) {
				ret = val_1.b8[i_1] - val_2.b8[i_2];
				goto return_point;
			}

			if(val_1.b8[i_1] == '\0') {
				ret = 0;
				goto return_point;
			}

			i_1++;
			i_2++;
		}

		if(i_1 == sizeof(Word)) {
			val_1.WORD_FIELD_NAME[0] = tx->ReadWord(next_word_addr_1++);
			i_1 = 0;
		}

		if(i_2 == sizeof(Word)) {
			val_2.WORD_FIELD_NAME[0] = tx->ReadWord(next_word_addr_2++);
			i_2 = 0;
		}
	}

return_point:
	return ret;
}

inline int wlpdstm::strncmp_tx(Transaction *tx, const char *str_1, const char *str_2, size_t n) {
	if(n == 0) {
		return 0;
	}

	Word *next_word_addr_1 = (Word *)((uintptr_t)str_1 & ~MASK);
	Word *next_word_addr_2 = (Word *)((uintptr_t)str_2 & ~MASK);
	uintptr_t i_1 = (uintptr_t)str_1 & MASK;
	uintptr_t i_2 = (uintptr_t)str_2 & MASK;
	read_write_conv_t val_1, val_2;
	int ret;
	size_t count = 0;
	
	val_1.WORD_FIELD_NAME[0] = tx->ReadWord(next_word_addr_1++);
	val_2.WORD_FIELD_NAME[0] = tx->ReadWord(next_word_addr_2++);
	
	while(true) {
		while(i_1 < sizeof(Word) && i_2 < sizeof(Word)) {
			if(val_1.b8[i_1] != val_2.b8[i_2]) {
				ret = val_1.b8[i_1] - val_2.b8[i_2];
				goto return_point;
			}
			
			if(val_1.b8[i_1] == '\0' || count == n - 1) {
				ret = 0;
				goto return_point;
			}
			
			i_1++;
			i_2++;
			count ++;
		}
		
		if(i_1 == sizeof(Word)) {
			val_1.WORD_FIELD_NAME[0] = tx->ReadWord(next_word_addr_1++);
			i_1 = 0;
		}
		
		if(i_2 == sizeof(Word)) {
			val_2.WORD_FIELD_NAME[0] = tx->ReadWord(next_word_addr_2++);
			i_2 = 0;
		}
	}
	
return_point:
	return ret;
}

inline void wlpdstm::qsort_tx(Transaction *tx, void *base, size_t nel, size_t width,
							  int (*compar)(const void *, const void *)) {
	size_t size = nel * width;
	tx->LockMemoryBlock(base, size);
	uint8_t *buf = (uint8_t *)MemoryManager::Malloc(size);
	read_region(tx, (uint8_t *)base, size, buf);
	qsort(buf, nel, width, compar);
	write_region(tx, buf, size, (uint8_t *)base);
	MemoryManager::Free(buf);
}

///////////////////////////////////////////
// tx versions of standard functions end //
///////////////////////////////////////////
