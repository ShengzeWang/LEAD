/*
 * Copyright (c) 2003-2005 Jinyang Li
 *                    Massachusetts Institute of Technology
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "marquesobserver.h"
#include "../p2psim/node.h"
#include "../p2psim/args.h"
#include "../p2psim/network.h"
#include "../protocols/protocolfactory.h"

#include <iostream>
#include <set>
#include <algorithm>
#include <stdio.h>


using namespace std;

MarquesObserver* MarquesObserver::_instance = 0;

MarquesObserver* MarquesObserver::Instance(Args *a)
{
  if(!_instance)
    _instance = New MarquesObserver(a);
  return _instance;
}


MarquesObserver::MarquesObserver(Args *a) : _type("Chord_overlay")
{
  _instance = this;
  if (a)
    _oracle_num = a->nget( "oracle", 0, 10 );
  else
    _oracle_num = 0;

  _totallivenodes = 0;

  assert(_type.find("Chord") == 0 || _type.find("Koorde") == 0);

  ids.clear();
  const set<Node*> *l = Network::Instance()->getallnodes();
    Chord_overlay::IDMap n;
  for(set<Node*>::iterator pos = l->begin(); pos != l->end(); ++pos) {
      Chord_overlay *t = dynamic_cast<Chord_overlay*>(*pos);
    n.ip = t->ip();
    n.id = t->id();
    n.timestamp = 0;
    ids.push_back(n);
    if (_oracle_num)
      t->registerObserver(this);
  }
  sort(ids.begin(),ids.end(),Chord_overlay::IDMap::cmp);
#ifdef CHORD_DEBUG
  for (uint i = 0; i < ids.size(); i++) 
    printf("%qx %u\n", ids[i].id, ids[i].ip);
#endif
}

vector<Chord_overlay::IDMap>
MarquesObserver::get_sorted_nodes()
{
  assert(ids.size()>0);
  return ids;
}

void
MarquesObserver::static_simulation()
{
  const set<Node*> *l = Network::Instance()->getallnodes();
  Topology *t = Network::Instance()->gettopology();
  uint query = 0;
  double all = 0.0;

  for(set<Node*>::iterator pos = l->begin(); pos != l->end(); ++pos) {
      Chord_overlay *start = dynamic_cast<Chord_overlay*>(*pos);
    for (uint i = 0; i < 10; i++) {
      unsigned lat = 0;
        Chord_overlay *curr = start;
        Chord_overlay::CHID key = ConsistentHash::getRandID();
        Chord_overlay::IDMap next = curr->next_hop(key);
      while (next.ip!=curr->ip()) {
	lat += t->latency(curr->ip(),next.ip);
	curr = dynamic_cast<Chord_overlay*>(Network::Instance()->getnode(next.ip));
	next = curr->next_hop(key);
      }
      lat += t->latency(next.ip,start->ip());
      all += (double)lat;
      query++;
    }
  }
  printf("%.3f %u\n",all/query,query);
}

MarquesObserver::~MarquesObserver()
{
}

void
MarquesObserver::kick(Observed *o, ObserverInfo *oi)
{
  if (!oi) return;
  char *event = (char *) oi;
  assert( event );
  string event_s(event);

  assert(_oracle_num);
    Chord_overlay *n = (Chord_overlay *) o;
  assert( n );
  
  set<Node*>::iterator pos;
    Chord_overlay *c = 0;
  const set<Node*> *l = Network::Instance()->getallnodes();
 
  if( event_s == "join" ) {
    //ids.clear();
#ifdef CHORD_DEBUG
    printf("MarquesObserver oracle node %u,%qx joined\n", n->ip(), n->id());
#endif
    //add this newly joined node to the sorted list of alive nodes
    vector<Chord_overlay::IDMap>::iterator p;
    p = upper_bound(ids.begin(),ids.end(),n->idmap(),Chord_overlay::IDMap::cmp);
    if (p->ip != n->ip()) 
      ids.insert(p,1,n->idmap()); 
    for (pos = l->begin(); pos != l->end(); ++pos) {
      c = (Chord_overlay *)(*pos);
      assert(c);
      if (c->alive() && c->ip() != n->ip()) {
	c->oracle_node_joined(n->idmap());
      }
    }
    n->initstate();

  } else if (event_s == "crash") {
#ifdef CHORD_DEBUG
    printf("%llu MarquesObserver oracle node %u,%qx crashed\n", now(), n->ip(), n->id());
#endif
    //delete this crashed node to the sorted list of alive nodes
    vector<Chord_overlay::IDMap>::iterator p;
    p = find(ids.begin(),ids.end(),n->idmap());
    assert(p->ip == n->ip());
    ids.erase(p);

    for (pos = l->begin(); pos != l->end(); ++pos) {
      c = (Chord_overlay *)(*pos);
      assert(c);
      if (c->alive() && c->ip() != n->ip()) 
	c->oracle_node_died(n->idmap());
    }
  }
}
