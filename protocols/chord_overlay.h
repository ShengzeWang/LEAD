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

#ifndef __CHORD_OVERLAY_H
#define __CHORD_OVERLAY_H

#include "../p2psim/p2protocol.h"
#include "consistenthash.h"
#include "../p2psim/network.h"
#include "../protocols/chord.h"
#include <map>



#define TIMEOUT_RETRY 5

//#define RECORD_FETCH_LATENCY

#define TIMEOUT(src,dst) (Network::Instance()->gettopology()->latency(src,dst)<=1000)?_to_multiplier*2*Network::Instance()->gettopology()->latency(src,dst):1000

#define PKT_OVERHEAD 20

#define TYPE_USER_LOOKUP 0
#define TYPE_JOIN_LOOKUP 1
#define TYPE_FINGER_LOOKUP 2
#define TYPE_FIXSUCC_UP 3
#define TYPE_FIXSUCCLIST_UP 4
#define TYPE_FIXPRED_UP 5
#define TYPE_FINGER_UP 6
#define TYPE_PNS_UP 7
#define TYPE_MISC 8

#define MIN_BASIC_TIMER 100

class LocTable_mnodes;
#include <cstdint>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <cmath>


class iNode{
public:
    uint64_t id_;
    iNode* predecessor_;
    iNode* group_leader;
    map<string, string> data_;// key, value
    unsigned long long num_of_pairs = 100000;
    vector<iNode*> fingerTable_;
    unsigned Euclidean_x = 0;
    unsigned Euclidean_y = 0;

    iNode(uint64_t id){
        id_ = id;
        fingerTable_.push_back(this);
        predecessor_ = this;
    }

    // Hash key to ID
    static uint64_t hash(string key){
        //int len = key.length();
        //uint32_t seed = 0;
        //uint64_t hash_value= MurmurHash64A(&key, len, seed);
        int hash_value = stoi(key);
        uint64_t id = hash_value % 256;
        //cout<< hash_value << "=" << id<<endl;
        return id;
    }

    // set Topology
    void set_Euclidean(unsigned x,unsigned y){
        Euclidean_x = x;
        Euclidean_y = y;
    }

    // set Group Leader
    void set_GroupLeader(iNode* gl){
        group_leader = gl;
    }


    // Get distance between to IDs
    static uint64_t distance(uint64_t n1_id,uint64_t n2_id){
        if(n1_id <= n2_id){
            return n2_id -n1_id;
        }else{
            return 256 - n1_id + n2_id;
        }
    }

    //ask node n to find key's successor
    iNode* find_successor(uint64_t id, int* rtts, int is_lookup){
        iNode* current_node = this;
        if (is_lookup == 1){
            cout<<"Lookup Key_id: " << id << " from node " <<id_<< endl;
        }
        int rtt = 0;
        while (true){
            if(current_node->id_ == id){
                rtt = Euclidean_RTT(current_node);
                *rtts += rtt;
                if (is_lookup == 1){
                    cout<<"Found: " <<id_<< " on Node " << current_node->id_ << endl;
                    cout<<"Return Trip RTT: (" << current_node->id_ << "," << id_ << ")  " <<rtt<< endl;
                }
                return current_node;
            }
            if(distance(current_node->id_, id) <= distance(current_node->fingerTable_[0]->id_, id)){
                rtt = Euclidean_RTT(current_node->fingerTable_[0]);
                *rtts += rtt;
                if (is_lookup == 1){
                    cout<<"Found: " <<id_<< " on Node " << current_node->fingerTable_[0]->id_ << endl;
                    cout<<"Return Trip RTT: (" << current_node->fingerTable_[0]->id_ << "," << id_ << ")  " <<rtt<< endl;
                }
                return current_node->fingerTable_[0];
            }
            iNode* nextNode = current_node->fingerTable_.back();
            for (int i = 0; i < current_node->fingerTable_.size()-1; ++i) {
                if(distance(current_node->fingerTable_[i]->id_, id) < distance(current_node->fingerTable_[i+1]->id_, id)){
                    nextNode = current_node->fingerTable_[i];
                }
            }
            rtt = current_node->Euclidean_RTT(nextNode);
            if (is_lookup == 1){
                cout<<"Path: ("<<current_node->id_<<","<<nextNode->id_<< ")  Local RTT: "<< rtt << endl;
            }
            *rtts += rtt;
            current_node = nextNode;
        }
    }

