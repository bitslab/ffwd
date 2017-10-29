
fccsynch.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <liblock_fccsynch_destroy_lock>:
{
    /* No need to unmap impl ? */
//	munmap(lock->impl->tail, r_align(sizeof(fccsynch_node), PAGE_SIZE));

    return 0;
}
   0:	31 c0                	xor    eax,eax
   2:	c3                   	ret    
   3:	66 66 66 66 2e 0f 1f 	data32 data32 data32 nop WORD PTR cs:[rax+rax*1+0x0]
   a:	84 00 00 00 00 00 

0000000000000010 <liblock_fccsynch_init_library>:
	//SFENCE();
	return cur_node->ret;
}

static void do_liblock_init_library(fccsynch)()
{}
  10:	f3 c3                	repz ret 
  12:	66 66 66 66 66 2e 0f 	data32 data32 data32 data32 nop WORD PTR cs:[rax+rax*1+0x0]
  19:	1f 84 00 00 00 00 00 

0000000000000020 <liblock_fccsynch_on_thread_exit>:
}

static void do_liblock_on_thread_exit(fccsynch)(struct thread_descriptor *desc)
{
//    munmap(my_node, r_align(sizeof(fccsynch_node), PAGE_SIZE));
}
  20:	f3 c3                	repz ret 
  22:	66 66 66 66 66 2e 0f 	data32 data32 data32 data32 nop WORD PTR cs:[rax+rax*1+0x0]
  29:	1f 84 00 00 00 00 00 

0000000000000030 <liblock_fccsynch_declare_server>:
{
    fatal("not implemented");
}

static void do_liblock_declare_server(fccsynch)(struct core *core)
{}
  30:	f3 c3                	repz ret 
  32:	66 66 66 66 66 2e 0f 	data32 data32 data32 data32 nop WORD PTR cs:[rax+rax*1+0x0]
  39:	1f 84 00 00 00 00 00 

0000000000000040 <liblock_fccsynch_relock_in_cs>:
{
    fatal("not implemented");
}

