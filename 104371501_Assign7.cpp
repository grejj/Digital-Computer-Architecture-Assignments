/* Digital Computer Architecture - ELEC4480 */

/* One-Level Cache */

/* CACHE
parameters:
1) tagbits          (derived by subtracting first 3 parameters by 32)
2) rowbits          (total bits used in address decoder of SRAM - 2^rowbits total rows, allowable values 1-12)
3) blockbits        (number of words per tag in the cache - 2^blockbits, allowable values 0-4)
4) offsetbits       (always 2 since this is a 32-bit memory system)
5) associativity    (associativity of 1 implies "direct cache", allowable values are 1,2,4,8,16)
*/

/*  
| 31                           32 bit address                          0 |
|      tagbits     |      rowbits     |     blockbits   |   offsetbits   |
*/

#include <bits/stdc++.h> 
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <map>
using namespace std;

#define M 32

//#define DEBUG

struct CacheLine {
    int index;
    int tag;
    bool valid;
    bool dirty;
    int count;
};

class Cache {
    private:
        std::vector<std::vector<CacheLine>> cache;
        std::vector<CacheLine>::iterator it;
        std::vector<CacheLine>::iterator it2;
        int m_ram_size;
        int m_rows;

        unsigned m_offsetbits;
        unsigned m_indexbits;
        unsigned m_associativity;
        unsigned m_tagbits;

        unsigned m_hit_cost;
        unsigned m_miss_cost_read;
        unsigned m_write_back_cost;

        unsigned m_total_cycles{0};
        unsigned m_hits{0};
        unsigned m_misses{0};
        unsigned m_write_backs{0};

        int m_indexbits_mask;
        int m_tagbits_mask;

        int m_total_1{0};
        int m_total_2{0};
        int m_total_3{0};
        int m_total_4{0};
        int m_total_5{0};
        int m_total_6{0};
        int m_total_7{0};
        int m_total_8{0};
        int m_total_9{0};

    public:
        Cache(int bit_number, unsigned offsetbits, unsigned blockbits, unsigned rowbits, unsigned associativity, unsigned tagbits) { 

            m_offsetbits = offsetbits;
            m_indexbits = blockbits + rowbits;
            m_associativity = associativity;
            m_tagbits = tagbits;
            m_ram_size = pow(2,rowbits) * (m_tagbits + 2 + pow(2,blockbits) * 32) * m_associativity;  
            m_rows = pow(2,m_indexbits);

            if (m_associativity == 1)
                m_hit_cost = 1 + rowbits / 2;
            else
                m_hit_cost = 1 + rowbits / 2 + cbrt(m_associativity);

            // address decode for DRAM + transfer cycles as datapath to DRAM in system is only 32 bits
            m_miss_cost_read = 20 + pow(2,blockbits);

            // cost when cache line is dirty needs to be replaced
            // 1 (for address transfer) + 2^blockbits (data transfer to DRAM before cache entry available for use again)
            m_write_back_cost = 1 + pow(2,blockbits);
        
            int m_offsetbits_mask = (int)(pow(2,m_offsetbits) - 1);
            int m_blockbits_mask = (int)(pow(2,blockbits) - 1);
            int m_rowbits_mask = (int)(pow(2,rowbits) - 1);
            int m_tagbits_mask = (int)(pow(2,m_tagbits) - 1);

            // initialize
            CacheLine line;
            for (int j=0; j < m_associativity; j++) {
                cache.push_back(std::vector<CacheLine>());
                for (int i=0; i < m_rows; i++) {
                    line.count = i;
                    line.dirty = false;
                    line.index = i;
                    line.valid = false;
                    line.tag = 0;
                    cache.at(j).push_back(line);
                }
            }
        }

        void flush() {
            CacheLine line;
            // flush out dirty cache lines at the end
            for (int j=0; j < m_associativity; j++) {
                for (it = cache.at(j).begin(); it != cache.at(j).end(); it++) {
                    if (it->dirty) {
                        m_write_backs++;
                        cache.at(j).erase(it);
                        
                        line.count = 0;
                        line.dirty = true;
                        line.index = 0;
                        line.valid = false;
                        line.tag = 0;
                        cache.at(j).insert(it, line);
                    }
                }
            }
        }

        std::pair<int,std::vector<CacheLine>::iterator> getLRU(int index) {
            int lowest_count = INT_MAX;
            int set;
            for (int j=0; j < m_associativity; j++) {
                for (it = cache.at(j).begin(); it != cache.at(j).end(); it++) {
                    if (it->index == index) {
                        if (it->count < lowest_count) {
                            it2 = it;
                            lowest_count = it->count;
                            set = j;
                        }
                        //cout << "lowest_count: " << lowest_count << " item count: " << std::get<4>(*it) << endl;
                    }
                }
            }
#ifdef DEBUG
            cout << "Oldest item in cache is at index: " << std::get<0>(*it2) << endl;
#endif
            return std::pair<int,std::vector<CacheLine>::iterator>(set,it2);
        }

