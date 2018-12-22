/*
 *  Copyright (c) 2008 Dustin Spicuzza <dustin@virtualroadside.com>
 *  Copyright (c) 2018 Zuoshui Zhou <zuoshui.zhou@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2.1 of the GNU Lesser General Public
 *  License as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 * 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#ifndef QRTREE_HPP
#define QRTREE_HPP

#include <iterator>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <stack>
#include <queue>
#include "qrnode.hpp"

#define QRTREE_REINSERT_P 0.30
#define QRTREE_CHOOSE_SUBTREE_P 32

struct QRTree{
private:
    int dim;
    int min_child;
    int max_child;
    std::size_t _size;
    Innernode *_root;
    // typedef Leafnode* value_type;

    Leafnode *front;
    Leafnode *end;
    std::size_t _size_full;

public:
    // for insertion
    Innernode* ChooseSubTree(Innernode *inode, const QRBoundingBox *bb) const;
    Innernode* Insert(Leafnode *leaf, Innernode *inode, bool firstInLevel = true);
    // to insert a subtree 
    Innernode* Insert(Innernode* toInsert, Innernode *inode, bool firstInLevel = true);
    Innernode* OverflowTreatment(Innernode *level, bool firstInLevel);
    Innernode* Split(Innernode *inode);
    void Reinsert(Innernode *inode);

    // for query
    void  Innerquery(Innernode* inode, const QRBoundingBox &bb, std::vector<Leafnode> *result);

    // for deletion
    // find a single leaf node, not massive leaves ovelapping a specific region
    void FindLeaf(Innernode* inode, const QRNode &tar, std::vector<Leafnode*> &toDelete);

    // UnderflowTreatment, only for single node removal, not for massive operations
    void CondenseTree(Leafnode *del);

    // 因为外部输入的矩形框内点删除的算法会破坏队列的结构，因此禁止矩形删除，只保留点删除功能，而且只能删除front点
    // reinsert功能呢？？因为点的寿命从它最初被插入到树开始算，树调整过程中，点一直存在，对外未表现出插入与删除的
    // 特征，因此不计入寿命的考虑，不算是新插入的点
    void DeleteLeaf(Leafnode *leaf);
    void Destroy(Innernode* inode);

    QRTree(std::size_t s,int dim = 2, int min_child = 10, int max_child = 20):
       _size_full(s), dim(dim), min_child(min_child), max_child(max_child), _size(0), _root(nullptr){}
    ~QRTree(){Destroy(_root);}

    std::vector<Leafnode>* Query(const QRBoundingBox &bb);
    void InsertData(Circle tar);
    void Delete(QRNode target);
    
    std::size_t Get_size(){return _size;}
    Innernode *Get_root(){return _root;}

};



#endif
