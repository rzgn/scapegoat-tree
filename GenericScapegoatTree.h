/**
 * GenericScapegoatTree.h is a template Scapegoat Tree that stores
 * objects of generic types along with an optional "less-than" function 
 * which must provide a strict ordering of the contained type.
 * 
 * Authors: Ryan Guan (rzgn) and Tyler Packard (TPackard), 2021
 * 
 * Stanford CS166 project partners: Nali Welinder and Parker Jou
 */ 

#include <cstddef>    // for std::size_t
#include <stack>      // for scapegoat-node-finding stack
#include <cmath>      // for floor, log
#include <stdexcept>  // for std::invalid_argument
#include <iostream>   // for std::cout, flush
#include <iomanip>    // for std::setw

/**
 * Default comparison function: uses < to compare two objects of generic type.
 * As a result, either a comparison function must be specified when creating
 * the tree, or the contained type should overload operator <.
 */ 
template<typename T>
bool defaultIsLessThan(const T& lhs, const T& rhs) {
  return lhs < rhs;
}

template<typename T>
class ScapegoatTree {
  public:
    /**
     * Constructs a new, empty scapegoat tree with the provided alpha value
     * and provided comparison function.
     * 
     * Throws std::invalid_argument if alpha is not within (0.5, 1).
     */
    ScapegoatTree(double alpha, bool isLessThan(const T&, const T&)) {
      checkInvalidAlpha(alpha);
      this->alpha = alpha;
      this->isLessThan = isLessThan;
    }

    /**
     * Constructs a new, empty scapegoat tree with the provided alpha value
     * and the < operator as the comparison function.
     * 
     * Throws std::invalid_argument if alpha is not within (0.5, 1).
     */
    ScapegoatTree(double alpha) {
      checkInvalidAlpha(alpha);
      this->alpha = alpha;
      this->isLessThan = &defaultIsLessThan;
    }

    /**
     * Frees all memory allocated by this scapegoat tree.
     * 
     * Time complexity: O(N)
     */
    ~ScapegoatTree() {
      // Algorithm to deallocate iteratively in O(1) space thanks to Leo Shamis.
      while (root) {
        if (! root->left) {
          // Case 1: The root has no left child; delete and replace it with right child.
          Node* next = root->right;
          delete root;
          root = next;
        } else {
          // Case 2: Rotate the root's left child into the root's place.
          Node* leftChild = root->left;
          root->left = leftChild->right;
          leftChild->right = root;
          root = leftChild;
        }
      }
    }

    /**
     * Returns whether the given key is present in the tree.
     * 
     * Time complexity: O(log N)
     */
    bool search(T key) const {
      Node* curr = root;
      while (curr) {
        if      (isLessThan(key, curr->key)) curr = curr->left;
        else if (isLessThan(curr->key, key)) curr = curr->right;
        else /* equal, by strict ordering */ return true;
      }
      return false;
    }

    /**
     * Inserts the given key into the scapegoat tree. If the element was added,
     * this function returns true. If the element already existed, this
     * function returns false and does not modify the tree.
     * 
     * Time complexity: amortized O(log N), worst-case O(N)
     * Space complexity: O(log N)
     */
    bool insert(T key) {
      std::stack<Node *> insertionPath; // Stack of ancestors of the inserted nodes
      insertionPath.push(nullptr);      // The "root's parent"

      // Find the insertion point and its parent.
      Node* prev = nullptr;
      Node* curr = root;
      while (curr) {
        insertionPath.push(curr);
        prev = curr;

        if      (isLessThan(key, curr->key)) curr = curr->left;
        else if (isLessThan(curr->key, key)) curr = curr->right;
        else /* equal, by strict ordering */ return false; // key already present
      }

      // Make the node to insert.
      Node* node = new Node;
      node->key  = key;
      node->left = node->right = nullptr;

      // Wire the new node into the tree. 
      if (! prev) {
        root = node;
      } else if (isLessThan(key, prev->key)) {
        prev->left = node;
      } else /*  key > prev->key */ {
        prev->right = node;
      }

      // Update tree information.
      size++;
      if (size > maxSize) maxSize = size;

      // If there exist ancestors and the inserted node is deep, find scapegoat and rebuild.
      size_t deepHeight = getAlphaDeepHeight(size);
      
      // Insertion height equals the number of ancestors of the inserted node.
      // We subtract 1 from the stack's size since we pushed a dummy node 
      // (nullptr) onto the insertion path and heights start from 0. 
      size_t insertionHeight = insertionPath.size() - 1;
      if (insertionHeight >= 1 && insertionHeight > deepHeight) {
        Scapegoat scapegoat = findScapegoat(insertionPath);
        rebuild(scapegoat);
      }

      return true;
    }

