#ifndef CUCKOO_H
#define CUCKOO_H
#include <iostream>
#include <random>
#include <cstring>
#include "MurmurHash3.h"
#include "city.h"
#include "graph.h"
using namespace std;
#define USE_GRAPH
// #define DEBUG
template <typename keytype, typename valuetype>
class cuckoo_hash {
private:
    size_t capacity_;               // =0x10000 那么下标值可以取[0,0b1111]
    size_t entry_cnt_;
    int max_depth_{500};            // 一次普通驱逐的最大次数
    int kick_cnt_{0};               // 总共进行的驱逐次数
    int kick_over_bound_cnt_{0};    // 一次插入操作的驱逐次数超过阈值的次数
    int kick_path_max_len_{10000};  // 驱逐路径的最大值 当驱逐路径超过这个值 直接重构
    int reconstruction_cnt_{0};     // 进行重构的次数

    size_t item_cnt_{0};            // 元素计数
    int seed1_,seed2_;
    bool need_reconstruction_{false};
    std::mt19937 generator_;
    std::uniform_int_distribution<int> distribution_;

    struct Bucket {
        keytype key;
        valuetype value;
    };
    Bucket** table_;
    std::vector<Bucket> buffer_;
    std::vector<Bucket> wait_list_;
    class graph graph_;
    

    size_t hash1(const keytype& key) const ;
    size_t hash2(const keytype& key) const ;
    bool kick(const keytype& key, const valuetype& value,id_t kick_pos, int depth);
    bool is_full_bucket(id_t pos) const;
public:

    // 构造函数
    cuckoo_hash(size_t capacity,size_t entry_cnt,int max_depth);
    ~cuckoo_hash();

    // 查询
    bool lookup(const keytype& key, valuetype& value) const;

    // 插入
    bool insert(const keytype& key, const valuetype& value);

    // 删除
    bool erase(const keytype& key);

    // 重构
    bool reconstruction();

    // 将table_中的元素拷贝至buffer
    void copy_to_buffer();

    // 淘汰计数
    int get_kick_cnt(){return kick_cnt_;}

    // 占用率
    float get_occupancy();

    // 查看是不是需要重构
    bool get_need_reconstruction(){return need_reconstruction_;}

    // 判断有没有一条可行的驱逐路径
    bool judge_evict_path(id_t src_node_id,std::vector<id_t>& path) const;

    // 根据给定的路径完成驱逐操作
    bool kick_according_to_path(const keytype key,const valuetype value,std::vector<id_t> path);

    int get_kick_over_bound_cnt(){return kick_over_bound_cnt_;};

    int get_reconstruction_cnt(){return reconstruction_cnt_;}

    size_t get_item_cnt_(){return item_cnt_;}

    void print_info();
    
};


#endif