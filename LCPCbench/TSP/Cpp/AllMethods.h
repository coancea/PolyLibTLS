Tree*** Tree::array_iter;
int     Tree::array_dim;

void Tree::fillIteratorHelp(int dim, int ind1, int ind2) {
  if(ind1 >= dim) return;

  const int ind_left = ind2<<1;

  Tree::array_iter[ind1][ind2] = this;
  this->left ->fillIteratorHelp(dim, ind1+1, ind_left);
  this->right->fillIteratorHelp(dim, ind1+1, ind_left+1);
}

void Tree::fillIterator(int sz_conq) {
  Tree::array_dim  = log2(this->sz) - log2(sz_conq); 
  Tree::array_iter = new Tree**[array_dim]; 
//  cout<<"DIM: "<<array_dim<<endl;

  for(int kk=0; kk<Tree::array_dim; kk++) {
    Tree** tmp = new Tree*[1<<kk];
    Tree::array_iter[kk] = tmp;
  }

  this->fillIteratorHelp(Tree::array_dim, 0, 0);
}


timeval start_time_seq, end_time_seq;
unsigned long running_time_seq = 0;

Tree* Tree::tsp(int sz) {
  this->fillIterator(sz);

  int level = Tree::array_dim - 1;
  {
    /** conquer phase **/
    TreeIterator iter(level);

    int num_it = 0;
    gettimeofday( &start_time_seq, 0 );

    while( iter.hasMoreElements() ) {
		TreeClos clos = iter.nextElement();
		Tree* tree = clos.get();
		clos.set(tree->conquer());
        num_it++;
    }

    gettimeofday( &end_time_seq, 0 );
    cout<<"Time Conquer SEQ: "<<DiffTime(&end_time_seq,&start_time_seq)<<endl;
    cout<<"Number Iterations: "<<num_it<<endl;
  }

  /** merge phase **/
  int num_it = 0;
  gettimeofday( &start_time_seq, 0 );

  while(level>0) {
    level--;
    TreeIterator iter(level);
	TreeIterator iter_p_1(level+1);
    int count = 0;
    while( iter.hasMoreElements() ) {
		TreeClos clos = iter.nextElement();
		Tree* tree = clos.get();

		Tree* left  = iter_p_1.nextElement().get();
		Tree* right = iter_p_1.nextElement().get(); 

		Tree* merged = tree->merge(left, right);
		clos.set(merged);
        num_it++;
    }
  }

  gettimeofday( &end_time_seq, 0 );
  cout<<"Time MERGE SEQ: "<<DiffTime(&end_time_seq,&start_time_seq)<<endl;
  cout<<"Number Iterations: "<<num_it<<endl;


  /** final return **/
  return array_iter[0][0];
}


Tree* Tree::tsp_orig(int sz)
{
  if (this->sz <= sz) return conquer();

  Tree* leftval  = left->tsp_orig(sz);
  Tree* rightval = right->tsp_orig(sz); 

  return merge(leftval, rightval);
}


/**********************************************/
/***************** TLS support ****************/
/**********************************************/


Tree* Tree::tsp_TLS(int sz)
{
  this->fillIterator (sz);
  this->tsp_conq_TLS (sz);
  this->tsp_merge_TLS(sz);
  return array_iter[0][0];
}

timeval start_time_par, end_time_par;
unsigned long running_time_par = 0;

/*** conquer phase TLS ***/
void Tree::tsp_conq_TLS(int sz) {
  int level = Tree::array_dim - 1;
  TreeIterator iter(level);

  for(int i=0; i<PROCS_NR; i++) {
    SpTh* thr = allocateThread<SpTh>(i, 64);
    thr->init(&iter);
    ttmm.registerSpecThread(thr,i);
  }

  gettimeofday( &start_time_par, 0 );

  ttmm.speculate<SpTh>();

  gettimeofday( &end_time_par, 0 );

  cout<<"Total Number of rollbacks: "<<ttmm.nr_of_rollbacks<<endl;
  cout<<"Time Conquer TLS: "<<DiffTime(&end_time_par,&start_time_par)<<endl;

  //exit(0);
}