    /**
     * Remove a key from the tree. If the element was removed, this function 
     * returns true. If the element did not exist in the tree, this function
     * returns false and doesn't modify the tree.
     * 
     * Time complexity: amortized O(log N), worst-case O(N)
     */
    bool remove(T key) {
      // Find node containing the key to remove and its parent.
      Node* prev = nullptr;
      Node* curr = root;
      while (true) {
        if (! curr) return false;   // Deletion failed if the key doesn't exist
        
        bool keyLess = isLessThan(key, curr->key);
        bool keyGreater = isLessThan(curr->key, key);

        if (! keyLess && ! keyGreater) break; // Found the node to delete

        prev = curr;

        if      (keyLess)    curr = curr->left;
        else if (keyGreater) curr = curr->right;
      }

      // Remove the node from the tree and clean up its memory.
      if (curr->left && curr->right) {
        removeNodeWithTwoChildren(curr);
      } else {
        removeNodeWithoutChild(curr, prev);
      }

      // Finally, rebuild the entire tree if necessary. 
      size--;
      if (size <= alpha * maxSize) {
        rebuild( { root, nullptr, size } );
      }

      return true;
    }

    /**
     * Returns whether the scapegoat tree is loosely alpha-height balanced.
     */
    bool verify() const {
      VerificationData treeProperties = verifyHelper(root);
      return size >= alpha * maxSize && treeProperties.balanced && treeProperties.isBST;
    }

    /**
     * Prints a pre-order traversal of the tree in a nice format for debugging.
     */
    void printDebugInfo() const {
      printDebugInfoRec(root, 0);
      std::cout << std::flush;
    }

  private:
    // Helper structs:

    /** 
     * Represents a standard BST node, holding its key and children. 
     */
    struct Node {
      T      key;

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

    // Pointer to comparator function for keys, the < operator if not provided.
    bool (*isLessThan)(const T&, const T&); 

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
     * Throws std::invalid_argument if alpha is not within (0.5, 1).
     */ 
    void checkInvalidAlpha(double alpha) const {
      if (alpha <= kMinAlpha || alpha >= kMaxAlpha) {
        throw std::invalid_argument("Alpha not in range (0.5, 1)!");
      }
    }

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
    size_t getSubtreeSize(Node *node) {
      if (! node)  return 0;
      return 1 + getSubtreeSize(node->left) + getSubtreeSize(node->right);
    }

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
    Scapegoat findScapegoat(std::stack<Node *>& insertionPath) {
      // Retrieve parent (curr) and grandparent (parent) of inserted node.
      Node *curr = insertionPath.top();
      insertionPath.pop();
      Node *parent = insertionPath.top();
      insertionPath.pop();

      // Track current subtree size and index 
      // (i where curr is the i-th ancestor of inserted node).
      size_t currSize = getSubtreeSize(curr);
      size_t currIndex = 1;

      while (parent) {
        // Found scapegoat if we exhausted the stack, 
        // or if index is greater than alpha-deep height.
        if (currIndex > getAlphaDeepHeight(currSize)) {
          break;
        }

        // Update the size without revisiting any nodes already visited.
        if (parent->left == curr) {
          currSize += 1 + getSubtreeSize(parent->right);
        } else {
          currSize += 1 + getSubtreeSize(parent->left);
        }

        // Get the next ancestor from the stack.
        curr = parent;
        parent = insertionPath.top();
        insertionPath.pop();

        currIndex++;
      }

      return { curr, parent, currSize };
    }

    /** 
     * Remove subroutine: 
     * Removes the given node from the tree by swapping its key with 
     * its in-order successor / predecessor node's key.
     * 
     * Parameters: node has two children.
     * Postcondition: the memory of the node wired out of the tree is freed.
     */
    void removeNodeWithTwoChildren(Node *node) {
      Node *curr;
      Node *prev = node;
      
      if (replaceWithSucc) {
        // Find the in-order successor of node:
        // the node with minimum key in its right subtree.
        curr = node->right;

        if (! curr->left) {
          // node's right child has no left child, so it is the successor.
          node->right = curr->right;
        }
        else {
          // Otherwise, descend the left spine of node's right subtree.
          while (curr->left) {
            prev = curr;
            curr = curr->left;
          }

          // Splice the successor out of the tree (it has at most a right child.)
          prev->left = curr->right;
        }
      } else {
        // Find the in-order predecessor of node:
        // the node with maximum key in its left subtree.
        curr = node->left;

        if (! curr->right) {
          // node's left child has no right child, so it is the predecessor.
          node->left = curr->left;
        }
        else {
          // Otherwise, descend the right spine of node's left subtree.
          while (curr->right) {
            prev = curr;
            curr = curr->right;
          }

          // Splice the predecessor out of the tree (it has at most a left child.)
          prev->right = curr->left;
        }
      }

      // Swapping the successor/predecessor's key with the key to be removed
      // maintains the BST ordering. 
      node->key = curr->key;
      delete curr;

      replaceWithSucc = !replaceWithSucc;
    }

