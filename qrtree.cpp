/*
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


#include "qrtree.hpp"

// end is last element, not its next position
void QRTree::InsertData(Circle tar){
    Leafnode* newLeaf =new Leafnode{tar}; // no dynamic memery needed
  
    // if tree is still empty
    if(_root == nullptr){
        _root = new Innernode(tar.x-tar.r, tar.x+tar.r, tar.y-tar.r, tar.y+tar.r);
        _root->leafchild = true;
        _root->parent = nullptr;

        _root->child.reserve(min_child);
        _root->child.push_back(newLeaf);

        newLeaf->prev = nullptr;
        newLeaf->next = nullptr;
        front = newLeaf;
        end = newLeaf;
    }
    else{
        newLeaf->prev = end;
        end->next = newLeaf;
        end = newLeaf;
        Insert(newLeaf, _root);
    }
        
    _size++;
    if(_size > _size_full){
        front = front->next;
        DeleteLeaf(front->prev);
        --_size;
    }
}

// parameter bb is the bound of leaf node
Innernode* QRTree::ChooseSubTree(Innernode *inode, const QRBoundingBox *bb) const{

    // quod this function only used here, thus defined locally
    auto findMinOverlapEnlargement = [inode, bb](std::size_t sort_length){
        double overlap_min = 0.0;
        auto omIndex = inode->child.begin();
        for(auto i = inode->child.begin(); i != inode->child.begin() + sort_length; ++i){
            // secure copy of node i
            // Note, i is iterator to vector element, vector element is pointer to child node, so ppointer
            auto copy_range = (*i)->range;
            QRNode copy_node{copy_range[0].first, copy_range[0].second, copy_range[1].first, copy_range[1].second};
            copy_node.expandToContain(*bb);

            double overlap = 0;
            for(auto j = inode->child.begin(); j != inode->child.end(); ++j){
                overlap += copy_node.overlapArea(**j);
            }

            if(overlap < overlap_min || ((overlap_min < 1e-7) && (overlap_min > -1e-7))){
                overlap_min = overlap;
                omIndex = i;
            }

        }
        // it must be an Innernode, or this method won't be called
        return static_cast<Innernode*>(*omIndex);
    };

    // 如果往下两层就是叶子
    if((static_cast<Innernode*>(inode->child[0]))->leafchild){

        if((max_child > QRTREE_CHOOSE_SUBTREE_P * 2/3) && (inode->child.size() > QRTREE_CHOOSE_SUBTREE_P)){
            // 这里的functor的需求是，使得面积的增大为升序，最小的在前
            std::partial_sort(inode->child.begin(), inode->child.begin() + QRTREE_CHOOSE_SUBTREE_P,
                inode->child.end(), AscendingSortByAreaEnlargement(bb));
          
            return findMinOverlapEnlargement(QRTREE_CHOOSE_SUBTREE_P);  
        }
        return findMinOverlapEnlargement(inode->child.size());
    }

    // else
    return static_cast<Innernode*>(* std::min_element(inode->child.begin(), inode->child.end(), AscendingSortByAreaEnlargement(bb)));
}

Innernode* QRTree::Insert(Leafnode *leaf, Innernode *inode, bool firstInLevel){
    inode->expandToContain(*leaf);  // type may not compatible

    if(inode->leafchild){
        inode->child.push_back(leaf);
        leaf->parent = inode;       // update parent pointer
    }
    else{
        
        Innernode *tmp_node = Insert(leaf, ChooseSubTree(inode, leaf), firstInLevel);

        // no overflow
        if(!tmp_node)
            return nullptr;

        // otherwise
        inode->child.push_back(tmp_node);
        tmp_node->parent = inode;
    }

    // after insertion, whether this node overflows
    if(inode->child.size() > max_child){
        // only OT could return a non-null pointer
        return OverflowTreatment(inode, firstInLevel);
    }
    return nullptr;
}

Innernode* QRTree::OverflowTreatment(Innernode *level, bool firstInLevel){
    if(level != _root && firstInLevel){
        Reinsert(level);
        return nullptr;
    }

    // new node generated
    Innernode* splitItem = Split(level);

    if(level == _root){
        Innernode *newRoot = new Innernode();
        newRoot->leafchild = false;

        newRoot->child.reserve(min_child);
        newRoot->child.push_back(_root);
        newRoot->child.push_back(splitItem);

        newRoot->init();
        for(auto i: newRoot->child){
            newRoot->expandToContain(*i);
        }

        _root->parent = newRoot;
        splitItem->parent = newRoot;

        _root = newRoot;
        _root->parent = nullptr;
        
        return nullptr;   
    }

    return splitItem;
}

Innernode* QRTree::Split(Innernode *inode){
    Innernode *newNode = new Innernode();
    newNode->child.reserve(min_child);
    newNode->leafchild = inode->leafchild;

    // child number
    const std::size_t child_n = inode->child.size();
    // distribution number
    const std::size_t distro_n = child_n - 2*min_child + 1;

    std::size_t split_axis = dim + 1, split_range = 0, split_index = 0;

    int split_margin = 0;

    QRNode R1, R2;

    // 对每个维度
    for(std::size_t axis = 0; axis < dim; ++axis){
        int margin = 0;
		double overlap = 0, dist_area, dist_overlap;
		std::size_t dist_range = 0, dist_index = 0;
		
		dist_area = dist_overlap = std::numeric_limits<double>::max();

        for(std::size_t r =0; r <2; ++r){
            if(r == 0)
                std::sort(inode->child.begin(), inode->child.end(), AscendingSortByFirstRange(axis));
            else
                std::sort(inode->child.begin(), inode->child.end(), AscendingSortBySecondRange(axis));

            // 对每个distro
            for(std::size_t k =0; k< distro_n; ++k){
                double area = 0;

                R1.init();
                std::for_each(inode->child.begin(), inode->child.begin() + k + min_child, ExpandNode(&R1));
                R2.init();
                std::for_each(inode->child.begin()+ k + min_child, inode->child.end(), ExpandNode(&R2));

                margin += R1.perimeter() + R2.perimeter();
                area += R1.area() + R2.area();
                overlap = R1.overlapArea(R2);

                if(overlap < dist_overlap || (overlap == dist_overlap && area < dist_area)){
                    dist_range  = r;
                    dist_index = min_child+k;
                    dist_overlap = overlap;
                    dist_area = area;
                }
            }
        }

        // 第一个条件仅仅是为了开始split_margin为0可以继续运行
        if(split_axis == dim + 1 || split_margin > margin){
            split_axis = axis;
            split_margin = margin;
            split_range = dist_range;
            split_index = dist_index;
        }
    }

    if(split_range == 0)
        std::sort(inode->child.begin(), inode->child.end(), AscendingSortByFirstRange(split_axis));
    
    else if(split_axis != dim -1)
        std::sort(inode->child.begin(), inode->child.end(), AscendingSortBySecondRange(split_axis));

    newNode->child.assign(inode->child.begin() + split_index, inode->child.end());
    
    inode->child.erase(inode->child.begin() + split_index, inode->child.end());

    inode->init();
    std::for_each(inode->child.begin(), inode->child.end(), ExpandNode(inode));

    newNode->init();
    std::for_each(newNode->child.begin(), newNode->child.end(), ExpandNode(newNode));

    // 更新本点与孩子的关系
    if(!newNode->leafchild)
        for(auto i: newNode->child)
            static_cast<Innernode*>(i)->parent = newNode;
    else
        for(auto i: newNode->child)
            static_cast<Leafnode*>(i)->parent = newNode;
    

    return newNode;

}

void QRTree::Reinsert(Innernode *inode){
    std::vector<QRNode*> removed_items;

    const std::size_t n_items = inode->child.size();
    // 如果30%的M是存在的，则使用这个值，否则就使用1，依P327左下段的描述。只reinsert这些元素
    const std::size_t p = (std::size_t)((double)n_items * QRTREE_REINSERT_P) > 0 ? (std::size_t)((double)n_items * QRTREE_REINSERT_P) : 1;

    // RI1
    assert(n_items == max_child + 1);

    // RI 2
    std::partial_sort(inode->child.begin(), inode->child.end() - p, inode->child.end(), AscendingSortByDistance(inode));

    if(inode->leafchild){
        removed_items.assign(inode->child.end()-p, inode->child.end());

        inode->child.erase(inode->child.end() - p, inode->child.end());
    
        // RI3
        inode->init();
        std::for_each(inode->child.begin(), inode->child.end(), ExpandNode(inode));

        for(auto i : removed_items){
            Insert(static_cast<Leafnode*>(i), _root, false);
        }
    }
    else{
        for(auto i: inode->child)
            Reinsert(static_cast<Innernode*>(i));
    }
    
}

std::vector<Leafnode>* QRTree::Query(const QRBoundingBox &bb){
    auto result = new std::vector<Leafnode>;
    Innerquery(_root, bb, result);
    return result;
}

void QRTree::Innerquery(Innernode* inode, const QRBoundingBox &bb, std::vector<Leafnode>* result){
    // S2
    if(inode->leafchild){
        for(auto i: inode->child){
            if(i->overlaps(bb))
                result->push_back(*(static_cast<Leafnode*>(i)));
        }
    }

    // S1 
    else{
        for(auto i: inode->child){
            if(i->overlaps(bb)){
                Innerquery(static_cast<Innernode*>(i), bb, result);
            }
        }
    }
    return;
}

void QRTree::DeleteLeaf(Leafnode *leaf){
    // D2
    auto x = find(leaf->parent->child.begin(), leaf->parent->child.end(), leaf);

    // could x be end???
    if(x != leaf->parent->child.end())
        leaf->parent->child.erase(x);
    else
        std::cout << "not found\n";     // not found的圆在别处存在，本处已经不是了, 
        
    // D3, no leaf removed yet
    CondenseTree(leaf);

    if(leaf->prev)
        leaf->prev->next = leaf->next;
    if(leaf->next)
        leaf->next->prev = leaf->prev;
    
    delete leaf; 
    // std::cout << "delete done\n"; 

    // D4：当_root的孩子是叶子时，不能改变层次 
    if((_root->child.size() == 1) && (!_root->leafchild)){
        auto oroot = _root;
        _root = static_cast<Innernode*>(_root->child[0]); 
        delete oroot;
    }

}

void QRTree::Delete(QRNode target){
    std::vector<Leafnode*> toDelete;
    // QRNode target{tar.x-tar.r, tar.x+tar.r, tar.y-tar.r, tar.y+tar.r};
    FindLeaf(_root, target, toDelete);

    for(auto i:toDelete){
        // D2
        auto x = find(i->parent->child.begin(), i->parent->child.end(), i);

        // could x be end???
        if(x != i->parent->child.end())
            i->parent->child.erase(x);
        else
            std::cout << "not found\n";     // not found的圆在别处存在，本处已经不是了, 
        
        // D3, no leaf removed yet
        CondenseTree(i);

        if(i->prev)
            i->prev->next = i->next;
        if(i->next)
            i->next->prev = i->prev;
    
        delete i; 
        // std::cout << "delete done\n"; 
        
    }

    // D4：当_root的孩子是叶子时，不能改变层次
    
    if((_root->child.size() == 1) && (!_root->leafchild)){
        auto oroot = _root;
        _root = static_cast<Innernode*>(_root->child[0]); 
        delete oroot;
    }
}

// not Guttman's Algorithm, since in my case usually a region not a specific node
// would be removed, so there are must massive leaf nodes which overlap the target
// region to be deleted
void QRTree::FindLeaf(Innernode* inode, const QRNode &tar, std::vector<Leafnode*> &toDelete){
    if(inode->leafchild){
        for(auto i: inode->child){
            if(i->overlaps(tar))
                toDelete.push_back(static_cast<Leafnode*>(i));
        }
        return;
    }
    else{
        for(auto i: inode->child){
            if(i->overlaps(tar)){
                FindLeaf(static_cast<Innernode*>(i), tar, toDelete);
            }
        }
        return;
    }
}

void QRTree::CondenseTree(Leafnode *del){
    // CT1
    auto N = del->parent;
    auto P = N->parent;
    std::vector<Innernode *> Q;
  
    // CT2: if N is the root, goto CT6
    // quod _root update not so on time, old _root may cause problem
    while(N != _root){
        // otherweise let P be the parent of N, and let En be N's entry in P
        P = N->parent;
  
        // CT3: if N has fewer than m entries,
        if(N->child.size() < min_child){
            // delete En from P(since no two nodes share same boundingbox)
            // type not compatible
            auto x = find(P->child.begin(), P->child.end(), static_cast<QRNode*>(N));
            if(x != P->child.end())
                P->child.erase(x);

            // and add N to Q
            Q.push_back(N);
        }
        // CT4: if N has not been elimanated, adjust EnI to tightly contain all entries in N
        else{
            N->init();
            std::for_each(N->child.begin(), N->child.end(), ExpandNode(N));
        }

        // CT5: set N = P and repeat from CT2
        N = P;
        
    }

    // CT6: reinsert all entries of nodes in set Q. don't have to use Guttman's method,
    // since redistribution may generate a better performance.
    // find all leaves of a certain node.

    for(auto i: Q){
        std::stack<Innernode *> toVisit;
        auto j = i;

        if(j->leafchild){
            for(auto k: j->child)
                Insert(static_cast<Leafnode*>(k), _root);
        }
        else{

            // different implements than Destory, one with stack, one with queue, better with queue
            auto goLeftMost = [&toVisit](Innernode *node){
                do{
                    for(auto k = node->child.rbegin(); k != node->child.rend(); ++k)
                        toVisit.push(static_cast<Innernode*>(*k));
                    // must have child
                    node = (toVisit.top());
                    toVisit.pop();
                }while(!node->leafchild);
                toVisit.push(node);
            };

            Innernode* item;
            goLeftMost(j);

            while(!toVisit.empty()){
                item = toVisit.top();
                toVisit.pop();

                if(!item->leafchild){
                    goLeftMost(item);
                    continue;
                }
                    
                for(auto n: item->child)
                    Insert(static_cast<Leafnode*>(n), _root);
     
            };

        }
    }

}

void QRTree::Destroy(Innernode* inode){
    std::vector<Innernode*> toDelete;
    while(!inode->leafchild){
        for(auto i: inode->child){
            toDelete.push_back(static_cast<Innernode*>(i));
        }
        delete inode;
        inode = toDelete.front();
        toDelete.erase(toDelete.begin());
    }

    for(auto i: toDelete){
        for(auto j: i->child){
            delete j;
            --_size;
        }
        delete i;
    }
}
