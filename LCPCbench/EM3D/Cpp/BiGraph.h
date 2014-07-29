#ifndef ATOMIC_OPS
#define ATOMIC_OPS
#include "Util/AtomicOps.h"
#endif 

#include "Node.h"
#include "Graph.h"


timeval start_time_seq, end_time_seq;
unsigned long running_time_seq = 0;

/** 
 * A class that represents the irregular bipartite graph used in
 * EM3D.  The graph contains two linked structures that represent the
 * E nodes and the N nodes in the application.
 **/
class BiGraph
{
public:
  /**
   * Nodes that represent the electrical field.
   **/
  Node* eNodes;
  /**
   * Nodes that representhe the magnetic field.
   **/
  Node* hNodes;

  /**
   * Construct the bipartite graph.
   * @param e the nodes representing the electric fields
   * @param h the nodes representing the magnetic fields
   **/ 
  BiGraph(Node* e, Node* h)
  {
    eNodes = e;
    hNodes = h;
  }

  /**
   * Create the bi graph that contains the linked list of
   * e and h nodes.
   * @param numNodes the number of nodes to create
   * @param numDegree the out-degree of each node
   * @param verbose should we print out runtime messages
   * @return the bi graph that we've created.
   **/
  static BiGraph* create(int numNodes, int numDegree, bool verbose)
  {
    Node::initSeed(783);

    // making nodes (we create a table)
    if (verbose) cout<<"making nodes (tables in orig. version)"<<endl;
    Node** hTable = Node::fillTable(numNodes, numDegree);
    Node** eTable = Node::fillTable(numNodes, numDegree);

    // making neighbors
    if (verbose) cout<<"updating from and coeffs"<<endl;
    for (Enumeration e = hTable[0]->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      n->makeUniqueNeighbors(eTable, numNodes);
    }
    for (Enumeration e = eTable[0]->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      n->makeUniqueNeighbors(hTable, numNodes);
    }

    // Create the fromNodes and coeff field
    if (verbose) cout<<"filling from fields"<<endl;
    for (Enumeration e = hTable[0]->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      n->makeFromNodes();
    }
    for (Enumeration e = eTable[0]->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      n->makeFromNodes();
    }

    // Update the fromNodes
    for (Enumeration e = hTable[0]->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      n->updateFromNodes();
    }
    for (Enumeration e = eTable[0]->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      n->updateFromNodes();
    }

    BiGraph* g = new BiGraph(eTable[0], hTable[0]);
    return g;
  }

  /** 
   * Update the field values of e-nodes based on the values of
   * neighboring h-nodes and vice-versa.
   **/
  void compute()
  {
    gettimeofday( &start_time_seq, 0 );


    for (Enumeration e = eNodes->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      n->computeNewValue();
    }

    gettimeofday( &end_time_seq, 0 );

    cout<<"Time COMPUTE SEQ: "<<DiffTime(&end_time_seq, &start_time_seq)<<endl;

    for (Enumeration e = hNodes->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      n->computeNewValue();
    }
  }

  void compute_TLS();

  /**
   * Override the toString method to print out the values of the e and h nodes.
   * @return a string contain the values of the e and h nodes.
   **/
  void toPrint()
  {
    for (Enumeration e = eNodes->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      cout<<"E: "; n->toPrint(); cout<<" ";
    }
    cout<<endl;

    for (Enumeration e = hNodes->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      cout<<"H: "; n->toPrint(); cout<<" ";
    }
    cout<<endl;
  }

};





