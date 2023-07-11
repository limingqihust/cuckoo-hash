#ifndef GRAPH_H
#define GRAPH_H
#include <vector>
#include <iostream>
#include <unordered_map>
#include <set>
#include <queue>
#define INF 0x3f3f3f3f
class graph
{
private:
    id_t max_id_;
    std::set<id_t> free_list_;                                  // 具有空闲位置的节点
    std::unordered_map< id_t,std::set<id_t> > graph_;       // 图
public:
    graph()=default;
    graph(id_t max_node_id);
    ~graph()=default;
    void add_free_node(id_t id);
    void remove_free_node(id_t id);
    void add_edge(id_t id_x,id_t id_y);
    bool shortest_path_from_src(id_t src_node_id,std::vector<id_t>& path) const;
    void reconstruct();
};

#endif