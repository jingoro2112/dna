#ifndef LINKLIST_HPP
#define LINKLIST_HPP
/*------------------------------------------------------------------------------*/

#include "object_tpool.hpp"

//-----------------------------------------------------------------------------
template<class L> class CLinkList
{
public:

	inline CLinkList( int preAlloc =0, int maxFreeSize =-1, void (*clear)( L& item ) =0 );
	~CLinkList() { clear(); }

	inline void clear();

	unsigned int count() const { return m_count; }

	void setMaxFreeSize( int size ) { m_nodePool.setMaxFreeSize( size ); }

	inline L* addHead(); // slightly faster (if it matters)
	inline L* addTail();
	inline L* add() { return addTail(); }
	inline L* addBeforeCurrent();
	inline L* addAfterCurrent();

	inline bool removeCurrent();
	inline bool remove( L* item );

	void popFirst() { getFirst(); removeCurrent(); }
	void popHead() { getHead(); removeCurrent(); }
	void popLast() { getLast(); removeCurrent(); }
	void popTail() { getTail(); removeCurrent(); }
	L popFirstItem() { L ret = getFirstItem(); removeCurrent(); return ret; }
	L popHeadItem() { L ret = getHeadItem(); removeCurrent(); return ret; }
	L popLastItem() { L ret = getLastItem(); removeCurrent(); return ret; }
	L popTailItem() { L ret = getTailItem(); removeCurrent(); return ret; }

	L* getHead() { m_current = m_list; return getCurrent(); }
	L* getTail() { m_current = m_tail; return getCurrent(); }
	L* getFirst() { m_current = m_list; return getCurrent(); }
	L* getLast() { m_current = m_tail; return getCurrent(); }
	L getHeadItem() { m_current = m_list; return getCurrentItem(); }
	L getTailItem() { m_current = m_tail; return getCurrentItem(); }
	L getFirstItem() { m_current = m_list; return getCurrentItem(); }
	L getLastItem() { m_current = m_tail; return getCurrentItem(); }

	L* getNext() { m_current = m_current ? m_current->nextIter : m_list; return getCurrent(); }
	L getNextItem() { m_current = m_current ? m_current->nextIter : m_list; return getCurrentItem(); }
	L* getPrev() { m_current = m_current ? m_current->prevIter : m_list; return getCurrent(); }
	L* getCurrent() const { return m_current ? &m_current->item : 0; }
	L getCurrentItem() const { return m_current ? m_current->item : 0; }

private:

	struct Node
	{
		L item;
		Node *nextIter;
		Node *prevIter;
	};

public:
	class CIterator
	{
	public:
		CIterator( CLinkList<L> &list ) { m_list = &list; m_current = 0; }
		void removeCurrent() { if ( m_current ) { m_list.remove( &m_current->item ); m_current = 0; } }
		L* getFirst() { m_current = m_list->m_list; return m_current ? &(m_current->item) : 0; }
		L* getNext() { m_current = m_current ? m_current->nextIter : m_list->m_list; return m_current ? &(m_current->item) : 0; }
	private:
		CLinkList<L> *m_list;
		Node *m_current;
	};

private:
	CObjectTPool<Node> m_nodePool;

	Node *m_list;
	Node *m_tail;
	Node *m_current;
	unsigned int m_count;
	void (*m_clear)( L& item );
};

//-----------------------------------------------------------------------------
template<class L> CLinkList<L>::CLinkList( int prePool, int maxFreeSize, void (*clear)( L& item ) ) :
	m_nodePool( prePool, maxFreeSize )
{
	m_list = 0;
	m_tail = 0;
	m_current = 0;
	m_count = 0;
	m_clear = clear;
}

//-----------------------------------------------------------------------------
template<class L> void CLinkList<L>::clear()
{
	while( m_list )
	{
		Node* next = m_list->nextIter;
		if ( m_clear )
		{
			m_clear( m_list->item );
		}

		m_nodePool.release( m_list );
		m_list = next;
	}

	m_tail = 0;
	m_current = 0;
	m_count = 0;
}

