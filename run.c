/* ------------------------------------------------------------------------- 
 * Project 1 - Comparing Scheduling Policies of a Single Server Queue (M/M/1)
 * CS 791.1003 CS Performance Optimization of Computer Systems
 * Spring 2020
 * University of Nevada, Reno
 * Author: Farhan Sadique
 * ------------------------------------------------------------------------- 
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "rngs.h"

// ============= Copied from ssq3.c =============
#define START         0.0 
double lambda  = 2.0;
double mu      = 1.5;
double rho     = 1.5/2.0;
double arrival = 0.0;
long int init_seed = 0;

double Exponential(double m) { 
  return (-m * log(1.0 - Random())); 
}
double GetArrival() {
  SelectStream(0); 
  arrival += Exponential(lambda);
  return (arrival);
} 
double GetService() {
  SelectStream(1);
  return (Exponential(mu));
}  

// ============= define, enum, struct =============
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
typedef enum { False, True } bool;                         // Typdef boolean
typedef enum {FCFS, LCFS, SJF, SRTF, PS} POLICY;

struct JOB {                                              
  long long int id, arrival, service, 
    remaining, priority, completion, delay;
} emptyjob; 

struct RESULT {
  POLICY policy;
  bool preemptive;
  const char *policy_name;
  long long int last_index;
  float last_arrival, last_completion, 
    totdelay, totservice, totwait;
  long int seed;
  double mu, lambda, rho;
  long long int maxqlen;
};

// ============= JOB Queue Global Variables =============
#define QMAX 1000000
#define NJOB 1000000
struct JOB *queue;
int inq = 0; 

// ========= JOB Queue Functions =========           
bool qisempty();                                           // True if job queue empty
bool qisfull();                                            // True if job queue full
void qswap(int i, int j);                                  // Swap two processes (i,j) in JOB queue
void heapifyup();                                          // heapify queue starting from leaf
void heapifydown();                                        // heapify queue starting from root
void qput(struct JOB data);                                // put job in queue
struct JOB qget();                                         // get job from queue
struct JOB qpeek();                                        // peek into job queue
void qprint();                                             // print job queue as binary heap

// ================ Misc. Functions ================
void print_result(struct RESULT result);
void fprintf_result(struct RESULT result);

// =============== Simulation Core ===============
struct RESULT simulate(struct JOB alljob[NJOB], POLICY policy, bool preemptive){

  struct JOB this, next, prev = emptyjob,
    INFjob = {.arrival = GetArrival()*10000000};
  
  long long int index=0, maxqlen=0;
  long long int begin, tentative, until, elapsed, totdelay=0;;
  
  do {
    do {
      qput(alljob[index++]);
      
      if (inq > maxqlen)
        maxqlen = inq;
      
      if (inq >= QMAX){
        printf("\nQueue Overflow! Increase QMAX (line 59)\n"
        "Seed=%ld Policy=%d Preemptive=%d rho=%f\n",
        init_seed, policy, preemptive, rho); // debug
        exit(-1);
      }
      
      next = index < NJOB-1 ? alljob[index] : INFjob; 
      if (index==1) break;
    } while (next.arrival <= this.completion);
    
    do {
      this = qget();
      begin = MAX(prev.completion, this.arrival);
      tentative = begin + this.remaining;
      until = preemptive ? tentative < next.arrival ? tentative : next.arrival : tentative;
      elapsed = until - begin;
      this.remaining -= elapsed;
      
      if (policy == SRTF) this.priority = this.remaining;
      
      this.delay += (begin - this.completion);
      this.completion = until;
      prev = this;
      
      //printf("this.id: %lld  next.arrival: %lld  begin: %lld  tentative: %lld  until: %lld  elapsed: %lld  this.remaining: %lld  this.delay: %lld  this.completion: %lld\n", this.id, next.arrival, begin, tentative, until, elapsed, this.remaining, this.delay, this.completion);
      ;
      if (this.remaining) qput(this);
      else totdelay += this.delay;
      
 
    } while (!(this.remaining > 0 | qisempty()));
    
  } while (!(qisempty() & index == NJOB));
  
  long long int totwait, totservice=0;
  for (int i = 0; i < NJOB; i++)
    totservice += alljob[i].service;
  totwait = totdelay + totservice; 
  
  struct RESULT result = {
    policy, preemptive, "default", index, 
    ((float)this.arrival)/1000, ((float)this.completion)/1000, 
    ((float) totdelay)/1000, ((float) totservice)/1000, 
    ((float) totwait)/1000, init_seed, mu, lambda, rho, maxqlen
  };
 
  fprintf_result(result);
  
  return result;
}


// =============== Global Variables ===============

FILE *fstat;

void main(){
  queue = malloc(QMAX * sizeof(*queue));
  
  fstat= fopen("./results/stats.csv", "w");
  
  fprintf(fstat, "seed,lambda,mu,rho,policy_name,preemptive,"
    "last_index,last_arrival,last_completion,totdelay,totservice,"
    "totwait,interarr,wait,delay,service,innode,inq,util,maxqlen\n");
  
  struct RESULT result;
  
  for (double t_rho = 0.1; t_rho < 2.01; t_rho += 0.1){
    rho = t_rho;  mu = lambda * rho;
    
    for (long sd = 1; sd <= 1000000; sd *= 10){
      init_seed = sd; PlantSeeds(sd); 
      arrival = 0.0;
  
      struct JOB* alljob = malloc(NJOB * sizeof(struct JOB));  // alljob is a large array, so malloc is required
  
      for (int i = 0; i < NJOB; i++){
        alljob[i].id = i;
        alljob[i].arrival = GetArrival()*1000;
        alljob[i].service = GetService()*1000;
        alljob[i].remaining = alljob[i].service;
        alljob[i].completion = alljob[i].arrival;
        alljob[i].delay = 0;
      }
      
      // FCFS
      for (int i = 0; i < NJOB; i++)
        alljob[i].priority = alljob[i].arrival;
      result = simulate(alljob, FCFS, False);  // Non-preemptive
      result = simulate(alljob, FCFS, True);   // Preemptive
      
      // LCFS
      for (int i = 0; i < NJOB; i++)
        alljob[i].priority = -alljob[i].arrival;
      result = simulate(alljob, LCFS, False);
      result = simulate(alljob, LCFS, True);
      
      // SJF
      for (int i = 0; i < NJOB; i++)
        alljob[i].priority = alljob[i].service;
      result = simulate(alljob, SJF, False);
      result = simulate(alljob, SJF, True);
      
      // SRTF
      for (int i = 0; i < NJOB; i++)
        alljob[i].priority = alljob[i].remaining;
      result = simulate(alljob, SRTF, True);
      
      //PS
      SelectStream(2);
      for (int i = 0; i < NJOB; i++)
        alljob[i].priority = Random();
      result = simulate(alljob, PS, False);
      result = simulate(alljob, PS, True);
      
      free(alljob);
      fflush(fstat);
    }
 
  }
  
  fclose(fstat);
  printf("\n== Simulation Successful!\n");
  /*
  
  fprintf(fout, "arrival, service\n");
  for (int i = 0; i < NJOB; i++)
    fprintf(fout, "%ll, %llu\n", alljob[i].arrival, alljob[i].service);
  
  fclose(fout);
  */
}



