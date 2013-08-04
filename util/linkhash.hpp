#ifndef LINKHASH_HPP
#define LINKHASH_HPP
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "object_tpool.hpp"
#include <memory.h>

//-----------------------------------------------------------------------------
// LinkHash is a highly optimized hash list which has some very
// important properties:
//
// - node-memory is pooled, and released only when the list is destroyed
// - hash entries maintain a linked-list of all entries, so iterating
//   is very fast
// - in-place iteration
// - no effort is wasted making this list thread-safe, that's up to you
//   if necessary
template <class H> class CLinkHash
{
public:

	inline H* add( const unsigned long long key );
	void addItem( H item, const unsigned long long key );
	inline bool remove( const unsigned long long key );
	inline void clear();
	unsigned int count() const { return m_count; }
	inline void resize( unsigned int newBucketCount );
	H* getFirst() { m_current = m_head; return m_current ? &(m_current->item) : 0; }
	H getFirstItem() { m_current = m_head; return m_current ? m_current->item : 0; }
	H* getNext() { m_current = m_current ? m_current->nextIter : m_head; return getCurrent(); }
	H getNextItem() { m_current = m_current ? m_current->nextIter : m_head; return getCurrentItem(); }
	H* getCurrent() { return m_current ? &(m_current->item) : 0; }
	H getCurrentItem() { return m_current ? m_current->item : 0; }
	unsigned long long getCurrentKey() { return m_current ? m_current->key : 0; }

	void invertLinkedList(); // so the iterate-order matches the add-order

	H* get( const unsigned long long key ) const
	{
		for( const Node *N = m_list[key % m_mod]; N ; N=N->next )
		{
			if ( N->key == key )
			{
				return (H *)&(N->item);
			}
		}
		return 0;
	}

	H getItem( const unsigned long long key ) const
	{
		for( const Node *N = m_list[key % m_mod]; N ; N=N->next )
		{
			if ( N->key == key )
			{
				return N->item;
			}
		}
		return 0;
	}

	// maxPoolSize of 0 means "don't pool"
	inline CLinkHash( int preAllocSize =0, int maxPoolSize =-1, void (*clear)( H& item ) =0 );

	~CLinkHash() { clear(); delete[] m_list; }

private:
	struct Node
	{
		H item;
		unsigned long long key;
		Node *next; // next in the hash link chain
		Node *nextIter; // list is doubly-linked
		Node *prevIter;
	};

public:
	class CIterator
	{
	public:
		CIterator( CLinkHash<H> &list ) { m_list = &list; m_current = 0; }
		inline void removeCurrent();
		H* getFirst() { m_current = m_list->m_head; return m_current ? &(m_current->item) : 0; }
		H* getNext() { m_current = m_current ? m_current->nextIter : m_list->m_head; return m_current ? &(m_current->item) : 0; }
		unsigned long long getCurrentKey() { return m_current ? m_current->key : 0; }
	private:
		CLinkHash<H> *m_list;
		Node *m_current;
	};

private:

	CObjectTPool<Node> m_nodePool;
	
	Node *m_head;
	Node *m_current;
	Node **m_list;
	unsigned int m_mod;
	unsigned int m_count;
	unsigned int m_loadThreshold;
	void (*m_clear)( H& item );
};

//-----------------------------------------------------------------------------
template<class H> CLinkHash<H>::CLinkHash( int preAllocSize, int maxPoolSize, void (*clear)( H& item ) ) :
	m_nodePool( preAllocSize, maxPoolSize )
{
	m_head = 0;
	m_current = 0;
	m_list = 0;
	m_mod = 0;
	m_count = 0;
	m_clear = clear;

	resize( 4 );
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::CIterator::removeCurrent()
{
	if ( !m_current )
	{
		return;
	}

	Node* newEntry = m_current->prevIter; // make sure the list stays sane after the removal
	m_list->remove( m_current->key );
	m_current = newEntry;
}

//-----------------------------------------------------------------------------
template<class H> H* CLinkHash<H>::add( const unsigned long long key )
{
	const int p = (int)(key % m_mod);

	Node *N = m_nodePool.get();
	N->key = key;
	N->next = m_list[p];
	m_list[p] = N;

	if ( m_head )
	{
		m_head->prevIter = N;
	}
	N->nextIter = m_head;
	N->prevIter = 0;
	m_head = N;

	m_count++;

	if ( m_count > m_loadThreshold )
	{
		resize( m_mod * 2 );
	}

	return &N->item;
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::addItem( H item, const unsigned long long key )
{
	const int p = (int)(key % m_mod);

	Node *N = m_nodePool.get();

	N->item = item;
	N->key = key;
	N->next = m_list[p];
	m_list[p] = N;

	if ( m_head )
	{
		m_head->prevIter = N;
	}
	N->nextIter = m_head;
	N->prevIter = 0;
	m_head = N;

	m_count++;

	if ( m_count > m_loadThreshold )
	{
		resize( m_mod * 2 );
	}
}

//-----------------------------------------------------------------------------
template<class H> bool CLinkHash<H>::remove( unsigned long long key )
{
	// find the position
	const int p = (int)(key % m_mod);

	// iterate through, find/unlink the item
	Node *prev = 0;
	for( Node *node = m_list[p]; node ; node = node->next )
	{
		if ( node->key == key )
		{
			if ( node == m_current )
			{
				m_current = node->prevIter;
			}

			if ( prev )
			{
				prev->next = node->next;
			}
			else
			{
				m_list[p] = node->next;
			}

			if ( node == m_head )
			{
				m_head = node->nextIter;

				if ( m_head )
				{
					m_head->prevIter = 0;
				}
			}
			else
			{
				if ( node->prevIter )
				{
					node->prevIter->nextIter = node->nextIter;
				}

				if ( node->nextIter )
				{
					node->nextIter->prevIter = node->prevIter;
				}
			}

			if ( m_clear )
			{
				m_clear( node->item );
			}
			m_nodePool.release( node );

			m_count--;
			return true;
		}

		prev = node;
	}

	return false;
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::resize( unsigned int newBucketCount )
{
	m_mod = newBucketCount < 4 ? 4 : newBucketCount;

	Node **oldList = m_list;
	m_list = new Node*[ m_mod ];
	memset( m_list, 0, m_mod * sizeof(Node*) );

	for( Node *N = m_head ; N ; N = N->nextIter )
	{
		int bucket = (int)(N->key % m_mod);
		N->next = m_list[bucket];
		m_list[bucket] = N;
	}

	m_loadThreshold = m_mod - (m_mod / 4);
	delete[] oldList;
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::clear()
{
	for( unsigned int l=0; l<m_mod ; l++ )
	{
		while( m_list[l] )
		{
			Node *N = m_list[l]->next;

			if ( m_clear )
			{
				m_clear( m_list[l]->item );
			}
			m_nodePool.release( m_list[l] );
			m_list[l] = N;
		}
	}

	m_head = 0;
	m_current = 0;
	m_count = 0;
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::invertLinkedList()
{
	if ( !m_head )
	{
		return;
	}

	Node *prev = 0;
	for(;;)
	{
		Node *next = m_head->nextIter;
		m_head->nextIter = prev;
		if ( !next )
		{
			return;
		}
		prev = m_head;
		m_head = next;
	}
}

#endif
