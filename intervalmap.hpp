#ifndef INTERVALMAP_H
#define INTERVALMAP_H

#include <iostream>
#include <iterator> //std::prev
#include <limits>   // std::numeric_limits
#include <memory>   //std::shared_ptr
#include <map>

template <typename K, typename V>
class IntervalMap
{

public:
    ~IntervalMap(){
	treemap.clear();
    }

    void insert(K start, K end, V value)
    {
        if (start >= end)
            return;

	// The entry just before the end index
	auto tmp = treemap.upper_bound(end);
	auto end_prev_entry = tmp == treemap.end() ? tmp : --tmp;
	auto added_end = treemap.end();

	// If it doesn't exist (empty map)
	if(end_prev_entry == treemap.end()){
	    V v{};
	    added_end = treemap.insert_or_assign(treemap.begin(), end, v);
	}
	// Or if it has value different from the insertion
	else if(end_prev_entry->second != value)
	    // Add it back at the end
	    added_end = treemap.insert_or_assign(end_prev_entry, end, end_prev_entry->second);
	
	// The entry just before the start index
	tmp = treemap.upper_bound(start);
	auto start_prev_entry = tmp == treemap.end() ? tmp : --tmp;
	auto added_start = treemap.end();
	// If it has value different from the insertion
	if(start_prev_entry == treemap.end() || start_prev_entry->second != value)
	    // Add the start node of the insertion
	    added_start = treemap.insert_or_assign(start_prev_entry, start, value);

	// Delete everything else inside
	// map.erase(start, end) deletes every node with key in the range [start, end)

	// The key to start deleting from is the key after the start node we added
	// (We don't want to delete a node we just added)
	auto del_start = added_start == treemap.end() ? std::next(start_prev_entry) : std::next(added_start);
	auto del_end = added_end == treemap.end() ? end_prev_entry : added_end;
	auto del_end_next = std::next(del_end);

	// If the node after the end is of the same type of the end, delete it
	// We cannot just expand del_end (++del_end) otherwise interval limits get messed up
	if(del_end != treemap.end() && del_end_next != treemap.end() && del_end->second ==
		del_end_next->second) treemap.erase(del_end_next);

	// Delete everything in between
	if(del_start != treemap.end() && (del_end==treemap.end() || del_start->first <
		    del_end->first)) treemap.erase(del_start, del_end);
    }

    void remove(K at)
    {
        treemap.erase(at);
    }

    auto at(K index)
    {
	const auto tmp = treemap.lower_bound(index);
	const auto r = tmp != treemap.begin() && tmp->first!=index ? std::prev(tmp) : tmp;
	return r;
    }

    void print()
    {
        for (auto i = treemap.begin(); i != treemap.end(); i++)
            std::cout << i->first << ": " << (int)(i->second) << "\n";
	if(!treemap.empty()) std::cout << "end key: " << std::prev(treemap.end())->first << "\n";
    }

    std::unique_ptr<V[]> toArray(int *length)
    {
        if (treemap.empty())
        {
            *length = 0;
            return nullptr;
        }

        const auto &end = std::prev(treemap.end());
        *length = end->first;
        if(*length == 0) return nullptr;
        
	std::unique_ptr<V[]> arr(new V[*length]);
        auto start = treemap.begin();
        for (auto i = std::next(treemap.begin()); i != treemap.end(); i++)
        {
            for (int k = start->first; k < i->first; k++)
                arr[k] = start->second;
            start = i;
        }

        return arr;
    }

    void fromArray(V *arr, int length)
    {
        treemap.clear();

        if (length == 0)
            return;

        V prev = arr[0];
        unsigned int prev_start = 0;
        for (unsigned int i = 1; i < length; i++)
        {
            if (prev != arr[i])
            {
                insert(prev_start, i, prev);
                prev_start = i;
            }
            prev = arr[i];
        }
        insert(prev_start, length, prev);
    }

    auto begin(){
	return treemap.begin();
    }
    
    auto end(){
	return treemap.end();
    }

private:
    std::map<K, V> treemap{};
};

#endif