    //update the finger table
    void updateFingerTable(){
        while(fingerTable_.size()>1){
            fingerTable_.pop_back();
        }
        for (int i = 1; i < 8; ++i) {
            int rtts = 0;
            uint64_t next_id = id_ + pow(2, i);
            int is_lookup = 0;
            fingerTable_.push_back(find_successor(next_id % 8, &rtts, is_lookup));
        }
    }

    //node n joins the network
    void join(iNode* newNode){
        /*
        if (fingerTable_[0] == this){
            predecessor_ = newNode;
            fingerTable_[0]= newNode;
            newNode->predecessor_ = this;
            newNode->fingerTable_[0] = this;
            return;
        }*/
        int rtts= 0;
        int is_lookup = 0;
        iNode* successor = find_successor(newNode->id_, &rtts, is_lookup);
        if(newNode->id_ == successor->id_){
            cout << "ID conflict "<<newNode->id_<<endl;
            return;
        }
        // migrate key pairs
        vector<string> key_vector;
        for (auto & itr: successor->data_) {
            uint64_t id = hash(itr.first);
            if (distance(id, newNode->id_) < distance(id, successor->id_)){
                newNode->data_[itr.first] = itr.second;
                key_vector.push_back(itr.first);
            }
        }
        for (string i : key_vector) {
            cout<<"*********************"<<endl;
            cout<<"migrate key "<<i<<" from node "<<successor->id_<<" to node "<<newNode->id_<<endl;
            successor->data_.erase(i);
        }
        newNode->fingerTable_[0] = successor;
        newNode->predecessor_ = successor->predecessor_;
        successor->predecessor_->fingerTable_[0] = newNode;
        successor->predecessor_ = newNode;

        newNode->updateFingerTable();
        refreshFingerTables();
    }
    //Insert a key-value pair in the DHT
    void insert(string key, string value){
        uint64_t id = hash(key);
        int rtts = 0;
        int is_lookup = 0;
        iNode* key_successor = find_successor(id, &rtts, is_lookup);
        key_successor->data_[key] = value;
    }
    // Look up a key in the DHT
    int lookup(string key){
        uint64_t id = hash(key);
        int rtts = 0;
        int is_lookup = 0;
        iNode* key_successor = find_successor(id, &rtts, is_lookup);
        //cout<< "Look-up result of key "<< key << " from node " << id_ << " with path ["<< id_ << "," << key_successor->id_<<"] value is "<<key_successor->data_[key] <<endl;
        // cout<< "Inner Chord Latency: "<< rtts << endl;
        return rtts;
    }
    //implement DHT key deletion
    void leave(iNode* target_node){
        iNode* successor = target_node->fingerTable_[0];
        for (auto & itr: target_node->data_) {
            successor->data_[itr.first] = itr.second;
        }
        target_node->predecessor_->fingerTable_[0] = successor;
        //delete[] target_node;
        refreshFingerTables();
    }

    //Got RTT based on Euclidean Topology
    int Euclidean_RTT(iNode* another){
        int ip1x = another->Euclidean_x;
        int ip1y = another->Euclidean_y;
        int RTT = -1;
        if (ip1x == Euclidean_x & ip1y == Euclidean_y){
            RTT = 0;
        }else{
            RTT= (int) hypot(labs(ip1x - Euclidean_x), labs(ip1y - Euclidean_y));
        }
        return RTT;
    }

    void refreshFingerTables(){
        updateFingerTable();
        iNode* successor_node = fingerTable_[0];
        while (successor_node != this){
            successor_node->updateFingerTable();
            successor_node = successor_node->fingerTable_[0];
        }
    }

