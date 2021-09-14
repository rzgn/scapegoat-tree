/**
 * Authors: Ryan Guan (rzgn) and Tyler Packard (TPackard), 2021
 * 
 * Destructor, tree search code, and printing functions adapted from
 * Red-Black Tree starter files from Stanford's CS166 course in Spring 2021.
 */ 

#include "ScapegoatTree.h"

#include <stdexcept>  // for std::invalid_argument
#include <iostream>   // for std::cout, flush
#include <iomanip>    // for std::setw


ScapegoatTree::ScapegoatTree(double alpha) {
  // Tree's alpha value must be larger than 0.5 and smaller than 1.
  if (alpha <= kMinAlpha || alpha >= kMaxAlpha) {
    throw std::invalid_argument("Alpha not in range (0.5, 1)!");
  }
  this->alpha = alpha;
}

ScapegoatTree::~ScapegoatTree() {
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

bool ScapegoatTree::search(int key) const {
  Node* curr = root;
  while (curr) {
    if      (key == curr->key)   return true;
    else if (key <  curr->key)   curr = curr->left;
    else /*  key >  curr->key */ curr = curr->right;
  }
  return false;
}

bool ScapegoatTree::insert(int key) {
  std::stack<Node *> insertionPath; // Stack of ancestors of the inserted nodes
  insertionPath.push(nullptr);      // The "root's parent"

  // Find the insertion point and its parent.
  Node* prev = nullptr;
  Node* curr = root;
  while (curr) {
    insertionPath.push(curr);
    prev = curr;

    if      (key == curr->key)   return false;       // Key already present
    else if (key <  curr->key)   curr = curr->left;
    else /*  key >  curr->key */ curr = curr->right;
  }

  // Make the node to insert.
  Node* node = new Node;
  node->key  = key;
  node->left = node->right = nullptr;

  // Wire the new node into the tree. 
  if (! prev) {
    root = node;
  } else if (key < prev->key) {
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

bool ScapegoatTree::remove(int key) {
  // Find node containing the key to remove and its parent.
  Node* prev = nullptr;
  Node* curr = root;
  while (true) {
    if (! curr)  return false;   // Deletion failed if the key doesn't exist
    
    if (key == curr->key) break; // Found the node to delete

    prev = curr;
    if      (key < curr->key) curr = curr->left;
    else if (key > curr->key) curr = curr->right;
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

void ScapegoatTree::removeNodeWithTwoChildren(Node *node) {
  // Find the in-order successor/predecessor to node, splice it out of the tree
  // and clean up its memory, and swap its key with node's key, essentially 
  // deleting node's original key from the tree. 

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

void ScapegoatTree::removeNodeWithoutChild(Node *node, Node *parent) {
  // Wire the node being deleted out of the tree:
  // Point the appropriate child pointer of the parent 
  // to a subtree of the node being deleted.

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

/* Recursively generates the size of the subtree rooted at node. */
size_t ScapegoatTree::getSubtreeSize(Node *node) {
  if (! node) {
    return 0;
  } 
  return 1 + getSubtreeSize(node->left) + getSubtreeSize(node->right);
}

ScapegoatTree::Scapegoat ScapegoatTree::findScapegoat(std::stack<Node *>& insertionPath) {
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

ScapegoatTree::Node *ScapegoatTree::flatten(Node *treeRoot, Node *listHead) {
  // If there's nothing to prepend, the list stays the same.
  if (! treeRoot) return listHead;

  // Otherwise, prepend the right subtree, then the root, then its left subtree.
  treeRoot->right = flatten(treeRoot->right, listHead);
  return flatten(treeRoot->left, treeRoot);
}

ScapegoatTree::Node *ScapegoatTree::buildTree(size_t treeSize, Node *listHead) {
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

void ScapegoatTree::rebuild(Scapegoat scapegoat) {
  // Dummy node will become the end of our linked list.
  Node dummy = { -1, nullptr, nullptr };

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

bool ScapegoatTree::verify() const {
  VerificationData treeProperties = verifyHelper(root);
  return size >= alpha * maxSize && treeProperties.balanced && treeProperties.isBST;
}

ScapegoatTree::VerificationData ScapegoatTree::verifyHelper(Node *node) const {
  // Base case: if tree is empty, size is 0 and tree is balanced.
  if (! node) {
    return { true, true, 0, -1 };
  }

  // Check if left child's key < current node's key < right child's key. 
  VerificationData left = verifyHelper(node->left);
  VerificationData right = verifyHelper(node->right);

  bool isBST = left.isBST && right.isBST;
  if (node->left) isBST = isBST && node->left->key < node->key; 
  if (node->right) isBST = isBST && node->right->key > node->key;

  // Check if current node is loosely alpha-height balanced.
  size_t size = left.size + right.size + 1;
  int height = std::max(left.height, right.height) + 1;
  int max_height = getAlphaDeepHeight(size) + 1;

  return { height <= max_height, isBST, size, height };
}

void ScapegoatTree::printDebugInfo() const {
  printDebugInfoRec(root, 0);
  std::cout << std::flush;
}

void ScapegoatTree::printDebugInfoRec(Node* root, unsigned indent) const {
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