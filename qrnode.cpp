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
 

#include "qrnode.hpp"

QRBoundingBox::QRBoundingBox(double x1, double x2, double y1, double y2){
    range[0].first = x1;
    range[0].second = x2;
    range[1].first = y1;
    range[1].second = y2;
}

// 将起点设为最大，终点设为最小，
void QRBoundingBox::init(){
    for (std::size_t axis = 0; axis < 2; axis++){
		range[axis].first = std::numeric_limits<double>::max();
		range[axis].second = -std::numeric_limits<double>::max();
	}
}

bool QRBoundingBox::expandToContain(const QRBoundingBox &bb){
    bool modified = false;
    for(int i =0; i<2; ++i){
        if(range[i].first > bb.range[i].first){
            range[i].first = bb.range[i].first;
            modified = true;
        }
        if(range[i].second < bb.range[i].second){
            range[i].second = bb.range[i].second;
            modified = true;
        }
    }
    return modified;
}

double QRBoundingBox::perimeter() const{
    return range[1].second - range[1].first + range[0].second - range[0].second;
}

double QRBoundingBox::area() const{
    return (range[1].second - range[1].first) * (range[0].second - range[0].second);
}

bool QRBoundingBox::contains(const QRBoundingBox &bb) const{
    for(int i =0; i<2; ++i){
        if(range[i].first > bb.range[i].first)
            return false;
        if(range[i].second < bb.range[i].second)
            return false;
    }
    return true;
}

// 如果一个点与搜寻目标的各个维度上的range，都有相交，即为overlap，其反为，存在一个维度，range不相交错。
bool QRBoundingBox::overlaps(const QRBoundingBox &bb) const{
    for(int i =0; i<2; ++i)
        if (range[i].first > bb.range[i].second || bb.range[i].first > range[i].second)
				return false;
    return true;
}

double QRBoundingBox::overlapArea(const QRBoundingBox &bb) const{
    double area = 0.0;
    for(int i =0; i<2; ++i){
        const double a1 = range[i].first;
        const double a2 = range[i].second;
        const double b1 = bb.range[i].first;
        const double b2 = bb.range[i].second;

        if(a1 < b1){
            if(b1 < a2){
                if(b2 < a2)
                    area *= (b2 - b1);
                else
                    area *= (a2 - b1);
                continue;
            }
            else if(a1 < b2){
                if(a2 < b2)
                    area *= (a2 - a1);
                else
                    area *= (b2 - a1);
                continue;
            }
        }
        return 0;
    }
    return area;
}

double QRBoundingBox::distance(const QRBoundingBox &bb) const{
    double dist = 0.0;
    for(std::size_t i = 0; i < 2; ++i){
        double x1 = (range[i].first + range[i].second)/2;
        double x2 = (bb.range[i].first + bb.range[i].second)/2;

        dist +=  (x1 - x2) * (x1 - x2);
    }
    return dist;
}

int Innernode::getLevel(){
    int level = 1;
    auto i = this;
    while(!i->leafchild){// 只要本点的孩子不是叶子，则继续
        // 写在一起编译出问题，不知道为啥，就分开了
        auto j = i->child[0];
        i = static_cast<Innernode*>(j);
        ++level;
    }
    return level;
}
