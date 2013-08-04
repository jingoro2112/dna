#ifndef OBJECT_TPOOL_HPP
#define OBJECT_TPOOL_HPP
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

// so I was experimenting with object pooling one day and ended up with
// this class. It pre-allocates a chunk of objects for good
// L1/2 cache locality, and is basically a handy way of handling a pool
// of 'whatevers'
// its intrusive by pooling a derived class of whatever you template it
// as


//------------------------------------------------------------------------------
template<class T> class CObjectTPool
{
public:

	inline CObjectTPool( const unsigned int preAllocate =0, // how many items to pre-alloc
						 const int maxFreeSize =-1, // -1 = unlimited, 0 = do not pool
						 void(*clear)(T& item) =0 ); // called when nodes are freed (NOT created)

	inline ~CObjectTPool() { truncate(0); delete[] m_preAllocated; }

	inline T* get();
	inline void release( T* object );

	void setMaxFreeSize( const int maxSize ) { m_maxFreeSize = (unsigned int)maxSize; truncate(maxSize); }
	inline unsigned int truncate( const int downTo );

private:

	//-------------------------
	class Node : public T
	{
		private:
			friend class CObjectTPool;
			Node *CObjectTPool_Node_next; // name very unlikely to collide
	};

	Node *m_preAllocated;
	Node *m_preAllocatedEndpoint;

	Node *m_freeList;
	Node *m_preAllocatedFreeList;

	int m_maxFreeSize;
	int m_freeSize;

	void (*m_clear)(T& item);
};

//------------------------------------------------------------------------------
template<class T> CObjectTPool<T>::CObjectTPool( const unsigned int preAllocate /*=0*/,
											   const int maxFreeSize /*=-1*/,
											   void(*clear)(T& item) /*=0*/ )
{
	m_clear = clear;
	m_freeList = 0;
	m_preAllocatedFreeList = 0;
	m_maxFreeSize = maxFreeSize;

	if ( preAllocate )
	{
		m_preAllocated = new Node[ preAllocate ];
		m_preAllocatedEndpoint = m_preAllocated + (preAllocate * sizeof(Node));

		m_freeSize = preAllocate;
		for( unsigned int i=0; i<preAllocate ; i++ )
		{
			release( &(m_preAllocated[i]) );
		}
	}
	else
	{
		m_preAllocated = 0;
		m_freeSize = 0;
	}
}

//------------------------------------------------------------------------------
template<class T> T* CObjectTPool<T>::get()
{
	T* ret;
	if ( m_preAllocatedFreeList )
	{
		m_freeSize--;
		ret = m_preAllocatedFreeList;
		m_preAllocatedFreeList = m_preAllocatedFreeList->CObjectTPool_Node_next;
		return ret;
	}

	if ( m_freeList )
	{
		if ( m_freeList )
		{
			m_freeSize--;
			ret = m_freeList;
			m_freeList = m_freeList->CObjectTPool_Node_next;
		}
		else
		{
			ret = new Node;
		}

		return ret;
	}

	return new Node;
}

//------------------------------------------------------------------------------
template<class T> void CObjectTPool<T>::release( T* node )
{
	if ( !node )
	{
		return;
	}

	Node *toFree = static_cast<Node*>(node);
	if ( !toFree )
	{
		return;
	}

	if ( m_clear )
	{
		m_clear( *node );
	}

	// part of the pre-allocated block?
	if ( m_preAllocated 
		 && node >= m_preAllocated
		 && node < m_preAllocatedEndpoint )
	{
		m_freeSize++;
		toFree->CObjectTPool_Node_next = m_preAllocatedFreeList;
		m_preAllocatedFreeList = toFree;
	}
	else if( m_maxFreeSize >= 0 // exceeding desired free size?
			 && m_freeSize >= m_maxFreeSize )
	{
		delete toFree;
	}
	else
	{
		m_freeSize++;
		toFree->CObjectTPool_Node_next = m_freeList;
		m_freeList = toFree;
	}
}

//------------------------------------------------------------------------------
template<class T> unsigned int CObjectTPool<T>::truncate( const int downTo )
{
	if ( downTo < 0 || !m_freeList )
	{
		return 0;
	}

	int itemsTrimmed = 0;

	// guaranteed to have only new-ed items in the free list
	Node* node = m_freeList;
	for( ; m_freeSize > downTo && node ; m_freeSize-- )
	{
		Node* next = node->CObjectTPool_Node_next;
		itemsTrimmed++;
		delete node;
		node = next;
	}

	return itemsTrimmed;
}

#endif
