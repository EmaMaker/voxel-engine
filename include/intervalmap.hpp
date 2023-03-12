#ifndef INTERVALMAP_H
#define INTERVALMAP_H

#include <iostream>
#include <iterator> //std::prev
#include <memory>   //std::shared_ptr
#include <map>

template <typename V>
class IntervalMap
{

public:
    ~IntervalMap(){
	treemap.clear();
    }

    void insert(int start, int end, V value)
    {
        if (start >= end)
            return;

        // higherEntry -> upper_bound
        // lowerEntry -> find()--
        // c++ has no builtin function to find the greater key LESS THAN the supplied key. The solution I've found is to get an iterator to the key with find() and traverse back with std::prev

        const auto &tmp = treemap.lower_bound(end);
        const auto &end_prev_entry = tmp != treemap.begin() && tmp != treemap.end() ? std::prev(tmp) : tmp; // first element before end
        const auto &end_next_entry = treemap.upper_bound(end);                                              // first element after end

        const auto &tmp1 = treemap.lower_bound(start);
        const auto &start_entry = tmp1 != treemap.begin() && tmp1 != treemap.end() ? std::prev(tmp1) : tmp1; // first element before start

        V v{};
        // insert the start key. Replaces whatever value is already there. Do not place if the element before is of the same value
        treemap[start] = value;
        if (end_next_entry == treemap.end())
        {
            if (end_prev_entry != treemap.end() && end_prev_entry->first > start)
                treemap[end] = (V)(end_prev_entry->second);
            else
                treemap[end] = v;
        }
        else
        {
            if (end_next_entry->first > end + 1)
                treemap[end] = (V)(end_prev_entry->second);
        }

        treemap.erase(treemap.upper_bound(start), treemap.lower_bound(end));
    }

    void remove(int at)
    {
        treemap.erase(at);
    }

    V at(int index)
    {
        return treemap.lower_bound(index)->second;
    }

    void print()
    {
        for (auto i = treemap.begin(); i != treemap.end(); i++)
            std::cout << i->first << ": " << (int)(i->second) << "\n";
        std::cout << "end key: " << std::prev(treemap.end())->first << "\n";
    }

    std::unique_ptr<V[]> toArray(int *length)
    {
        if (treemap.empty())
        {
            // std::cout << "List is empty" << std::endl;
            *length = 0;
            return nullptr;
        }

        const auto &end = std::prev(treemap.end());

        *length = end->first;
        if(*length == 0) return nullptr;
        
        // std::cout << "Length: " << *length << "\n";
	std::unique_ptr<V[]> arr(new V[*length]);

        auto start = treemap.begin();
        for (auto i = std::next(treemap.begin()); i != treemap.end(); i++)
        {
            // std::cout << "creating list from " << start->first << " to " << i->first << " of type " << (int)(start->second) << "\n";
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

private:
    std::map<int, V> treemap{};
};

#endif
