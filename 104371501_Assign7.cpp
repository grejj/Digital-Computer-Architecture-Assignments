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

class Cache {
    private:
        // value = (index, tag, valid, dirty, count)
        std::vector<std::tuple<int,int,bool,bool,int>> cache;
        std::vector<std::tuple<int,int,bool,bool,int>>::iterator it;
        std::vector<std::tuple<int,int,bool,bool,int>>::iterator it2;
        int m_size;
        int m_bits;

        int m_rows;

        unsigned m_offsetbits;
        unsigned m_blockbits;
        unsigned m_rowbits;
        unsigned m_associativity;
        unsigned m_tagbits;

        unsigned m_hit_cost;
        unsigned m_miss_cost_read;
        unsigned m_write_back_cost;

        unsigned m_total_cycles{0};
        unsigned m_hits{0};
        unsigned m_misses{0};
        unsigned m_write_backs{0};

        int m_offsetbits_mask;
        int m_blockbits_mask;
        int m_rowbits_mask;
        int m_tagbits_mask;

    public:
        Cache(int bit_number, unsigned offsetbits, unsigned blockbits, unsigned rowbits, unsigned associativity, unsigned tagbits) { 

            m_bits = bit_number;
            m_offsetbits = offsetbits;
            m_blockbits = blockbits;
            m_rowbits = rowbits;
            m_associativity = associativity;
            m_tagbits = tagbits;
            m_size = pow(2,m_rowbits) * (m_tagbits + 2 + pow(2,m_blockbits) * m_bits) * m_associativity;  

            m_rows = pow(2,m_rowbits+m_blockbits);
            //cache.resize(m_rows);

            if (m_associativity == 1)
                m_hit_cost = 1 + m_rowbits / 2;
            else
                m_hit_cost = 1 + m_rowbits / 2 + cbrt(m_associativity);
            //cout << "HIT COST: " << m_hit_cost << endl;

            // address decode for DRAM + transfer cycles as datapath to DRAM in system is only 32 bits
            m_miss_cost_read = 20 + pow(2,m_blockbits);
            //cout << "MISS COST READ: " << miss_cost_read << endl;

            // cost when cache line is dirty needs to be replaced
            // 1 (for address transfer) + 2^blockbits (data transfer to DRAM before cache entry available for use again)
            m_write_back_cost = 1 + pow(2,m_blockbits);
            //cout << "WRITE BACK COST: " << write_back_cost << endl;
        
            int m_offsetbits_mask = (int)(pow(2,m_offsetbits) - 1);
            int m_blockbits_mask = (int)(pow(2,m_blockbits) - 1);
            int m_rowbits_mask = (int)(pow(2,m_rowbits) - 1);
            int m_tagbits_mask = (int)(pow(2,m_tagbits) - 1);

            // initialize
            for (int i=0; i < m_rows; i++) {
                cache.push_back(std::tuple<int,int,bool,bool,int>(i,0,false,true,i));
            }
        }

        ~Cache() {
            // flush out dirty cache lines at the end
            for (it = cache.begin(); it != cache.end(); it++) {
                int dirty = std::get<3>(*it);
                if (dirty) {
                    cache.erase(it);
                    cache.insert(it, std::tuple<int,int,bool,bool,int>(0,0,false,true,0));
                }
            }
        }

        std::vector<std::tuple<int,int,bool,bool,int>>::iterator getLRU() {
            int lowest_count = INT_MAX;
            for (it = cache.begin(); it != cache.end(); it++) {
                if (std::get<4>(*it) < lowest_count) {
                    it2 = it;
                    lowest_count = std::get<4>(*it);
                }
                //cout << "lowest_count: " << lowest_count << " item count: " << std::get<4>(*it) << endl;
            }
#ifdef DEBUG
            cout << "Oldest item in cache is at index: " << std::get<0>(*it2) << endl;
#endif
            return it2;
        }