    void display(){
        cout << "--------- Node id: "<< id_ << " ---------"<< endl;
        cout<<"Euclidean Topology: ("<< Euclidean_x << " , "<<Euclidean_y<<")"<<endl;
        /*
        cout << "Data: "<< endl;
        for (auto & itr : data_) {
            cout << "key: "<< itr.first << "   value: "<< itr.second<< endl;
        }*/
        cout << "successor: " << fingerTable_[0]->id_<< endl;
        cout << "Predecessor: " << predecessor_->id_<< endl;

        cout<<"num_of_pairs: "<< num_of_pairs<<endl;
        /*
        cout << "FingerTables:"<< endl;
        for (long long i = 0; i < fingerTable_.size(); ++i) {
            long long left = (id_ + int(pow(2, i))) % SIZE;
            long long right = (id_ + int(pow(2, i + 1))) % SIZE;
            cout << "k =" << i << " [" << left << "," << right << ") Successor = " << fingerTable_[i]->id_<< endl;
        }*/
        cout << "---------------------------"<< endl;
        cout << "***************************"<< endl;
    }
    void display_all(){
        display();
        iNode* successor = fingerTable_[0];
        while (successor != this){
            successor->display();
            successor = successor->fingerTable_[0];
        }
    }

};

class Chord_overlay : public P2Protocol {
public:

  typedef ConsistentHash::CHID CHID;

    class IDMap{
    public:
        ConsistentHash::CHID id; //consistent hashing ID for the node
        IPAddress ip; //the IP address for the node
        Time timestamp; //some kind of heartbeat sequence number
        Time alivetime; //how long this node has stayed alive
        map<CHID, CHID> *data_address;
        bool IS_VNODE = false;

        static bool cmp(const IDMap& a, const IDMap& b) { return (a.id <= b.id);}
        bool operator==(const IDMap a) { return (a.id == id); }
    };

  Chord_overlay(IPAddress i,  Args& a, LocTable_mnodes *l = NULL, const char *name=NULL);
  virtual ~Chord_overlay();
  virtual string proto_name() { return "Chord"; }

  // Functions callable from events file.
  virtual void join(Args*);
  virtual void leave(Args*);
  virtual void crash(Args*);
  virtual void lookup(Args*);
  virtual void query(Args*);
  //virtual void insert(Args*);
  virtual void display(Args*);
  virtual void range_query_leanred(Args*);
  virtual void range_query_native(Args*);
  virtual void nodeevent (Args *) {};
  virtual void eat_all(string input_file);
  virtual void print_query_stats();
  virtual void print_query_stats_batch();

  struct get_predsucc_args {
    bool pred; //need to get predecessor?
    int m; //number of successors wanted 0
  };
  struct get_predsucc_ret {
    vector<IDMap> v;
    IDMap dst;
    IDMap n;
  };
  struct notify_args {
    IDMap me;
  };
  struct notify_ret {
    int dummy;
  };
  struct alert_args {
    IPAddress dst;
    IDMap n;
  };
  struct hop_info {
    IDMap from;
    IDMap to;
    uint hop;
  };
  struct next_args {
    IDMap src;
    CHID key;
    uint m;
    uint alpha; //get m out of the first all successors
    bool retry;
    uint type;
    vector<IDMap> deadnodes;
  };
  struct next_ret {
    bool done;
    vector<IDMap> v;
    vector<IDMap> next;
    bool correct;
    IDMap dst;
    IDMap lastnode;
  };
  struct nextretinfo{
    hop_info link;
    next_ret ret;
    bool free;
  };
  struct find_successors_args {
    IDMap src;
    CHID key;
    uint m;
    uint all;
  };
  struct find_successors_ret {
    vector<IDMap> v;
    IDMap last;
    IDMap dst;
    Time latency;
  };
  struct lookup_path {
    IDMap n;
    bool tout;
  };
    struct key_pair {
        // hash_id, original_key
        CHID hash_id;
        CHID original_key;
        sklist_entry<key_pair> sortlink_;
        key_pair(CHID h, CHID o) : hash_id(h), original_key(o) {}
    };

    struct idmapcompare{
        idmapcompare() {}
        int operator() (CHID a, CHID b) const
        { if (a == b){
                return 0;
            }else if (a < b){
                return -1;
            }else{
                return 1;}
        }
    };
  struct next_recurs_args {
    uint type;
    CHID key;
    IPAddress ipkey;
    uint m;
    IDMap src;
  };
  struct next_recurs_ret {
    vector<IDMap> v;
    vector<lookup_path> path;
    bool correct;
    uint finish_time;
    IDMap lasthop;
    IDMap prevhop;
    IDMap nexthop;
    skiplist<key_pair, CHID, &key_pair::hash_id, &key_pair::sortlink_, idmapcompare> *data_address;
    IPAddress successor;
  };
  struct lookup_args{
    CHID key;
    IPAddress ipkey;
    Time start;
    Time latency;
    uint retrytimes;
    Time total_to;
    uint num_to;
    uint hops;
    bool is_insert;
    CHID or_key;
    CHID hash_id;
    CHID query_range;
    skiplist<key_pair, CHID, &key_pair::hash_id, &key_pair::sortlink_, idmapcompare> *data_address;
  };



