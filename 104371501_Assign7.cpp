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

struct CacheLine {
    int index;
    int tag;
    bool valid;
    bool dirty;
    int count;
};

class Cache {
    private:
        std::vector<std::vector<CacheLine>> cache;  // 2D array of cache lines (rows x sets)
        std::vector<CacheLine>::iterator it;        // couple of iterators for 2D array
        std::vector<CacheLine>::iterator it2;
        
        int m_ram_size{0};                          // size of ram
        int m_rows{0};                              // number of rows in a set

        unsigned m_offsetbits{2};                   // offset bits (always 2)
        unsigned m_indexbits{0};                    // index bits (combined row and block bits)
        unsigned m_associativity{0};                // level of associativity
        unsigned m_tagbits{0};                      // tag bits (32 - index bits - offset bits)

        unsigned m_hit_cost{0};                     // cycle cost of read/write cache hit
        unsigned m_miss_cost_read{0};               // cycle cost of read cache miss
        unsigned m_write_back_cost{0};              // cycle cost of writing cache line to memory

        unsigned m_total_cycles{0};                 // running sum of total cycles
        unsigned m_hits{0};                         // running sum of number of cache hits
        unsigned m_misses{0};                       // running sum of number of cache misses
        unsigned m_write_backs{0};                  // running sum of number of write backs to memory

