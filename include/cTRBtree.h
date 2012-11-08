// Copyright �2012 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"
// Insert/delete implementations modified from Arjan van den Boogaard (2004)
// http://archive.gamedev.net/archive/reference/programming/features/TStorage/TStorage.h

#ifndef __C_TRB_TREE_H__BSS__
#define __C_TRB_TREE_H__BSS__

#include "bss_compare.h"
#include "bss_alloc.h"
#include "LLBase.h"

namespace bss_util {
  template<class Key, class Data>
  struct BSS_COMPILER_DLLEXPORT TRB_Node : LLBase<TRB_Node<Key,Data>>
  {
    inline explicit TRB_Node(TRB_Node<Key,Data>* pNIL) : left(pNIL), right(pNIL), color(0), parent(0) { next=0; prev=0; }
    inline TRB_Node(Key k, Data d, TRB_Node<Key,Data>* pNIL) : key(k), data(d), left(pNIL), right(pNIL), color(1), parent(0) { next=0; prev=0; }
    Data data;
    Key key;
    char color; // 0 - black, 1 - red, -1 - duplicate
    TRB_Node<Key,Data>* parent;
    union {
      struct {
        TRB_Node<Key,Data>* left;
        TRB_Node<Key,Data>* right;
      };
      TRB_Node<Key,Data>* children[2];
    };
  };

  // Threaded red-black tree with duplicate values
  template<class Key, class Data, char (*CFunc)(const Key&, const Key&)=CompT<Key>, typename Alloc=Allocator<TRB_Node<Key,Data>>>
	class BSS_COMPILER_DLLEXPORT cTRBtree : protected cAllocTracker<Alloc>
  {
  public:
		// Constructor - takes an optional allocator
    inline explicit cTRBtree(Alloc* allocator=0) : cAllocTracker<Alloc>(allocator), _first(0), _last(0), _root(pNIL) {}
    inline cTRBtree(cTRBtree&& mov) : cAllocTracker<Alloc>(std::move(mov)), _first(mov.first), _last(mov._last), _root(mov._root)
    {
      mov._first=0;
      mov._last=0;
      mov._root=pNIL;
    }
		// Destructor
    inline ~cTRBtree() { Clear(); }
    // Retrieves a given node by key if it exists
    inline TRB_Node<Key,Data>* Get(const Key& key) const { return _get(key); }
    // Retrieves the node closest to the given key.
    inline TRB_Node<Key,Data>* GetNear(const Key& key, bool before=true) const { return _getnear(key,before); }
		// Inserts a key with the associated data
		inline TRB_Node<Key,Data>* Insert(const Key& key, Data data)
    {
      TRB_Node<Key,Data>* node= _allocate(1);
      new(node) TRB_Node<Key,Data>(key,data,pNIL);
      _insert(node);
      return node;
    }
    // Searches for a node with the given key and removes it if found, otherwise returns false.
    inline bool Remove(const Key& key) { return Remove(_get(key)); }
    // Removes the given node. Returns false if node is null
		inline bool Remove(TRB_Node<Key,Data>* node)
    {
      if(!node) return false;
      _remove(node);
      _deallocate(node,1);
      return true;
    }
    // Replaces the given key with newkey
    inline bool ReplaceKey(const Key& key, const Key& newkey) { return ReplaceKey(_get(key),newkey); }
    // Replaces the key of the given node to newkey
		inline bool ReplaceKey(TRB_Node<Key,Data>* node, const Key& newkey)
    {
      if(!node) return false;
      _remove(node);
      new(node) TRB_Node<Key,Data>(newkey,node->data,pNIL);
      _insert(node);
      return true;
    }
    // Clears the tree (called by destructor)
		inline void Clear()
	  {
		  TRB_Node<Key,Data>* cur=_first;
		  TRB_Node<Key,Data>* hold;

		  while(cur!=0) // Walk through the tree using the linked list and deallocate everything
		  {
			  hold = cur;
        cur = cur->next;
			  _deallocate(hold,1);
		  }

		  _first = 0;
		  _last = 0;
		  _root = pNIL;
	  }
    // Returns first element
		inline TRB_Node<Key,Data>* GetFirst() const { return _first; }
    // Returns last element
		inline TRB_Node<Key,Data>* GetLast() const { return _last; }
    
    inline cTRBtree& operator=(cTRBtree&& mov) // Move assignment operator
    {
      Clear();
      cAllocTracker<Alloc>::operator=(std::move(mov));
      _first=mov._first;
      _last=mov._last;
      _root=mov._root;
      mov._first=0;
      mov._last=0;
      mov._root=pNIL;
    }