  // RPC handlers.
  void null_handler (void *args, IDMap *ret);
  void get_predsucc_handler(get_predsucc_args *, get_predsucc_ret *);
  void notify_handler(notify_args *, notify_ret *);
  void alert_handler(alert_args *, void *);
  void next_handler(next_args *, next_ret *);
  void find_successors_handler(find_successors_args *, find_successors_ret *);
  void find_successors_handler_query(find_successors_args *, find_successors_ret *);
  void final_recurs_hop(next_recurs_args *args, next_recurs_ret *ret);
  void next_recurs_handler(next_recurs_args *, next_recurs_ret *);
  void lookup_internal(lookup_args *a);
  void query_internal(lookup_args *a);
  void display_node(){
      cout<< "..............................................."<<endl;
      cout<< "............Current Node ip: "<< me.ip << "............."<<endl;
      cout<< "Consistent Hashing ID: "<< me.id <<endl;
      // cout<< "Alive Time: "<< me.alivetime <<endl;
      // cout<< "_prev_succ: "<< _prev_succ <<endl;
      cout << "Data: "<< endl;
      key_pair* current = key_pairs.first();
      std::cout << "key_pairs0***********************************" << std::endl;
      while (current) {
          std::cout << "Hash ID: " << current->hash_id << ", Original Key: " << current->original_key << std::endl;
          current = key_pairs.next(current);
      }
      cout << "Real Node IP:"<<endl;
      cout<<real_node_ip<<endl;
      cout << "Virtual Node Pairs (IP): "<< endl;
      for (int i = 0; i < _pairs.size(); ++i) {
          std::cout << _pairs[i] << " ";
      }
      std::cout << std::endl;
// real node ip, virtual node id, num_of_key_value_pairs
// cout<< real_node_ip << ","<<me.ip<<","<<data_.size()<<endl;
  }
  void migrate_data(lookup_args *args, next_recurs_ret *ret) {
        skiplist<key_pair, CHID, &key_pair::hash_id, &key_pair::sortlink_, idmapcompare> *new_node_data_address = args->data_address;
        CHID new_node_id = args->key;
        ret->data_address = &key_pairs; // return data_ to new node
        // migrate key pairs
        vector<CHID> key_vector;
        key_pair* current = key_pairs.first();
        while (current) {
            CHID current_id = current->hash_id;
            CHID current_key = current->original_key;
            if (ConsistentHash::distance(current_id, new_node_id) < ConsistentHash::distance(current_id, me.id)) {
                key_pair* k = new key_pair(current_id, current_key);
                new_node_data_address->insert(k);
                key_vector.push_back(current_id);
            } else{
                break;
            }
            current = key_pairs.next(current);
        }
      if (!key_vector.empty()){
          for (CHID i: key_vector) {
              key_pairs.remove(i);
          }
      }
  }

  void alert_delete(alert_args *aa);

  CHID id() { return me.id; }
  IDMap idmap() { return me;}
  virtual void initstate();
  virtual bool stabilized(vector<CHID>);
  bool check_correctness(CHID k, vector<IDMap> v);
  virtual void oracle_node_died(IDMap n);
  virtual void oracle_node_joined(IDMap n);
  void add_edge(int *matrix, int sz);
  virtual  void return_data_add(lookup_args *args, next_recurs_ret *ret);

  virtual void dump();

  IDMap next_hop(CHID k);

  char *ts();
  string header(); //debug message header
  static string printID(CHID id);

  void stabilize();
  virtual void reschedule_basic_stabilizer(void *);

