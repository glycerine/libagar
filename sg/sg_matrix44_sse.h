/*
 * Copyright (c) 2007 Hypertriton, Inc. <http://hypertriton.com/>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_SSE

static __inline__ SG_Matrix
SG_MatrixMult44_SSE(SG_Matrix A, SG_Matrix B)
{
	SG_Matrix AB;

	AB.m1 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	        _mm_mul_ps(_mm_shuffle_ps(A.m1,A.m1,_MM_SHUFFLE(0,0,0,0)),B.m1),
		_mm_mul_ps(_mm_shuffle_ps(A.m1,A.m1,_MM_SHUFFLE(1,1,1,1)),B.m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A.m1,A.m1,_MM_SHUFFLE(2,2,2,2)),B.m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A.m1,A.m1,_MM_SHUFFLE(3,3,3,3)),B.m4)
	);
	AB.m2 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	        _mm_mul_ps(_mm_shuffle_ps(A.m2,A.m2,_MM_SHUFFLE(0,0,0,0)),B.m1),
	        _mm_mul_ps(_mm_shuffle_ps(A.m2,A.m2,_MM_SHUFFLE(1,1,1,1)),B.m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A.m2,A.m2,_MM_SHUFFLE(2,2,2,2)),B.m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A.m2,A.m2,_MM_SHUFFLE(3,3,3,3)),B.m4)
	);
	AB.m3 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	        _mm_mul_ps(_mm_shuffle_ps(A.m3,A.m3,_MM_SHUFFLE(0,0,0,0)),B.m1),
	        _mm_mul_ps(_mm_shuffle_ps(A.m3,A.m3,_MM_SHUFFLE(1,1,1,1)),B.m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A.m3,A.m3,_MM_SHUFFLE(2,2,2,2)),B.m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A.m3,A.m3,_MM_SHUFFLE(3,3,3,3)),B.m4)
	);
	AB.m4 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	        _mm_mul_ps(_mm_shuffle_ps(A.m4,A.m4,_MM_SHUFFLE(0,0,0,0)),B.m1),
		_mm_mul_ps(_mm_shuffle_ps(A.m4,A.m4,_MM_SHUFFLE(1,1,1,1)),B.m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A.m4,A.m4,_MM_SHUFFLE(2,2,2,2)),B.m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A.m4,A.m4,_MM_SHUFFLE(3,3,3,3)),B.m4)
	);
	return (AB);
}

static __inline__ void
SG_MatrixMult44v_SSE(SG_Matrix *A, const SG_Matrix *B)
{
	SG_Matrix AB;

	AB.m1 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	     _mm_mul_ps(_mm_shuffle_ps(A->m1,A->m1,_MM_SHUFFLE(0,0,0,0)),B->m1),
	     _mm_mul_ps(_mm_shuffle_ps(A->m1,A->m1,_MM_SHUFFLE(1,1,1,1)),B->m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A->m1,A->m1,_MM_SHUFFLE(2,2,2,2)),B->m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A->m1,A->m1,_MM_SHUFFLE(3,3,3,3)),B->m4)
	);
	AB.m2 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	     _mm_mul_ps(_mm_shuffle_ps(A->m2,A->m2,_MM_SHUFFLE(0,0,0,0)),B->m1),
	     _mm_mul_ps(_mm_shuffle_ps(A->m2,A->m2,_MM_SHUFFLE(1,1,1,1)),B->m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A->m2,A->m2,_MM_SHUFFLE(2,2,2,2)),B->m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A->m2,A->m2,_MM_SHUFFLE(3,3,3,3)),B->m4)
	);
	AB.m3 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	     _mm_mul_ps(_mm_shuffle_ps(A->m3,A->m3,_MM_SHUFFLE(0,0,0,0)),B->m1),
	     _mm_mul_ps(_mm_shuffle_ps(A->m3,A->m3,_MM_SHUFFLE(1,1,1,1)),B->m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A->m3,A->m3,_MM_SHUFFLE(2,2,2,2)),B->m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A->m3,A->m3,_MM_SHUFFLE(3,3,3,3)),B->m4)
	);
	AB.m4 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	     _mm_mul_ps(_mm_shuffle_ps(A->m4,A->m4,_MM_SHUFFLE(0,0,0,0)),B->m1),
	     _mm_mul_ps(_mm_shuffle_ps(A->m4,A->m4,_MM_SHUFFLE(1,1,1,1)),B->m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A->m4,A->m4,_MM_SHUFFLE(2,2,2,2)),B->m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A->m4,A->m4,_MM_SHUFFLE(3,3,3,3)),B->m4)
	);
	A->m1 = AB.m1;
	A->m2 = AB.m2;
	A->m3 = AB.m3;
	A->m4 = AB.m4;
}

static __inline__ void
SG_MatrixMult44pv_SSE(SG_Matrix *C, const SG_Matrix *A, const SG_Matrix *B)
{
	C->m1 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	     _mm_mul_ps(_mm_shuffle_ps(A->m1,A->m1,_MM_SHUFFLE(0,0,0,0)),B->m1),
	     _mm_mul_ps(_mm_shuffle_ps(A->m1,A->m1,_MM_SHUFFLE(1,1,1,1)),B->m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A->m1,A->m1,_MM_SHUFFLE(2,2,2,2)),B->m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A->m1,A->m1,_MM_SHUFFLE(3,3,3,3)),B->m4)
	);
	C->m2 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	     _mm_mul_ps(_mm_shuffle_ps(A->m2,A->m2,_MM_SHUFFLE(0,0,0,0)),B->m1),
	     _mm_mul_ps(_mm_shuffle_ps(A->m2,A->m2,_MM_SHUFFLE(1,1,1,1)),B->m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A->m2,A->m2,_MM_SHUFFLE(2,2,2,2)),B->m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A->m2,A->m2,_MM_SHUFFLE(3,3,3,3)),B->m4)
	);
	C->m3 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	     _mm_mul_ps(_mm_shuffle_ps(A->m3,A->m3,_MM_SHUFFLE(0,0,0,0)),B->m1),
	     _mm_mul_ps(_mm_shuffle_ps(A->m3,A->m3,_MM_SHUFFLE(1,1,1,1)),B->m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A->m3,A->m3,_MM_SHUFFLE(2,2,2,2)),B->m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A->m3,A->m3,_MM_SHUFFLE(3,3,3,3)),B->m4)
	);
	C->m4 = _mm_add_ps(
	    _mm_add_ps(_mm_add_ps(
	     _mm_mul_ps(_mm_shuffle_ps(A->m4,A->m4,_MM_SHUFFLE(0,0,0,0)),B->m1),
	     _mm_mul_ps(_mm_shuffle_ps(A->m4,A->m4,_MM_SHUFFLE(1,1,1,1)),B->m2)
	    ),
	    _mm_mul_ps(_mm_shuffle_ps(A->m4,A->m4,_MM_SHUFFLE(2,2,2,2)),B->m3)),
	    _mm_mul_ps(_mm_shuffle_ps(A->m4,A->m4,_MM_SHUFFLE(3,3,3,3)),B->m4)
	);
}

static __inline__ void
SG_MatrixCopy44_SSE(SG_Matrix *mDst, const SG_Matrix *mSrc)
{
	mDst->m1 = mSrc->m1;
	mDst->m2 = mSrc->m2;
	mDst->m3 = mSrc->m3;
	mDst->m4 = mSrc->m4;
}

__BEGIN_DECLS
extern const SG_MatrixOps44 sgMatOps44_SSE;

SG_Matrix SG_MatrixInvert44_SSE(const SG_Matrix);
SG_Matrix SG_MatrixInvert44p_SSE(const SG_Matrix *);
__END_DECLS

#endif /* HAVE_SSE */