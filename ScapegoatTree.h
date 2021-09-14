/**
 * ScapegoatTree.h provides the interface for a Scapegoat Tree, an intuitive 
 * and tunable self-balancing binary search tree that stores no additional 
 * information at each node, for integers.
 * 
 * Authors: Ryan Guan (rzgn) and Tyler Packard (TPackard), 2021
 * 
 * CS166 project partners: Nali Welinder and Parker Jou
 * 
 * Scapegoat tree concept and algorithm for scapegoat finding explained in 
 * "Scapegoat Trees" by Igal Galperin and Ronald Rivest, 1993:
 * https://people.csail.mit.edu/rivest/pubs/GR93.pdf
 * 
 * See also page 77 onward in "On Consulting a Set of Experts and Searching" 
 * by Igal Galperin, 1996: 
 * http://publications.csail.mit.edu/lcs/pubs/pdf/MIT-LCS-TR-700.pdf
 */ 

#include <cstddef>  // for std::size_t
#include <stack>    // for scapegoat-node-finding stack
#include <cmath>    // for floor, log

/**
 * Class representing a Scapegoat Tree. 
 */ 
class ScapegoatTree {
  public:
    /**
     * Constructs a new, empty scapegoat tree with the provided alpha value.
     * Throws std::invalid_argument if alpha is not within [0.5, 1).
     * 
     * Time complexity: O(1)
     */
    ScapegoatTree(double alpha);

    /**
     * Frees all memory allocated by this scapegoat tree.
     * 
     * Time complexity: O(N)
     */
    ~ScapegoatTree();

    /**
     * Returns whether the given key is present in the tree.
     * 
     * Time complexity: O(log N)
     */
    bool search(int key) const;

    /**
     * Inserts the given key into the scapegoat tree. If the element was added,
     * this function returns true. If the element already existed, this
     * function returns false and does not modify the tree.
     * 
     * Time complexity: amortized O(log N), worst-case O(N)
     * Space complexity: O(log N)
     */
    bool insert(int key);

    /**
     * Remove a key from the tree. If the element was removed, this function 
     * returns true. If the element did not exist in the tree, this function
     * returns false and doesn't modify the tree.
     * 
     * Time complexity: amortized O(log N), worst-case O(N)
     */
    bool remove(int key);

    /**
     * Returns whether the scapegoat tree is loosely alpha-height balanced.
     */
    bool verify() const;

    /**
     * Prints a pre-order traversal of the tree in a nice format for debugging.
     */
    void printDebugInfo() const;

  private:
    // Helper structs:

    /** 
     * Represents a standard BST node, holding its key and children. 
     */
    struct Node {
      int    key;

      Node*  left;
      Node*  right;
    };

    /** 
     * Contains information about the "scapegoat node" useful for a rebuild.
     */
    struct Scapegoat {
      Node* scapegoat;  // The node at the root of the imbalanced subtree
      Node* parent;     // The parent of the imbalanced subtree, nullptr if scapegoat == root
      
      size_t treeSize;  // The size of the scapegoat node's subtree 
    };

    /**
     * Data used to verify the correctness of a subtree.
     */ 
    struct VerificationData {
      bool balanced;  // Whether or not the subtree is loosely alpha-height balanced.
      bool isBST;     // Whether or not the subtree follows BST ordering.

      size_t size;    // Size of the subtree.
      int height;     // Height of the subtree (-1 if the tree is empty)
    };


    // Attributes of the tree:

    Node* root = nullptr;
    size_t size = 0;      // Current size of tree.
    size_t maxSize = 0;   // Max size of tree since last rebuild.

    /** 
     * An alpha-weight-balanced node can have one subtree as large as 
     * alpha * the total number of nodes in its subtree.
     */
    static constexpr double kDefaultAlpha = 0.5;
    static constexpr double kMinAlpha = 0.5;
    static constexpr double kMaxAlpha = 1.0;
    double alpha = kDefaultAlpha; 

