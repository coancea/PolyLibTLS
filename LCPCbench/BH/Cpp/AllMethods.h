
unsigned int DiffTimeClone(timeval* time2, timeval* time1) {
	unsigned int diff = 
       (time2->tv_sec-time1->tv_sec)*1000000 + (time2->tv_usec-time1->tv_usec);
	return diff;
}

double myRand(double seed)
{
    double t = 16807.0*seed + 1;
    
    seed = t - (2147483647.0 * floor(t / 2147483647.0));
    return seed;
}

  /**
   * Generate a floating point random number.  Used by
   * the original BH benchmark.
   *
   * @param xl lower bound
   * @param xh upper bound
   * @param r seed
   * @return a floating point randon number
   **/
double xRand(double xl, double xh, double r)
{
    double res = xl + (xh-xl)*r/2147483647.0;
    return res;
}

/****************************************/
/********* Node Methods *****************/
/****************************************/

int Node::oldSubindex(MathVector ic, int l)
{
    int i = 0;
    for (int k = 0; k < MathVector::NDIM; k++) {
      if (((int)ic.value(k) & l) != 0)
        i += Cell::NSUB >> (k + 1);
    }
    return i;
}



HG Node::gravSub(HG hg)     // SHOULD CHANGE THE SIGNATURE TO WORK WITH POINTERS???
{
    MathVector dr;
    dr.subtraction(pos, hg.pos0);

    double drsq = dr.dotProduct() + (EPS * EPS);
    double drabs = sqrt(drsq);

    double phii = mass / drabs;
    hg.phi0 -= phii;
    double mor3 = phii / drsq;
    dr.multScalar(mor3);
    hg.acc0.addition(dr);
    return hg;
}


/****************************************/
/********* Cell Methods *****************/
/****************************************/



Node* Cell::loadTree(Body* p, MathVector xpic, int l, Tree* tree)
{
    // move down one level
    int   si = oldSubindex(xpic, l);
    Node* rt = subp[si];
    if (rt != NULL) 
      subp[si] = rt->loadTree(p, xpic, l >> 1, tree);
    else 
      subp[si] = p;
    return this;
}



double Cell::hackcofm()
{
    double mq = 0.0;
    MathVector tmpPos; // = new MathVector();
    MathVector tmpv;   //   = new MathVector();
    for (int i=0; i < NSUB; i++) {
      Node* r = subp[i];
      if (r != NULL) {
        double mr = r->hackcofm();
        mq = mr + mq;
        tmpv.multScalar(r->pos, mr);
        tmpPos.addition(tmpv);
      }
    }
    mass = mq;
    pos = tmpPos;
    pos.divScalar(mass);

    return mq;
}


HG Cell::walkSubTree(double dsq, HG hg)
{
    if (subdivp(dsq, hg)) {
      for (int k = 0; k < Cell::NSUB; k++) {
        Node* r = subp[k];
        if (r != NULL)
          hg = r->walkSubTree(dsq / 4.0, hg);
      }
    } else {
      hg = gravSub(hg);   
    }
    return hg;
}


bool Cell::subdivp(double dsq, HG hg)
{
    MathVector dr; //= new MathVector();
    dr.subtraction(pos, hg.pos0);
    double drsq = dr.dotProduct();

    // in the original olden version drsp is multiplied by 1.0
    return (drsq < dsq);
}

void Cell::toPrint()
{
    cout<<"Cell ";
    this->Node::toPrint();
}


/****************************************/
/********* Body Methods *****************/
/****************************************/

bool Enumerate::hasMoreElements() const { return (current != NULL); }

Body* Enumerate::nextElement() {
    Body* retval = current;

    if(this->forw)   current = current->next;
    else             current = current->procNext;

    return retval;
}