        void update(int address, int count, bool write) {

            address = (address >> m_offsetbits);

            int index = (address & ((int)(pow(2,m_indexbits) - 1)));
            address = (address >> (m_indexbits));

            int tag = address;

            for (int j=0; j < m_associativity; j++) {    
                for (it=cache.at(j).begin(); it != cache.at(j).end(); it++) {
                    if (it->index == index && it->tag == tag && it->valid) {
#ifdef DEBUG
                        cout << "Already stored in cache. Update as recently used value." << endl;
#endif
                        m_total_cycles += m_hit_cost;
                        m_hits++;

                        // move it to front, or most recently used
                        it->count = count;
                        if (write)
                            it->dirty = true; 
                        return;
                    }
                }
            }
#ifdef DEBUG
            cout << "Item not in cache" << endl;
#endif
            m_misses++;

            std::pair<int,std::vector<CacheLine>::iterator> lru = getLRU(index);

            if (lru.second->dirty && lru.second->valid && lru.second->tag != tag) {
                // write back to memory first
                m_total_cycles += m_write_back_cost;
                m_write_backs++;
            }

            if (lru.second->dirty) m_total_1++;
            if (lru.second->valid) m_total_2++;
            if (write) m_total_3++;

            CacheLine line;
            line.count = count;
            if (write)
                line.dirty = true;
            else
                line.dirty = false;
            line.index = index;
            line.tag = tag;
            line.valid = true;

            // remove it from cache
            cache.at(lru.first).erase(lru.second);
            // add item to cache as valid and dirty 
#ifdef DEBUG
            cout << "Adding item to cache." << endl;
#endif

            cache.at(lru.first).insert(lru.second, line);
 
            if (write)
                m_total_cycles += m_hit_cost;
            else
                m_total_cycles += m_miss_cost_read;
        }

        void print() {
            int i;
//#ifdef DEBUG 
            cout << "Current Cache:" << endl;
//#endif
            for (int j=0; j < m_associativity; j++) {    
                cout << "SET " << j << endl;
                for (it=cache.at(j).begin(); it != cache.at(j).end(); it++) {
//#ifdef DEBUG 
                    cout << "Index:" << it->index << " tag:" << it->tag << " valid: " << it->valid << " dirty: " << it->dirty << " count: " << it->count << endl;
//#endif        
                }
            }
//#ifdef DEBUG
            cout << "\n";
//#endif
        }

        void printSize() {
#ifdef DEBUG
            cout << "Cache size:" << cache.size() << endl;
#endif
        }

        void printTotals() {
            cout << "Total1: " << m_total_1 << endl;
            cout << "Total2: " << m_total_2 << endl;
            cout << "Total3: " << m_total_3 << endl;
        }

        unsigned cycles()       { return m_total_cycles; }
        unsigned size()         { return m_ram_size; }
        unsigned hits()         { return m_hits; }
        unsigned misses()       { return m_misses; }
        unsigned write_backs()  { return m_write_backs; }
};

int main() {
    // open file
    ifstream quick4k;
    string mat_name = "quick16k.txt";
    quick4k.open(mat_name);
    
    // setup cache with parameters
    unsigned offsetbits = 2;
    unsigned rowbits = 6;
    unsigned blockbits = 0;
    unsigned associativity = 2;
    unsigned tagbits = M - rowbits - blockbits - offsetbits;
    Cache cache(M, offsetbits, blockbits, rowbits, associativity, tagbits);

    unsigned line_count = 256;
    if (quick4k.is_open()) {
        string read_write;
        int address;
        while (quick4k >> read_write) {
            quick4k >> address;

            //if (257 == line_count)
            //   break;

            if (read_write == "W")
                cache.update(address, line_count, true);
            else
                cache.update(address, line_count, false);
            line_count++;  
            ///cache.print();          
        }
    }
    else {
        cout << "Unable to open " << mat_name << endl;
    }
    cache.print();
    cache.flush();
    cache.printTotals();

    /* output */
    cout << "********************" << endl;
    cout << "MAT NAME: " << mat_name << endl;
    cout << "ROWBITS: " << rowbits << endl;
    cout << "BLOCKBITS: " << blockbits << endl;
    cout << "ASSOCIATIVITY: " << associativity << endl;
    cout << "TOTAL CYCLES: " << cache.cycles() << endl;
    cout << "RAM SIZE: " << cache.size() << endl;
    cout << "HITS: " << cache.hits() << endl;
    cout << "MISSES: " << cache.misses() << endl;
    cout << "WRITE BACKS: " << cache.write_backs() << endl;
}