  bool inited() {return _inited;};
  char *print_path(vector<lookup_path> &p, char *tmp);


protected:
  //chord parameters
  uint _nsucc;
  uint _allfrag;
  uint _timeout;
  uint _to_multiplier;
  bool _stab_basic_running;
  uint _stab_basic_timer;
  uint _stab_succlist_timer;
  uint _stab_basic_outstanding;
  uint _max_lookup_time;
  uint _frag;
  uint _alpha;
  int _asap;
  uint _recurs;
  uint _recurs_direct;
  uint _stopearly_overshoot;
  IDMap _wkn;
  uint _join_scheduled;
  uint _parallel;
  uint _learn;
  uint _ipkey;
  uint _last_succlist_stabilized;
  uint _random_id;

  LocTable_mnodes *loctable;
  LocTable_mnodes *learntable;
  IDMap me;
  CHID _prev_succ;
  uint i0;
  vector<IDMap> lastscs;
  bool _isstable;
  bool _inited;
  //map<CHID, CHID> data_;// hash_id, original_key
  skiplist<key_pair, CHID, &key_pair::hash_id, &key_pair::sortlink_, idmapcompare> key_pairs;
  CHID real_node_ip;
  int _num_of_keys;
  int _batch_size;
  int group_leader_ip;
  iNode *group_leader;
  vector<iNode*> inner_network; //init the network

#ifdef RECORD_FETCH_LATENCY
  static vector<uint> _fetch_lat;
#endif

  virtual vector<IDMap> find_successors_recurs(CHID key, uint m, uint type, IDMap *lasthop = NULL, lookup_args *a = NULL);
  virtual vector<IDMap> find_successors(CHID key, uint m, uint type, IDMap *lasthop = NULL, lookup_args *a = NULL);
  virtual void learn_info(IDMap n);
  virtual bool replace_node(IDMap n, IDMap &replacement);

  template<class BT, class AT, class RT>
    bool failure_detect(IDMap dst, void (BT::* fn)(AT *, RT *), AT *args, RT *ret,
	uint type, uint num_args_id = 0, uint num_args_else = 0, int num_retry=TIMEOUT_RETRY);

  void fix_successor(void *x=NULL);
  void fix_predecessor();
  void fix_successor_list();
  void check_static_init();
  void record_stat(IPAddress src, IPAddress dst, uint type, uint num_ids, uint num_else = 0);
  void record_lookupstat(uint num, uint type);

private:
  Time _last_join_time;
  static vector<uint> rtable_sz;
  vector<IPAddress> _pairs;
};

#define LOC_REPLACEMENT 0
#define LOC_HEALTHY 1
#define LOC_ONCHECK 2
#define LOC_DEAD 6

class LocTable_mnodes{

  public:
    struct idmapwrap {
        Chord_overlay::IDMap n;
        Chord_overlay::CHID id;
	sklist_entry<idmapwrap> sortlink_;
	bool is_succ;
	int status;
        Chord_overlay::CHID fs;
        Chord_overlay::CHID fe;
	ConsistentHash::CHID follower;
	idmapwrap(Chord_overlay::IDMap x) {
	  n = x;
	  id = x.id;
	  status = 0;
	  fs = fe = 0;
	  is_succ = false;
	  follower = 0;
	}
    };

    struct idmapcompare{
      idmapcompare() {}
      int operator() (ConsistentHash::CHID a, ConsistentHash::CHID b) const
      { if (a == b){
              return 0;
      }else if (a < b){
          return -1;
      }else{
          return 1;}
      }
    };
    LocTable_mnodes();
  void init (Chord_overlay::IDMap me);
  virtual ~LocTable_mnodes();

    idmapwrap *get_naked_node(ConsistentHash::CHID id);
    Chord_overlay::IDMap succ(ConsistentHash::CHID id, int status = LOC_HEALTHY);
    vector<Chord_overlay::IDMap> succs(ConsistentHash::CHID id, unsigned int m, int status = LOC_HEALTHY);
    vector<Chord_overlay::IDMap> preds(Chord_overlay::CHID id, uint m, int status = LOC_HEALTHY, double to=0.0);
    vector<Chord_overlay::IDMap> between(ConsistentHash::CHID start, ConsistentHash::CHID end, int status = LOC_HEALTHY);
    Chord_overlay::IDMap pred(Chord_overlay::CHID id, int status = LOC_ONCHECK);
    void checkpoint();
    void print();