void Body::expandBox(Tree* tree, int nsteps)
{
    MathVector rmid; // = new MathVector();

    bool inbox = icTest(tree);
    while (!inbox) {
      double rsize = tree->rsize;
      rmid.addScalar(tree->rmin, 0.5 * rsize);
      
      for (int k = 0; k < MathVector::NDIM; k++) {
        if (pos.value(k) < rmid.value(k)) {
          double rmin = tree->rmin.value(k);
          tree->rmin.value(k, rmin - rsize);
        }
      }
      tree->rsize = 2.0 * rsize;
      if (tree->root != NULL) {
        MathVector ic = tree->intcoord(rmid);
        int k = oldSubindex(ic, IMAX >> 1);
        Cell* newt = new Cell();
        newt->subp[k] = tree->root;
        tree->root = newt;
        inbox = icTest(tree);
      }
    }
}


bool Body::icTest(Tree* tree)
{
    double pos0 = pos.value(0);
    double pos1 = pos.value(1);
    double pos2 = pos.value(2);
    
    // by default, it is in bounds
    bool result = true;

    double xsc = (pos0 - tree->rmin.value(0)) / tree->rsize;
    if (!(0.0 < xsc && xsc < 1.0)) {
      result = false;
    }

    xsc = (pos1 - tree->rmin.value(1)) / tree->rsize;
    if (!(0.0 < xsc && xsc < 1.0)) {
      result = false;
    }

    xsc = (pos2 - tree->rmin.value(2)) / tree->rsize;
    if (!(0.0 < xsc && xsc < 1.0)) {
      result = false;
    }

    return result;
}

double Body::hackcofm()
{
    return mass;
}


Node* Body::loadTree(Body* p, MathVector xpic, int l, Tree* tree)
{
    // create a Cell
    Cell* retval = new Cell();
    int si = subindex(tree, l);
    // attach this Body node to the cell
    retval->subp[si] = this;

    // move down one level
    si = oldSubindex(xpic, l);
    Node* rt = retval->subp[si];
    if (rt != NULL) 
      retval->subp[si] = rt->loadTree(p, xpic, l >> 1, tree);
    else 
      retval->subp[si] = p;
    return retval;
}



int Body::subindex(Tree* tree, int l)
{
    MathVector xp; //= new MathVector();

    double xsc = (pos.value(0) - tree->rmin.value(0)) / tree->rsize;
    xp.value(0, floor(IMAX * xsc));

    xsc = (pos.value(1) - tree->rmin.value(1)) / tree->rsize;
    xp.value(1, floor(IMAX * xsc));

    xsc = (pos.value(2) - tree->rmin.value(2)) / tree->rsize;
    xp.value(2, floor(IMAX * xsc));

    int i = 0;
    for (int k = 0; k < MathVector::NDIM; k++) {
      if (((int)xp.value(k) & l) != 0) {
        i += Cell::NSUB >> (k + 1);
      }
    }
    return i;
}


void Body::hackGravity(double rsize, Node* root)
{
    MathVector pos0 = pos;

    HG hg(this, pos);                    
    hg = root->walkSubTree(rsize * rsize, hg);
    phi = hg.phi0;
    newAcc = hg.acc0;
}

HG Body::walkSubTree(double dsq, HG hg)
{
    if (this != hg.pskip)
      hg = gravSub(hg);                          
    return hg;
}




/****************************************/
/********* Tree Methods *****************/
/****************************************/