void Tree::tsp_merge_TLS(int sz) {
  int level = Tree::array_dim - 1;

  for(int i=0; i<PROCS_NR; i++) {
    SpThMerge* thr = allocateThread<SpThMerge>(i, 64);
    ttmm.registerSpecThread(thr,i);
#if defined (NESTED_TLS)
    ;
#else
	TreeIterator iter(--level);
	TreeIterator iter_p_1(level+1);
    thr->init(&iter, &iter_p_1, level);
#endif
  }


  int num_rolls = 0;
  gettimeofday( &start_time_par, 0 );

#if defined (NESTED_TLS)

  while(level>0) {
    level--;
    TreeIterator iter(level);
	TreeIterator iter_p_1(level+1);

    if(level>12) {
     //cout<<"Starting the merge for level: "<<level<<endl;
      ttmm.init();
      for(int i=0; i<PROCS_NR; i++) {
        SpThMerge* thr = (SpThMerge*)ttmm.getSpecThread(i);
        thr->reset(); 
        thr->init(&iter, &iter_p_1, level);
        ttmm.registerSpecThread(thr,i);
      }
   
      ttmm.speculate<SpThMerge>();
    
      num_rolls += ttmm.nr_of_rollbacks;
     //cout<<"Ending the merge for level: "<<level<<endl;
    } else {
      while( iter.hasMoreElements() ) {
        TreeClos clos = iter.nextElement();
        Tree* tree = clos.get();

        Tree* left  = iter_p_1.nextElement().get();
        Tree* right = iter_p_1.nextElement().get(); 

        Tree* merged = tree->merge(left, right);
        clos.set(merged);
      }
    }
  }

#else
{
    ttmm.init();
    ttmm.speculate<SpThMerge>();
    num_rolls += ttmm.nr_of_rollbacks;
}
#endif

  gettimeofday( &end_time_par, 0 );

  cout<<"Total Number of rollbacks: "<<num_rolls<<endl;
  cout<<"Time Merge TLS: "<<DiffTime(&end_time_par,&start_time_par)<<endl; 
}


/************************************************/
/***************** CONQUER TLS ******************/
/************************************************/

Tree* Tree::makeList_TLS(SpTh* th) {
    Tree *myleft, *myright;
    Tree *tleft,  *tright;
    Tree* retval = this;
    
	Tree* lft = th->specLD<MN, &mn>(&this->left);

    // head of left list
    if (lft != NULL) 
      myleft = lft->makeList_TLS(th);
    else
      myleft = NULL;

	Tree* rht = th->specLD<MN, &mn>(&this->right);

    // head of right list
    if (rht != NULL)
      myright = rht->makeList_TLS(th);
    else
      myright = NULL;

    if (myright != NULL) { 
      retval = myright; 
      //rht->next = this;
      th->specST<MN, &mn>(&rht->next, this);
    }

    if (myleft != NULL) { 
      retval = myleft;
      Tree* tmp; 
      if (myright != NULL)
        tmp = myright;
      else
        tmp = this;

      //left->next = tmp;
      th->specST<MN, &mn>(&lft->next, tmp);
    }
	
    //next = NULL;
	th->specST<MN, &mn>(&this->next, (Tree*)NULL);
    
    return retval;
} 

Tree* Tree::conquer_TLS(SpTh* th) {
      // create the list of nodes
    Tree* t = makeList_TLS(th);
 
    // Create initial cycle 
    Tree* cycle = t;
    t = th->specLD<MN, &mn>(&t->next);         // t = t->next;
    th->specST<MN, &mn>(&cycle->next, cycle);  // cycle->next = cycle;
    th->specST<MN, &mn>(&cycle->prev, cycle);  //cycle->prev = cycle;

//    cout<<"In conquer TLS: "<<th->getID()<<" "<<(unsigned long)(void*)t<<" &t.next: "
//        <<(void*)(&t->next)<<" &cycle.next: "<<(void*)(&cycle->next)<<endl;

    // Loop over remaining points 
    Tree* donext;
    for (; t != NULL; t = donext) {
      donext = th->specLD<MN, &mn>(&t->next); //donext = t->next; /* value won't be around later */
      Tree* min = cycle;
      double mindist = t->distance(cycle);

      //for (Tree* tmp = cycle->next; tmp != cycle; tmp=tmp->next) {
      for( Tree* tmp = th->specLD<MN, &mn>(&cycle->next); tmp != cycle; tmp=th->specLD<MN, &mn>(&tmp->next) ) {
        double test = tmp->distance(t);
        if (test < mindist) {
          mindist = test;
          min = tmp;
        } /* if */
      } /* for tmp... */

      //Tree* next = min->next;
      Tree* next = th->specLD<MN, &mn>(&min->next);
      //Tree* prev  = min->prev;
      Tree* prev = th->specLD<MN, &mn>(&min->prev);

      double mintonext = min->distance(next);
      double mintoprev = min->distance(prev);
      double ttonext = t->distance(next);
      double ttoprev = t->distance(prev);

      if ((ttoprev - mintoprev) < (ttonext - mintonext)) {
        /* insert between min and prev */
        //prev->next = t;
        th->specST<MN, &mn>(&prev->next, t);
        //t->next = min;
        th->specST<MN, &mn>(&t->next, min);
        //t->prev = prev;
        th->specST<MN, &mn>(&t->prev, prev);
        //min->prev = t;
        th->specST<MN, &mn>(&min->prev, t);
      } else {
        //next->prev = t;
        th->specST<MN, &mn>(&next->prev, t);
        //t->next = next;
        th->specST<MN, &mn>(&t->next, next);
        //min->next = t;
        th->specST<MN, &mn>(&min->next, t);
        //t->prev = min;
        th->specST<MN, &mn>(&t->prev, min);
      }
    } /* for t... */

    return cycle;
}
  

