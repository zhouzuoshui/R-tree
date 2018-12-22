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


#ifndef QRNODE_HPP
#define QRNODE_HPP
#include <utility>
#include <vector>
#include <functional>
#include <algorithm>
#include <limits>
#include "circle.hpp"

struct QRBoundingBox{
    
    std::pair<double, double> range[2];

    QRBoundingBox(){}
    QRBoundingBox(double x1, double x2, double y1, double y2);
    void init();
    bool expandToContain(const QRBoundingBox &bb);

    double perimeter() const;
    double area() const;
    bool contains(const QRBoundingBox &bb) const;
    bool overlaps(const QRBoundingBox &bb) const;

    double overlapArea(const QRBoundingBox &bb) const;
    double distance(const QRBoundingBox &bb) const;
};

typedef QRBoundingBox QRNode;

struct Innernode: public QRNode{
    Innernode(){}
    Innernode(double x1, double x2, double y1, double y2)
        :QRNode(x1, x2, y1, y2){}
    std::vector<QRNode*> child;
    bool leafchild;
    Innernode* parent;
    int getLevel();
};


// here Circle member is just a placeholder, you may replace it with any useful info.
// hence the constructor should be rewroten
struct Leafnode: public QRNode{
    Circle cir;
    Innernode* parent;
    Leafnode(Circle tar):cir(tar){
        range[0].first = tar.x - tar.r;
        range[0].second = tar.x + tar.r;
        range[1].first = tar.y - tar.r;
        range[1].second = tar.y + tar.r;
    }
    Leafnode* prev;
    Leafnode* next;
};



// 构造函数接受的参数是叶子，作为基准，成员函数的参数是分支，分支包含叶子之后增加的面积作为排序的原则，以升序排列
// 这里用的指针，可以用const引用来实现
struct AscendingSortByAreaEnlargement: public std::binary_function<const QRNode * const, const QRNode * const, bool>{
    const QRNode item;

    explicit AscendingSortByAreaEnlargement(const QRNode *addThisItem):item(*addThisItem){}

    bool operator() (const QRNode * const bi1, const QRNode * const bi2) const{
        QRNode tmp1 = *bi1;
        QRNode tmp2 = *bi2;

        double area1 = tmp1.area();
        double area2 = tmp2.area();

        tmp1.expandToContain(item);
        tmp2.expandToContain(item);

        return tmp1.area() - area1 < tmp2.area() - area2;
    }
};

struct AscendingSortByFirstRange: public std::binary_function<const QRNode * const, const QRNode * const, bool>{
    const std::size_t axis;

    explicit AscendingSortByFirstRange(const std::size_t axis):axis(axis){}

    bool operator() (const QRNode * const bi1, const QRNode * const bi2) const{
        return bi1->range[axis].first < bi2->range[axis].first;
    }
};

struct AscendingSortBySecondRange: public std::binary_function<const QRNode * const, const QRNode * const, bool>{
    const std::size_t axis;

    explicit AscendingSortBySecondRange(const std::size_t axis):axis(axis){}

    bool operator() (const QRNode * const bi1, const QRNode * const bi2) const{
        return bi1->range[axis].second < bi2->range[axis].second;
    }
};


struct ExpandNode: std::unary_function<const QRNode* const, void>{
    QRNode *node;
    ExpandNode(QRNode* bb): node(bb){}

    void operator() (const QRNode* const item){
        node->expandToContain(*item);
    }
};

struct AscendingSortByDistance: public std::binary_function<const QRNode * const, const QRNode * const, bool>{
    const QRNode* center;

    explicit AscendingSortByDistance(const QRNode * const _center): center(_center){}

    bool operator() (const QRNode* const bi1, const QRNode* const bi2) const {
        return bi1->distance(*center) < bi2->distance(*center);
    }
};

#endif
