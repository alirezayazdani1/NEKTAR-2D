#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <veclib.h>
#include "nektar.h"

#ifdef PARALLEL

#ifdef PBC_1D
MPI_Comm get_MPI_COMM();

MPI_Comm MPI_COMM_SPLIT = MPI_COMM_NULL;

MPI_Comm MPI_COMM_BC[26];

/* define communicators for inlets and outlets */
MPI_Comm  Communicator_inlet[26];
MPI_Comm  Communicator_outlet[26];
MPI_Comm  Communicator_inlet_plus_ROOT[26];
MPI_Comm  Communicator_outlet_plus_ROOT[26];
#endif

void unreduce (double *x, int n);
void reduce   (double *x, int n, double *work);
static int numproc;
static int my_node;

void init_comm (int *argc, char **argv[])
{
  int info, nprocs,                      /* Number of processors */
      mytid;                             /* My task id */

  info = MPI_Init (argc, argv);                 /* Initialize */
  if (info != MPI_SUCCESS) {
    fprintf (stderr, "MPI initialization error\n");
    exit(1);
  }
  gsync();

  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);         /* Number of processors */
  MPI_Comm_rank(MPI_COMM_WORLD, &mytid);          /* my process id */

  pllinfo.nprocs = numproc = nprocs;
  pllinfo.procid = my_node = mytid;

  MPI_Barrier  (MPI_COMM_WORLD);                  /* sync before work */

#ifdef PBC_1D
	int color;
  info = MPI_Comm_split ( MPI_COMM_WORLD, color, mytid, &MPI_COMM_SPLIT);
  if (info != MPI_SUCCESS) {
    fprintf (stderr, "MPI split error\n");
    exit(1);
  }

  MPI_Comm_size(MPI_COMM_SPLIT, &nprocs);         /* Number of processors */
  MPI_Comm_rank(MPI_COMM_SPLIT, &mytid);          /* my process id */

  pllinfo.nprocs = numproc = nprocs;
  pllinfo.procid = my_node = mytid;

  MPI_Barrier  (MPI_COMM_SPLIT);                  /* sync before work */
#endif

  return;
}

#ifdef PBC_1D
void create_comm_BC(int Nout, int *face_counter){

  int my_color,info,n_out;
  my_color = MPI_UNDEFINED;

  for (n_out = 0; n_out < Nout; n_out++){
    if (face_counter[n_out] != 0  || mynode() == 0  )
      my_color = 1;
  }

  info = MPI_Comm_split(get_MPI_COMM(), my_color, mynode(), &MPI_COMM_BC[0]);
  if (info != MPI_SUCCESS) {
      fprintf (stderr, "scatter_topology_nektar: MPI split error\n");
      exit(1);
  }
  if (my_color != 1)
    MPI_COMM_BC[n_out]=MPI_COMM_NULL;

  for (n_out = 1; n_out < 26; n_out++)
     MPI_COMM_BC[n_out]=MPI_COMM_NULL;
}