/************************************************/
/****************** MERGE TLS *******************/
/************************************************/

void  Tree::reverse_TLS(SpThMerge* th) {
    //Tree* prev = this->prev;
    Tree* prev = th->specLD<MN, &mn>(&this->prev); 
    //prev->next = NULL;
    th->specST<MN, &mn>(&prev->next, (Tree*)NULL);
    //this->prev = NULL;
    th->specST<MN, &mn>(&this->prev, (Tree*)NULL);

    Tree* back = this;
    Tree* tmp = this;
    // reverse the list for the other nodes
    Tree* next;
    //for (Tree* t = this->next; t != NULL; back = t, t = next) {
    for (Tree* t = th->specLD<MN, &mn>(&this->next); t != NULL; back = t, t = next) {
      //next = t->next;
      next = th->specLD<MN, &mn>(&t->next);
      //t->next = back;
      th->specST<MN, &mn>(&t->next, back);
      //back->prev = t;
      th->specST<MN, &mn>(&back->prev, t);
    }

    // reverse the list for this node
    //tmp->next = prev;
    th->specST<MN, &mn>(&tmp->next, prev);
    //prev->prev = tmp;
    th->specST<MN, &mn>(&prev->prev, tmp);
}


//th->specLD<MN, &mn>(&cycle->next);
//th->specST<MN, &mn>(&t->next, min);