//-----------------------------------------------------------------------------
template<class L> L* CLinkList<L>::addHead()
{
	Node* N= m_nodePool.get();

	m_count++;

	N->nextIter = m_list;
	if ( m_list )
	{
		m_list->prevIter = N;
	}
	N->prevIter = 0;
	m_list = N;

	if ( !m_tail )
	{
		m_tail = m_list;
	}

	return &(N->item);
}

//-----------------------------------------------------------------------------
template<class L> L* CLinkList<L>::addTail()
{
	Node* N= m_nodePool.get();

	m_count++;

	N->nextIter = 0;

	if ( m_tail )
	{
		N->prevIter = m_tail;
		m_tail->nextIter = N;
	}
	else
	{
		N->prevIter = 0;
		m_list = N;
	}

	m_tail = N;

	return &(N->item);
}

//-----------------------------------------------------------------------------
template<class L> L* CLinkList<L>::addBeforeCurrent()
{
	if ( !m_current )
	{
		return 0;
	}

	Node* N = m_nodePool.get();

	m_count++;

	N->nextIter = m_current;
	N->prevIter = m_current->prevIter;
	m_current->prevIter = N;
	if ( N->prevIter )
	{
		N->prevIter->nextIter = N;
	}
	else
	{
		m_list = N;
	}

	return &(N->item);
}

//-----------------------------------------------------------------------------
template<class L> L* CLinkList<L>::addAfterCurrent()
{
	if ( !m_current )
	{
		return 0;
	}

	Node* N = m_nodePool.get();

	m_count++;

	N->nextIter = m_current->nextIter;
	m_current->nextIter = N;
	N->prevIter = m_current;
	if ( N->nextIter )
	{
		N->nextIter->prevIter = N;
	}
	else
	{
		m_tail = N;
	}

	return &(N->item);
}

//-----------------------------------------------------------------------------
template<class L> bool CLinkList<L>::removeCurrent()
{
	if ( !m_current )
	{
		return false;
	}

	m_count--;

	if ( m_current == m_list )
	{
		if ( m_current->nextIter )
		{
			m_current->nextIter->prevIter = 0;
			m_list = m_current->nextIter;
		}
		else
		{
			m_list = 0;
			m_tail = 0;
		}

		if ( m_clear )
		{
			m_clear( m_current->item );
		}

		m_nodePool.release( m_current );
		m_current = 0;	
	}
	else
	{
		Node *prev = m_current->prevIter;
		m_current->prevIter->nextIter = m_current->nextIter;
		if ( m_current->nextIter )
		{
			m_current->nextIter->prevIter = prev;
		}
		else
		{
			m_tail = prev;
		}

		if ( m_clear )
		{
			m_clear( m_current->item );
		}

		m_nodePool.release( m_current );
		m_current = prev;
	}

	return true;
}

//-----------------------------------------------------------------------------
template<class L> bool CLinkList<L>::remove( L* item )
{
	if ( !item )
	{
		return false;
	}

	for ( Node *node = m_list ; node ; node = node->nextIter )
	{
		if ( &(node->item) == item )
		{
			m_count--;

			if ( node == m_list )
			{
				if ( node->nextIter )
				{
					node->nextIter->prevIter = 0;
					m_list = node->nextIter;
				}
				else
				{
					m_list = 0;
					m_tail = 0;
				}
			}
			else
			{
				Node *prev = node->prevIter;
				node->prevIter->nextIter = node->nextIter;
				if ( node->nextIter )
				{
					node->nextIter->prevIter = prev;
				}
				else
				{
					m_tail = prev;
				}
			}

			if ( m_clear )
			{
				m_clear( node->item );
			}

			m_nodePool.release( node );
			return true;
		}
	}

	return false;
}

#endif