  protected:
	  inline TRB_Node<Key,Data>* _get(const Key& key) const
	  {
      TRB_Node<Key,Data>* cur=_root;

      while(cur!=pNIL)
      {
        switch(CFunc(key,cur->key)) //This is faster then if/else statements
        {
        case -1: cur=cur->left; break;
        case 1: cur=cur->right; break;
        default: return cur;
        }
      }

      return 0;
	  }
	  inline TRB_Node<Key,Data>* _getnear(const Key& key, bool before) const
	  {
      TRB_Node<Key,Data>* cur=_root;
      TRB_Node<Key,Data>* parent;
      char res=0;

      while(cur!=pNIL)
      {
        parent=cur;
        switch(res=CFunc(key,cur->key)) //This is faster then if/else statements
        {
        case -1: cur=cur->left; break;
        case 1: cur=cur->right; break;
        default: return cur;
        }
      }

      if(before)
        return (res<0 || !parent->next)?parent:parent->next;
      else
        return (res>0 || parent->prev)?parent:parent->prev;
    }
    inline void BSS_FASTCALL _leftrotate(TRB_Node<Key,Data>* node)
    {
      TRB_Node<Key,Data>* r=node->right;

      node->right=r->left;
      if(node->right!=pNIL) node->right->parent=node;
      if(r != pNIL) r->parent=node->parent;

      if(node->parent)
        node->parent->children[node->parent->right == node]=r;
      else
        _root=r;

		  r->left = node;
		  if (node != pNIL) node->parent = r;
    }
    inline void BSS_FASTCALL _rightrotate(TRB_Node<Key,Data>* node)
    {
      TRB_Node<Key,Data>* r = node->left;

      node->left = r->right;
      if(node->left!=pNIL) node->left->parent=node;
	    if(r != pNIL) r->parent=node->parent;

      if(node->parent)
        node->parent->children[node->parent->right == node]=r;
      else
        _root=r;

      r->right = node;
		  if(node != pNIL) node->parent = r;
    }
	  inline void _insert(TRB_Node<Key,Data>* node)
	  {
		  TRB_Node<Key,Data>* cur=_root;
		  TRB_Node<Key,Data>* parent=0;
		  int c;
     
      while(cur!=pNIL)
      {
        parent=cur;
			  switch(c = CFunc(node->key, cur->key))
        {
        case -1: cur=cur->left; break;
        case 1: cur=cur->right; break;
        default: // duplicate
          if(cur->next!=0 && cur->next->color==-1)
          {
            if((cur=_treenext(cur))!=0) LLInsertFull(node,cur,_first);
            else _last=LLAdd(node,_last);
          }
          else
            LLInsertAfterFull(node,cur,_last);

          node->color=-1; //set color to duplicate
          return; //terminate, we have nothing else to do since this node isn't actually in the tree
        }
      }
    
      if(parent!=0)
      {
        node->parent=parent; //set parent

        if(c<0) //depending on if its less than or greater than the parent, set the appropriate child variable
        {
          parent->left = node;
          LLInsertFull(node,parent,_first); //Then insert into the appropriate side of the list
        }
        else
        {
          parent->right = node;
          if(parent->next!=0 && parent->next->color==-1)
          {
            if((parent=_treenextsub(parent))==0) _last=LLAdd(node,_last);
            else LLInsertFull(node, parent, _first);
          }
          else
            LLInsertAfterFull(node, parent, _last);
        }
			  _fixinsert(node);
      }
      else //this is the root node so re-assign
      {
        _first=_last=_root=node; //assign to _first and _last
        _root->color=0; //root is always black (done below)
      }
	  }
	  inline void _fixinsert(TRB_Node<Key,Data>* node)
	  {
		  while(node != _root && node->parent->color == 1)
		  {
			  if(node->parent == node->parent->parent->left)
			  {
				  TRB_Node<Key,Data>* y = node->parent->parent->right;
				  if(y->color == 1)
				  {
					  node->parent->color = 0;
					  y->color = 0;
					  node->parent->parent->color = 1;
					  node = node->parent->parent;
				  }
				  else
				  {
					  if(node == node->parent->right)
					  {
						  node = node->parent;
						  _leftrotate(node);
					  }

					  node->parent->color = 0;
					  node->parent->parent->color = 1;
					  _rightrotate(node->parent->parent);
				  }
			  }
			  else
			  {
				  TRB_Node<Key,Data>* y = node->parent->parent->left;
				  if(y->color == 1)
				  {
					  node->parent->color = 0;
					  y->color = 0;
					  node->parent->parent->color = 1;
					  node = node->parent->parent;
				  }
				  else
				  {
					  if(node == node->parent->left)
					  {
						  node = node->parent;
						  _rightrotate(node);
					  }
					  node->parent->color = 0;
					  node->parent->parent->color = 1;
					  _leftrotate(node->parent->parent);
				  }
			  }
		  }

		  _root->color = 0;
	  }
	  inline void _remove(TRB_Node<Key,Data>* node)
	  {
		  if(node->color == -1) { LLRemove(node,_first,_last); return; }
		  if(node->next && node->next->color == -1) { _replacenode(node, node->next); LLRemove(node,_first,_last); return; }

		  LLRemove(node,_first,_last);
		  TRB_Node<Key,Data>*  y;
		  TRB_Node<Key,Data>*  z;
		  bool balance;

      if(node->left != pNIL && node->right != pNIL)
        y=_findmin(node->right);
      else
        y = node;
			
      z = y->children[y->left == pNIL];
		  z->parent = y->parent;

		  if(y->parent!=0)
        y->parent->children[y == y->parent->right] = z;
		  else
			  _root = z;

		  balance = (y->color == 0);

		  if(y != node) _replacenode(node, y);
		  if(balance) _fixdelete(z);
	  }
	  inline void _fixdelete(TRB_Node<Key,Data>* node)
	  {
		  while (node != _root && node->color == 0)
		  {
			  if (node == node->parent->left)
			  {
				  TRB_Node<Key,Data>* w = node->parent->right;
				  if (w->color == 1)
				  {
					  w->color = 0;
					  node->parent->color = 1;
					  _leftrotate(node->parent);
					  w = node->parent->right;
				  }
				  if (w->left->color == 0 && w->right->color == 0)
				  {
					  w->color = 1;
					  node = node->parent;
				  }
				  else
				  {
					  if (w->right->color == 0)
					  {
						  w->left->color = 0;
						  w->color = 1;
						  _rightrotate(w);
						  w = node->parent->right;
					  }
					  w->color = node->parent->color;
					  node->parent->color = 0;
					  w->right->color = 0;
					  _leftrotate(node->parent);
					  node = _root;
				  }
			  }
			  else
			  {
				  TRB_Node<Key,Data>* w = node->parent->left;
				  if (w->color == 1) {
					  w->color = 0;
					  node->parent->color = 1;
					  _rightrotate (node->parent);
					  w = node->parent->left;
				  }
				  if (w->right->color == 0 && w->left->color == 0)
				  {
					  w->color = 1;
					  node = node->parent;
				  }
				  else
				  {
					  if (w->left->color == 0)
					  {
						  w->right->color = 0;
						  w->color = 1;
						  _leftrotate(w);
						  w = node->parent->left;
					  }
					  w->color = node->parent->color;
					  node->parent->color = 0;
					  w->left->color = 0;
					  _rightrotate(node->parent);
					  node = _root;
				  }
			  }
		  }
		  node->color = 0;
	  }
	  inline void _replacenode(TRB_Node<Key,Data>* node, TRB_Node<Key,Data>* y)
	  {
      y->color = node->color;
      y->left = node->left;
      y->right = node->right;
      y->parent = node->parent;

      if (y->parent!=0)
        y->parent->children[y->parent->right == node] = y;
      else
	      _root = y;

      y->left->parent = y;
      y->right->parent = y;
	  }