Tree* Tree::merge_TLS(Tree* a, Tree* b, SpThMerge* th) {
    // Compute location for first cycle
    Tree*   min = a;
    double mindist = distance(a);
    Tree*   tmp = a;

    //for (a = a->next; a != tmp; a = a->next) {
    for ( a = th->specLD<MN, &mn>(&a->next); a != tmp; a = th->specLD<MN, &mn>(&a->next) ) {
      double test = distance(a);
      if (test < mindist) {
        mindist = test;
        min = a;
      }
    }

    //Tree* next = min->next;
    Tree* next = th->specLD<MN, &mn>(&min->next);
    //Tree* prev = min->prev;
    Tree* prev = th->specLD<MN, &mn>(&min->prev);

    double mintonext = min->distance(next);
    double mintoprev = min->distance(prev);
    double ttonext   = distance(next);
    double ttoprev   = distance(prev);

    Tree *p1, *n1;
    double tton1, ttop1;
    if ((ttoprev - mintoprev) < (ttonext - mintonext)) {
      // would insert between min and prev
      p1 = prev;
      n1 = min;
      tton1 = mindist;
      ttop1 = ttoprev;
    } else { 
      // would insert between min and next
      p1 = min;
      n1 = next;
      ttop1 = mindist;
      tton1 = ttonext;
    }
 
    // Compute location for second cycle
    min = b;
    mindist = distance(b);
    tmp = b;

    //for (b = b->next; b != tmp; b = b->next) {
    for ( b = th->specLD<MN, &mn>(&b->next); b != tmp; b = th->specLD<MN, &mn>(&b->next) ) {
      double test = distance(b);
      if (test < mindist) {
        mindist = test;
        min = b;
      }
    }

    //next = min->next;
    next = th->specLD<MN, &mn>(&min->next);
    //prev = min->prev;
    prev = th->specLD<MN, &mn>(&min->prev);

    mintonext = min->distance(next);
    mintoprev = min->distance(prev);
    ttonext = this->distance(next);
    ttoprev = this->distance(prev);
    
    Tree *p2, *n2;
    double tton2, ttop2;
    if ((ttoprev - mintoprev) < (ttonext - mintonext)) {
      // would insert between min and prev
      p2 = prev;
      n2 = min;
      tton2 = mindist;
      ttop2 = ttoprev;
    } else { 
      // would insert between min andn ext 
      p2 = min;
      n2 = next;
      ttop2 = mindist;
      tton2 = ttonext;
    }

    // Now we have 4 choices to complete:
    // 1:t,p1 t,p2 n1,n2
    // 2:t,p1 t,n2 n1,p2
    // 3:t,n1 t,p2 p1,n2
    // 4:t,n1 t,n2 p1,p2 
    double n1ton2 = n1->distance(n2);
    double n1top2 = n1->distance(p2);
    double p1ton2 = p1->distance(n2);
    double p1top2 = p1->distance(p2);

    mindist = ttop1 + ttop2 + n1ton2; 
    int choice = 1;

    double test = ttop1 + tton2 + n1top2;
    if (test < mindist) {
      choice = 2;
      mindist = test;
    }

    test = tton1 + ttop2 + p1ton2;
    if (test < mindist) {
      choice = 3;
      mindist = test;
    }
  
    test = tton1 + tton2 + p1top2;
    if (test < mindist) 
      choice = 4;

    switch (choice) {
    case 1:
      // 1:p1,this this,p2 n2,n1 -- reverse 2!
      n2->reverse_TLS(th);
      //p1->next = this;
      th->specST<MN, &mn>(&p1->next, this);
      //this->prev = p1;
      th->specST<MN, &mn>(&this->prev, p1);
      //this->next = p2;
      th->specST<MN, &mn>(&this->next, p2);
      //p2->prev = this;
      th->specST<MN, &mn>(&p2->prev, this);
      //n2->next = n1;
      th->specST<MN, &mn>(&n2->next, n1);
      //n1->prev = n2;
      th->specST<MN, &mn>(&n1->prev, n2);
      break;
    case 2:
      // 2:p1,this this,n2 p2,n1 -- OK
      //p1->next = this;
      th->specST<MN, &mn>(&p1->next, this);
      //this->prev = p1;
      th->specST<MN, &mn>(&this->prev, p1);
      //this->next = n2;
      th->specST<MN, &mn>(&this->next, n2);
      //n2->prev = this;
      th->specST<MN, &mn>(&n2->prev, this);
      //p2->next = n1;
      th->specST<MN, &mn>(&p2->next, n1);
      //n1->prev = p2;
      th->specST<MN, &mn>(&n1->prev, p2);
      break;
    case 3:
      // 3:p2,this this,n1 p1,n2 -- OK
      //p2->next = this;
      th->specST<MN, &mn>(&p2->next, this);
      //this->prev = p2;
      th->specST<MN, &mn>(&this->prev, p2);
      //this->next = n1;
      th->specST<MN, &mn>(&this->next, n1);
      //n1->prev = this;
      th->specST<MN, &mn>(&n1->prev, this);
      //p1->next = n2;
      th->specST<MN, &mn>(&p1->next, n2);
      //n2->prev = p1;
      th->specST<MN, &mn>(&n2->prev, p1);
      break;
    case 4:
      // 4:n1,this this,n2 p2,p1 -- reverse 1!
      n1->reverse_TLS(th);
      //n1->next = this;
      th->specST<MN, &mn>(&n1->next, this);
      //this->prev = n1;
      th->specST<MN, &mn>(&this->prev, n1);
      //this->next = n2;
      th->specST<MN, &mn>(&this->next, n2);
      //n2->prev = this;
      th->specST<MN, &mn>(&n2->prev, this);
      //p2->next = p1;
      th->specST<MN, &mn>(&p2->next, p1);
      //p1->prev = p2;
      th->specST<MN, &mn>(&p1->prev, p2);
      break;
    }
    return this;
}


/************************************************/
/*************** ITERATORS TLS ******************/
/************************************************/


Tree* TreeClos::get_TLS(SpThMerge* th) {
  return th->specLD<MN,&mn>(this->ptr);
}

Tree* TreeClos::get_TLS(SpTh* th) {
  return th->specLD<MN,&mn>(this->ptr);
}


void  TreeClos::set_TLS(Tree* t, SpTh* th) {
  th->specST<MN, &mn>(this->ptr, t);
}

void  TreeClos::set_TLS(Tree* t, SpThMerge* th) {
  th->specST<MN, &mn>(this->ptr, t);
}


bool     TreeIterator::hasMoreElements_TLS(SpTh* th) {
  return ( th->specLD<MI,&mi>(&counter) < this->dim );
}

bool     TreeIterator::hasMoreElements_TLS(SpThMerge* th) {
  return ( th->specLD<MI,&mi>(&counter) < this->dim );
}


TreeClos TreeIterator::nextElement_TLS(SpTh* th) {
  Tree** ptr = &Tree::array_iter[level][counter];
  TreeClos l( ptr );
  th->specST<MI, &mi>( &counter, th->specLD<MI,&mi>(&counter)+1 );
  return l;
}

TreeClos TreeIterator::nextElement_TLS(SpThMerge* th) {
  Tree** ptr = &Tree::array_iter[level][counter];
  TreeClos l( ptr );
  th->specST<MI, &mi>( &counter, th->specLD<MI,&mi>(&counter)+1 );
  return l;
}