int create_comm_BC_inlet_outlet(int Ninl, int *Nfaces_per_inlet,  int *Nnodes_inlet,
                                int Nout, int *Nfaces_per_outlet, int *Nnodes_outlet){
  /* create communicators for inlets and outlets */

  int i, index, error_code;
  error_code = 0;
   for (index = 0; index < Ninl; index++){
     if (Nfaces_per_inlet[index] > 0)
        i = 1;
     else
        i = MPI_UNDEFINED;

     error_code = MPI_Comm_split (get_MPI_COMM() , i, mynode(), &Communicator_inlet[index]);
     if (error_code != MPI_SUCCESS ) return 1;
     if (i == MPI_UNDEFINED) Communicator_inlet[index] = MPI_COMM_NULL;

     if (mynode() == 0)
        i = 1;
     error_code = MPI_Comm_split (get_MPI_COMM() , i, mynode(), &Communicator_inlet_plus_ROOT[index]);
     if (error_code != MPI_SUCCESS) return 1;
     if (i == MPI_UNDEFINED) Communicator_inlet_plus_ROOT[index] = MPI_COMM_NULL;

     if (Communicator_inlet_plus_ROOT[index] != MPI_COMM_NULL){
        error_code = MPI_Comm_size(Communicator_inlet_plus_ROOT[index],&Nnodes_inlet[index]);
  if (error_code != MPI_SUCCESS ) return 1;
     }
   }
   for (index = 0; index < Nout; index++){
     if (Nfaces_per_outlet[index] > 0)
        i = 1;
     else
        i = MPI_UNDEFINED;

     error_code = MPI_Comm_split (get_MPI_COMM() , i, mynode(), &Communicator_outlet[index]);
     if (error_code != MPI_SUCCESS ) return 1;
     if (i == MPI_UNDEFINED) Communicator_outlet[index] = MPI_COMM_NULL;


     if (mynode() == 0)
        i = 1;
     error_code = MPI_Comm_split (get_MPI_COMM() , i, mynode(), &Communicator_outlet_plus_ROOT[index]);
     if (error_code != MPI_SUCCESS ) return 1;
     if (i == MPI_UNDEFINED) Communicator_outlet_plus_ROOT[index] = MPI_COMM_NULL;

     if (Communicator_outlet_plus_ROOT[index] != MPI_COMM_NULL){
       error_code = MPI_Comm_size(Communicator_outlet_plus_ROOT[index],&Nnodes_outlet[index]);
       if (error_code != MPI_SUCCESS ) return 1;
     }
   }
  return 0;
}

MPI_Comm get_MPI_COMM(){
  return MPI_COMM_SPLIT;
}

#endif

void exit_comm(){

  MPI_Finalize();

  return;
}

void gsync ()
{
  int info;

  info = MPI_Barrier(MPI_COMM_WORLD);

  return;
}


int numnodes ()
{
  int np;

  MPI_Comm_size(MPI_COMM_WORLD, &np);         /* Number of processors */

  return np;
}


int mynode ()
{
  int myid;

  MPI_Comm_rank(MPI_COMM_WORLD, &myid);          /* my process id */
  
  return myid;
}

 
void csend (int type, void *buf, int len, int node, int pid)
{

  MPI_Send (buf, len, MPI_BYTE, node, type, MPI_COMM_WORLD);
  
  return;
}

void crecv (int typesel, void *buf, int len)
{
  MPI_Status status;

  MPI_Recv (buf, len, MPI_BYTE, MPI_ANY_SOURCE, typesel, MPI_COMM_WORLD, &status);
  
  return;
}


void msgwait (MPI_Request *request)
{
  MPI_Status status;

  MPI_Wait (request, &status);

  return;
}

