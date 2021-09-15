# scapegoat-tree

C++ implementation of Scapegoat Trees: self-balancing binary search trees that store no additional information at each node, have tunable balance specifications, and do not rely on tree rotations. The height of a Scapegoat Tree will be logarithmic in its size; insertion and deletion operations run in amortized logarithmic time and worst-case linear time.

First written for the final project in Stanford's CS166 course in Spring 2021.

Further reading on Scapegoat Trees:
* [Wikipedia article](https://en.wikipedia.org/wiki/Scapegoat_tree)
* [Lecture notes](https://www.cs.umd.edu/class/fall2020/cmsc420-0201/Lects/lect12-scapegoat.pdf), Mount 2020
* ["Scapegoat Trees"](https://people.csail.mit.edu/rivest/pubs/GR93.pdf), Galperin and Rivest 1993 (discusses rebuilding method implemented here)
* ["Scapegoat Trees"](http://publications.csail.mit.edu/lcs/pubs/pdf/MIT-LCS-TR-700.pdf), Galperin 1996 (page 77 onward — discusses alternative rebuilding method)
* ["Improving Partial Rebuilding by Using Simple Balance Criteria"](https://user.it.uu.se/~arnea/ps/partb.pdf), Andersson 1989
