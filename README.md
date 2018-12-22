# R-tree
R*-Tree in C++, not template.

This is modified based on [Dustin Spicuzza's code](https://github.com/virtuald/r-star-tree), 
which is wroten using generics, that may offer a more easy-to-use option. Thanks to his work! 
Since for my specific project no metaprogramming needed, so I just removed it. 

Basicly I adopted Spicuzza's Insert method, rewrite Delete and Query method, 
and added FIFO feature for leaf nodes, to keep leaf amount constant. Thus 
this is not a dedicated storage model.

Based on Guttman: R-trees: A Dynamic Index Structure for Spatial Searching,
and Beckmann: The R*-tree: An Efficient and Robust Access Method for 
Points and Rectangles