void Tree::createTestData(int nbody)
{
    MathVector cmr; // = new MathVector();
    MathVector cmv; // = new MathVector();

    Body* head = new Body();
    Body* prev = head;

    double rsc  = 3.0 * PI / 16.0;
    double vsc  = sqrt(1.0 / rsc);
    double seed = 123.0;

    for (int i = 0; i < nbody; i++) {
      Body* p = new Body();

      prev->setNext(p);
      prev = p;
      p->mass = 1.0/(double)nbody;
      
      seed      = myRand(seed);
      double t1 = xRand(0.0, 0.999, seed);
      t1        = pow(t1, (-2.0/3.0)) - 1.0;
      double r  = 1.0 / sqrt(t1);

      double coeff = 4.0;
      for (int k = 0; k < MathVector::NDIM; k++) {
        seed = myRand(seed);
        r = xRand(0.0, 0.999, seed);
        p->pos.value(k, coeff*r);
      }
      
      cmr.addition(p->pos);

      double x, y;
      do {
        seed = myRand(seed);
        x    = xRand(0.0, 1.0, seed);
        seed = myRand(seed);
        y    = xRand(0.0, 0.1, seed);
      } while (y > x*x * pow(1.0 - x*x, 3.5));

      double v = sqrt(2.0) * x / pow(1 + r*r, 0.25);

      double rad = vsc * v;
      double rsq;
      do {
        for (int k = 0; k < MathVector::NDIM; k++) {
          seed     = myRand(seed);
          p->vel.value(k, xRand(-1.0, 1.0, seed));
        }
        rsq = p->vel.dotProduct();
      } while (rsq > 1.0);
      double rsc1 = rad / sqrt(rsq);
      p->vel.multScalar(rsc1);
      cmv.addition(p->vel);
    }

    // mark end of list
    prev->setNext(NULL);
    // toss the dummy node at the beginning and set a reference to the first element
    bodyTab = head->getNext();

    cmr.divScalar((double)nbody);
    cmv.divScalar((double)nbody);

    prev = NULL;
    
    for (Enumerate e = bodyTab->elements(); e.hasMoreElements(); ) {
      Body* b = e.nextElement();
      b->pos.subtraction(cmr);
      b->vel.subtraction(cmv);
      b->setProcNext(prev);
      prev = b;
    }
    
    // set the reference to the last element
    bodyTabRev = prev;
}



void Tree::makeTree(int nstep)
{
    for (Enumerate e = bodiesRev(); e.hasMoreElements(); ) {
      Body* q = e.nextElement();
      if (q->mass != 0.0) {
        q->expandBox(this, nstep);
        MathVector xqic = intcoord(q->pos);
        if (root == NULL) {
          root = q;
        } else {
          root = root->loadTree(q, xqic, Node::IMAX >> 1, this);
        }
      }
    }
    root->hackcofm();
}

/*** SEQUENTIAL ***/

const double dthf = 0.5 * DTIME;

timeval start_time_seq, end_time_seq;
unsigned long running_time_seq = 0;


void Tree::vp(Body* p, int nstep)
{
    MathVector dacc; // = new MathVector();
    MathVector dvel; // = new MathVector();
    
    int num_it = 0;

    gettimeofday( &start_time_seq, 0 );

    for (Enumerate e = p->elementsRev(); e.hasMoreElements(); ) {
      Body* b = e.nextElement();
      MathVector acc1 = b->newAcc;
      if (nstep > 0) {
        dacc.subtraction(acc1, b->acc);
        dvel.multScalar(dacc, dthf);
        dvel.addition(b->vel);
        b->vel = dvel;
      }
      b->acc = acc1;
      dvel.multScalar(b->acc, dthf);

      MathVector vel1 = b->vel;
      vel1.addition(dvel);
      MathVector dpos = vel1;
      dpos.multScalar(DTIME);
      dpos.addition(b->pos);
      b->pos = dpos;
      vel1.addition(dvel);
      b->vel = vel1;

      num_it++;
    }

    gettimeofday( &end_time_seq, 0 );

    cout<<"Time seq vp: "<<DiffTimeClone(&end_time_seq,&start_time_seq)<<endl;
    cout<<"Number Iterations second enum: "<<num_it<<endl;
}


void Tree::stepSystem(int nstep)
{
    // free the tree
    root = NULL;

    makeTree(nstep);

    int num_it = 0;
    
    gettimeofday( &start_time_seq, 0 );

    // compute the gravity for all the particles
    for (Enumerate e = bodyTabRev->elementsRev(); e.hasMoreElements(); ) {
      Body* b = e.nextElement();
      b->hackGravity(rsize, root);
      num_it++;
    }

    gettimeofday( &end_time_seq, 0 );
    cout<<"Time HackGravity SEQ: "<<DiffTimeClone(&end_time_seq,&start_time_seq)<<endl;

   
    cout<<"Number Iterations first  enum: "<<num_it<<endl;
   
    vp(bodyTabRev, nstep);

}