    /** 
     * Remove subroutine: 
     * Removes the given node from the tree by replacing it with 
     * its only child, or nullptr if it is a leaf.
     * 
     * Parameters: node has at most one child, parent is its parent or 
     *    nullptr if node is the root of the tree.
     * Postcondition: node has been wired out of the tree, and its memory is freed.
     */
    void removeNodeWithoutChild(Node *node, Node *parent) {
      Node *child = node->left ? node->left : node->right;

      if (root == node) { // node is the root: replace the root
        root = child;
      } else if (parent->left == node) { // node is a left child
        parent->left = child;
      } else /* if (parent->right == node) */ { // node is a right child
        parent->right = child;
      }

      // Free memory of the original node.
      delete node;
    }

    /** 
     * Rebuild the subtree rooted at the given scapegoat node
     * to be perfectly balanced, and rewire it to its parent.
     * 
     * Time complexity: O(size of subtree to be rebuilt)
     * Space complexity: O(height of subtree to be rebuilt)
     */
    void rebuild(Scapegoat scapegoat) {
      // Dummy node will become the end of our linked list.
      Node dummy;

      // Flatten the tree into a linked list.
      Node *nodeList = flatten(scapegoat.scapegoat, &dummy);

      // Rebuild the linked list into a tree.
      buildTree(scapegoat.treeSize, nodeList);

      // Now, dummy's left child is the root of the new rebuilt subtree,
      // so we may wire the rebuilt subtree back into the tree.
      if (! scapegoat.parent) {
        root = dummy.left;
        maxSize = size;
      } else if (scapegoat.scapegoat == scapegoat.parent->left) {
        scapegoat.parent->left = dummy.left;
      } else /* if (scapegoat.scapegoat == scapegoat.parent->right) */ {
        scapegoat.parent->right = dummy.left;
      }
    }

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
    Node* flatten(Node *treeRoot, Node *listHead) {
      // If there's nothing to prepend, the list stays the same.
      if (! treeRoot) return listHead;

      // Otherwise, prepend the right subtree, then the root, then its left subtree.
      treeRoot->right = flatten(treeRoot->right, listHead);
      return flatten(treeRoot->left, treeRoot);
    }

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
    Node* buildTree(size_t treeSize, Node *listHead) {
      // No tree to build: resulting tree is nullptr, return rewired 1st element of list.
      if (treeSize == 0) {
        listHead->left = nullptr;
        return listHead;
      }
      // Otherwise, recursively build the tree:

      double halfTreeSize = (treeSize - 1.0) / 2.0;

      // Node whose left child is a balanced tree equivalent of the first half of the list
      // and right child is the unaltered second half of the list.
      Node *firstHalf = buildTree(static_cast<size_t>(ceil(halfTreeSize)), listHead);

      // (n+1)th node in the original list, whose left child is the balanced tree 
      // equivalent of the second half of the list.
      Node *secondHalf = buildTree(static_cast<size_t>(floor(halfTreeSize)), firstHalf->right);

      // Rewire so that firstHalf is now the tree formed of n elements from the list.
      firstHalf->right = secondHalf->left;
      
      // Return a node whose left child is the equivalent tree, as specified.
      secondHalf->left = firstHalf;
      return secondHalf;
    }

    /**
     * Verify helper function for a specific node.
     * Returns information verifying whether the subtree rooted at the given node 
     * is loosely alpha-height balanced and a proper BST - see VerificationData struct.
     */
    VerificationData verifyHelper(Node *node) const {
      // Base case: if tree is empty, size is 0 and tree is balanced.
      if (! node) {
        return { true, true, 0, -1 };
      }

      // Check if left child's key < current node's key < right child's key. 
      VerificationData left = verifyHelper(node->left);
      VerificationData right = verifyHelper(node->right);

      bool isBST = left.isBST && right.isBST;
      if (node->left)  isBST = isBST && isLessThan(node->left->key, node->key); 
      if (node->right) isBST = isBST && isLessThan(node->key, node->right->key);

      // Check if current node is loosely alpha-height balanced.
      size_t size = left.size + right.size + 1;
      int height = std::max(left.height, right.height) + 1;
      int max_height = getAlphaDeepHeight(size) + 1;

      return { height <= max_height, isBST, size, height };
    }

    /**
     * PrintDebugInfo helper.
     * Prints information about this root and its subtrees 
     * at the given level of indentation.
     */ 
    void printDebugInfoRec(Node* root, unsigned indent) const {
      if (! root) {
        std::cout << std::setw(indent) << "" << "null" << '\n';
      } else {
        std::cout << std::setw(indent) << "" << "Node       " << root << '\n';
        std::cout << std::setw(indent) << "" << "Key:       " << root->key << '\n';
        std::cout << std::setw(indent) << "" << "Left Child:" << '\n';
        printDebugInfoRec(root->left,  indent + 4);
        std::cout << std::setw(indent) << "" << "Right Child:" << '\n';
        printDebugInfoRec(root->right, indent + 4);
      }
    }
};
