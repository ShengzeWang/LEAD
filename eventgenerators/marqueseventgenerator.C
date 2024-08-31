/*
 * Copyright (c) 2003-2005 Jeremy Stribling (strib@mit.edu)
 *                    Thomer M. Gil (thomer@csail.mit.edu)
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
 *
 *
 * See the HOWTO in eventgenerators/churneventgenerator.h .
 */

#include "marqueseventgenerator.h"
#include "../p2psim/eventqueue.h"
#include "../p2psim/network.h"
#include "../events/p2pevent.h"
#include "../events/simevent.h"
#include <math.h>
#include <time.h>
#include <list>
#include <stdlib.h>
#include <iostream>
#include "../observers/datastoreobserver.h"
#include "../learned_hash_function/rmi.h"
//#include "../learned_hash_function/pgm.cpp"
#include <random>


using namespace std;

MarquesEventGenerator::MarquesEventGenerator(Args *args) {
    // args->display();
    if ((*args)["wkn"] == "") {
        _wkn_string = "1";
        _wkn = 1;
    } else {
        _wkn_string = (*args)["wkn"];
        _wkn = args->nget<IPAddress>("wkn", 1, 10);
    }

    _lifemean = args->nget("lifemean", 3600000, 10); //0 means no failure
    _deathmean = args->nget("deathmean", _lifemean, 10); //0 means no failure
    _lookupmean = args->nget("lookupmean", 3600000, 10);
    _alpha = args->fget("alpha", 1.0);
    _beta = args->nget("beta", 1800000, 10);
    _pareto = args->nget("pareto", 0, 10);
    _uniform = args->nget("uniform", 0, 10);
    if (_pareto && _uniform)
        abort();

    if ((*args)["exittime"] == "") {
        _exittime_string = "200000";
        _exittime = 200000;
    } else {
        _exittime_string = (*args)["exittime"];
        _exittime = args->nget("exittime", 200000, 10);
    }

    Node::set_collect_stat_time(args->nget("stattime", 0, 10));

    _ipkeys = args->nget("ipkeys", 0, 10);
    _datakeys = args->nget("datakeys", 0, 10);

    _ips = NULL;

    EventQueue::Instance()->registerObserver(this);
}

void MarquesEventGenerator::run() {
    // first register the exit event
    vector<string> simargs;
    simargs.push_back(_exittime_string);
    simargs.push_back("exit");
    SimEvent *se = New SimEvent(&simargs);
    add_event(se);
    // Load L1
    std::cout << "RMI status: " << rmi::load("../learned_hash_function/rmi_data") << std::endl;

    // start all nodes at a random time between 1 and n (except the wkn, who
    // starts at 1)
    int batch_size = 100;
    vector<IPAddress> *_ips = Network::Instance()->getallfirstips();
    IPAddress ip = 0;
    int num_of_vnode = 5;
    int num_of_keys = 100000; // Simulation of num of key value pairs per virtual node
    for (u_int i = 0; i < _ips->size(); i = i + 1) {
        //cout<< _ips->size()<<endl;
        ip = (*_ips)[i];
        Args *a = New Args();
        (*a)["wellknown"] = _wkn_string;
        (*a)["first"] = _wkn_string; //a hack
        (*a)["num_of_vnode"] = to_string(num_of_vnode);
        (*a)["num_of_keys"] = to_string(num_of_keys);
        (*a)["batch_size"] = to_string(batch_size);
        int pair_group = ip / (num_of_vnode);
        int pair_num = ip % (num_of_vnode);
        if (pair_num == 0) {
            pair_group -= 1;
        }
        (*a)["pair_group"] = to_string(pair_group);
        u_int jointime;
        if (ip == _wkn) {
            jointime = 1;
            //jointime = i;
        } else {
            // add one to the mod factor because typical 2^n network sizes
            // make really bad mod factors
            //jointime = (random()%(_ips->size()+1)) + 1;
            jointime = 1;
        }
        if (now() + jointime < _exittime) {
            P2PEvent *e = New P2PEvent(now() + jointime, ip, "join", a);
            add_event(e);
        } else {
            delete a;
            a = NULL;
        }
        /*
        if (single_key_look_up) {
            // also schedule their first lookup
            a = New Args();
            Args *b = New Args();
            Time tolookup = next_exponential(_lookupmean);
            string s = get_lookup_key();
            (*a)["key"] = s;
            if (_lookupmean > 0 && now() + jointime + tolookup < _exittime) {
                P2PEvent *e = New P2PEvent(now() + jointime + tolookup, ip, "lookup", a);
                //add_event(e);
            } else {
                delete a;
            }
        }*/
    }
    int query_type = 2; // set the experiment type
    unsigned long long or_key = 17856454719605225760;
    unsigned long long start_loc = ConsistentHash::ip2chid(or_key);
    int range = 1000;
    int num_of_query = range/batch_size;
    int delay_between_batch = 200;
    int system_delay = 0;
    //int  random_node_ip = 1 + std::rand() % _ips->size();
    switch (query_type) {
        case 1:
            printf("Experiment - Range Query Batch enabled\n");
            for (int i = 0; i < num_of_query; ++i) {
                //cout<<"start_loc: "<<start_loc<<endl;
                Args *b = New Args();
                (*b)["wellknown"] = _wkn_string;
                (*b)["first"] = _wkn_string; //a hac
                (*b)["num_of_vnode"] = to_string(num_of_vnode);
                (*b)["key"] = to_string(start_loc);
                P2PEvent *e = New P2PEvent(now() + 1000000 + delay_between_batch, 6, "lookup", b);
                add_event(e);
                or_key += 1;
                start_loc = ConsistentHash::ip2chid(or_key);
                system_delay += delay_between_batch;
            }
            printf("simulted_delays: %d\n", system_delay);
            break;
        case 2:
            printf("Experiment - Range Query LearnedDHT enabled\n");

            //average keys can be get in one hop = num_of_keys/2 << query_range

            //cout<<"start_loc: "<<start_loc<<endl;
            Args *b = New Args();
            (*b)["wellknown"] = _wkn_string;
            (*b)["first"] = _wkn_string; //a hac
            (*b)["num_of_vnode"] = to_string(num_of_vnode);
            (*b)["key"] = to_string(start_loc);
            P2PEvent *e = New P2PEvent(now() + 1000000 + delay_between_batch, 6, "lookup", b);
            add_event(e);
            or_key += 1;
            start_loc = ConsistentHash::ip2chid(or_key);
            system_delay += delay_between_batch;

            printf("simulted_delays: %d\n", system_delay);
            break;
    }
    EventQueue::Instance()->go();
}