    public:
        Cache(unsigned blockbits, unsigned rowbits, unsigned associativity, unsigned tagbits) 
        :   m_indexbits         {blockbits + rowbits},
            m_associativity     {associativity},
            m_tagbits           {tagbits},
            m_ram_size          {(int)(pow(2,rowbits) * (tagbits + 2 + pow(2,blockbits) * 32) * associativity)},
            m_rows              {(int)(pow(2,blockbits + rowbits))},
            m_miss_cost_read    {(unsigned)(20 + pow(2,blockbits))},
            m_write_back_cost   {(unsigned)(1 + pow(2,blockbits))}
        { 
            // set hit cost based on associativity level
            if (m_associativity == 1) m_hit_cost = 1 + rowbits / 2; else m_hit_cost = 1 + rowbits / 2 + cbrt(m_associativity);

            // initialize cache with empty cache lines
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

        /* when done, make sure to flush cache by writing all dirty data to memory */
        void flush() {
            CacheLine line;
            for (int j=0; j < m_associativity; j++) {
                for (it = cache.at(j).begin(); it != cache.at(j).end(); it++) {
                    if (it->dirty) {
                        // write dirty data back to memory
                        m_write_backs++;
                        m_total_cycles += m_write_back_cost;
                    }
                }
            }
        }

        /* get the oldest data for index, returns set oldest data is in and iterator to that set */
        std::pair<int,std::vector<CacheLine>::iterator> getLRU(int index) {
            int lowest_count = INT_MAX;
            int set;
            for (int j=0; j < m_associativity; j++) {
                for (it = cache.at(j).begin(); it != cache.at(j).end(); it++) {
                    if (it->index == index) {
                        if (it->count < lowest_count) {
                            // set values when oldest data is found
                            it2 = it;
                            lowest_count = it->count;
                            set = j;
                        }
                    }
                }
            }
            return std::pair<int,std::vector<CacheLine>::iterator>(set,it2);
        }

        /* main cache function called on every read/write */
        void update(int address, int count, bool write) {

            // remove lower offset bits
            address = (address >> m_offsetbits);

            // get index value
            int index = (address & ((int)(pow(2,m_indexbits) - 1)));
            address = (address >> (m_indexbits));

            // whatever is left is the tag
            int tag = address;

            // iterate through cache to try to find data and cache hit
            for (int j=0; j < m_associativity; j++) {    
                for (it=cache.at(j).begin(); it != cache.at(j).end(); it++) {
                    if (it->index == index && it->tag == tag && it->valid) {
                        // if find correct data and it's valid, we have a cache hit
                        m_total_cycles += m_hit_cost;
                        m_hits++;

                        // set data as most recently used
                        it->count = count;

                        // if we are writing, data know has become dirty
                        if (write)
                            it->dirty = true; 
                        return;
                    }
                }
            }

            // even we didn't find data in cache, we have a cache miss
            m_misses++;

            // get the oldest data in the cache that we will replace with data we want
            std::pair<int,std::vector<CacheLine>::iterator> lru = getLRU(index);

            // if data is dirty and valid, make sure to write back to memory before deleting from cache
            if (lru.second->dirty && lru.second->valid && lru.second->tag != tag) {
                m_total_cycles += m_write_back_cost;
                m_write_backs++;
            }

            // setup new cache line to insert
            CacheLine line;
            line.count = count;
            if (write)
                line.dirty = true;
            else
                line.dirty = false;
            line.index = index;
            line.tag = tag;
            line.valid = true;

            // replace oldest data in cache with latest accessed data
            cache.at(lru.first).erase(lru.second);
            cache.at(lru.first).insert(lru.second, line);
 
            // write miss has same cost as a hit
            if (write)
                m_total_cycles += m_hit_cost;
            else
                m_total_cycles += m_miss_cost_read;
        }

        unsigned cycles()       { return m_total_cycles; }
        unsigned size()         { return m_ram_size; }
        unsigned hits()         { return m_hits; }
        unsigned misses()       { return m_misses; }
        unsigned write_backs()  { return m_write_backs; }
};

int main() {

    std::map<string,std::tuple<int,int,int>> files;
    // lab manual testing files/parameters
    //files.insert(std::pair<string,std::tuple<int,int,int>>("quick4k.txt", std::make_tuple(8,0,1)));
    //files.insert(std::pair<string,std::tuple<int,int,int>>("quick16k.txt", std::make_tuple(6,0,2)));
    //files.insert(std::pair<string,std::tuple<int,int,int>>("quick64k.txt", std::make_tuple(4,0,4)));
    files.insert(std::pair<string,std::tuple<int,int,int>>("quick256k.txt", std::make_tuple(10,0,8)));
    //files.insert(std::pair<string,std::tuple<int,int,int>>("quick1M.txt", std::make_tuple(8,0,1)));
    //files.insert(std::pair<string,std::tuple<int,int,int>>("random4k.txt", std::make_tuple(2,0,16)));
    //files.insert(std::pair<string,std::tuple<int,int,int>>("random16k.txt", std::make_tuple(7,1,1)));
    //files.insert(std::pair<string,std::tuple<int,int,int>>("random64k.txt", std::make_tuple(5,2,1)));
    //files.insert(std::pair<string,std::tuple<int,int,int>>("random256k.txt", std::make_tuple(3,3,1)));
    //files.insert(std::pair<string,std::tuple<int,int,int>>("random1M.txt", std::make_tuple(8,0,1)));

    // quick4k


    // iterate through all test files and parameters
    for (std::map<string,std::tuple<int,int,int>>::iterator it = files.begin(); it != files.end(); it++) {
        
        // time each iteration
        const clock_t begin_time = clock();

        // open file
        ifstream file;
        string mat_name = it->first;
        file.open(mat_name);

        // setup cache with parameters
        unsigned rowbits = std::get<0>(it->second);
        unsigned blockbits = std::get<1>(it->second);
        unsigned associativity = std::get<2>(it->second);
        unsigned tagbits = 32 - rowbits - blockbits - 2;
        Cache cache(blockbits, rowbits, associativity, tagbits);

        // read file, line by line loading and updating cache
        unsigned line_count = 256;
        if (file.is_open()) {
            string read_write;
            while (file >> read_write) {
                int address;
                file >> address;

                if (read_write == "W")
                    cache.update(address, line_count, true);
                else
                    cache.update(address, line_count, false);
                line_count++;  
            }
        }
        else {
            cout << "Unable to open " << mat_name << endl;
        }
        cache.flush();

    // output results 
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
    cout << "TIME: " << float(clock()-begin_time) / CLOCKS_PER_SEC << " sec" << endl;
    
    }


    



}