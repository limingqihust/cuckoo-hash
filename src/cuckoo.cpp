#include "cuckoo.h"

template<typename keytype,typename valuetype>
cuckoo_hash<keytype,valuetype>::cuckoo_hash(size_t capacity,size_t entry_cnt,int max_depth) : capacity_(capacity),entry_cnt_(entry_cnt),max_depth_(max_depth)
{
    table_=new Bucket* [capacity];
    for(int i=0;i<capacity;i++)
    {
        table_[i]=new Bucket [entry_cnt];
    }
    generator_=std::mt19937(std::random_device()());
    distribution_=std::uniform_int_distribution<int>(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    seed1_=distribution_(generator_);
    seed2_=distribution_(generator_);

    // 初始化驱逐路径图
    graph_=graph(capacity-1);
    // 初始化free_list_
    for(int i=0;i<capacity_;i++)
    {
        graph_.add_free_node(i);
    }
}

template<typename keytype,typename valuetype>
cuckoo_hash<keytype,valuetype>::~cuckoo_hash()
{
    for(int i=0;i<capacity_;i++)
    {
        delete [] table_[i];
    }
    delete [] table_;
}

/*
* 第一个哈希函数
*/
template<typename keytype,typename valuetype>
size_t cuckoo_hash<keytype,valuetype>::hash1(const keytype& key) const
{
    int index;
    const char* str = reinterpret_cast<const char*>(&key);
    MurmurHash3_x86_32(str, sizeof(int), seed1_, &index);
    return index % capacity_;
}

/**
 * 第二个哈希函数
*/
template<typename keytype,typename valuetype>
size_t cuckoo_hash<keytype,valuetype>::hash2(const keytype& key) const {
    int index;
    const char* str = reinterpret_cast<const char*>(&key);
    MurmurHash3_x86_32(str, sizeof(int), seed2_, &index);
    return index % capacity_;
}

/**
 * {kick_key,kick_value} 本来在kick_pos处 现在要被驱逐到其他位置
*/
template<typename keytype,typename valuetype>
bool cuckoo_hash<keytype,valuetype>::kick(const keytype& kick_key, const valuetype& kick_value, id_t kick_pos,int depth) {
    
#ifndef USE_GRAPH
    if (depth > max_depth_) {                      
        need_reconstruction_=true;
        kick_over_bound_cnt_++;
        wait_list_.push_back({kick_key,kick_value});
        return false;
    }
#endif
#ifdef USE_GRAPH
    if(depth>max_depth_)                                 // 驱逐次数超过了驱逐阈值 要检测是不是存在一条路径
    {
        kick_over_bound_cnt_++;
        std::vector<id_t> path;
        id_t nxt_pos=kick_pos ^ hash1(hash2(kick_key));
        if(!judge_evict_path(nxt_pos,path))             // 不存在一条可行的驱逐路径 重构
        {
            wait_list_.push_back({kick_key,kick_value});
            return false;
        }
        else                                            // 找到一条路径 根据这条路径kick
        {
            if(!kick_according_to_path(kick_key,kick_value,path))
                return false;
            // 在这里按照path执行insert
            return true;
        }
    }
#endif

    id_t nxt_pos=kick_pos ^ hash1(hash2(kick_key));

    for(int i=0;i<entry_cnt_;i++)
    {
        Bucket& bucket = table_[nxt_pos][i];
        if (bucket.key == kick_key) {
            bucket.value = kick_value;
#ifdef DEBUG
                printf("{%d,%d} in\n",kick_key,kick_value);
#endif
#ifdef USE_GRAPH
            if(is_full_bucket(nxt_pos))
                graph_.remove_free_node(nxt_pos);
#endif
            return true;
        } else if (bucket.key == keytype()) {
            bucket.key = kick_key;
            bucket.value = kick_value;
#ifdef DEBUG
                printf("{%d,%d} in\n",kick_key,kick_value);
#endif
#ifdef USE_GRAPH
            if(is_full_bucket(nxt_pos))
                graph_.remove_free_node(nxt_pos);
#endif
            return true;
        }
    }

    // 需要剔除 nxt_pos一定没有空闲位置了
    kick_cnt_++;
    int kick_index = distribution_(generator_) % entry_cnt_;
    Bucket& bucket = table_[nxt_pos][kick_index];
    
    keytype out_key = bucket.key;
    valuetype out_value = bucket.value;
    bucket = {kick_key,kick_value};
    bucket.key=kick_key;
    bucket.value=kick_value;
#ifdef DEBUG
        printf("{%d,%d} in\n",kick_key,kick_value);
        printf("{%d,%d} in\n",out_key,out_value);
#endif
    return kick(out_key, out_value, nxt_pos,depth+1);
}

/**
 * 查询 存储在value中 未命中返回false 命中返回true
*/
template<typename keytype,typename valuetype>
bool cuckoo_hash<keytype,valuetype>::lookup(const keytype& key, valuetype& value) const {
    int index_1=hash1(key);
    for(int i=0;i<entry_cnt_;i++)
    {
        const Bucket& bucket=table_[index_1][i];
        if(bucket.key==key){
            value=bucket.value;
            return true;
        }
    }

    int index_2=index_1 ^ hash1(hash2(key));
    for(int i=0;i<entry_cnt_;i++)
    {
        const Bucket& bucket=table_[index_2][i];
        if(bucket.key==key){
            value=bucket.value;
            return true;
        }
    }

    return false;
}

/**
 * 插入操作
*/
template<typename keytype,typename valuetype>
bool cuckoo_hash<keytype,valuetype>::insert(const keytype& key, const valuetype& value) {
    valuetype none;
    if (lookup(key,none)) {
        return true;
    }
    int index_1=hash1(key);
    int index_2=index_1 ^ hash1(hash2(key));
    #ifdef USE_GRAPH
        graph_.add_edge(index_1,index_2);
    #endif
    for(int i=0;i<entry_cnt_;i++)
    {
        Bucket& bucket1 = table_[index_1][i];
        if (bucket1.key == key) {               
            bucket1.value = value;
#ifdef USE_GRAPH
            if(is_full_bucket(index_1))
                graph_.remove_free_node(index_1);
#endif
            return true;
        }
        else if (bucket1.key == keytype()) {    
            bucket1.key = key;
            bucket1.value = value;
#ifdef USE_GRAPH
            if(is_full_bucket(index_1))
                graph_.remove_free_node(index_1);
#endif
            return true;
        }
    }

    
    for(int i=0;i<entry_cnt_;i++)
    {
        Bucket& bucket2 = table_[index_2][i];
        if (bucket2.key == key) {
            bucket2.value = value;
#ifdef USE_GRAPH
            if(is_full_bucket(index_2))
                graph_.remove_free_node(index_2);
#endif
            return true;
        } else if (bucket2.key == keytype()) {
            bucket2.key = key;
            bucket2.value = value;
#ifdef USE_GRAPH
            if(is_full_bucket(index_2))
                graph_.remove_free_node(index_2);
#endif
            return true;
        }
    }
    
    // 需要剔除 一定都没有空闲位置
    int kick_bucket= (distribution_(generator_) % 2)?index_1:index_2;
    int kick_index = distribution_(generator_) % entry_cnt_;
    Bucket& bucket = table_[kick_bucket][kick_index];
    keytype kick_key=bucket.key;
    valuetype kick_value=bucket.value;
    bucket={key,value};
    #ifdef DEBUG
        printf("{%d,%d} in\n",key,value);
        printf("{%d,%d} out\n",kick_key,kick_value);
    #endif
    return kick(kick_key, kick_value,kick_bucket,0);
}


template<typename keytype,typename valuetype>
bool cuckoo_hash<keytype,valuetype>::erase(const keytype& key) {
    valuetype none;
    if (!lookup(key,none)) {
        return false;
    }

    int index_1=hash1(key);
    for(int i=0;i<entry_cnt_;i++)
    {
        Bucket& bucket = table_[index_1][i];
        if (bucket.key == key) {
            bucket.key = keytype();
            bucket.value = valuetype();
            return true;
        }
    }

    int index_2=hash2(key);
    for(int i=0;i<entry_cnt_;i++)
    {
        Bucket& bucket = table_[index_2][i];
        if (bucket.key == key) {
            bucket.key = keytype();
            bucket.value = valuetype();
            return true;
        }
    }

    return false;
}

/*
* 计算占用率
*/
template<typename keytype,typename valuetype>
float cuckoo_hash<keytype,valuetype>::get_occupancy()
{
    int cnt=0;
    for(int i=0;i<capacity_;i++)
    {
        for(int j=0;j<entry_cnt_;j++)
        {
            if(table_[i][j].key!=keytype())
            {
                cnt++;
            }
        }
    }
    item_cnt_=cnt;
    return (float)cnt/((capacity_)*entry_cnt_);
    
}

/**
 * 重构 成功返回true 失败返回false
*/
template<typename keytype,typename valuetype>
bool cuckoo_hash<keytype,valuetype>::reconstruction()
{
    reconstruction_cnt_++;

    // 先清空table_
    for(int i=0;i<capacity_;i++)
    {
        for(int j=0;j<entry_cnt_;j++)
        {
            table_[i][j].key=keytype();
        }
    }
    graph_.reconstruct();
    seed1_=distribution_(generator_);
    seed2_=distribution_(generator_);
    for(int i=0;i<buffer_.size();i++)
    {
        Bucket item=buffer_[i];
        if(!insert(item.key,item.value))
        {
            return false;
        }
    }
    return true;
}


/**
 * 将table_中的内容拷贝到buffer中
*/
template<typename keytype,typename valuetype>
void cuckoo_hash<keytype,valuetype>::copy_to_buffer()
{
    while(!buffer_.empty())
        buffer_.pop_back();
    for(int i=0;i<capacity_;i++)
    {
        for(int j=0;j<entry_cnt_;j++)
        {
            if(table_[i][j].key!=keytype())
                buffer_.push_back(table_[i][j]);
        }
    }
    while(!wait_list_.empty())
    {
        buffer_.push_back(wait_list_.back());
        wait_list_.pop_back();
    }
}


template<typename keytype,typename valuetype>
void cuckoo_hash<keytype,valuetype>::print_info()
{
    std::cout<<"cuckoo capacity:\t"<<capacity_<<std::endl;
    std::cout<<"entry cnt per bucket:\t"<<entry_cnt_<<std::endl;
    std::cout<<"max depth:\t"<<max_depth_<<std::endl;
    std::cout<<"occupany:\t"<<get_occupancy()*100<<"%"<<std::endl;
    std::cout<<"item cnt:\t"<<item_cnt_<<std::endl;
    std::cout<<"kick cnt:\t"<<kick_cnt_<<std::endl;
    std::cout<<"kick over bound cnt:\t"<<kick_over_bound_cnt_<<std::endl;
    std::cout<<"reconstruction cnt:\t"<<reconstruction_cnt_<<std::endl;
}

/**
 * 找出一条从src_node_id节点开始的驱逐路径
*/
template<typename keytype,typename valuetype>
bool cuckoo_hash<keytype,valuetype>::judge_evict_path(id_t src_node_id,std::vector<id_t>& path) const
{
    if(!graph_.shortest_path_from_src(src_node_id,path))
    {
        std::cout<<"do not exist a available evict path,need reconstruction"<<std::endl;
        return false;
    }
    else if(path.size()>kick_path_max_len_)
    {
        std::cout<<"evict path is too long,need reconstruction"<<std::endl;
        return false;
    }
    return true;
}

/**
 * 按照给定的路径进行kick操作
*/
template<typename keytype,typename valuetype>
bool cuckoo_hash<keytype,valuetype>::kick_according_to_path(const keytype key,const valuetype value,std::vector<id_t> path)
{
    id_t pos_to_insert;
    keytype key_to_insert=key,kick_key;
    valuetype value_to_insert=value,kick_value;
    for(int i=path.size()-1;i>=1;i--)
    {
        pos_to_insert=path[i];
        int entry_to_insert;
        for(entry_to_insert=0;entry_to_insert<entry_cnt_;entry_to_insert++)
        {
            if(pos_to_insert^hash1(hash2(table_[pos_to_insert][entry_to_insert].key))==path[i-1])
            {
                kick_key=table_[pos_to_insert][entry_to_insert].key;
                kick_value=table_[pos_to_insert][entry_to_insert].value;
                break;
            }
        }
        table_[pos_to_insert][entry_to_insert]={key_to_insert,value_to_insert};
        key_to_insert=kick_key;
        value_to_insert=kick_value;
    }
    pos_to_insert=path[0];
    for(int j=0;j<entry_cnt_;j++)
    {
        if(table_[pos_to_insert][j].key==keytype())
        {
            table_[pos_to_insert][j]={key_to_insert,value_to_insert};
            if(is_full_bucket(pos_to_insert))
            {
                graph_.remove_free_node(pos_to_insert);
            }
            return true;
        }
    }
    std::cout<<"the last pos of path don not have free entry"<<std::endl;
    return false;
}

/**
 * 判断该bucket是否已满 如已满将其从free_list删掉
*/
template<typename keytype,typename valuetype>
bool cuckoo_hash<keytype,valuetype>::is_full_bucket(id_t pos) const
{
    for(int i=0;i<entry_cnt_;i++)
    {
        if(table_[pos][i].key==keytype())
            return true;
    }
    return false;
}


/*******************************************************************************/
template
cuckoo_hash<int,int>::cuckoo_hash(size_t capacity,size_t entry_cnt,int max_depth);

template
cuckoo_hash<int,int>::~cuckoo_hash();

template
size_t cuckoo_hash<int,int>::hash1(const int& key) const;

template
size_t cuckoo_hash<int,int>::hash2(const int& key) const;

template
bool cuckoo_hash<int,int>::kick(const int& kick_key, const int& kick_value,id_t kick_pos,int depth);

template
bool cuckoo_hash<int,int>::lookup(const int& key, int& value) const;

template
bool cuckoo_hash<int,int>::insert(const int& key, const int& value);

template
bool cuckoo_hash<int,int>::erase(const int& key);

template 
float cuckoo_hash<int,int>::get_occupancy();

template
bool cuckoo_hash<int,int>::reconstruction();

template
void cuckoo_hash<int,int>::copy_to_buffer();

template
void cuckoo_hash<int,int>::print_info();

template
bool cuckoo_hash<int,int>::judge_evict_path(id_t src_node_id,std::vector<id_t>& path) const;

template
bool cuckoo_hash<int,int>::is_full_bucket(id_t pos) const;