void MarquesEventGenerator::kick(Observed *o, ObserverInfo *oi) {
    assert(oi);

    Event *ev = (Event *) oi;
    assert(ev);

    if (ev->name() != "P2PEvent") {
        // not a p2p event, so we don't care
        return;
    }
    P2PEvent *p2p_observed = (P2PEvent *) ev;
    assert(p2p_observed);

    Args *a = New Args();
    IPAddress ip = p2p_observed->node->first_ip();
    if (p2p_observed->type == "join") {

        // the wellknown can't crash also if lifemean is zero, this node won't die
        p2p_observed->node->record_join();
        if ((ip != _wkn) && (_lifemean > 0)) {

            // pick a time for this node to die
            Time todie = 0;
            while (!todie) {
                if (_uniform)
                    todie = next_uniform(_lifemean);
                else if (_pareto)
                    todie = next_pareto(_alpha, _beta);
                else
                    todie = next_exponential(_lifemean);
            }
            if (now() + todie < _exittime) {
                P2PEvent *e = New P2PEvent(now() + todie, ip, "crash", a);
                //add_event(e);
            }

        } else {
            delete a;
        }

    } else if (p2p_observed->type == "crash") {
        p2p_observed->node->record_crash();
        // pick a time for the node to rejoin
        Time tojoin = 0;
        while (!tojoin) {
            if (_uniform)
                tojoin = next_uniform(_deathmean);
            else if (_pareto)
                tojoin = next_pareto(_alpha, _beta);
            else
                tojoin = next_exponential(_deathmean);
        }
        (*a)["wellknown"] = _wkn_string;
        //cout << now() << ": joining " << ip << " in " << tojoin << " ms" << endl;
        if (now() + tojoin < _exittime) {
            P2PEvent *e = New P2PEvent(now() + tojoin, ip, "join", a);
            //add_event(e);
        } else {
            delete a;
        }

    } else if (p2p_observed->type == "lookup") {
        // pick a time for the next lookup
        Time tolookup = next_exponential(_lookupmean);
        string s = get_lookup_key();
        (*a)["key"] = s;
        if (now() + tolookup < _exittime) {
            //      cout << now() << ": Scheduling lookup to " << ip << " in " << tolookup
            //	   << " for " << (*a)["key"] << endl;
            P2PEvent *e = New P2PEvent(now() + tolookup, ip, "lookup", a);
            //add_event(e);
        } else {
            delete a;
        }

    } else {
        delete a;
    }

}

Time MarquesEventGenerator::next_uniform(u_int mean) {
    //time is uniformly distributed between 0.1*mean and 1.9*mean
    double x = ((double) random() / (double) (RAND_MAX));
    Time rt = (Time) ((0.1 + 1.8 * x) * mean);
    return rt;
}

Time MarquesEventGenerator::next_pareto(double a, u_int b) {
    double x = ((double) random() / (double) (RAND_MAX));
    double xx = exp(log(1 - x) / a);
    Time rt = (Time) ((double) b / xx);
    //printf("CHEESE %llu %.3f\n",rt,xx);
    return rt;
}

Time MarquesEventGenerator::next_exponential(u_int mean) {
    assert(mean > 0);
    double x = ((double) random() / (double) (RAND_MAX));
    u_int rt = (u_int) ((-(mean * 1.0)) * log(1 - x));
    return (Time) rt;

}

string MarquesEventGenerator::get_lookup_key() {
    if (_datakeys) {
        DataItem vd = DataStoreObserver::Instance(NULL)->get_random_item();
        char buf[10];
        sprintf(buf, "%llX", vd.key);
        return string(buf);
    }

    if ((!_ips) || Network::Instance()->changed()) {
        _ips = Network::Instance()->getallfirstips();
    }

    if (_ipkeys) {
        // for Kelips, use only keys equal to live IP addresses.
        for (int iters = 0; iters < 50; iters++) {
            IPAddress ip = (*_ips)[random() % _ips->size()];
            IPAddress currip = Network::Instance()->first2currip(ip);
            if (Network::Instance()->alive(currip)) {
                char buf[10];
                sprintf(buf, "%x", currip);
                return string(buf);
            }
        }
        assert(0);//XXX wierd; assertion wont' fire
    }
    // look up random 64-bit keys
    char buffer[20];
    // random() returns only 31 random bits.
    // so we need three to ensure all 64 bits are random.
    unsigned long long a = random();
    unsigned long long b = random();
    unsigned long long c = random();
    unsigned long long x = (a << 48) ^ (b << 24) ^ (c >> 4);
    sprintf(buffer, "%llX", x);
    return string(buffer);
}
