/*
 * bighashmap.{cc,hh} -- a hash table template that supports removal
 * Eddie Kohler
 *
 * Copyright (c) 2000 Mazu Networks, Inc.
 * Copyright (c) 2003 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#ifndef CLICK_BIGHASHMAP_CC
#define CLICK_BIGHASHMAP_CC

#include "../p2psim//bighashmap.hh"
#include "../p2psim/bighashmap_arena.hh"

#define BIGHASHMAP_REARRANGE_ON_FIND 1

template <class K, class V>
void
HashMap<K, V>::initialize(HashMap_ArenaFactory *factory, int initial_nbuckets)
{
  _nbuckets = initial_nbuckets;
  _buckets = new Elt *[_nbuckets];
  for (int i = 0; i < _nbuckets; i++)
    _buckets[i] = 0;
  set_dynamic_resizing(true);

  _n = 0;

  set_arena(factory);
}

template <class K, class V>
HashMap<K, V>::HashMap()
  : _default_value(), _arena(0)
{
  initialize(0, DEFAULT_INITIAL_NBUCKETS);
}

template <class K, class V>
HashMap<K, V>::HashMap(const V &def, HashMap_ArenaFactory *factory)
  : _default_value(def), _arena(0)
{
  initialize(factory, DEFAULT_INITIAL_NBUCKETS);
}

template <class K, class V>
void
HashMap<K, V>::copy_from(const HashMap<K, V> &o)
  // requires that 'this' is empty and has the same number of buckets as 'o'
  // and the same resize policy
{
  for (int i = 0; i < _nbuckets; i++) {
    Elt **pprev = &_buckets[i];
    *pprev = 0;
    for (const Elt *e = o._buckets[i]; e; e = e->next) {
      Elt *ee = reinterpret_cast<Elt *>(_arena->alloc());
      new(reinterpret_cast<void *>(&ee->key)) K(e->key);
      new(reinterpret_cast<void *>(&ee->value)) V(e->value);
      ee->next = 0;
      *pprev = ee;
      pprev = &ee->next;
    }
  }
}

template <class K, class V>
HashMap<K, V>::HashMap(const HashMap<K, V> &o)
  : _buckets(new Elt *[o._nbuckets]), _nbuckets(o._nbuckets),
    _default_value(o._default_value), _n(o._n), _capacity(o._capacity),
    _arena(o._arena)
{
  _arena->use();
  copy_from(o);
}

template <class K, class V>
HashMap<K, V> &
HashMap<K, V>::operator=(const HashMap<K, V> &o)
{
  if (&o != this) {
    clear();
    _default_value = o._default_value;
    if (_nbuckets < o._nbuckets)
      resize0(o._nbuckets);
    _nbuckets = o._nbuckets;
    copy_from(o);
  }
  return *this;
}

template <class K, class V>
HashMap<K, V>::~HashMap()
{
  for (int i = 0; i < _nbuckets; i++)
    for (Elt *e = _buckets[i]; e; ) {
      Elt *next = e->next;
      e->key.~K();
      e->value.~V();
      _arena->afree(e);
      e = next;
    }
  delete[] _buckets;
  _arena->unuse();
}

template <class K, class V>
void
HashMap<K, V>::set_dynamic_resizing(bool on)
{
  if (!on)
    _capacity = 0x7FFFFFFF;
  else if (_nbuckets >= MAX_NBUCKETS)
    _capacity = 0x7FFFFFFE;
  else
    _capacity = DEFAULT_RESIZE_THRESHOLD * _nbuckets;
}

template <class K, class V>
void
HashMap<K, V>::set_arena(HashMap_ArenaFactory *factory)
{
  assert(empty());
  if (_arena)
    _arena->unuse();
  _arena = HashMap_ArenaFactory::get_arena(sizeof(Elt), factory);
  _arena->use();
}

template <class K, class V>
inline int
HashMap<K, V>::bucket(const K &key) const
{
  return ((unsigned) hashcode(key)) % _nbuckets;
}

template <class K, class V>
typename HashMap<K, V>::Pair *
HashMap<K, V>::find_pair(const K &key) const
{
#if BIGHASHMAP_REARRANGE_ON_FIND
  Elt *prev = 0;
  int b = bucket(key);
  for (Elt *e = _buckets[b]; e; prev = e, e = e->next)
    if (e->key == key) {
      if (prev) {
        // move to front
        prev->next = e->next;
	e->next = _buckets[b];
	_buckets[b] = e;
      }
      return e;
    }
  return 0;
#else
  for (Elt *e = _buckets[bucket(key)]; e; e = e->next)
    if (e->key == key)
      return e;
  return 0;
#endif
}


template <class K, class V>
void
HashMap<K, V>::resize0(int new_nbuckets)
{
  Elt **new_buckets = new Elt *[new_nbuckets];
  for (int i = 0; i < new_nbuckets; i++)
    new_buckets[i] = 0;

  int old_nbuckets = _nbuckets;
  Elt **old_buckets = _buckets;
  _nbuckets = new_nbuckets;
  _buckets = new_buckets;
  if (dynamic_resizing())
    set_dynamic_resizing(true);	// reset threshold
  
  for (int i = 0; i < old_nbuckets; i++)
    for (Elt *e = old_buckets[i]; e; ) {
      Elt *n = e->next;
      int b = bucket(e->key);
      e->next = new_buckets[b];
      new_buckets[b] = e;
      e = n;
    }

  delete[] old_buckets;
}

template <class K, class V>
void
HashMap<K, V>::resize(int want_nbuckets)
{
  int new_nbuckets = 1;
  while (new_nbuckets < want_nbuckets && new_nbuckets < MAX_NBUCKETS)
    new_nbuckets = ((new_nbuckets + 1) << 1) - 1;
  assert(new_nbuckets > 0 && new_nbuckets <= MAX_NBUCKETS);
  if (_nbuckets != new_nbuckets)
    resize0(new_nbuckets);
}

template <class K, class V>
bool
HashMap<K, V>::insert(const K &key, const V &value)
{
  int b = bucket(key);
  for (Elt *e = _buckets[b]; e; e = e->next)
    if (e->key == key) {
      e->value = value;
      return false;
    }

  if (_n >= _capacity) {
    resize(_nbuckets + 1);
    b = bucket(key);
  }
  
  if (Elt *e = reinterpret_cast<Elt *>(_arena->alloc())) {
    new(reinterpret_cast<void *>(&e->key)) K(key);
    new(reinterpret_cast<void *>(&e->value)) V(value);
    e->next = _buckets[b];
    _buckets[b] = e;
    _n++;
  }
  return true;
}

template <class K, class V>
bool
HashMap<K, V>::remove(const K &key)
{
  int b = bucket(key);
  Elt *prev = 0;
  Elt *e = _buckets[b];
  while (e && !(e->key == key)) {
    prev = e;
    e = e->next;
  }
  if (e) {
    if (prev)
      prev->next = e->next;
    else
      _buckets[b] = e->next;
    e->key.~K();
    e->value.~V();
    _arena->afree(e);
    _n--;
    return true;
  } else
    return false;
}

template <class K, class V>
typename HashMap<K, V>::Pair *
HashMap<K, V>::find_pair_force(const K &key, const V &default_value)
{
  int b = bucket(key);
  for (Elt *e = _buckets[b]; e; e = e->next)
    if (e->key == key)
      return e;
  if (_n >= _capacity) {
    resize(_nbuckets + 1);
    b = bucket(key);
  }
  if (Elt *e = reinterpret_cast<Elt *>(_arena->alloc())) {
    new(reinterpret_cast<void *>(&e->key)) K(key);
    new(reinterpret_cast<void *>(&e->value)) V(default_value);
    e->next = _buckets[b];
    _buckets[b] = e;
    _n++;
    return e;
  } else
    return 0;
}

template <class K, class V>
void
HashMap<K, V>::clear()
{
  for (int i = 0; i < _nbuckets; i++) {
    for (Elt *e = _buckets[i]; e; ) {
      Elt *next = e->next;
      e->key.~K();
      e->value.~V();
      _arena->afree(e);
      e = next;
    }
    _buckets[i] = 0;
  }
  _n = 0;
}

template <class K, class V>
void
HashMap<K, V>::swap(HashMap<K, V> &o)
{
  Elt **t_elts;
  V t_v;
  int t_int;
  HashMap_Arena *t_arena;

  t_elts = _buckets; _buckets = o._buckets; o._buckets = t_elts;
  t_int = _nbuckets; _nbuckets = o._nbuckets; o._nbuckets = t_int;
  t_v = _default_value; _default_value = o._default_value; o._default_value = t_v;

  t_int = _n; _n = o._n; o._n = t_int;
  t_int = _capacity; _n = o._capacity; o._capacity = t_int;

  t_arena = _arena; _arena = o._arena; o._arena = t_arena;
}

template <class K, class V>
_HashMap_const_iterator<K, V>::_HashMap_const_iterator(const HashMap<K, V> *hm, bool begin)
  : _hm(hm)
{
  int nb = _hm->_nbuckets;
  typename HashMap<K, V>::Elt **b = _hm->_buckets;
  for (_bucket = 0; _bucket < nb && begin; _bucket++)
    if (b[_bucket]) {
      _elt = b[_bucket];
      return;
    }
  _elt = 0;
}

template <class K, class V>
void
_HashMap_const_iterator<K, V>::operator++(int)
{
  if (_elt->next)
    _elt = _elt->next;
  else {
    int nb = _hm->_nbuckets;
    typename HashMap<K, V>::Elt **b = _hm->_buckets;
    for (_bucket++; _bucket < nb; _bucket++)
      if (b[_bucket]) {
	_elt = b[_bucket];
	return;
      }
    _elt = 0;
  }
}

#if 0
static int
HashMap_partition_elts(void **elts, int left, int right)
{
  void *pivot = elts[(left + right) / 2];

  // loop invariant:
  // elts[i] < pivot for all left_init <= i < left
  // elts[i] > pivot for all right < i <= right_init
  while (left < right) {
    if (elts[left] < pivot)
      left++;
    else if (elts[right] > pivot)
      right--;
    else {
      void *x = elts[left];
      elts[left] = elts[right];
      elts[right] = x;
    }
  }

  return left;
}

void
HashMap_qsort_elts(void **elts, int left, int right)
{
  if (left < right) {
    int split = HashMap_partition_elts(elts, left, right);
    HashMap_qsort_elts(elts, left, split);
    HashMap_qsort_elts(elts, split, right);
  }
}
#endif


// void * partial specialization

template <class K>
void
HashMap<K, void *>::initialize(HashMap_ArenaFactory *factory, int initial_nbuckets)
{
  _nbuckets = initial_nbuckets;
  _buckets = new Elt *[_nbuckets];
  for (int i = 0; i < _nbuckets; i++)
    _buckets[i] = 0;
  set_dynamic_resizing(true);

  _n = 0;

  set_arena(factory);
}

template <class K>
HashMap<K, void *>::HashMap()
  : _default_value(0), _arena(0)
{
  initialize(0, DEFAULT_INITIAL_NBUCKETS);
}

template <class K>
HashMap<K, void *>::HashMap(void *def, HashMap_ArenaFactory *factory)
  : _default_value(def), _arena(0)
{
  initialize(factory, DEFAULT_INITIAL_NBUCKETS);
}

template <class K>
void
HashMap<K, void *>::copy_from(const HashMap<K, void *> &o)
{
  for (int i = 0; i < _nbuckets; i++) {
    Elt **pprev = &_buckets[i];
    *pprev = 0;
    for (const Elt *e = o._buckets[i]; e; e = e->next) {
      Elt *ee = reinterpret_cast<Elt *>(_arena->alloc());
      new(reinterpret_cast<void *>(&ee->key)) K(e->key);
      ee->value = e->value;
      ee->next = 0;
      *pprev = ee;
      pprev = &ee->next;
    }
  }
}

template <class K>
HashMap<K, void *>::HashMap(const HashMap<K, void *> &o)
  : _buckets(new Elt *[o._nbuckets]), _nbuckets(o._nbuckets),
    _default_value(o._default_value), _n(o._n), _capacity(o._capacity),
    _arena(o._arena)
{
  _arena->use();
  copy_from(o);
}

template <class K>
HashMap<K, void *> &
HashMap<K, void *>::operator=(const HashMap<K, void *> &o)
{
  if (&o != this) {
    clear();
    _default_value = o._default_value;
    if (_nbuckets < o._nbuckets)
      resize0(o._nbuckets);
    _nbuckets = o._nbuckets;
    copy_from(o);
  }
  return *this;
}

template <class K>
HashMap<K, void *>::~HashMap()
{
  for (int i = 0; i < _nbuckets; i++)
    for (Elt *e = _buckets[i]; e; ) {
      Elt *next = e->next;
      e->key.~K();
      _arena->afree(e);
      e = next;
    }
  delete[] _buckets;
  _arena->unuse();
}

template <class K>
void
HashMap<K, void *>::set_dynamic_resizing(bool on)
{
  if (!on)
    _capacity = 0x7FFFFFFF;
  else if (_nbuckets >= MAX_NBUCKETS)
    _capacity = 0x7FFFFFFE;
  else
    _capacity = DEFAULT_RESIZE_THRESHOLD * _nbuckets;
}

template <class K>
void
HashMap<K, void *>::set_arena(HashMap_ArenaFactory *factory)
{
  assert(empty());
  if (_arena)
    _arena->unuse();
  _arena = HashMap_ArenaFactory::get_arena(sizeof(Elt), factory);
  _arena->use();
}

template <class K>
inline int
HashMap<K, void *>::bucket(const K &key) const
{
  return ((unsigned) hashcode(key)) % _nbuckets;
}

template <class K>
typename HashMap<K, void *>::Pair *
HashMap<K, void *>::find_pair(const K &key) const
{
#if BIGHASHMAP_REARRANGE_ON_FIND
  Elt *prev = 0;
  int b = bucket(key);
  for (Elt *e = _buckets[b]; e; prev = e, e = e->next)
    if (e->key == key) {
      if (prev) {
        // move to front
        prev->next = e->next;
	e->next = _buckets[b];
	_buckets[b] = e;
      }
      return e;
    }
  return 0;
#else
  for (Elt *e = _buckets[bucket(key)]; e; e = e->next)
    if (e->key == key)
      return e;
  return 0;
#endif
}


template <class K>
void
HashMap<K, void *>::resize0(int new_nbuckets)
{
  Elt **new_buckets = new Elt *[new_nbuckets];
  for (int i = 0; i < new_nbuckets; i++)
    new_buckets[i] = 0;

  int old_nbuckets = _nbuckets;
  Elt **old_buckets = _buckets;
  _nbuckets = new_nbuckets;
  _buckets = new_buckets;
  if (dynamic_resizing())
    set_dynamic_resizing(true);	// reset threshold
  
  for (int i = 0; i < old_nbuckets; i++)
    for (Elt *e = old_buckets[i]; e; ) {
      Elt *n = e->next;
      int b = bucket(e->key);
      e->next = new_buckets[b];
      new_buckets[b] = e;
      e = n;
    }

  delete[] old_buckets;
}

template <class K>
void
HashMap<K, void *>::resize(int want_nbuckets)
{
  int new_nbuckets = 1;
  while (new_nbuckets < want_nbuckets && new_nbuckets < MAX_NBUCKETS)
    new_nbuckets = ((new_nbuckets + 1) << 1) - 1;
  assert(new_nbuckets > 0 && new_nbuckets <= MAX_NBUCKETS);
  if (_nbuckets != new_nbuckets)
    resize0(new_nbuckets);
}

template <class K>
bool
HashMap<K, void *>::insert(const K &key, void *value)
{
  int b = bucket(key);
  for (Elt *e = _buckets[b]; e; e = e->next)
    if (e->key == key) {
      e->value = value;
      return false;
    }

  if (_n >= _capacity) {
    resize(_nbuckets + 1);
    b = bucket(key);
  }
  
  if (Elt *e = reinterpret_cast<Elt *>(_arena->alloc())) {
    new(reinterpret_cast<void *>(&e->key)) K(key);
    e->value = value;
    e->next = _buckets[b];
    _buckets[b] = e;
    _n++;
  }
  return true;
}

template <class K>
bool
HashMap<K, void *>::remove(const K &key)
{
  int b = bucket(key);
  Elt *prev = 0;
  Elt *e = _buckets[b];
  while (e && !(e->key == key)) {
    prev = e;
    e = e->next;
  }
  if (e) {
    if (prev)
      prev->next = e->next;
    else
      _buckets[b] = e->next;
    e->key.~K();
    _arena->afree(e);
    _n--;
    return true;
  } else
    return false;
}

template <class K>
typename HashMap<K, void *>::Pair *
HashMap<K, void *>::find_pair_force(const K &key, void *default_value)
{
  int b = bucket(key);
  for (Elt *e = _buckets[b]; e; e = e->next)
    if (e->key == key)
      return e;
  if (_n >= _capacity) {
    resize(_nbuckets + 1);
    b = bucket(key);
  }
  if (Elt *e = reinterpret_cast<Elt *>(_arena->alloc())) {
    new(reinterpret_cast<void *>(&e->key)) K(key);
    e->value = default_value;
    e->next = _buckets[b];
    _buckets[b] = e;
    _n++;
    return e;
  } else
    return 0;
}

template <class K>
void
HashMap<K, void *>::clear()
{
  for (int i = 0; i < _nbuckets; i++) {
    for (Elt *e = _buckets[i]; e; ) {
      Elt *next = e->next;
      e->key.~K();
      _arena->afree(e);
      e = next;
    }
    _buckets[i] = 0;
  }
  _n = 0;
}

template <class K>
void
HashMap<K, void *>::swap(HashMap<K, void *> &o)
{
  Elt **t_elts;
  void *t_v;
  int t_int;
  HashMap_Arena *t_arena;

  t_elts = _buckets; _buckets = o._buckets; o._buckets = t_elts;
  t_int = _nbuckets; _nbuckets = o._nbuckets; o._nbuckets = t_int;
  t_v = _default_value; _default_value = o._default_value; o._default_value = t_v;

  t_int = _n; _n = o._n; o._n = t_int;
  t_int = _capacity; _n = o._capacity; o._capacity = t_int;

  t_arena = _arena; _arena = o._arena; o._arena = t_arena;
}


template <class K>
_HashMap_const_iterator<K, void *>::_HashMap_const_iterator(const HashMap<K, void *> *hm, bool begin)
  : _hm(hm)
{
  int nb = _hm->_nbuckets;
  typename HashMap<K, void *>::Elt **b = _hm->_buckets;
  for (_bucket = 0; _bucket < nb && begin; _bucket++)
    if (b[_bucket]) {
      _elt = b[_bucket];
      return;
    }
  _elt = 0;
}

template <class K>
void
_HashMap_const_iterator<K, void *>::operator++(int)
{
  if (_elt->next)
    _elt = _elt->next;
  else {
    int nb = _hm->_nbuckets;
    typename HashMap<K, void *>::Elt **b = _hm->_buckets;
    for (_bucket++; _bucket < nb; _bucket++)
      if (b[_bucket]) {
	_elt = b[_bucket];
	return;
      }
    _elt = 0;
  }
}

#endif