/**** PARALLEL *****/

timeval start_time_par, end_time_par;
unsigned long running_time_par = 0;

void Tree::vpPAR(Body* p, int nstep)
{
    Tree::vp(p, nstep);
}

void Tree::stepSystemPAR(int nstep)
{
    // free the tree
    root = NULL;

    makeTree(nstep);

    Enumerate iter = bodyTabRev->elementsRev();

    for(int i=0; i<PROCS_NR; i++) {
      SpecTh_hg* thr = allocateThread<SpecTh_hg>(i, 64);
      thr->init(&iter, root, rsize);
      ttmm_hg.registerSpecThread(thr,i);
    }

    gettimeofday( &start_time_par, 0 );

    ttmm_hg.speculate<SpecTh_hg>();

    gettimeofday( &end_time_par, 0 );

    cout<<"Time HackGravity PAR: "<<DiffTimeClone(&end_time_par,&start_time_par)<<endl;

    // compute the gravity for all the particles
    //for (Enumerate e = bodyTabRev->elementsRev(); e.hasMoreElements(); ) {
    //  Body* b = e.nextElement();
    //  b->hackGravity(rsize, root);
    //}

    vpPAR(bodyTabRev, nstep);

}


/**** TLS ****/

void Tree::vpTLS(Body* p, int nstep)
{
    MathVector dacc; // = new MathVector();
    MathVector dvel; // = new MathVector();

    for (Enumerate e = p->elementsRev(); e.hasMoreElements(); ) {
      Body* b = e.nextElement();
      MathVector acc1 = b->newAcc;
      if (nstep > 0) {
        dacc.subtraction(acc1, b->acc);
        dvel.multScalar(dacc, dthf);
        dvel.addition(b->vel);
        b->vel = dvel;
      }
      b->acc = acc1;
      dvel.multScalar(b->acc, dthf);

      MathVector vel1 = b->vel;
      vel1.addition(dvel);
      MathVector dpos = vel1;
      dpos.multScalar(DTIME);
      dpos.addition(b->pos);
      b->pos = dpos;
      vel1.addition(dvel);
      b->vel = vel1;
    }
}


void Tree::stepSystemTLS(int nstep)
{
    root = NULL;

    makeTree(nstep);

    Enumerate iter = bodyTabRev->elementsRev();

    for(int i=0; i<PROCS_NR; i++) {
      SpTh* thr = allocateThread<SpTh>(i, 64);
      thr->init(&iter, root, rsize);
      ttmm_TLS_hg.registerSpecThread(thr,i);
    }

    gettimeofday( &start_time_par, 0 );

    ttmm_TLS_hg.speculate<SpTh>();

    gettimeofday( &end_time_par, 0 );

    cout<<"Total Number of rollbacks: "<<ttmm_TLS_hg.nr_of_rollbacks<<endl;
    cout<<"Time HackGravity TLS: "<<DiffTimeClone(&end_time_par,&start_time_par)<<endl;


    // compute the gravity for all the particles
    //for (Enumerate e = bodyTabRev->elementsRev(); e.hasMoreElements(); ) {
    //  Body* b = e.nextElement();
    //  b->hackGravity(rsize, root);
    //}

    vp(bodyTabRev, nstep);
}

/****************************************************/
/******************* TLS duplicates *****************/
/****************************************************/
//umm.specLD<SpM, &sp_m>(Z, this);
//this->specST<SCm,&sc_m>(data+j+1, data_ip1 - wd_imag);

/********** Node TLS support ****************************/

