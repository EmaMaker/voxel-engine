#include <iostream>
#include <string>
#include <thread>

#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/tbb_allocator.h"

typedef oneapi::tbb::concurrent_hash_map<int, int, oneapi::tbb::tbb_allocator<int>> CTable;

CTable table;
void f(){
    /*while(table.size() > 0){
	std::cout << "---------------\n";
	    oneapi::tbb::parallel_for(table.range(), [=](Table::range_type &r){
		for(Table::const_iterator a = r.begin(); a != r.end(); a++){
		    std::cout << a->first << ": " << a->second << std::endl;
		}
	    });
    }*/
}

int main(){
    std::thread t = std::thread(f);

    //Table::accessor a;
    /*table.emplace(a, std::make_pair(0, "zero"));
    table.emplace(a, std::make_pair(1, "one"));
    table.emplace(a, std::make_pair(2, "two"));
    table.emplace(a, std::make_pair(3, "three"));
    table.emplace(a, std::make_pair(4, "four"));
    table.emplace(a, std::make_pair(5, "five"));*/

    t.join();
}