double dclock(void)
{
  double time;

  time = MPI_Wtime();

  return time;

}
void gimax (int *x, int n, int *work) { 
  register int i;

  MPI_Allreduce (x, work, n, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

  /* *x = *work; */
  icopy(n,work,1,x,1);

  return;
}

void gdmax (double *x, int n, double *work)
{
  register int i;

  MPI_Allreduce (x, work, n, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

  /* *x = *work; */
  dcopy(n,work,1,x,1);

  return;
}


void gdsum (double *x, int n, double *work)
{
  register int i;

  MPI_Allreduce (x, work, n, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

  /* *x = *work; */
  dcopy(n,work,1,x,1);

  return;
}

void gisum (int *x, int n, int *work)
{
  register int i;

  MPI_Allreduce (x, work, n, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  /* *x = *work; */
  icopy(n,work,1,x,1);

  return;
}

void ifexists(double *in, double *inout, int *n, MPI_Datatype *size);

void BCreduce(double *bc, Bsystem *Ubsys){
#if 1
  gs_gop(Ubsys->pll->known,bc,"A");
#else
  register int i;
  int      ngk = Ubsys->pll->nglobal - Ubsys->pll->nsolve;
  int      nlk = Ubsys->nglobal - Ubsys->nsolve;
  double *n1,*n2;
  static MPI_Op MPI_Ifexists = NULL;
  
  if(!MPI_Ifexists){
    MPI_Op_create((MPI_User_function *)ifexists, 1, &MPI_Ifexists);
  } 

  n1 = dvector(0,ngk-1);
  n2 = dvector(0,ngk-1);

  memset(n1,'\0',ngk*sizeof(double));
  memset(n2,'\0',ngk*sizeof(double));

  /* fill n1 with values from bc  */
  for(i = 0; i < nlk; ++i) n1[Ubsys->pll->knownmap[i]] = bc[i];
  
  /* receive list from other processors and check against local */
  MPI_Allreduce (n1, n2, ngk, MPI_DOUBLE, MPI_Ifexists, MPI_COMM_WORLD);
  /* fill bc with values values from  n1 */
  for(i = 0; i < nlk; ++i) bc[i] = n2[Ubsys->pll->knownmap[i]];

  free(n1);  free(n2);
#endif
}


void GatherBlockMatrices(Element *U,Bsystem *B){
  double *edge, *face;
  
  if(LGmax <=2) return;

  switch(B->Precon){
  case Pre_Block:
    edge  = B->Pmat->info.block.iedge[0];
    face  = B->Pmat->info.block.iface[0];
    break;
  case Pre_LEnergy:
    edge  = B->Pmat->info.lenergy.iedge[0];
    face  = B->Pmat->info.lenergy.iface[0];
    break;
  default:
    error_msg(Unknown preconditioner in GatherBlockMatrices);
    break;
  }

  gs_gop(B->egather,edge,"+");
  gs_gop(B->fgather,face,"+");

}

void Set_Comm_GatherBlockMatrices(Element *U, Bsystem *B){
  register int i,j;
  int nes = B->ne_solve;
  int nfs = B->nf_solve;
  int nel = B->nel;
  int l, *map, start, one=1, Lskip, *pos, *Ledge, *Lface;
  double *edge, *face;
  Edge   *e;
  Face   *f;
  Element *E;
  extern  Element_List *Mesh;
  
  if(LGmax <=2) return;

  switch(B->Precon){
  case Pre_Block:
    Ledge = B->Pmat->info.block.Ledge;
    Lface = B->Pmat->info.block.Lface;
    break;
  case Pre_LEnergy:
    Ledge = B->Pmat->info.lenergy.Ledge;
    Lface = B->Pmat->info.lenergy.Lface;
    break;
  default:
    error_msg(Unknown preconditioner in GatherBlockMatrices);
    break;
  }

  pos = ivector(0,max(nes,nfs));
  
  /* make up numbering list based upon solvemap */
  /* assumed fixed L order */
  
  pos[0] = 0; 
  for(i = 1; i < nes+1; ++i)
    pos[i] = pos[i-1] + Ledge[i-1]*(Ledge[i-1]+1)/2;
  
  map = ivector(0,pos[nes]);

  Lskip = LGmax-2;
  Lskip = Lskip*(Lskip+1)/2;
  for(E=U; E; E = E->next)
    for(j = 0; j < E->Nedges; ++j){
      e = E->edge + j;
      if(e->gid < nes){
	/* allocate starting location based on global mesh */
	start = Mesh->flist[pllinfo.eloop[e->eid]]->edge[j].gid*Lskip;
	l     = Ledge[e->gid];
	l     = l*(l+1)/2;
	iramp(l,&start,&one,map+pos[e->gid],1);
      }
    }
  
  B->egather = gs_init(map,pos[nes],option("GSLEVEL"));
  free(map);
  
  pos[0] = 0; 
  for(i = 1; i < nfs+1; ++i)
    pos[i] = pos[i-1] +  Lface[i-1]*(Lface[i-1]+1)/2;
  
  map = ivector(0,pos[nfs]);
  
  Lskip = (LGmax-2)*(LGmax-2);
  Lskip = Lskip*(Lskip+1)/2;
  for(E=U;E; E = E->next)
    for(j = 0; j < E->Nfaces; ++j){
      f = E->face + j;
      if(f->gid < nfs){
	/* allocate starting location based on global mesh */
	start = Mesh->flist[pllinfo.eloop[f->eid]]->face[j].gid*Lskip;
	l     = Lface[f->gid];
	l     = l*(l+1)/2;
	iramp(l,&start,&one,map+pos[f->gid],1);
      }
    }
  
  B->fgather = gs_init(map,pos[nfs],option("GSLEVEL"));
  free(map);
  free(pos);
}

void unreduce (double *x, int n)
{
  int nprocs = numnodes(),
      pid    = mynode(),
      k, i;

  ROOTONLY
    for (k = 1; k < nprocs; k++)
      csend (MSGTAG + k, x, n*sizeof(double), k, 0);
  else
    crecv (MSGTAG + pid, x, n*sizeof(double));
  
  return;
}

void reduce (double *x, int n, double *work)
{
  int nprocs = numnodes(),
      pid    = mynode(),
      k, i;

  ROOTONLY {
    for (i = 0; i < n; i++) work[i] = x[i];
    for (k = 1; k < nprocs; k++) {
      crecv (MSGTAG + k, x, n*sizeof(double));
      for (i = 0; i < n; i++) work[i] += x[i];
    }
    for (i = 0; i < n; i++) x[i] = work[i];    
  } else
    csend (MSGTAG + pid, x, n*sizeof(double), 0, 0);
  
  return;
}

void ifexists(double *in, double *inout, int *n, MPI_Datatype *size){
  int i;
  
  for(i = 0; i < *n; ++i)
    inout[i] = (in[i] != 0.0)? in[i] : inout[i];
  
}

void parallel_gather(double *w, Bsystem *B){
  gs_gop(B->pll->solve,w,"+");
}



#ifdef METIS /* redefine default partitioner to be metis */


extern "C"
{
  void METIS_PartGraphRecursive(int &, int *, int *, int *, int *, int &, 
				int &, int &, int *, int *, int *); 
}

static void pmetis(int &nel, int *xadj, int *adjncy, int *vwgt,
		   int *ewgt, int& wflag, int& nparts,int *option,
		   int &num, int* edgecut,int *partition){

  METIS_PartGraphRecursive(nel,xadj,adjncy,vwgt,ewgt,wflag,
			   num, nparts,option,edgecut,partition);
}

void default_partitioner(Element_List *EL, int *partition){
  register int i,j;
  int eDIM  = EL->fhead->dim();
  int nel = EL->nel;
  int medg,edgecut,cnt;
  int *xadj, *adjncy;
  int opt[5];
  Element *E;

  ROOTONLY
    fprintf(stdout,"Partitioner         : using pmetis \n");
      
  /* count up number of local edges in patch */
  medg =0;
  if(eDIM == 2)
    for(E = EL->fhead; E; E= E->next){
      for(j = 0; j < E->Nedges; ++j) 
	if(E->edge[j].base) ++medg;
    }
  else
    for(E = EL->fhead; E; E= E->next){
      for(j = 0; j < E->Nfaces; ++j) 
	if(E->face[j].link) ++medg;
    }
  
  xadj      = ivector(0,nel);
  adjncy    = ivector(0,medg-1);
  
  izero(nel+1,xadj,1);
  cnt = 0;
  if(eDIM == 2)
    for(i = 0; i < nel; ++i){
      E = EL->flist[i];
      xadj[i+1] = xadj[i];
      for(j = 0; j < E->Nedges; ++j){
	if(E[i].edge[j].base){
	  if(E[i].edge[j].link){
	    adjncy[cnt++] = E->edge[j].link->eid;
	    xadj[i+1]++;
	  }
	  else{
	    adjncy[cnt++] = E->edge[j].base->eid;
	    xadj[i+1]++;
	  }
	}
      }
    }
  else
    for(i = 0; i < nel; ++i){
      E = EL->flist[i];
      xadj[i+1] = xadj[i];
      for(j = 0; j < E->Nfaces; ++j) 
	if(E->face[j].link){
	  adjncy[cnt++] = E->face[j].link->eid;
	  xadj[i+1]++;
	}
    }
  
  opt[0] = 0;
  int num, wflag;
  num = wflag = 0;
  pmetis(nel,xadj,adjncy,0,0,wflag,pllinfo.nprocs,opt,num,
	 &edgecut,partition);
    
  free(xadj); free(adjncy); 
}
#endif

/* gather edges from other patches */
void exchange_sides(int Nfields, Element_List **Us){ 
  register int   i,j,k,n;
  int            cnt, qface, qedg, *lid;
  int            ncprocs = pllinfo.ncprocs;
  ConInfo        *cinfo = pllinfo.cinfo;
  static double  **buf;

  if(!buf){
    int lenmax = 0;
    buf = (double **)malloc(ncprocs*sizeof(double *));
    for(i = 0; i < ncprocs; ++i){
      lenmax = max(lenmax,Nfields*cinfo[i].datlen-1);
      buf[i] = dvector(0,Nfields*cinfo[i].datlen-1);
    }
  }
  
  if(Us[0]->fhead->dim() == 2){
    Edge           *e;
    /* fill up data buffer and send*/
    for(i = 0; i < pllinfo.ncprocs; ++i){
      for(n = 0,cnt = 0; n < Nfields; ++n)
	for(j = 0; j < cinfo[i].nedges; ++j){
	  e = Us[n]->flist[cinfo[i].elmtid[j]]->edge + cinfo[i].edgeid[j];
	  qedg = e->qedg;
	  dcopy(qedg,e->h,1,buf[i] + cnt,1);
	  cnt += qedg;
	}
      SendRecvRep(buf[i],Nfields*cinfo[i].datlen*sizeof(double),
		  cinfo[i].cprocid);
    }
    
    /* unpack*/
    for(i = 0; i < pllinfo.ncprocs; ++i){
      for(n = 0,cnt = 0; n < Nfields; ++n)
	for(j = 0; j < cinfo[i].nedges; ++j){
	  e = Us[n]->flist[cinfo[i].elmtid[j]]->edge[cinfo[i].edgeid[j]].link;
	  qedg = e->qedg;
	  dcopy(qedg,buf[i] + cnt,1,e->h,1);
	  cnt += qedg;
	}
    }
  }
  else{
    Face  *f;
    /* fill up data buffer and send*/
    for(i = 0; i < pllinfo.ncprocs; ++i){
      for(n = 0,cnt = 0; n < Nfields; ++n)
	for(j = 0; j < cinfo[i].nedges; ++j){
	  f = Us[n]->flist[cinfo[i].elmtid[j]]->face + cinfo[i].edgeid[j];

	  if(Us[n]->flist[cinfo[i].elmtid[j]]->Nfverts(f->id) == 3)
	    lid =  Tri_nmap(f->qface,f->con);
	  else
	    lid = Quad_nmap(f->qface,f->con);
	  
	  qface = f->qface*f->qface;
	  for(k=0;k<qface;++k)
	    buf[i][cnt+k] = f->h[lid[k]];
	  cnt += qface;
	}

      SendRecvRep(buf[i],Nfields*cinfo[i].datlen*sizeof(double),
		  cinfo[i].cprocid);
    }
    
    /* unpack*/
    for(i = 0; i < pllinfo.ncprocs; ++i){
      for(n = 0,cnt = 0; n < Nfields; ++n)
	for(j = 0; j < cinfo[i].nedges; ++j){
	  f = Us[n]->flist[cinfo[i].elmtid[j]]->face[cinfo[i].edgeid[j]].link;
	  qface = f->qface*f->qface;
	  dcopy(qface,buf[i]+cnt,1,f->h,1);
	  cnt += qface;
	  if(f->con)
	    fprintf(stderr,"Face con not zero in elmt %id, face %id\n",
		    f->eid,f->id);
	}
    }
  }
}

void SendRecvRep(void *buf, int len, int proc){
  MPI_Status status;

  MPI_Sendrecv_replace(buf, len, MPI_BYTE, proc, MSGTAG+pllinfo.procid, proc,
		       MSGTAG+proc, MPI_COMM_WORLD, &status);
}
#endif
