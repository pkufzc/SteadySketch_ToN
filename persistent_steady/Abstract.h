#ifndef ABSTRACT_H
#define ABSTRACT_H

#include <unordered_map>

#include "hash.h"

template<typename DATA_TYPE,typename COUNT_TYPE>
class Abstract{
public:
    Abstract(){}
    virtual ~Abstract(){};

    virtual bool Insert(DATA_TYPE *item, const int KEY_LEN) = 0;
    virtual COUNT_TYPE Query(const DATA_TYPE *item, const int KEY_LEN) = 0;
    virtual void NewWindow(const COUNT_TYPE window) = 0;

    inline uint32_t hash(const char* data, const int KEY_LEN, uint32_t seed = 0){
        return Hash::BOBHash32((uint8_t*)data, KEY_LEN, seed);
    }
};

#endif //ABSTRACT_H