    inline static TRB_Node<Key,Data>* _treenext(TRB_Node<Key,Data>* node)
	  {
		  if (node->right != pNIL) return _findmin(node->right);
      return _treenextsub(node);
	  }
    inline static TRB_Node<Key,Data>* _treenextsub(TRB_Node<Key,Data>* node)
	  {
      while (node->parent && node != node->parent->left)
			  node = node->parent;
		  return node->parent;
	  }
    inline static TRB_Node<Key,Data>* _findmin(TRB_Node<Key,Data>* node)
	  {
		  while (node->left != pNIL) node = node->left;
		  return node;
	  }

		TRB_Node<Key,Data>*  _first;
		TRB_Node<Key,Data>*  _last;
		TRB_Node<Key,Data>*  _root;
		static TRB_Node<Key,Data> NIL;
    static TRB_Node<Key,Data>* pNIL;
  };

  template<class K, class D, char (*C)(const K&, const K&), typename A>
	TRB_Node<K,D> cTRBtree<K,D,C,A>::NIL(&cTRBtree<K,D,C,A>::NIL);
  template<class K, class D, char (*C)(const K&, const K&), typename A>
  TRB_Node<K,D>* cTRBtree<K,D,C,A>::pNIL=&cTRBtree<K,D,C,A>::NIL;
}

#endif