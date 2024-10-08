/*
 * Copyright (c) 2003-2005 Jeremy Stribling (strib@mit.edu)
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

/* $Id: condvar.h,v 1.9 2005/04/15 20:51:26 thomer Exp $ */

#ifndef __CONDVAR_H
#define __CONDVAR_H

#include "../libtask/task.h"
#include "p2psim.h"
#include <set>
#include <unistd.h>
using namespace std;

class ConditionVar {
public:
  ConditionVar();
  ~ConditionVar();

  void wait();
  void wait_noblock(Channel *c);
  void notify();
  void notifyAll();

private:
  set<Channel*> *_waiters;
};

#endif // __CONDVAR_H
