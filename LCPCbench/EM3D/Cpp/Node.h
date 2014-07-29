#include <cstdlib> 
#include <ctime> 

//#define RANDOM_ALLOC

double nextDouble() {
  return ( static_cast<double>( rand() ) / static_cast<double>( RAND_MAX ) );
}

int    nextInt() {
  return rand();
}


static int structured_count = 0;
static int structured_base  = 0;

class Thread_EM3D_TLS;
class Enumeration;



/** 
 * This class implements nodes (both E- and H-nodes) of the EM graph. Sets
 * up random neighbors and propagates field values among neighbors.
 */
class Node 
{
public:
  /**
   * The value of the node.
   **/
  double value;

protected:
  /**
   * The next node in the list.
   **/
  Node* next;
  /**
   * Array of nodes to which we send our value.
   **/
  Node** toNodes;
  /**
   * Array of nodes from which we receive values.
   **/
  Node** fromNodes;
  /**
   * Coefficients on the fromNodes edges
   **/
  double* coeffs;
  /**
   * The number of fromNodes edges
   **/
  int fromCount;
  /**
   * Used to create the fromEdges - keeps track of the number of edges that have
   * been added
   **/
  int fromLength;

  const int toNodesLen;

public:

  Node* getNext() { return next; }

  /** 
   * Initialize the random number generator 
   **/
  static void initSeed(long seed)
  {
    srand(seed); 
  }

  /** 
   * Constructor for a node with given `degree'.   The value of the
   * node is initialized to a random value.
   **/
  Node(int deg) : toNodesLen(deg)
  {
    value = nextDouble();
    // create empty array for holding toNodes
    toNodes = new Node*[toNodesLen];

    next = NULL;
    fromNodes = NULL;
    coeffs = NULL;
    fromCount = 0;
    fromLength = 0;
  }

  /**
   * Create the linked list of E or H nodes.  We create a table which is used
   * later to create links among the nodes.
   * @param size the no. of nodes to create
   * @param degree the out degree of each node
   * @return a table containing all the nodes.
   **/
  static Node** fillTable(int size, int degree)
  {
    Node** table = new Node*[size];

    Node* prevNode = new Node(degree);
    table[0] = prevNode;
    for (int i = 1; i < size; i++) {
      Node* curNode = new Node(degree);
      table[i] = curNode;
      prevNode->next = curNode;
      prevNode = curNode;
    }
    return table;
  }

  /** 
   * Create unique `degree' neighbors from the nodes given in nodeTable.
   * We do this by selecting a random node from the give nodeTable to
   * be neighbor. If this neighbor has been previously selected, then
   * a different random neighbor is chosen.
   * @param nodeTable the list of nodes to choose from.
   **/
  void makeUniqueNeighbors(Node** nodeTable, int nodeTableLen)
  {
    for (int filled = 0; filled < toNodesLen; filled++) {
      int k;
      Node* otherNode;
#if defined( RANDOM_ALLOC )

      do {
        // generate a random number in the correct range
        int index = nextInt();
        if (index < 0) index = -index;
        index = index % nodeTableLen;

        // find a node with the random index in the given table
        otherNode = nodeTable[index];

        for (k = 0; k < filled; k++) {
          if (otherNode == toNodes[k]) break;
        }
      } while (k < filled);

#else
      
      int index = structured_base + filled;
      index     = index % nodeTableLen; 
      otherNode = nodeTable[index];

      if( filled == (toNodesLen-1) ) {
        int rrr = nextInt() & 255;
        if(rrr == 0) {
            int ind = nextInt();
            if (ind < 0) ind = -ind;
            ind = ind % nodeTableLen;
            bool was = false;
            for (int kk = 0; kk < filled; kk++) {
              if ( otherNode == toNodes[kk] ) was = true;
            }
            if(!was) otherNode = nodeTable[ind];
          
        }
      }

#endif

      // other node is definitely unique among "filled" toNodes
      toNodes[filled] = otherNode;

      // update fromCount for the other node
      otherNode->fromCount++;
    }
    structured_count++;
    if(structured_count == toNodesLen) { 
      structured_count = 0; 
      structured_base += toNodesLen; 
      structured_base = structured_base % nodeTableLen; 
    }
    //if(structured_count == nodeTableLen) structured_count = 0;
  }

  

  /** 
   * Allocate the right number of FromNodes for this node. This
   * step can only happen once we know the right number of from nodes
   * to allocate. Can be done after unique neighbors are created and known.
   *
   * It also initializes random coefficients on the edges.
   **/
  void makeFromNodes()
  {
    fromNodes = new Node*[fromCount]; // nodes fill be filled in later
    coeffs = new double[fromCount];
  }

  /**
   * Fill in the fromNode field in "other" nodes which are pointed to
   * by this node.
   **/
  void updateFromNodes()
  {
    for (int i = 0; i < toNodesLen; i++) {
      Node* otherNode = toNodes[i];
      int count = otherNode->fromLength++;
      otherNode->fromNodes[count] = this;
      otherNode->coeffs[count] = nextDouble();
    }
  }

  /** 
   * Get the new value of the current node based on its neighboring
   * from_nodes and coefficients.
   **/
  void computeNewValue()
  {
    for (int i = 0; i < fromCount; i++) {
      value -= coeffs[i] * fromNodes[i]->value;
    }
  }

  Enumeration elements();

  /**
   * Override the toString method to return the value of the node.
   * @return the value of the node.
   **/
  void toPrint()
  {
    cout<<"value "<<value<<", from_count "<<fromCount;
  }

  void computeNewValue_TLS(Thread_EM3D_TLS* th);
};



  /**
   * Return an enumeration of the nodes.
   * @return an enumeration of the nodes.
   **/
class Enumeration {
private:
  Node* current;

public:
  Enumeration(Node* c) { this->current = c; }

  bool hasMoreElements() { return (current != NULL); }

  Node* nextElement() {
	Node* retval = current;
	current = current->getNext();
	return retval;
  }

  bool  hasMoreElements_TLS(Thread_EM3D_TLS* th);
  Node* nextElement_TLS    (Thread_EM3D_TLS* th);
};

Enumeration Node::elements() {
    return Enumeration(this);
}