void print_result(struct RESULT result) {
  const char *preemptive = result.preemptive ? "Preemptive" : "Non-preemptive";
  
  switch(result.policy) {
    case FCFS : result.policy_name = "FCFS"; break;
    case LCFS : result.policy_name = "LCFS"; break;
    case SJF  : result.policy_name = "SJF";  break;
    case SRTF : result.policy_name = "SRTF"; break;
    case PS   : result.policy_name = "PS";   break;
    default   : result.policy_name = "UNKNOWN POLICY"; 
  }
  
  printf("\n%s %s for %ld jobs\n", preemptive, result.policy_name, result.last_index);
  printf("   average interarrival time = %6.2f\n", result.last_arrival / result.last_index);
  printf("   average wait ............ = %6.2f\n", result.totwait / result.last_index);
  printf("   average delay ........... = %6.2f\n", result.totdelay / result.last_index);
  printf("   average service time .... = %6.2f\n", result.totservice / result.last_index);
  printf("   average # in the node ... = %6.2f\n", result.totwait / result.last_completion);
  printf("   average # in the queue .. = %6.2f\n", result.totdelay / result.last_completion);
  printf("   utilization ............. = %6.2f\n", result.totservice / result.last_completion);  
}

           
void fprintf_result(struct RESULT result){
  switch(result.policy) {
    case FCFS : result.policy_name = "FCFS"; break;
    case LCFS : result.policy_name = "LCFS"; break;
    case SJF  : result.policy_name = "SJF";  break;
    case SRTF : result.policy_name = "SRTF"; break;
    case PS   : result.policy_name = "PS";   break;
    default   : result.policy_name = "UNKNOWN POLICY"; 
  }
  
  fprintf(
    fstat, "%ld,%f,%f,%f,%s,%d,%lld,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%lld\n", 
    result.seed, result.lambda, result.mu, result.rho, 
    result.policy_name, result.preemptive, result.last_index,
    result.last_arrival, result.last_completion, 
    result.totdelay, result.totservice, result.totwait, 
    result.last_arrival / result.last_index,
    result.totwait / result.last_index, 
    result.totdelay / result.last_index,
    result.totservice / result.last_index,
    result.totwait / result.last_completion,
    result.totdelay / result.last_completion,
    result.totservice / result.last_completion,
    result.maxqlen
  );
}           
           
// ================= Min HEAP for JOB Queue ================

bool qisempty() { return inq == 0; }

bool qisfull() { return inq == QMAX; }

void qswap(int i, int j){
  struct JOB t = queue[i];
  queue[i] = queue[j];
  queue[j] = t;    
}

void heapifyup(){ 
  int child = inq-1, parent = child/2;
  while(parent >= 0 && queue[parent].priority > queue[child].priority) {
    qswap(parent, child);
    child = parent;
    parent = child/2;
  } 
}

void heapifydown(){ 
  int i = 0;
  while (i < inq){
    int min = i, left = 2*i+1, right = 2*i+2;
    if (left < inq && queue[min].priority > queue[left].priority) min = left;
    if (right < inq && queue[min].priority > queue[right].priority) min = right;
    if (min != i) qswap(min, i);
    else break;
    i = min;
  }
}

void qput(struct JOB JOB){
  queue[inq++] = JOB;
  heapifyup();
}

struct JOB qpeek(){ return queue[0]; }

struct JOB qget(){
    struct JOB job;
    if (qisempty()) return job;
    job = queue[0];
    inq--;
    qswap(0, inq);
    heapifydown();
    return job;
}

void qprint(){
  printf("\n  ID   Priority    Arrival    Srvc     Rem    Complet      Delay\n------------------------------------------------------------------\n");
  for (int i=0; i < inq; i++) {
    printf("%4lld  %9lld  %9lld  %6lld  %6lld  %9lld  %9lld\n",
      queue[i].id, queue[i].priority, queue[i].arrival, 
      queue[i].service, queue[i].remaining,
      queue[i].completion, queue[i].delay);
  }
    
}