static void do_liblock_relock_in_cs(fccsynch)(liblock_lock_t *lock)
{
  40:	53                   	push   rbx
    fatal("not implemented");
  41:	48 8b 1d 00 00 00 00 	mov    rbx,QWORD PTR [rip+0x0]        # 48 <liblock_fccsynch_relock_in_cs+0x8>

# ifdef __va_arg_pack
__extern_always_inline int
fprintf (FILE *__restrict __stream, __const char *__restrict __fmt, ...)
{
  return __fprintf_chk (__stream, __USE_FORTIFY_LEVEL - 1, __fmt,
  48:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 4f <liblock_fccsynch_relock_in_cs+0xf>
  4f:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 56 <liblock_fccsynch_relock_in_cs+0x16>
  56:	be 01 00 00 00       	mov    esi,0x1
  5b:	31 c0                	xor    eax,eax
  5d:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
  60:	e8 00 00 00 00       	call   65 <liblock_fccsynch_relock_in_cs+0x25>
  65:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
  68:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # 6f <liblock_fccsynch_relock_in_cs+0x2f>
  6f:	ba 0f 00 00 00       	mov    edx,0xf
  74:	be 01 00 00 00       	mov    esi,0x1
  79:	e8 00 00 00 00       	call   7e <liblock_fccsynch_relock_in_cs+0x3e>
  7e:	48 8b 33             	mov    rsi,QWORD PTR [rbx]
  81:	bf 0a 00 00 00       	mov    edi,0xa
  86:	e8 00 00 00 00       	call   8b <liblock_fccsynch_relock_in_cs+0x4b>
  8b:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
  8e:	4c 8d 0d 00 00 00 00 	lea    r9,[rip+0x0]        # 95 <liblock_fccsynch_relock_in_cs+0x55>
  95:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 9c <liblock_fccsynch_relock_in_cs+0x5c>
  9c:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # a3 <liblock_fccsynch_relock_in_cs+0x63>
  a3:	41 b8 04 01 00 00    	mov    r8d,0x104
  a9:	be 01 00 00 00       	mov    esi,0x1
  ae:	31 c0                	xor    eax,eax
  b0:	e8 00 00 00 00       	call   b5 <liblock_fccsynch_relock_in_cs+0x75>
  b5:	e8 00 00 00 00       	call   ba <liblock_fccsynch_relock_in_cs+0x7a>
  ba:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

00000000000000c0 <liblock_fccsynch_unlock_in_cs>:
{
//    munmap(my_node, r_align(sizeof(fccsynch_node), PAGE_SIZE));
}

static void do_liblock_unlock_in_cs(fccsynch)(liblock_lock_t *lock)
{
  c0:	53                   	push   rbx
    fatal("not implemented");
  c1:	48 8b 1d 00 00 00 00 	mov    rbx,QWORD PTR [rip+0x0]        # c8 <liblock_fccsynch_unlock_in_cs+0x8>
  c8:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # cf <liblock_fccsynch_unlock_in_cs+0xf>
  cf:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # d6 <liblock_fccsynch_unlock_in_cs+0x16>
  d6:	be 01 00 00 00       	mov    esi,0x1
  db:	31 c0                	xor    eax,eax
  dd:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
  e0:	e8 00 00 00 00       	call   e5 <liblock_fccsynch_unlock_in_cs+0x25>
  e5:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
  e8:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # ef <liblock_fccsynch_unlock_in_cs+0x2f>
  ef:	ba 0f 00 00 00       	mov    edx,0xf
  f4:	be 01 00 00 00       	mov    esi,0x1
  f9:	e8 00 00 00 00       	call   fe <liblock_fccsynch_unlock_in_cs+0x3e>
  fe:	48 8b 33             	mov    rsi,QWORD PTR [rbx]
 101:	bf 0a 00 00 00       	mov    edi,0xa
 106:	e8 00 00 00 00       	call   10b <liblock_fccsynch_unlock_in_cs+0x4b>
 10b:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 10e:	4c 8d 0d 00 00 00 00 	lea    r9,[rip+0x0]        # 115 <liblock_fccsynch_unlock_in_cs+0x55>
 115:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 11c <liblock_fccsynch_unlock_in_cs+0x5c>
 11c:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 123 <liblock_fccsynch_unlock_in_cs+0x63>
 123:	41 b8 ff 00 00 00    	mov    r8d,0xff
 129:	be 01 00 00 00       	mov    esi,0x1
 12e:	31 c0                	xor    eax,eax
 130:	e8 00 00 00 00       	call   135 <liblock_fccsynch_unlock_in_cs+0x75>
 135:	e8 00 00 00 00       	call   13a <liblock_fccsynch_unlock_in_cs+0x7a>
 13a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

0000000000000140 <liblock_fccsynch_cond_destroy>:
{
    fatal("not implemented");
}

static int do_liblock_cond_destroy(fccsynch)(liblock_cond_t *cond)
{
 140:	53                   	push   rbx
    fatal("not implemented"); 
 141:	48 8b 1d 00 00 00 00 	mov    rbx,QWORD PTR [rip+0x0]        # 148 <liblock_fccsynch_cond_destroy+0x8>
 148:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 14f <liblock_fccsynch_cond_destroy+0xf>
 14f:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 156 <liblock_fccsynch_cond_destroy+0x16>
 156:	be 01 00 00 00       	mov    esi,0x1
 15b:	31 c0                	xor    eax,eax
 15d:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 160:	e8 00 00 00 00       	call   165 <liblock_fccsynch_cond_destroy+0x25>
 165:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
 168:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # 16f <liblock_fccsynch_cond_destroy+0x2f>
 16f:	ba 0f 00 00 00       	mov    edx,0xf
 174:	be 01 00 00 00       	mov    esi,0x1
 179:	e8 00 00 00 00       	call   17e <liblock_fccsynch_cond_destroy+0x3e>
 17e:	48 8b 33             	mov    rsi,QWORD PTR [rbx]
 181:	bf 0a 00 00 00       	mov    edi,0xa
 186:	e8 00 00 00 00       	call   18b <liblock_fccsynch_cond_destroy+0x4b>
 18b:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 18e:	4c 8d 0d 00 00 00 00 	lea    r9,[rip+0x0]        # 195 <liblock_fccsynch_cond_destroy+0x55>
 195:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 19c <liblock_fccsynch_cond_destroy+0x5c>
 19c:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 1a3 <liblock_fccsynch_cond_destroy+0x63>
 1a3:	41 b8 ea 00 00 00    	mov    r8d,0xea
 1a9:	be 01 00 00 00       	mov    esi,0x1
 1ae:	31 c0                	xor    eax,eax
 1b0:	e8 00 00 00 00       	call   1b5 <liblock_fccsynch_cond_destroy+0x75>
 1b5:	e8 00 00 00 00       	call   1ba <liblock_fccsynch_cond_destroy+0x7a>
 1ba:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

00000000000001c0 <liblock_fccsynch_cond_broadcast>:
{ 
    fatal("not implemented");
}

static int do_liblock_cond_broadcast(fccsynch)(liblock_cond_t *cond)
{
 1c0:	53                   	push   rbx
    fatal("not implemented");
 1c1:	48 8b 1d 00 00 00 00 	mov    rbx,QWORD PTR [rip+0x0]        # 1c8 <liblock_fccsynch_cond_broadcast+0x8>
 1c8:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 1cf <liblock_fccsynch_cond_broadcast+0xf>
 1cf:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 1d6 <liblock_fccsynch_cond_broadcast+0x16>
 1d6:	be 01 00 00 00       	mov    esi,0x1
 1db:	31 c0                	xor    eax,eax
 1dd:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 1e0:	e8 00 00 00 00       	call   1e5 <liblock_fccsynch_cond_broadcast+0x25>
 1e5:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
 1e8:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # 1ef <liblock_fccsynch_cond_broadcast+0x2f>
 1ef:	ba 0f 00 00 00       	mov    edx,0xf
 1f4:	be 01 00 00 00       	mov    esi,0x1
 1f9:	e8 00 00 00 00       	call   1fe <liblock_fccsynch_cond_broadcast+0x3e>
 1fe:	48 8b 33             	mov    rsi,QWORD PTR [rbx]
 201:	bf 0a 00 00 00       	mov    edi,0xa
 206:	e8 00 00 00 00       	call   20b <liblock_fccsynch_cond_broadcast+0x4b>
 20b:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 20e:	4c 8d 0d 00 00 00 00 	lea    r9,[rip+0x0]        # 215 <liblock_fccsynch_cond_broadcast+0x55>
 215:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 21c <liblock_fccsynch_cond_broadcast+0x5c>
 21c:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 223 <liblock_fccsynch_cond_broadcast+0x63>
 223:	41 b8 e5 00 00 00    	mov    r8d,0xe5
 229:	be 01 00 00 00       	mov    esi,0x1
 22e:	31 c0                	xor    eax,eax
 230:	e8 00 00 00 00       	call   235 <liblock_fccsynch_cond_broadcast+0x75>
 235:	e8 00 00 00 00       	call   23a <liblock_fccsynch_cond_broadcast+0x7a>
 23a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

0000000000000240 <liblock_fccsynch_cond_signal>:
{ 
    fatal("not implemented");
}

static int do_liblock_cond_signal(fccsynch)(liblock_cond_t *cond)
{ 
 240:	53                   	push   rbx
    fatal("not implemented");
 241:	48 8b 1d 00 00 00 00 	mov    rbx,QWORD PTR [rip+0x0]        # 248 <liblock_fccsynch_cond_signal+0x8>
 248:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 24f <liblock_fccsynch_cond_signal+0xf>
 24f:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 256 <liblock_fccsynch_cond_signal+0x16>
 256:	be 01 00 00 00       	mov    esi,0x1
 25b:	31 c0                	xor    eax,eax
 25d:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 260:	e8 00 00 00 00       	call   265 <liblock_fccsynch_cond_signal+0x25>
 265:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
 268:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # 26f <liblock_fccsynch_cond_signal+0x2f>
 26f:	ba 0f 00 00 00       	mov    edx,0xf
 274:	be 01 00 00 00       	mov    esi,0x1
 279:	e8 00 00 00 00       	call   27e <liblock_fccsynch_cond_signal+0x3e>
 27e:	48 8b 33             	mov    rsi,QWORD PTR [rbx]
 281:	bf 0a 00 00 00       	mov    edi,0xa
 286:	e8 00 00 00 00       	call   28b <liblock_fccsynch_cond_signal+0x4b>
 28b:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 28e:	4c 8d 0d 00 00 00 00 	lea    r9,[rip+0x0]        # 295 <liblock_fccsynch_cond_signal+0x55>
 295:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 29c <liblock_fccsynch_cond_signal+0x5c>
 29c:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 2a3 <liblock_fccsynch_cond_signal+0x63>
 2a3:	41 b8 e0 00 00 00    	mov    r8d,0xe0
 2a9:	be 01 00 00 00       	mov    esi,0x1
 2ae:	31 c0                	xor    eax,eax
 2b0:	e8 00 00 00 00       	call   2b5 <liblock_fccsynch_cond_signal+0x75>
 2b5:	e8 00 00 00 00       	call   2ba <liblock_fccsynch_cond_signal+0x7a>
 2ba:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

00000000000002c0 <liblock_fccsynch_cond_timedwait>:
}

static int do_liblock_cond_timedwait(fccsynch)(liblock_cond_t *cond,
                                              liblock_lock_t *lock,
                                              const struct timespec *ts)
{ 
 2c0:	53                   	push   rbx
    fatal("not implemented");
 2c1:	48 8b 1d 00 00 00 00 	mov    rbx,QWORD PTR [rip+0x0]        # 2c8 <liblock_fccsynch_cond_timedwait+0x8>
 2c8:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 2cf <liblock_fccsynch_cond_timedwait+0xf>
 2cf:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 2d6 <liblock_fccsynch_cond_timedwait+0x16>
 2d6:	be 01 00 00 00       	mov    esi,0x1
 2db:	31 c0                	xor    eax,eax
 2dd:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 2e0:	e8 00 00 00 00       	call   2e5 <liblock_fccsynch_cond_timedwait+0x25>
 2e5:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
 2e8:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # 2ef <liblock_fccsynch_cond_timedwait+0x2f>
 2ef:	ba 0f 00 00 00       	mov    edx,0xf
 2f4:	be 01 00 00 00       	mov    esi,0x1
 2f9:	e8 00 00 00 00       	call   2fe <liblock_fccsynch_cond_timedwait+0x3e>
 2fe:	48 8b 33             	mov    rsi,QWORD PTR [rbx]
 301:	bf 0a 00 00 00       	mov    edi,0xa
 306:	e8 00 00 00 00       	call   30b <liblock_fccsynch_cond_timedwait+0x4b>
 30b:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 30e:	4c 8d 0d 00 00 00 00 	lea    r9,[rip+0x0]        # 315 <liblock_fccsynch_cond_timedwait+0x55>
 315:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 31c <liblock_fccsynch_cond_timedwait+0x5c>
 31c:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 323 <liblock_fccsynch_cond_timedwait+0x63>
 323:	41 b8 d5 00 00 00    	mov    r8d,0xd5
 329:	be 01 00 00 00       	mov    esi,0x1
 32e:	31 c0                	xor    eax,eax
 330:	e8 00 00 00 00       	call   335 <liblock_fccsynch_cond_timedwait+0x75>
 335:	e8 00 00 00 00       	call   33a <liblock_fccsynch_cond_timedwait+0x7a>
 33a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

0000000000000340 <liblock_fccsynch_cond_wait>:
}

static int do_liblock_cond_wait(fccsynch)(liblock_cond_t *cond,
									     liblock_lock_t *lock)
{ 
 340:	53                   	push   rbx
    fatal("not implemented");
 341:	48 8b 1d 00 00 00 00 	mov    rbx,QWORD PTR [rip+0x0]        # 348 <liblock_fccsynch_cond_wait+0x8>
 348:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 34f <liblock_fccsynch_cond_wait+0xf>
 34f:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 356 <liblock_fccsynch_cond_wait+0x16>
 356:	be 01 00 00 00       	mov    esi,0x1
 35b:	31 c0                	xor    eax,eax
 35d:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 360:	e8 00 00 00 00       	call   365 <liblock_fccsynch_cond_wait+0x25>
 365:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
 368:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # 36f <liblock_fccsynch_cond_wait+0x2f>
 36f:	ba 0f 00 00 00       	mov    edx,0xf
 374:	be 01 00 00 00       	mov    esi,0x1
 379:	e8 00 00 00 00       	call   37e <liblock_fccsynch_cond_wait+0x3e>
 37e:	48 8b 33             	mov    rsi,QWORD PTR [rbx]
 381:	bf 0a 00 00 00       	mov    edi,0xa
 386:	e8 00 00 00 00       	call   38b <liblock_fccsynch_cond_wait+0x4b>
 38b:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 38e:	4c 8d 0d 00 00 00 00 	lea    r9,[rip+0x0]        # 395 <liblock_fccsynch_cond_wait+0x55>
 395:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 39c <liblock_fccsynch_cond_wait+0x5c>
 39c:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 3a3 <liblock_fccsynch_cond_wait+0x63>
 3a3:	41 b8 db 00 00 00    	mov    r8d,0xdb
 3a9:	be 01 00 00 00       	mov    esi,0x1
 3ae:	31 c0                	xor    eax,eax
 3b0:	e8 00 00 00 00       	call   3b5 <liblock_fccsynch_cond_wait+0x75>
 3b5:	e8 00 00 00 00       	call   3ba <liblock_fccsynch_cond_wait+0x7a>
 3ba:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

00000000000003c0 <liblock_fccsynch_cond_init>:
    if(callback)
        callback();
}

static int do_liblock_cond_init(fccsynch)(liblock_cond_t *cond)
{
 3c0:	53                   	push   rbx
 	fatal("not implemented");
 3c1:	48 8b 1d 00 00 00 00 	mov    rbx,QWORD PTR [rip+0x0]        # 3c8 <liblock_fccsynch_cond_init+0x8>
 3c8:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 3cf <liblock_fccsynch_cond_init+0xf>
 3cf:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 3d6 <liblock_fccsynch_cond_init+0x16>
 3d6:	be 01 00 00 00       	mov    esi,0x1
 3db:	31 c0                	xor    eax,eax
 3dd:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 3e0:	e8 00 00 00 00       	call   3e5 <liblock_fccsynch_cond_init+0x25>
 3e5:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
 3e8:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # 3ef <liblock_fccsynch_cond_init+0x2f>
 3ef:	ba 0f 00 00 00       	mov    edx,0xf
 3f4:	be 01 00 00 00       	mov    esi,0x1
 3f9:	e8 00 00 00 00       	call   3fe <liblock_fccsynch_cond_init+0x3e>
 3fe:	48 8b 33             	mov    rsi,QWORD PTR [rbx]
 401:	bf 0a 00 00 00       	mov    edi,0xa
 406:	e8 00 00 00 00       	call   40b <liblock_fccsynch_cond_init+0x4b>
 40b:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 40e:	4c 8d 0d 00 00 00 00 	lea    r9,[rip+0x0]        # 415 <liblock_fccsynch_cond_init+0x55>
 415:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 41c <liblock_fccsynch_cond_init+0x5c>
 41c:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 423 <liblock_fccsynch_cond_init+0x63>
 423:	41 b8 ce 00 00 00    	mov    r8d,0xce
 429:	be 01 00 00 00       	mov    esi,0x1
 42e:	31 c0                	xor    eax,eax
 430:	e8 00 00 00 00       	call   435 <liblock_fccsynch_cond_init+0x75>
 435:	e8 00 00 00 00       	call   43a <liblock_fccsynch_cond_init+0x7a>
 43a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

0000000000000440 <liblock_fccsynch_execute_operation>:
}

static void *do_liblock_execute_operation(fccsynch)(liblock_lock_t *lock,
                                                   void *(*pending)(void*),
                                                   void *val)
{
 440:	41 56                	push   r14
 442:	49 89 d6             	mov    r14,rdx
 445:	41 55                	push   r13
 447:	41 54                	push   r12
 449:	55                   	push   rbp
 44a:	53                   	push   rbx
 44b:	48 89 f3             	mov    rbx,rsi
	struct liblock_impl *impl = lock->impl;
 44e:	4c 8b 67 10          	mov    r12,QWORD PTR [rdi+0x10]
    fccsynch_node *next_node, *cur_node, *tmp_node, *tmp_node_next;
	int counter = 0;
	
    next_node = my_node;
 452:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # 459 <liblock_fccsynch_execute_operation+0x19>
 459:	e8 00 00 00 00       	call   45e <liblock_fccsynch_execute_operation+0x1e>
 45e:	48 8b 90 00 00 00 00 	mov    rdx,QWORD PTR [rax+0x0]
	next_node->next = NULL;
	next_node->wait = 1;
	next_node->completed = 0;
	
	cur_node = SWAP(&impl->tail, next_node);
 465:	48 89 d5             	mov    rbp,rdx
	struct liblock_impl *impl = lock->impl;
    fccsynch_node *next_node, *cur_node, *tmp_node, *tmp_node_next;
	int counter = 0;
	
    next_node = my_node;
	next_node->next = NULL;
 468:	48 c7 42 18 00 00 00 	mov    QWORD PTR [rdx+0x18],0x0
 46f:	00 
	next_node->wait = 1;
 470:	c6 42 10 01          	mov    BYTE PTR [rdx+0x10],0x1
	next_node->completed = 0;
 474:	c6 42 11 00          	mov    BYTE PTR [rdx+0x11],0x0
	
	cur_node = SWAP(&impl->tail, next_node);
 478:	49 87 2c 24          	xchg   QWORD PTR [r12],rbp
	cur_node->req = pending;
 47c:	48 89 5d 00          	mov    QWORD PTR [rbp+0x0],rbx
	cur_node->next = next_node;
	
	my_node = cur_node;
 480:	48 89 a8 00 00 00 00 	mov    QWORD PTR [rax+0x0],rbp
	next_node->wait = 1;
	next_node->completed = 0;
	
	cur_node = SWAP(&impl->tail, next_node);
	cur_node->req = pending;
	cur_node->next = next_node;
 487:	48 89 55 18          	mov    QWORD PTR [rbp+0x18],rdx
    }
 */

    counter = 0;
    tmp_node = cur_node;
    while (cur_node->wait) {
 48b:	0f b6 45 10          	movzx  eax,BYTE PTR [rbp+0x10]
 48f:	84 c0                	test   al,al
 491:	74 5d                	je     4f0 <liblock_fccsynch_execute_operation+0xb0>
 493:	48 89 e9             	mov    rcx,rbp
 496:	31 f6                	xor    esi,esi
 498:	31 c0                	xor    eax,eax
//              tmp_node_next->x = 1;
                counter++;
                tmp_node = tmp_node_next;
                if (!cur_node->wait) break;
            }
            if (counter == H) flag = 1;
 49a:	bf 01 00 00 00       	mov    edi,0x1
 49f:	eb 16                	jmp    4b7 <liblock_fccsynch_execute_operation+0x77>
 4a1:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 */

    counter = 0;
    tmp_node = cur_node;
    while (cur_node->wait) {
    	if (!flag) {
 4a8:	be 01 00 00 00       	mov    esi,0x1
                if (!cur_node->wait) break;
            }
            if (counter == H) flag = 1;
        }

        PAUSE();
 4ad:	f3 90                	pause  
    }
 */

    counter = 0;
    tmp_node = cur_node;
    while (cur_node->wait) {
 4af:	0f b6 55 10          	movzx  edx,BYTE PTR [rbp+0x10]
 4b3:	84 d2                	test   dl,dl
 4b5:	74 39                	je     4f0 <liblock_fccsynch_execute_operation+0xb0>
    	if (!flag) {
 4b7:	85 f6                	test   esi,esi
 4b9:	75 ed                	jne    4a8 <liblock_fccsynch_execute_operation+0x68>
 4bb:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
            while ((tmp_node_next = tmp_node->next) && counter < H)
 4c0:	48 8b 51 18          	mov    rdx,QWORD PTR [rcx+0x18]
 4c4:	48 85 d2             	test   rdx,rdx
 4c7:	74 15                	je     4de <liblock_fccsynch_execute_operation+0x9e>
 4c9:	3d ff f8 15 00       	cmp    eax,0x15f8ff
 4ce:	7f 0e                	jg     4de <liblock_fccsynch_execute_operation+0x9e>
            {
//              tmp_node_next->x = 1;
                counter++;
                tmp_node = tmp_node_next;
                if (!cur_node->wait) break;
 4d0:	0f b6 4d 10          	movzx  ecx,BYTE PTR [rbp+0x10]
    while (cur_node->wait) {
    	if (!flag) {
            while ((tmp_node_next = tmp_node->next) && counter < H)
            {
//              tmp_node_next->x = 1;
                counter++;
 4d4:	83 c0 01             	add    eax,0x1
                tmp_node = tmp_node_next;
                if (!cur_node->wait) break;
 4d7:	84 c9                	test   cl,cl

    counter = 0;
    tmp_node = cur_node;
    while (cur_node->wait) {
    	if (!flag) {
            while ((tmp_node_next = tmp_node->next) && counter < H)
 4d9:	48 89 d1             	mov    rcx,rdx
            {
//              tmp_node_next->x = 1;
                counter++;
                tmp_node = tmp_node_next;
                if (!cur_node->wait) break;
 4dc:	75 e2                	jne    4c0 <liblock_fccsynch_execute_operation+0x80>
            }
            if (counter == H) flag = 1;
 4de:	3d 00 f9 15 00       	cmp    eax,0x15f900
 4e3:	0f 44 f7             	cmove  esi,edi
 4e6:	eb c5                	jmp    4ad <liblock_fccsynch_execute_operation+0x6d>
 4e8:	0f 1f 84 00 00 00 00 	nop    DWORD PTR [rax+rax*1+0x0]
 4ef:	00 
/* 
    while (cur_node->wait)
        PAUSE();
*/

	if (cur_node->completed)
 4f0:	0f b6 45 11          	movzx  eax,BYTE PTR [rbp+0x11]
 4f4:	84 c0                	test   al,al
 4f6:	75 4e                	jne    546 <liblock_fccsynch_execute_operation+0x106>
	tmp_node = cur_node;

    counter = 0;

pre_combiner_loop:
    while (tmp_node->next && counter < H)
 4f8:	48 8b 45 18          	mov    rax,QWORD PTR [rbp+0x18]
 4fc:	48 89 eb             	mov    rbx,rbp
 4ff:	41 bd 00 f9 15 00    	mov    r13d,0x15f900
 505:	49 89 ec             	mov    r12,rbp
 508:	48 85 c0             	test   rax,rax
 50b:	75 0c                	jne    519 <liblock_fccsynch_execute_operation+0xd9>
 50d:	eb 31                	jmp    540 <liblock_fccsynch_execute_operation+0x100>
 50f:	90                   	nop
 510:	41 83 ed 01          	sub    r13d,0x1
 514:	74 2a                	je     540 <liblock_fccsynch_execute_operation+0x100>
 516:	4c 89 e3             	mov    rbx,r12
	{
		counter++;

		tmp_node_next = tmp_node->next;
 519:	4c 8b 63 18          	mov    r12,QWORD PTR [rbx+0x18]
        // bizarre, supprimer l'appel à la fonction améliore beaucoup les perfs
        // (21000->17000), même quand H est très grand... alors qu'enlever 
        // l'appel à la fonction dans rcl ne coûte rien. Autre chose de bizarre,
        // quand on passe d'un accès à une ligne de cache à zéro dans la section
        // critique, on ne gagne que mille cycles...
		tmp_node->ret = (tmp_node->req)(val);
 51d:	4c 89 f7             	mov    rdi,r14
 520:	48 8b 03             	mov    rax,QWORD PTR [rbx]
 523:	ff d0                	call   rax
 525:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
        // fonction qui fait ramer.
		//tmp_node->ret = (void *)tmp_node->req;
		// Bizarrement on rest à 17000 avec ça, alors qu'elle aussi ne fait
        // qu'un cache miss...
        //testcount();
        tmp_node->completed = 1;
 529:	c6 43 11 01          	mov    BYTE PTR [rbx+0x11],0x1
		tmp_node->wait = 0;
 52d:	c6 43 10 00          	mov    BYTE PTR [rbx+0x10],0x0
	tmp_node = cur_node;

    counter = 0;

pre_combiner_loop:
    while (tmp_node->next && counter < H)
 531:	49 8b 44 24 18       	mov    rax,QWORD PTR [r12+0x18]

		tmp_node_next = tmp_node->next;
        // Au vu du résultat de objdump, quel que soit le degré d'optimisation,
        // il n'y a pas d'instructions de préfetch qui sont ajoutées 
        // automatiquement.
        __builtin_prefetch(tmp_node_next, 1, 1);
 536:	41 0f 18 1c 24       	prefetcht2 BYTE PTR [r12]
	tmp_node = cur_node;

    counter = 0;

pre_combiner_loop:
    while (tmp_node->next && counter < H)
 53b:	48 85 c0             	test   rax,rax
 53e:	75 d0                	jne    510 <liblock_fccsynch_execute_operation+0xd0>

		tmp_node = tmp_node_next;
	}
post_combiner_loop:

	tmp_node->wait = 0;
 540:	41 c6 44 24 10 00    	mov    BYTE PTR [r12+0x10],0x0

	//SFENCE();
	return cur_node->ret;
}
 546:	5b                   	pop    rbx
post_combiner_loop:

	tmp_node->wait = 0;

	//SFENCE();
	return cur_node->ret;
 547:	48 8b 45 08          	mov    rax,QWORD PTR [rbp+0x8]
}
 54b:	5d                   	pop    rbp
 54c:	41 5c                	pop    r12
 54e:	41 5d                	pop    r13
 550:	41 5e                	pop    r14
 552:	c3                   	ret    
 553:	66 66 66 66 2e 0f 1f 	data32 data32 data32 nop WORD PTR cs:[rax+rax*1+0x0]
 55a:	84 00 00 00 00 00 

0000000000000560 <liblock_fccsynch_init_lock>:

static struct liblock_impl *do_liblock_init_lock(fccsynch)
                                (liblock_lock_t *lock,
                                 struct core *core,
                                 pthread_mutexattr_t *attr)
{
 560:	53                   	push   rbx
    struct liblock_impl *impl = liblock_allocate(sizeof(struct liblock_impl));
 561:	bf 40 00 00 00       	mov    edi,0x40
 566:	e8 00 00 00 00       	call   56b <liblock_fccsynch_init_lock+0xb>

    impl->tail = anon_mmap(r_align(sizeof(fccsynch_node), PAGE_SIZE));
 56b:	bf 00 10 00 00       	mov    edi,0x1000
static struct liblock_impl *do_liblock_init_lock(fccsynch)
                                (liblock_lock_t *lock,
                                 struct core *core,
                                 pthread_mutexattr_t *attr)
{
    struct liblock_impl *impl = liblock_allocate(sizeof(struct liblock_impl));
 570:	48 89 c3             	mov    rbx,rax

    impl->tail = anon_mmap(r_align(sizeof(fccsynch_node), PAGE_SIZE));
 573:	e8 00 00 00 00       	call   578 <liblock_fccsynch_init_lock+0x18>
 578:	48 89 03             	mov    QWORD PTR [rbx],rax
    
    impl->tail->req = NULL;
 57b:	48 8b 03             	mov    rax,QWORD PTR [rbx]
 57e:	48 c7 00 00 00 00 00 	mov    QWORD PTR [rax],0x0
    impl->tail->ret = NULL;
 585:	48 8b 03             	mov    rax,QWORD PTR [rbx]
 588:	48 c7 40 08 00 00 00 	mov    QWORD PTR [rax+0x8],0x0
 58f:	00 
    impl->tail->wait = 0;
 590:	48 8b 03             	mov    rax,QWORD PTR [rbx]
 593:	c6 40 10 00          	mov    BYTE PTR [rax+0x10],0x0
	impl->tail->completed = 0;
 597:	48 8b 03             	mov    rax,QWORD PTR [rbx]
 59a:	c6 40 11 00          	mov    BYTE PTR [rax+0x11],0x0
    impl->tail->next = NULL;
 59e:	48 8b 03             	mov    rax,QWORD PTR [rbx]
 5a1:	48 c7 40 18 00 00 00 	mov    QWORD PTR [rax+0x18],0x0
 5a8:	00 

    return impl;
}
 5a9:	48 89 d8             	mov    rax,rbx
 5ac:	5b                   	pop    rbx
 5ad:	c3                   	ret    
 5ae:	66 90                	xchg   ax,ax

00000000000005b0 <liblock_fccsynch_on_thread_start>:
{
    fatal("not implemented"); 
}

static void do_liblock_on_thread_start(fccsynch)(struct thread_descriptor *desc)
{
 5b0:	53                   	push   rbx
    my_node = anon_mmap(r_align(sizeof(fccsynch_node), PAGE_SIZE));
 5b1:	bf 00 10 00 00       	mov    edi,0x1000
 5b6:	e8 00 00 00 00       	call   5bb <liblock_fccsynch_on_thread_start+0xb>
 5bb:	48 89 c3             	mov    rbx,rax
 5be:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # 5c5 <liblock_fccsynch_on_thread_start+0x15>
 5c5:	e8 00 00 00 00       	call   5ca <liblock_fccsynch_on_thread_start+0x1a>
    
    my_node->req = NULL;
 5ca:	48 c7 03 00 00 00 00 	mov    QWORD PTR [rbx],0x0
    my_node->ret = NULL;
 5d1:	48 c7 43 08 00 00 00 	mov    QWORD PTR [rbx+0x8],0x0
 5d8:	00 
    fatal("not implemented"); 
}

static void do_liblock_on_thread_start(fccsynch)(struct thread_descriptor *desc)
{
    my_node = anon_mmap(r_align(sizeof(fccsynch_node), PAGE_SIZE));
 5d9:	48 89 98 00 00 00 00 	mov    QWORD PTR [rax+0x0],rbx
    
    my_node->req = NULL;
    my_node->ret = NULL;
    my_node->wait = 0;
 5e0:	c6 43 10 00          	mov    BYTE PTR [rbx+0x10],0x0
	my_node->completed = 0;
 5e4:	c6 43 11 00          	mov    BYTE PTR [rbx+0x11],0x0
    my_node->next = NULL;
 5e8:	48 c7 43 18 00 00 00 	mov    QWORD PTR [rbx+0x18],0x0
 5ef:	00 
}
 5f0:	5b                   	pop    rbx
 5f1:	c3                   	ret    
 5f2:	66 66 66 66 66 2e 0f 	data32 data32 data32 data32 nop WORD PTR cs:[rax+rax*1+0x0]
 5f9:	1f 84 00 00 00 00 00 

0000000000000600 <liblock_fccsynch_kill_library>:

static void do_liblock_init_library(fccsynch)()
{}

static void do_liblock_kill_library(fccsynch)()
{ printf("%d\n", count); }
 600:	48 8b 05 00 00 00 00 	mov    rax,QWORD PTR [rip+0x0]        # 607 <liblock_fccsynch_kill_library+0x7>
}

__extern_always_inline int
printf (__const char *__restrict __fmt, ...)
{
  return __printf_chk (__USE_FORTIFY_LEVEL - 1, __fmt, __va_arg_pack ());
 607:	48 8d 35 00 00 00 00 	lea    rsi,[rip+0x0]        # 60e <liblock_fccsynch_kill_library+0xe>
 60e:	bf 01 00 00 00       	mov    edi,0x1
 613:	8b 10                	mov    edx,DWORD PTR [rax]
 615:	31 c0                	xor    eax,eax
 617:	e9 00 00 00 00       	jmp    61c <liblock_fccsynch_kill_library+0x1c>
 61c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]

0000000000000620 <liblock_fccsynch_run>:

static void do_liblock_run(fccsynch)(void (*callback)())
{
    if(__sync_bool_compare_and_swap(&liblock_start_server_threads_by_hand,
 620:	48 8b 15 00 00 00 00 	mov    rdx,QWORD PTR [rip+0x0]        # 627 <liblock_fccsynch_run+0x7>

static void do_liblock_kill_library(fccsynch)()
{ printf("%d\n", count); }

static void do_liblock_run(fccsynch)(void (*callback)())
{
 627:	53                   	push   rbx
    if(__sync_bool_compare_and_swap(&liblock_start_server_threads_by_hand,
 628:	b8 01 00 00 00       	mov    eax,0x1
 62d:	31 c9                	xor    ecx,ecx
 62f:	f0 0f b1 0a          	lock cmpxchg DWORD PTR [rdx],ecx
 633:	75 0d                	jne    642 <liblock_fccsynch_run+0x22>
                                    1, 0) != 1)
        fatal("servers are not managed manually");
    if(callback)
 635:	48 85 ff             	test   rdi,rdi
 638:	74 06                	je     640 <liblock_fccsynch_run+0x20>
        callback();
}
 63a:	5b                   	pop    rbx
{
    if(__sync_bool_compare_and_swap(&liblock_start_server_threads_by_hand,
                                    1, 0) != 1)
        fatal("servers are not managed manually");
    if(callback)
        callback();
 63b:	31 c0                	xor    eax,eax
 63d:	ff e7                	jmp    rdi
 63f:	90                   	nop
}
 640:	5b                   	pop    rbx
 641:	c3                   	ret    

static void do_liblock_run(fccsynch)(void (*callback)())
{
    if(__sync_bool_compare_and_swap(&liblock_start_server_threads_by_hand,
                                    1, 0) != 1)
        fatal("servers are not managed manually");
 642:	48 8b 1d 00 00 00 00 	mov    rbx,QWORD PTR [rip+0x0]        # 649 <liblock_fccsynch_run+0x29>

# ifdef __va_arg_pack
__extern_always_inline int
fprintf (FILE *__restrict __stream, __const char *__restrict __fmt, ...)
{
  return __fprintf_chk (__stream, __USE_FORTIFY_LEVEL - 1, __fmt,
 649:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 650 <liblock_fccsynch_run+0x30>
 650:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 657 <liblock_fccsynch_run+0x37>
 657:	be 01 00 00 00       	mov    esi,0x1
 65c:	31 c0                	xor    eax,eax
 65e:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 661:	e8 00 00 00 00       	call   666 <liblock_fccsynch_run+0x46>
 666:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
 669:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # 670 <liblock_fccsynch_run+0x50>
 670:	ba 20 00 00 00       	mov    edx,0x20
 675:	be 01 00 00 00       	mov    esi,0x1
 67a:	e8 00 00 00 00       	call   67f <liblock_fccsynch_run+0x5f>
 67f:	48 8b 33             	mov    rsi,QWORD PTR [rbx]
 682:	bf 0a 00 00 00       	mov    edi,0xa
 687:	e8 00 00 00 00       	call   68c <liblock_fccsynch_run+0x6c>
 68c:	48 8b 3b             	mov    rdi,QWORD PTR [rbx]
 68f:	4c 8d 0d 00 00 00 00 	lea    r9,[rip+0x0]        # 696 <liblock_fccsynch_run+0x76>
 696:	48 8d 0d 00 00 00 00 	lea    rcx,[rip+0x0]        # 69d <liblock_fccsynch_run+0x7d>
 69d:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 6a4 <liblock_fccsynch_run+0x84>
 6a4:	41 b8 c7 00 00 00    	mov    r8d,0xc7
 6aa:	be 01 00 00 00       	mov    esi,0x1
 6af:	31 c0                	xor    eax,eax
 6b1:	e8 00 00 00 00       	call   6b6 <liblock_fccsynch_run+0x96>
 6b6:	e8 00 00 00 00       	call   6bb <liblock_fccsynch_run+0x9b>
 6bb:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]

00000000000006c0 <testcount>:
    char __pad[pad_to_cache_line(sizeof(fccsynch_node *))];
};

volatile int count = 0;
void testcount(void) {
    count++;
 6c0:	48 8b 05 00 00 00 00 	mov    rax,QWORD PTR [rip+0x0]        # 6c7 <testcount+0x7>
 6c7:	8b 10                	mov    edx,DWORD PTR [rax]
 6c9:	83 c2 01             	add    edx,0x1
 6cc:	89 10                	mov    DWORD PTR [rax],edx
}
 6ce:	c3                   	ret    

Disassembly of section .text.startup:

0000000000000000 <fccsynch_constructor_222>:
}

static void do_liblock_declare_server(fccsynch)(struct core *core)
{}

liblock_declare(fccsynch);
   0:	48 8d 35 00 00 00 00 	lea    rsi,[rip+0x0]        # 7 <fccsynch_constructor_222+0x7>
   7:	48 8d 3d 00 00 00 00 	lea    rdi,[rip+0x0]        # e <fccsynch_constructor_222+0xe>
   e:	e9 00 00 00 00       	jmp    13 <liblock_fccsynch_init_library+0x3>
