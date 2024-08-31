/* Updated by Shengze Wang 2023
 *
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

#include  "learned_dht.h"
#include "../observers/learneddhtobserver.h"
#include <iostream>

using namespace std;

extern bool static_sim;

VNode::VNode(IPAddress i, Args &a, LocTable_vnodes *l) : Chord_vnodes(i, a, l) {
    _base = a.nget<uint>("base", 2, 10);
    _fingerlets = a.nget<uint>("fingerlets", 1, 10);

    _stab_finger_running = false;
    _stab_finger_outstanding = 0;
    _stab_finger_timer = a.nget<uint>("fingertimer", 10000, 10);
}

void VNode::initstate() {
    vector<IDMap> ids = LearnedDHTObserver::Instance(NULL)->get_sorted_nodes();
    uint sz = ids.size();
    uint my_pos = find(ids.begin(), ids.end(), me) - ids.begin();
    assert(ids[my_pos].id == me.id);
    CHID min_lap = ids[(my_pos + 1) % sz].id - me.id;
    CHID lap = (CHID) - 1;
    IDMap tmpf;
    while (lap > min_lap) {
        lap = lap / _base;
        for (uint j = 1; j <= (_base - 1); j++) {
            if (lap * j < min_lap) continue;
            tmpf.id = lap * j + me.id;
            uint s_pos = upper_bound(ids.begin(), ids.end(), tmpf, Chord_vnodes::IDMap::cmp) - ids.begin();
            s_pos = s_pos % sz;
            if (ConsistentHash::between(tmpf.id, tmpf.id + lap, ids[s_pos].id))
                loctable->add_node(ids[s_pos]);
        }
    }
    Chord_vnodes::initstate();
}

void VNode::fix_fingers(bool restart) {

    vector<IDMap> scs = loctable->succs(me.id + 1, _nsucc);
    CDEBUG(3) << "fix_fingers start sz " << loctable->size() << endl;
    uint new_fingers, valid_fingers, skipped_fingers, dead_fingers, check_fingers, missing_finger;
    missing_finger = dead_fingers = new_fingers = valid_fingers = skipped_fingers = dead_fingers = check_fingers = 0;

    if (scs.size() == 0) return;

    vector<IDMap> v;
    CHID finger;
    Chord_vnodes::IDMap currf, prevf, prevfpred;
    bool ok;

    CHID lap = (CHID) - 1;

    prevf.ip = 0;
    prevfpred.ip = 0;

    while (1) {

        lap = lap / _base;
        for (uint j = (_base - 1); j >= 1; j--) {

            finger = lap * j + me.id;
            if (ConsistentHash::between(me.id, scs[scs.size() - 1].id, finger))
                goto FINGER_DONE;

            check_fingers++;
            currf = loctable->succ(finger);
            if (currf.ip == me.ip) currf.ip = 0;

            if ((!restart) && (currf.ip)) {
                if (ConsistentHash::between(finger, finger + lap, currf.id)) {
                    LocTable_vnodes::idmapwrap *naked = loctable->get_naked_node(currf.id);
                    assert(naked);
                    if ((now() - naked->n.timestamp) < _stab_finger_timer) {
                        skipped_fingers++;
                        continue;
                    } else {
                        assert(currf.ip != prevf.ip);
                        //just ping this finger to see if it is alive
                        prevf = currf;
                        prevfpred.ip = 0;

                        get_predsucc_args gpa;
                        get_predsucc_ret gpr;
                        gpa.pred = true;
                        gpa.m = (_fingerlets - 1);
                        ok = failure_detect(currf, &Chord_vnodes::get_predsucc_handler, &gpa, &gpr, TYPE_FINGER_UP, 0,
                                            0);
                        if (!alive()) return;
                        if (ok) {
                            valid_fingers++;
                            record_stat(currf.ip, me.ip, TYPE_FINGER_UP, 1 + gpr.v.size());
                            assert(gpr.dst.ip == currf.ip);
                            loctable->add_node(gpr.dst);
                            prevfpred = gpr.n;
                            if (ConsistentHash::between(finger, finger + lap, prevfpred.id))
                                loctable->add_node(prevfpred);
                            for (uint k = 0; k < gpr.v.size(); k++)
                                loctable->add_node(
                                        gpr.v[k]); //XXX: i am not careful about who to add, might add dead nodes again
                            continue;
                        } else {
                            dead_fingers++;
                            loctable->del_node(currf);
                        }
                    }
                } else {
                    missing_finger++;
                    if ((prevf.ip == currf.ip) && (prevfpred.ip)) {
                        if (ConsistentHash::between(finger, finger + lap, prevfpred.id)) {
                            loctable->add_node(prevfpred);
                            continue;
                        } else if (prevfpred.ip && ConsistentHash::between(prevfpred.id, prevf.id, finger)) {
                            //skip;
                            continue;
                        }
                    }
                }
            }
            if (_recurs)
                v = find_successors_recurs(finger, _fingerlets, TYPE_FINGER_LOOKUP, NULL);
            else
                v = find_successors(finger, _fingerlets, TYPE_FINGER_LOOKUP, 0);

            if (v.size() > 0)
                CDEBUG(3) << "fix_fingers " << j << " finger " << printID(finger)
                          << "get " << v[0].ip << "," << printID(v[0].id) << endl;
            new_fingers++;
            for (uint k = 0; k < v.size(); k++)
                loctable->add_node(v[k]); //XXX: might add dead nodes again and again
        }
    }
    FINGER_DONE:
    // migrate key pairs
    next_recurs_ret *ret = New next_recurs_ret;
    lookup_args *b = New lookup_args;
    b->data_address = &key_pairs;
    b->key = me.id;
    bool migrate_status = doRPC(loctable->succ(me.id + 1).ip, &Chord_vnodes::migrate_data, b, ret);
    CDEBUG(3) << "fix_fingers done sz " << loctable->size() << " fingers "
              << check_fingers << " skipped " << skipped_fingers << " valid "
              << valid_fingers << " dead " << dead_fingers << " missing " <<
              missing_finger << " new " << new_fingers << endl;
    return;
}

void VNode::join(Args *args) {
    //args->display();
    Chord_vnodes::join(args);
    if ((static_sim) || !alive()) return;
    //schedule finger stabilizer
    if (!_stab_finger_running) {
        _stab_finger_running = true;
        reschedule_finger_stabilizer((void *) 1); //a hack, no null means restart fixing fingres
    } else if (_join_scheduled == 0) {
        VNode::fix_fingers();
    }
}

void VNode::reschedule_finger_stabilizer(void *x) {
    //printf("%s start stabilizing\n",ts());
    if (!alive()) {
        _stab_finger_running = false;
        return;
    }

    _stab_finger_running = true;
    if (_stab_finger_outstanding > 0) {
    } else {
        _stab_finger_outstanding++;
        fix_fingers(x != NULL);
        _stab_finger_outstanding--;
        assert(_stab_finger_outstanding == 0);
    }
    delaycb(_stab_finger_timer, &VNode::reschedule_finger_stabilizer, (void *) 0);
}

bool VNode::stabilized(vector<CHID> lid) {
    bool ret = Chord_vnodes::stabilized(lid);
    if (!ret) return ret;

    uint sz = lid.size();
    uint my_pos = find(lid.begin(), lid.end(), me.id) - lid.begin();
    assert(lid[my_pos] == me.id);
    CHID min_lap = lid[(my_pos + 1) % sz] - me.id;

    vector<CHID>::iterator it;
    CHID finger;
    uint pos;

    CHID lap = (CHID) - 1;

    IDMap succ;
    uint numf = 0;
    while (lap > min_lap) {
        lap = lap / _base;
        for (uint j = 1; j <= (_base - 1); j++) {
            if ((lap * j) < min_lap) continue;
            finger = lap * j + me.id;
            it = upper_bound(lid.begin(), lid.end(), finger);
            pos = it - lid.begin();
            if (pos >= lid.size()) {
                pos = 0;
            }
            succ = loctable->succ(finger);
            if (lid[pos] != succ.id) {
                // printf("%s not stabilized, %qx,%d finger (%qx) should be %qx instead of (%u,%qx)\n", ts(), lap, j, finger, lid[pos], succ.ip, succ.id);
                return false;
            }
            numf++;
        }
    }
    return true;
}

void VNode::dump() {
    Chord_vnodes::dump();
    IDMap succ = loctable->succ(me.id + 1);
    CHID min_lap = succ.id - me.id;
    CHID lap = (CHID) - 1;
    CHID finger;

    while (lap > min_lap) {
        lap = lap / _base;
        for (uint j = 1; j <= (_base - 1); j++) {
            if ((lap * j) < min_lap) continue;
            finger = lap * j + me.id;
            succ = loctable->succ(finger);
            if (succ.ip > 0) {
                // printf("%qx: finger: %qx,%d : %qx : succ %qx\n", me.id, lap, j, finger, succ.id);
            }
        }
    }
}
