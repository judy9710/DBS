/*-------------------------------------------------------------------------
 *
 * buf_init.c
 *	  buffer manager initialization routines
 *
 * Portions Copyright (c) 1996-2022, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/buffer/buf_init.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "storage/buf_internals.h"
#include "storage/bufmgr.h"
#include "storage/proc.h"

BufferDescPadded *BufferDescriptors;
char	   *BufferBlocks;
ConditionVariableMinimallyPadded *BufferIOCVArray;
WritebackContext BackendWritebackContext;
CkptSortItem *CkptBufferIds;


/*
 * Data Structures:
 *		buffers live in a freelist and a lookup data structure. 
 *
 *
 * Buffer Lookup:
 *		Two important notes.  First, the buffer has to be
 *		available for lookup BEFORE an IO begins.  Otherwise
 *		a second process trying to read the buffer will
 *		allocate its own copy and the buffer pool will
 *		become inconsistent. // 데이터베이스의 일관성 보장을 위함
	
 *
 * Buffer Replacement:
 *		see freelist.c.  A buffer cannot be replaced while in
 *		use either by data manager or during IO. // 버퍼는 데이터 입출력 시에 교체될 수 없음
 *
 *
 * Synchronization/Locking:
 *
 * IO_IN_PROGRESS -- this is a flag in the buffer descriptor.
 *		It must be set when an IO is initiated and cleared at
 *		the end of the IO.  It is there to make sure that one
 *		process doesn't start to use a buffer while another is
 *		faulting it in.  see WaitIO and related routines.
 *
 * refcount --	Counts the number of processes holding pins on a buffer.
 *		A buffer is pinned during IO and immediately after a BufferAlloc().
 *		Pins must be released before end of transaction.  For efficiency the
 *		shared refcount isn't increased if an individual backend pins a buffer
 *		multiple times. Check the PrivateRefCount infrastructure in bufmgr.c.
 */


/*
 * Initialize shared buffer pool
 *
 * This is called once during shared-memory initialization (either in the
 * postmaster, or in a standalone backend).
 */
void
InitBufferPool(void)
{
	bool		foundBufs,
				foundDescs,
				foundIOCV,
				foundBufCkpt;

	/* Align descriptors to a cacheline boundary. */
	BufferDescriptors = (BufferDescPadded *)
		ShmemInitStruct("Buffer Descriptors",
						NBuffers * sizeof(BufferDescPadded),
						&foundDescs);

	BufferBlocks = (char *)
		ShmemInitStruct("Buffer Blocks",
						NBuffers * (Size) BLCKSZ, &foundBufs);

	/* Align condition variables to cacheline boundary. */
	BufferIOCVArray = (ConditionVariableMinimallyPadded *)
		ShmemInitStruct("Buffer IO Condition Variables",
						NBuffers * sizeof(ConditionVariableMinimallyPadded),
						&foundIOCV);
	// 각각 버퍼 디스크립터, 버퍼 블럭, 버퍼 입출력 배열을 선언했음

	/*
	 * The array used to sort to-be-checkpointed buffer ids is located in
	 * shared memory, to avoid having to allocate significant amounts of
	 * memory at runtime. As that'd be in the middle of a checkpoint, or when
	 * the checkpointer is restarted, memory allocation failures would be
	 * painful.
	 */
	CkptBufferIds = (CkptSortItem *)
		ShmemInitStruct("Checkpoint BufferIds",
						NBuffers * sizeof(CkptSortItem), &foundBufCkpt);
	// 공유 메모리에 버퍼 아이디를 정렬하기 위한 배열이 존재함. 따라서 실행시간에 상당량의 메모리를 할당하기 것을 방지하기 위함임.

	if (foundDescs || foundBufs || foundIOCV || foundBufCkpt)
	{
		/* should find all of these, or none of them */
		Assert(foundDescs && foundBufs && foundIOCV && foundBufCkpt);
		/* note: this path is only taken in EXEC_BACKEND case */
	}
	else
	{
		int			i;

		/*
		 * Initialize all the buffer headers.
		 */
		for (i = 0; i < NBuffers; i++)
		{
			BufferDesc *buf = GetBufferDescriptor(i);

			ClearBufferTag(&buf->tag);

			pg_atomic_init_u32(&buf->state, 0);
			buf->wait_backend_pgprocno = INVALID_PGPROCNO;

			buf->buf_id = i;

			/*
			 * Initially link all the buffers together as unused. Subsequent
			 * management of this list is done by freelist.c.
			 */
			buf->freeNext = i + 1;

			LWLockInitialize(BufferDescriptorGetContentLock(buf),
							 LWTRANCHE_BUFFER_CONTENT);

			ConditionVariableInit(BufferDescriptorGetIOCV(buf));
		}

		/* Correct last entry of linked list */
		GetBufferDescriptor(NBuffers - 1)->freeNext = FREENEXT_END_OF_LIST;
	}

	/* Init other shared buffer-management stuff */ 
	StrategyInitialize(!foundDescs);

	/* Initialize per-backend file flush context */ 
	WritebackContextInit(&BackendWritebackContext,
						 &backend_flush_after);
}

/*
 * BufferShmemSize
 *
 * compute the size of shared memory for the buffer pool including
 * data pages, buffer descriptors, hash tables, etc.
 */
Size
BufferShmemSize(void)
{
	Size		size = 0;

	/* size of buffer descriptors */
	size = add_size(size, mul_size(NBuffers, sizeof(BufferDescPadded)));
	/* to allow aligning buffer descriptors */
	size = add_size(size, PG_CACHE_LINE_SIZE);

	/* size of data pages */
	size = add_size(size, mul_size(NBuffers, BLCKSZ));

	/* size of stuff controlled by freelist.c */
	size = add_size(size, StrategyShmemSize());

	/* size of I/O condition variables */
	size = add_size(size, mul_size(NBuffers,
								   sizeof(ConditionVariableMinimallyPadded)));
	/* to allow aligning the above */
	size = add_size(size, PG_CACHE_LINE_SIZE);

	/* size of checkpoint sort array in bufmgr.c */
	size = add_size(size, mul_size(NBuffers, sizeof(CkptSortItem)));

	return size;
}