    /**
     * Represents whether a node being removed will be replaced with its 
     * successor (true) or predecessor (false). This instance variable is flipped
     * every time a node is removed, which empirically helps to maintain 
     * overall tree balance after many removals. 
     * 
     * See removeNodeWithTwoChildren method.
     */
    bool replaceWithSucc = true;


    // Helper functions:

    /**
     * Returns the alpha-deep height == floor(log_{1/alpha}(size)) 
     * for a subtree of given size.
     */ 
    size_t getAlphaDeepHeight(size_t size) const {
      return static_cast<size_t>(floor(log(size) / log(1 / alpha)));
    }

    /**
     * findScapegoat helper:
     * Returns the number of nodes in the subtree rooted at the given node.
     */
    size_t getSubtreeSize(Node *node);

    /**
     * Insert subroutine:
     * Given a stack of nodes representing the ancestors of an inserted node n
     * in order of descending depth in the tree, where n_i is the i-th ancestor
     * of node n, returns the deepest ancestor such that i > the alpha-deep-height
     * of n_i's subtree, as well as that ancestor's parent and subtree size. 
     * 
     * This ancestor is weight-imbalanced and therefore a suitable scapegoat
     * (see Galperin and Rivest for proof)
     * 
     * Precondition: there is at least one ancestor on the path
     *    (insertionPath contains at least 2 elements)
     */
    Scapegoat findScapegoat(std::stack<Node *>& insertionPath);

    /** 
     * Remove subroutine: 
     * Removes the given node from the tree by swapping its key with 
     * its in-order successor / predecessor node's key.
     * 
     * Parameters: node has two children.
     * Postcondition: the memory of the node wired out of the tree is freed.
     */
    void removeNodeWithTwoChildren(Node *node);

    /** 
     * Remove subroutine: 
     * Removes the given node from the tree by replacing it with 
     * its only child, or nullptr if it is a leaf.
     * 
     * Parameters: node has at most one child, parent is its parent or 
     *    nullptr if node is the root of the tree.
     * Postcondition: node has been wired out of the tree, and its memory is freed.
     */
    void removeNodeWithoutChild(Node *node, Node *parent);

    /** 
     * Rebuild the subtree rooted at the given scapegoat node
     * to be perfectly balanced, and rewire it to its parent.
     * 
     * Time complexity: O(size of subtree to be rebuilt)
     * Space complexity: O(height of subtree to be rebuilt)
     */
    void rebuild(Scapegoat scapegoat);

    /** 
     * Rebuild subroutine: 
     * Converts the tree rooted at treeRoot into a linked list
     * joined by right child pointers, prepends this list to listHead,
     * and returns the head of the resulting list.
     * 
     * Time complexity: O(subtree size of treeRoot)
     * Space complexity: O(subtree height of treeRoot)
     * Equivalent to FLATTEN(x, y) in Galperin and Rivest.
     */ 
    Node* flatten(Node *treeRoot, Node *listHead);

    /**
     * Rebuild subroutine:
     * 
     * Given a size n for the tree to build and the head of a linked list 
     * joined by right child pointers, constructs a 1/2-weight-balanced tree of
     * n nodes from the list, in-place. 
     * 
     * Returns a pointer to the (n+1)th node in the list whose left child 
     * holds the root of the equivalent tree.
     * 
     * Assumes that listHead's size is at least n+1.
     * 
     * Time complexity: O(treeSize)
     * Space complexity: O(log treeSize)
     * Equivalent to BUILD-TREE(n, x) in Galperin and Rivest.
     */ 
    Node* buildTree(size_t treeSize, Node *listHead);

    /**
     * Verify helper function for a specific node.
     * Returns information verifying whether the subtree rooted at the given node 
     * is loosely alpha-height balanced and a proper BST - see VerificationData struct.
     */
    VerificationData verifyHelper(Node *node) const;

    /**
     * PrintDebugInfo helper.
     * Prints information about this root and its subtrees 
     * at the given level of indentation.
     */ 
    void printDebugInfoRec(Node* root, unsigned indent) const;
};
