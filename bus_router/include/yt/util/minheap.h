#pragma once
#include"stdio.h"
#include<map>
#include<iostream>
#include <assert.h>
using namespace std;
const int EXTSIZE=1000;
const int MAXSIZE=100000;
const int SIZETHRESHOLD=1500;
template<typename T>
class MinHeap
{
	public:
		MinHeap():CurrentSize(0),AllocatedSize(1000)
		{
			heap=new(nothrow) T[AllocatedSize];
			if(!heap)
				assert(0);
		}

		~MinHeap()
		{
			if(heap)
			{
				delete []heap;
				heap =NULL;
			}
		}
		bool Insert(const T &x)
		{
			if(CurrentSize >= MAXSIZE)
			{
				return false;
			}
			if(CurrentSize==AllocatedSize)
			{
				AllocatedSize+=EXTSIZE;
				if(AllocatedSize > MAXSIZE)
					AllocatedSize = MAXSIZE;
				heap=( T *) realloc(heap,AllocatedSize * sizeof(T));
			}
			heap=Shrink();
			if(!heap)
			{
				return false;
			}
			heap[CurrentSize]=x;
			//m_map[x]=CurrentSize;
			FilterUp(CurrentSize);
			CurrentSize++;
			return true;

		}

		T RemoveMin()
		{
			T x=heap[0];
			heap[0]=heap[CurrentSize-1];
			CurrentSize--;
			/*
			typename multimap< T,int >::iterator it=m_map.find(x);
			if(it!=m_map.end())
				m_map.erase(it);
				*/
			typename multimap< T,int >::iterator it=Search(x,0);
			if(it!=m_map.end())
				m_map.erase(it);
			/*
			it=m_map.find(heap[0]);
			if(it!=m_map.end())
				m_map.erase(it);
				*/
			it=Search(heap[0],CurrentSize);
			if(it!=m_map.end())
				m_map.erase(it);
			if(CurrentSize==0)
				return x;
			FilterDown(0,CurrentSize-1); 
			heap=Shrink();
			return x;

		}

		bool  Del(T x )
		{
			typename multimap< T,int >::iterator it=m_map.find(x);
			if(it!=m_map.end())
			{
				int id=it->second;
				heap[id]=heap[CurrentSize-1];
				CurrentSize--;
				m_map.erase(it);
				if(id == CurrentSize)
					return true;
				/*
				it=m_map.find(heap[id]);
				if(it!=m_map.end())
					m_map.erase(it);
					*/
				it=Search(heap[id],CurrentSize);
				if(it!=m_map.end())
					m_map.erase(it);
				if(CurrentSize==0)
					return true;
				heap=Shrink();
				FilterDown(id,CurrentSize-1);
				return true;
			}
			return false;
		}

		T GetMin()
		{
			return heap[0];
		}

		bool IsEmpty() const
		{
			return CurrentSize==0;
		}
		bool IsFull() const
		{
			return CurrentSize==MAXSIZE;
		}

		void Clear()
		{
			CurrentSize=0;
			m_map.clear();
		}

		int MapSize()
		{
			return m_map.size();
		}

                int HeapSize()
		{
			return CurrentSize;
		}

		T * GetHeap()
		{
                         return heap;
		}
	private:
		T *heap;
		int CurrentSize;
		int AllocatedSize;
		multimap<T,int> m_map;
		T * Shrink()
		{
			if(AllocatedSize-CurrentSize >=SIZETHRESHOLD)
			{
				AllocatedSize=(CurrentSize/EXTSIZE+1)*EXTSIZE;
				return ( T *) realloc(heap,AllocatedSize * sizeof(T));
			}
			return heap;

		}
		typename multimap< T,int >::iterator Search(T x, int id)
		{
			typename multimap< T,int >::iterator beg=m_map.lower_bound(x),back=m_map.upper_bound(x);
			while(beg!=back)
			{       
				if(beg->second==id)
				{
					return beg;
				}
				else beg++;
			}
			return m_map.end();
							
		}
		void FilterUp(int start) 
		{
			int j=start,i=(j-1)/2; 
			T temp=heap[j];
			int flag=j;
			while(j>0)
			{
				if(heap[i]<=temp)
					break;
				else
				{
					heap[j]=heap[i];
					typename multimap< T,int >::iterator it=Search(heap[i],i);
					if(it!=m_map.end())	
						m_map.erase(it);
					/*
					   typename multimap< T,int >::iterator it=m_map.find(heap[i]);
					   if(it!=m_map.end())
					   m_map.erase(it);
					 */
					m_map.insert(make_pair(heap[i],j));
					j=i;
					i=(i-1)/2;
				}
			}
			heap[j]=temp;
			typename multimap< T,int >::iterator it=Search(temp,flag);
			if(it!=m_map.end())
				m_map.erase(it);
			m_map.insert(make_pair(temp,j));

		}
		void FilterDown(const int start,const int end) 
		{
			int i=start,j=2*i+1;
			T temp=heap[i];
			int flag =i;
			while(j<=end)
			{
				if( (j<end) && (heap[j]>heap[j+1]) )
					j++;
				if(temp<=heap[j])
					break;
				else
				{
					heap[i]=heap[j];
					/*
					typename multimap< T,int >::iterator it=m_map.find(heap[j]);
					if(it!=m_map.end())
						m_map.erase(it);
						*/
					typename multimap< T,int >::iterator it=Search(heap[j],j);
					if(it!=m_map.end())
						m_map.erase(it);
					m_map.insert(make_pair(heap[j],i));
					i=j;
					j=2*j+1;
				}

			}
			heap[i]=temp;
			/*
			typename multimap< T,int >::iterator it=m_map.find(temp);
			if(it!=m_map.end())
				m_map.erase(it);
				*/
			typename multimap< T,int >::iterator it=Search(temp,flag);
			if(it!=m_map.end())
				m_map.erase(it);
			m_map.insert(make_pair(temp,i));
		}

};