        void set(int address, int count) {

            int full_address = address;

            int offset = (address & m_offsetbits_mask);
            //cout << "address: " << address << endl;
            //cout << "offset: " << offset << endl;
            address = (address >> m_offsetbits);
            //cout << "address: " << address << endl;

            int index = (address & ((int)(pow(2,m_blockbits+m_rowbits) - 1)));
            //cout << "index: " << index << endl;  
            address = (address >> (m_blockbits+m_rowbits));

            //int block = (address & m_blockbits_mask);
            //cout << "address: " << address << endl;
            //cout << "block: " << block << endl;  
            //address = (address >> m_blockbits);

            //int row = (address & m_rowbits_mask);
            //cout << "address: " << address << endl;
            //cout << "row: " << row << endl;  
            //address = (address >> m_rowbits);

            int tag = address;
            //int tag = (address & m_tagbits_mask);
            //cout << "address: " << address << endl;
            //cout << "tag: " << tag << endl;   

            //m_total_cycles += m_hit_cost;

//            bool dirty = true;
//            bool valid = true;
//#ifdef DEBUG
//            cout << "Writing index:" << index << " tag:" << tag << " valid: " << valid << " dirty: " << dirty << "count: " << count << endl; 
//#endif
            for (it=cache.begin(); it != cache.end(); it++) {
                int cache_index =     std::get<0>(*it);
                int cache_tag =       std::get<1>(*it);
                int cache_valid =     std::get<2>(*it);
                int cache_dirty =     std::get<3>(*it);

                if (cache_index == index && cache_tag == tag && cache_valid) {
#ifdef DEBUG
                    cout << "Already stored in cache. Update as recently used value." << endl;
#endif
                    m_total_cycles += m_hit_cost;
                    m_hits++;

                    // move it to front, or most recently used
                    cache.erase(it);
                    cache.insert(it, std::tuple<int,int,bool,bool,int>(cache_index, cache_tag, cache_valid, cache_dirty, count));
                    return;
                }
            }
#ifdef DEBUG
            cout << "Item not in cache" << endl;
#endif
            m_misses++;

            // get LRU item
            if (m_associativity == 1){
#ifdef DEBUG
                cout << "Associativity is 1 so replacing only possible cache value. Will write back to memory first." << endl;
#endif            
                // add new item in it's place
                for (it=cache.begin(); it != cache.end(); it++) {
                    int cache_index =     std::get<0>(*it);
                    if (cache_index == index) {
                
                        bool valid = std::get<2>(*it);
                        bool dirty = std::get<3>(*it);
                        if (valid && dirty) {
                            // write back to memory first
                            m_total_cycles += m_write_back_cost;
                            m_write_backs++;
                        }
                        // remove it from cache
                        cache.erase(it);

                        dirty = true;
                        valid = true;
                        // add item to cache as valid and dirty 
#ifdef DEBUG
                        cout << "Adding item to cache." << endl;
#endif
                        cache.insert(it, std::tuple<int,int,bool,bool,int>(index,tag,valid,dirty,count));
                        break;
                    }
                }

            //}




            }
            else {

            

            // get LRU item
            it = getLRU();
            //cout << "m_rows: " << m_rows << " cache_size: " << cache.size() << endl;
            //if (cache.size() == m_rows) {
                // all of cache is used, need to start replacing, pop out LRU
#ifdef DEBUG
                cout << "Cache is full. Must replace LRU value. Will write-back first to memory before replacing." << endl;
#endif            
                bool valid = std::get<2>(*it);
                bool dirty = std::get<3>(*it);
                if (valid && dirty) {
                    // write back to memory first
                    m_total_cycles += m_write_back_cost;
                    m_write_backs++;
                }
                // remove it from cache
                cache.erase(it);
            //}

            dirty = true;
            valid = true;
            // add item to cache as valid and dirty 
#ifdef DEBUG
            cout << "Adding item to cache." << endl;
#endif
            cache.insert(it, std::tuple<int,int,bool,bool,int>(index,tag,valid,dirty,count));
            }
        }

