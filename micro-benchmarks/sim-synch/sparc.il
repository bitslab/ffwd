.inline MEMBAR_ALL, 0
	.volatile  
	membar #StoreStore | #LoadLoad | #LoadStore | #StoreLoad
	nop
.end

.inline MEMBAR_STORE
	.volatile  
	membar #StoreLoad
	nop
.end

.inline MEMBAR_LOAD
	.volatile  
	nop
.end

.inline CASPO
	.volatile  
	casx [%o0], %o1, %o2
	nop
	mov %o2, %o0
.end

.inline SWAPPO
	.volatile  
	swap [%o0], %o1
	nop
	mov %o1, %o0
.end

.inline POPC
	.volatile  
	popc %o0, %o1
	mov %o1, %o0
.end

.inline NOP
	.volatile  
	nop
.end