

/****************************************************/
/***************** Node TLS support *****************/
/****************************************************/

void Node::computeNewValue_TLS(SpTh* th) {
  for (int i = 0; i < fromCount; i++) {

    // value -= coeffs[i] * fromNodes[i]->value;

    //double coeffs_val = th->specLD<MD,&md>( & (coeffs[i]) );
	double coeffs_val = coeffs[i];

    double this_val   = th->specLD<MD,&md>( & (this->value) );
    double frNode_val = th->specLD<MD,&md>( & (fromNodes[i]->value) );
	//double frNode_val = fromNodes[i]->value;
    

    this_val  -= coeffs_val * frNode_val;

    th->specST<MD, &md>( & (this->value), this_val);
  }
}

/****************************************************/
/************** Enumeration TLS support *************/
/****************************************************/

bool  Enumeration::hasMoreElements_TLS(SpTh* th) {
  return (current != NULL);
}

Node* Enumeration::nextElement_TLS    (SpTh* th) {
  Node* retval = current;
  //th->specST<MN,&mn>(&this->current, current->getNext());
  current = current->getNext();
  return retval;
}

/****************************************************/
/***************** BiGraph TLS support **************/
/****************************************************/

timeval start_time_par, end_time_par;
unsigned long running_time_par = 0;

void BiGraph::compute_TLS() {

	Enumeration iter = eNodes->elements();

    for(int i=0; i<PROCS_NR; i++) {
      SpTh* thr = allocateThread<SpTh>(i, 64);
      thr->init(&iter);
      ttmm.registerSpecThread(thr,i);
    }


    gettimeofday( &start_time_par, 0 );
    {
      ttmm.speculate<SpTh>();

      //ttmm.cleanup();
      //iter = hNodes->elements();      

      //ttmm.speculate<SpTh>();
    }
    gettimeofday( &end_time_par, 0 );

    cout<<"Total Number of rollbacks: "<<ttmm.nr_of_rollbacks<<endl;
    cout<<"Time COMPUTE TLS: "<<DiffTime(&end_time_par,&start_time_par)<<endl;

	for (Enumeration e = hNodes->elements(); e.hasMoreElements(); ) {
      Node* n = e.nextElement();
      n->computeNewValue();
    }
}

