#include <iostream>
#include <random>
#include <chrono>
#include "cuckoo.h"
// #define TEST_SIZE 260000
// #define CAPACITY 0x7fff
// #define ENTRY_CNT 8
// #define MAX_DEPTH 500

// #define TEST_SIZE 252965
// #define CAPACITY 0xffff
// #define ENTRY_CNT 4
// #define MAX_DEPTH 500

// #define TEST_SIZE 234796
// #define CAPACITY 0x1ffff
// #define ENTRY_CNT 2
// #define MAX_DEPTH 5000

// #define CAPACITY 0x3ffff
// #define ENTRY_CNT 1
// #define MAX_DEPTH 5000

void init_test_array(int* array,int size)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    for(int i=1;i<size;i++)
    {
        array[i]=distribution(generator);
    }
}

template<typename keytype,typename valuetype>
void print_info(int test_array_size,cuckoo_hash<keytype,valuetype>& mycuckoo)
{
    printf("----------test done----------\n");
    printf("num of insert item:\t%d\n",test_array_size-1);
    mycuckoo.print_info();
}

template<typename keytype,typename valuetype>
bool check(int* test_array,cuckoo_hash<keytype,valuetype>* mycuckoo,int test_size)
{   
    for(int i=1;i<test_size;i++)
    {
        valuetype hash_value;
        if(!mycuckoo->lookup(i,hash_value))
        {
            printf("pairs{%d,%d} is not in cuckoo hash table\n",i,test_array[i]);
            return false;
        }
        else if(hash_value!=test_array[i])
        {
            printf("value of %d should be %d,but be %d\n",i,test_array[i],hash_value);
            return false;
        }
    }
    return true;
}

int main(int argc,char* argv[])
{
    if(argc!=5)
    {
        std::cout<<"Usage:\tcapacity\tentry_cnt\tmax_depth\ttest_size"<<std::endl;
        exit(0);
    }
    int test_capacity=atoi(argv[1]);
    int test_entry_cnt=atoi(argv[2]);
    int test_max_depth=atoi(argv[3]);
    int test_size=atoi(argv[4]);
    printf("============test============\n");
    printf("capacity:\t%d\n",test_capacity);
    printf("entry_cnt:\t%d\n",test_entry_cnt);
    printf("max_depth:\t%d\n",test_max_depth);
    printf("test_size:\t:%d\n",test_size);
    cuckoo_hash<int,int> mycuckoo(test_capacity,test_entry_cnt,test_max_depth);

    int test_array[test_size];
    init_test_array(test_array,test_size);
    printf("insert begin\n");
    auto now = std::chrono::high_resolution_clock::now();
    int start=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    for(int i=1;i<test_size;i++)
    {
        
        while(!mycuckoo.insert(i,test_array[i]))
        {
            #ifdef DEBUG
            printf("insert {%d,%d} error,need reconstruction\n",i,test_array[i]);
            #endif

            mycuckoo.copy_to_buffer();
            while(!mycuckoo.reconstruction())  {}   

            #ifdef DEBUG
            std::cout<<"construction done"<<std::endl;
            #endif
        }
    }
    printf("insert done\n");
    now = std::chrono::high_resolution_clock::now();
    int end=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    printf("time:\t%d ms\n",end-start);
    printf("time per item:\t%f\n",(float)(end-start)/test_size);
    if(!check(test_array,&mycuckoo,test_size))
    {
        exit(0);
    }
    std::cout<<"insert correct"<<std::endl;
    print_info(test_size,mycuckoo);
    exit(0);
}
