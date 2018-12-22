#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "qrtree.hpp"

#define NODES 2000
#define REGION_X 2000
#define REGION_Y 2000
#define DEL_MIN 1600
#define DEL_MAX 1950

int main(int argc, char* const argv[]){

    double r;
    QRTree tree{2000};

    QRBoundingBox bb{0, REGION_X, 0, REGION_Y};
    QRBoundingBox to_de{DEL_MIN, DEL_MAX, DEL_MIN, DEL_MAX};

    for(auto i = 0; i < NODES; ++i){
        Circle cir{};
        cir.x = rand()%REGION_X;
        cir.y = rand()%REGION_Y;
        cir.r = rand()%20;
        tree.InsertData(cir);
    }

    cv::Mat atomImage = cv::Mat::zeros(REGION_Y, REGION_X, CV_8UC3 );

    tree.Delete(to_de);

    auto ress = tree.Query(bb);
    for(auto i: *ress)
        cv::circle(atomImage, cv::Point(i.cir.x, i.cir.y), i.cir.r, cv::Scalar( 0, 255, 255 ), -1, 8);


    cv::imwrite("show.png", atomImage);
    
    return 0;
}