    bool update_ifexists(Chord_overlay::IDMap n, bool replacement=false);
    bool add_node(Chord_overlay::IDMap n, bool is_succ=false, bool assertadd=false,Chord_overlay::CHID fs=0,Chord_overlay::CHID fe=0, bool replacement=false);
    int add_check(Chord_overlay::IDMap n);
    void add_sortednodes(vector<Chord_overlay::IDMap> l);
    bool del_node(Chord_overlay::IDMap n, bool force=false);
    virtual void del_all();
    void notify(Chord_overlay::IDMap n);
    uint size(uint status=LOC_HEALTHY, double to = 0.0);
    uint succ_size();
    void last_succ(Chord_overlay::IDMap n);
    uint live_size(double to = 0.0);
    bool is_succ(Chord_overlay::IDMap n);
    void set_evict(bool v) { _evict = v; }
    void set_timeout(uint to) {_timeout = to;}

    //pick the next hop for lookup;
    virtual vector<Chord_overlay::IDMap> next_hops(Chord_overlay::CHID key, uint nsz = 1);
    virtual Chord_overlay::IDMap next_hop(Chord_overlay::CHID key);

    vector<Chord_overlay::IDMap> get_all(uint status=LOC_HEALTHY);
    Chord_overlay::IDMap first();
    Chord_overlay::IDMap last();
    Chord_overlay::IDMap search(ConsistentHash::CHID);
    int find_node(Chord_overlay::IDMap n);
    void dump();
    void stat();
    double pred_biggest_gap(Chord_overlay::IDMap &start, Chord_overlay::IDMap &end, Time stabtimer, double to = 0.0); //these two functions are too specialized
    uint sample_smallworld(uint est_n, Chord_overlay::IDMap &askwhom,Chord_overlay::IDMap &start, Chord_overlay::IDMap &end, double tt = 0.9, ConsistentHash::CHID maxgap = 0);
    void rand_sample(Chord_overlay::IDMap &askwhom, Chord_overlay::IDMap &start, Chord_overlay::IDMap &end);
    vector<Chord_overlay::IDMap> get_closest_in_gap(uint m, ConsistentHash::CHID start, ConsistentHash::CHID end, Chord_overlay::IDMap src, Time stabtime, double to);
    vector<Chord_overlay::IDMap> next_close_hops(ConsistentHash::CHID key, uint n, Chord_overlay::IDMap src, double to = 0.0);
    void print_ring() {
        Chord_overlay::IDMap succ1 = succ(me.id + 1);
        Chord_overlay::IDMap pred1 = pred(me.id - 1);
        std::cout << "pred: " << pred1.id << std::endl;
        std::cout << "succ: " << succ1.id << std::endl;
        /*
        stat();
        cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<< endl;
        cout << "Ring "<< endl;
        idmapwrap *current = ring.first(); //get the first element
        int ring_counter = 0;
        int is_succ_counter = 0;
        while (current) {
            ring_counter += 1;
            cout << "No. "<<ring_counter << endl;
            std::cout << "ID: " << current->id << std::endl;
            std::cout << "IP: " << current->n.ip << std::endl;
            //std::cout << "fs: " << current->fs << std::endl;
            //std::cout << "fe: " << current->fe << std::endl;
            //std::cout << "follower: " << current->follower << std::endl;
            std::cout << "is_succ: " << current->is_succ << std::endl;
            std::cout << "status: " << current->status << std::endl;
            is_succ_counter += current->is_succ;
            cout << "------------------------------"<< endl;
            current = ring.next(current); //get the next element
        }
        cout << "Total loctable items: "<<ring_counter << endl;
        cout << "is_succ_counter: "<<is_succ_counter << endl;
        cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<< endl;
         */
    }

  protected:
    bool _evict;
    skiplist<idmapwrap, ConsistentHash::CHID, &idmapwrap::id, &idmapwrap::sortlink_, idmapcompare> ring;
    Chord_overlay::IDMap me;
    uint _max;
    uint _timeout;
    ConsistentHash::CHID full;
    Time lastfull;


    //evict one node to make sure ring contains <= _max elements
    void evict();
};
#define CDEBUG(x) if(p2psim_verbose >= (x)) cout << header()

#endif //__CHORD_H