        void get(int address, int count) {

            int full_address = address;

            int offset = (address & m_offsetbits_mask);
            //cout << "address: " << address << endl;
            //cout << "offset: " << offset << endl;
            address = (address >> m_offsetbits);
            //cout << "address: " << address << endl;

            int index = (address & ((int)(pow(2,m_blockbits+m_rowbits) - 1)));
            //cout << "index: " << index << endl;  
            address = (address >> (m_blockbits+m_rowbits));

            //int block = (address & m_blockbits_mask);
            //cout << "address: " << address << endl;
            //cout << "block: " << block << endl;  
            //address = (address >> m_blockbits);

            //int row = (address & m_rowbits_mask);
            //cout << "address: " << address << endl;
            //cout << "row: " << row << endl;  
            //address = (address >> m_rowbits);

            int tag = address;
            //int tag = (address & m_tagbits_mask);
            //cout << "address: " << address << endl;
            //cout << "tag: " << tag << endl;    

            for (it=cache.begin(); it != cache.end(); it++) {
                int cache_index =     std::get<0>(*it);
                int cache_tag =       std::get<1>(*it);

                if (cache_index == index && cache_tag == tag) {

                    bool valid =    std::get<2>(*it);

                    if (valid) {
#ifdef DEBUG
                        cout << "Cache hit." << endl;
#endif
                        // we have cache hit
                        m_total_cycles += m_hit_cost;
                        m_hits++;
                        return;
                    }
                    else {
#ifdef DEBUG
                        cout << "Cache match found with invalid data." << endl;
#endif
                    }
                }
            }
            
#ifdef DEBUG
            cout << "Cache miss." << endl;
#endif
            m_misses++;
            m_total_cycles += m_miss_cost_read;

            // get LRU item
            if (m_associativity == 1){
#ifdef DEBUG
                cout << "Associativity is 1 so replacing only possible cache value. Will write back to memory first." << endl;
#endif            
                // add new item in it's place
                for (it=cache.begin(); it != cache.end(); it++) {
                    int cache_index =     std::get<0>(*it);
                    if (cache_index == index) {
                
                        bool valid = std::get<2>(*it);
                        bool dirty = std::get<3>(*it);
                        if (valid && dirty) {
                            // write back to memory first
                            m_total_cycles += m_write_back_cost;
                            m_write_backs++;
                        }
                        // remove it from cache
                        cache.erase(it);

                        dirty = true;
                        valid = true;
                        // add item to cache as valid and dirty 
#ifdef DEBUG
                        cout << "Adding item to cache." << endl;
#endif
                        cache.insert(it, std::tuple<int,int,bool,bool,int>(index,tag,valid,dirty,count));
                        break;
                    }
                }

            //}




            }
            else {

            it = getLRU();
            //cout << "m_rows: " << m_rows << " cache_size: " << cache.size() << endl;
            //if (cache.size() == m_rows) {
                // all of cache is used, need to start replacing, pop out LRU
#ifdef DEBUG
                cout << "Must replace LRU value. Will write-back first to memory before replacing." << endl;
#endif            
                bool valid = std::get<2>(*it);
                bool dirty = std::get<3>(*it);
                if (valid && dirty) {
                    // write back to memory first
                    m_total_cycles += m_write_back_cost;
                    m_write_backs++;
                }
                // remove it from cache
                cache.erase(it);
            //}

            dirty = true;
            valid = true;
            // add item to cache as valid and dirty 
#ifdef DEBUG
            cout << "Adding item to cache." << endl;
#endif
            cache.insert(it, std::tuple<int,int,bool,bool,int>(index,tag,valid,dirty,count));
            }            
        }

        void print() {
            int i;
#ifdef DEBUG 
            cout << "Current Cache:" << endl;
#endif
            for (i =0, it = cache.begin(); it != cache.end(); it++, i++) {
#ifdef DEBUG 
                cout << "Row" << i << ": " << "index:" << std::get<0>(*it) << " tag:" << std::get<1>(*it) << " valid: " << std::get<2>(*it) << " dirty: " << std::get<3>(*it) << " count: " << std::get<4>(*it) << endl;
#endif
            }
#ifdef DEBUG
            cout << "\n";
#endif
        }

        void printSize() {
#ifdef DEBUG
            cout << "Cache size:" << cache.size() << endl;
#endif
        }

        unsigned cycles()       { return m_total_cycles; }
        unsigned size()         { return m_size; }
        unsigned hits()         { return m_hits; }
        unsigned misses()       { return m_misses; }
        unsigned write_backs()  { return m_write_backs; }
};

int main() {
    string line;
    ifstream quick4k;
    string mat_name = "quick4k";
    quick4k.open("quick4k.txt");
    
    unsigned total_cycles = 0;
    unsigned hits = 0;
    unsigned misses = 0;
    unsigned write_backs = 0;

    unsigned offsetbits = 2;
    unsigned rowbits = 8;
    unsigned blockbits = 0;
    unsigned associativity = 1;
    unsigned tagbits = M - rowbits - blockbits - offsetbits;

    Cache cache(M, offsetbits, blockbits, rowbits, associativity, tagbits);

    unsigned line_count = 256;
    if (quick4k.is_open()) {
        string read_write;
        int address;
        while (quick4k >> read_write) {
            quick4k >> address;

            //if (10 == line_count)
            //    break;

            if ("W" == read_write) {
#ifdef DEBUG
                cout << "Writing to address: " << address << endl;
#endif
                cache.set(address, line_count);
            }
            else {
#ifdef DEBUG
                cout << "Reading from address: " << address << endl;
#endif
                cache.get(address, line_count); 
            }

            line_count++;

            cache.print();
            
        }
    }
    else {
        cout << "Unable to open quick4k.txt" << endl;
    }

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