HG Node::gravSub_TLS(HG hg, SpTh* th)     // SHOULD CHANGE THE SIGNATURE TO WORK WITH POINTERS???
{
    MathVector dr;
    MathVector tmp_tls;
    tmp_tls = th->specLD<MV,&mv>(&pos);
    dr.subtraction( tmp_tls, hg.pos0 );

    double drsq = dr.dotProduct() + (EPS * EPS);
    double drabs = sqrt(drsq);

//	non_blocking_synch(NULL);
//	cout<<"& (int) gravSub Node::mass : "<<(FULL_WORD)(void*)&mass<<" "<<hashD((ID_WORD)(void*)&mass)<<" "<<th->getID()<<endl;
//	release_synch(NULL);

    double phii = th->specLD<MD,&md>(&mass) / drabs;
    hg.phi0 -= phii;             // here specSTLD<MV, mv> 
    double mor3 = phii / drsq;
    dr.multScalar(mor3);
    hg.acc0.addition(dr);        // here specSTLD<MV, mv>
    return hg;
}

/********* Cell : public Node  TLS support **************/


bool Cell::subdivp_TLS(double dsq, HG hg, SpTh* th)
{
    MathVector dr; //= new MathVector();
    dr.subtraction( th->specLD<MV,&mv>(&pos), hg.pos0);
    double drsq = dr.dotProduct();

    // in the original olden version drsp is multiplied by 1.0
    return (drsq < dsq);
}

HG Cell::walkSubTree_TLS(double dsq, HG hg, SpTh* th)
{
    if (subdivp_TLS(dsq, hg, th)) {
      for (int k = 0; k < Cell::NSUB; k++) {
        Node* r = th->specLD<MN,&mn>(& subp[k]);
        if (r != NULL)
          hg = r->walkSubTree_TLS(dsq / 4.0, hg, th);
      }
    } else {
      hg = gravSub_TLS(hg, th);   
    }
    return hg;
}


/********** Body : public Node   TLS support ************/

void Body::hackGravity_TLS(double rsize, Node* root, SpTh* th)
{
    MathVector pos0 = th->specLD<MV,&mv>(&pos);

    HG hg(this, pos0);                
    hg = th->specLD<MN, &mn>(&root)->walkSubTree_TLS(rsize * rsize, hg, th);

//	non_blocking_synch(NULL);
//	cout<<"& (int) hackGrav ST Body::phi : "<<(FULL_WORD)(void**)&phi<<" "<<hashD((ID_WORD)(void*)&phi)<<" "<<th->getID()<<endl;
//	release_synch(NULL);

    th->specST<MD, &md>(&phi, hg.phi0);
    th->specST<MV, &mv>(&newAcc, hg.acc0);
}

HG Body::walkSubTree_TLS(double dsq, HG hg, SpTh* th)
{
    //Node* this_node = th->specLD<MN,&mn>( ((Node**) &this));
    Node* hg_pskip  = th->specLD<MNS,&mns>( ((Node**) &hg.pskip));
    if ( this != hg_pskip )
      hg = gravSub_TLS(hg, th);                          
    return hg;
}

/********** Enumerate   TLS support ************/


bool Enumerate::hasMoreElements_TLS(SpTh* th) const {
	Node* b  = th->specLD<MNS,&mns>( ((Node**) &this->current) ); 
	return ( b != NULL); 
}

Body* Enumerate::nextElement_TLS(SpTh* th) {

    Body* retval = (Body*) th->specLD<MN,&mn>( ((Node**) &this->current) );
    
    //if(this->forw)   current = current->next;
    //else             current = current->procNext;
	if(this->forw)  
		th->specST<MNS,&mns>( 
			(Node**) & this->current, 
			(Node*) th->specLD<MN,&mn>((Node**) & this->current->next) 
		);
	else            
		th->specST<MNS,&mns>( 
			(Node**) & this->current, 
			(Node*) th->specLD<MN,&mn>((Node**) & this->current->procNext) 
		);

    return retval;
}

/********************************************************/


