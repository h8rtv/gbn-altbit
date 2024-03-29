// Autor das edições: Heitor Tonel Ventura
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
    };

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#define A 0
#define B 1

//#define CUMULATIVE

#define TIMEOUT  20.f
#define BUF_SIZE 50
#define WINDOW_SIZE 8
#define SEQ_SPACE (2 * WINDOW_SIZE)

#define max(a,b) (((a) > (b)) ? (a) : (b))

struct pkt_queue {
  struct pkt* q;
  int queue_size;
  int front; 
  int back; 
};

struct sender {
  int seq;
  int timer_running;
  struct pkt_queue window;
};

struct rcver {
  int seq;
};

struct sender a_sender;
struct rcver b_rcver;
struct pkt_queue a_buffer;

int is_nack(struct pkt* packet) {
  return strcmp(packet->payload, "NACK") == 0;
}

void make_nack(struct pkt* packet) {
  strcpy(packet->payload, "NACK");
}


void init_queue(struct pkt_queue* queue, int size)
{
  queue->queue_size = size;
  queue->back = -1;
  queue->front = -1;
  queue->q = (struct pkt*) malloc(size * sizeof(struct pkt));
}

int is_empty(struct pkt_queue* queue)
{
  return queue->front == -1;
}

int is_full(queue)
  struct pkt_queue* queue;
{
  return ((queue->front == 0 && queue->back >= queue->queue_size - 1) || queue->front == queue->back + 1);
}

int queue_next(struct pkt_queue* queue, int current)
{
  if (current == queue->back || is_empty(queue)) 
    return queue->queue_size;

  return (current + 1) % queue->queue_size;
}

int queue_start(struct pkt_queue* queue)
{
  if (queue->front == -1) {
    return queue->queue_size;
  }
  return queue->front;
}

int queue_end(struct pkt_queue* queue)
{
  return queue->queue_size;
}

int queue_length(struct pkt_queue* queue)
{
  if (queue->front == -1)
    return 0;

  if (queue->front > queue->back)
    return (queue->queue_size - queue->front) + queue->back + 1;

  return queue->back - queue->front + 1;
}

void destroy_queue(struct pkt_queue* queue)
{
  free(queue->q);
}

void enqueue(struct pkt_queue* queue, struct pkt item)
{
  if (is_full(queue))
      return;
  if(queue->front == -1)
  {
    queue->front = 0;
    queue->back = 0;
  } else if (queue->back < queue->queue_size - 1) {
    queue->back++;
  } else {
    queue->back = 0;
  }

  queue->q[queue->back] = item;
}

struct pkt dequeue(queue)
  struct pkt_queue* queue;
{
  if (queue->front == -1) {
    struct pkt empty;
    return empty;
  }
  int front = queue->front; 

  if (queue->front == queue->back) {
    queue->front = -1; 
    queue->back = -1; 
  } else if (queue->front == queue->queue_size - 1) {
    queue->front = 0; 
  } else {
    queue->front++; 
  }
  return queue->q[front];
}

struct pkt print_queue(struct pkt_queue* queue, int payload)
{
  for (int i = queue_start(queue); i != queue_end(queue); i = queue_next(queue, i)) {
    if (payload) {
      printf("%c ", queue->q[i].payload[0]);
    } else {
      printf("%d ", queue->q[i].seqnum);
    }
  }
  printf("\n");
}

int calc_checksum(struct pkt* packet)
{
  int checksum = packet->seqnum + packet->acknum;
  for (int i = 0; i < 20; i++) {
    checksum += (int)packet->payload[i];
  }
  return checksum;
}

void set_checksum(struct pkt* packet)
{
  packet->checksum = calc_checksum(packet);
}

int is_corrupted(struct pkt* packet)
{
  return calc_checksum(packet) != packet->checksum;
}

int next_seq(int prev_seq)
{
  return (prev_seq + 1) % SEQ_SPACE;
}

int prev_seq(int next_seq)
{
  if (next_seq == 0) return SEQ_SPACE - 1;
  return next_seq - 1;
}


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  struct pkt packet;
  printf("Para enviar: %.20s\n", message.data);
  strcpy(packet.payload, message.data);
  enqueue(&a_buffer, packet);

  if (!is_full(&a_sender.window)) {
    if (a_sender.timer_running) {
      stoptimer(A);
      a_sender.timer_running = 0;
    }
    while (!is_full(&a_sender.window) && !is_empty(&a_buffer)) {
      packet = dequeue(&a_buffer);
      packet.seqnum = a_sender.seq;
      a_sender.seq = next_seq(a_sender.seq);
      packet.acknum = 0;
      set_checksum(&packet);
      tolayer3(A, packet);
      enqueue(&a_sender.window, packet);

      printf("Enviado: %.20s\n", packet.payload);
      printf("A_output SENT: seqnum: %d, acknum: %d, checksum: %d, payload: %.20s\n", packet.seqnum, packet.acknum, packet.checksum, packet.payload);
    }

    printf("A_output Janela atual(enq): ");
    print_queue(&a_sender.window, 0);
    printf("A_output Buffer Atual: ");
    print_queue(&a_buffer, 1);
    starttimer(A, TIMEOUT);
    a_sender.timer_running = 1;
  } else {
    printf("Janela cheia, %.20s bufferizado.\n", message.data);
    printf("A_output Janela atual(enq): ");
    print_queue(&a_sender.window, 0);
    printf("A_output Buffer Atual: ");
    print_queue(&a_buffer, 1);
  }
}

void B_output(struct msg message)  /* need be completed only for extra credit */
{
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  printf("A_input RCVD: seqnum: %d, acknum: %d, checksum: %d, payload: %.20s\n", packet.seqnum, packet.acknum, packet.checksum, packet.payload);
  if (is_corrupted(&packet)) {
    printf("A_input detectou pacote corrompido.\n");
  } else if (is_nack(&packet)) {
    printf("A_input recebeu um NACK.\n");
  } else {
#ifdef CUMULATIVE
    int count = 0;
    int found = 0;
    for (int i = queue_start(&a_sender.window); i != queue_end(&a_sender.window); i = queue_next(&a_sender.window, i)) {
      struct pkt cached_packet = a_sender.window.q[i];
      count++;
      if (cached_packet.seqnum == packet.acknum) {
        found = 1;
        break;
      }
    }
    if (found) {
      printf("A_input pacote(s): ");
      while (count > 0) {
        struct pkt rec = dequeue(&a_sender.window);
        printf("%d ", rec.seqnum);
        count--;
      }
      printf("reconhecido(s).\n");
    }
#else
    int front = queue_start(&a_sender.window);
    struct pkt front_packet = a_sender.window.q[front];
    if (front_packet.seqnum == packet.acknum) {
      struct pkt rec = dequeue(&a_sender.window);
      printf("A_input pacote %d reconhecido.\n", packet.acknum);
    }
#endif
    stoptimer(A);
    a_sender.timer_running = 0;
    if (!is_empty(&a_sender.window)) {
      starttimer(A, TIMEOUT);
      a_sender.timer_running = 1;
    }
    printf("A_input Janela atual (deq): ");
    print_queue(&a_sender.window, 0);
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  for (int i = queue_start(&a_sender.window); i != queue_end(&a_sender.window); i = queue_next(&a_sender.window, i)) {
    struct pkt resend_pkt = a_sender.window.q[i];
    printf("Reenviado (timeout): %.20s\n", resend_pkt.payload);
    printf("A_timerinterrupt SENT: seqnum: %d, acknum: %d, checksum: %d, payload: %.20s\n", resend_pkt.seqnum, resend_pkt.acknum, resend_pkt.checksum, resend_pkt.payload);
    tolayer3(A, resend_pkt);
  }
  starttimer(A, TIMEOUT);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  init_queue(&a_buffer, BUF_SIZE);
  init_queue(&a_sender.window, WINDOW_SIZE);
  a_sender.seq = 0;
  a_sender.timer_running = 0;
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  struct pkt ack;
  memset(ack.payload, 0, 20);
  ack.seqnum = 0;

  if (is_corrupted(&packet)) {
    printf("B_input detectou pacote corrompido.\n");
    ack.acknum = prev_seq(b_rcver.seq);
    make_nack(&ack);
  } else if (packet.seqnum != b_rcver.seq) {
    printf("B_input detectou pacote não esperado. Esperava %d e recebeu %d.\n", b_rcver.seq, packet.seqnum);
#ifdef CUMULATIVE
    ack.acknum = prev_seq(b_rcver.seq);
#else
    ack.acknum = packet.seqnum;
#endif
  } else {
    printf("Recebido: %.20s\n", packet.payload);
    ack.acknum = packet.seqnum;
    tolayer5(B, packet.payload);
    b_rcver.seq = next_seq(b_rcver.seq);
  }

  set_checksum(&ack);
  tolayer3(B, ack);

  printf("B_input RCVD: seqnum: %d, acknum: %d, checksum: %d, payload: %.20s\n", packet.seqnum, packet.acknum, packet.checksum, packet.payload);
  printf("B_input SENT: seqnum: %d, acknum: %d, checksum: %d, payload: %.20s\n", ack.seqnum, ack.acknum, ack.checksum, ack.payload);
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  b_rcver.seq = 0;
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */   
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c; 
  
   init();
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
}



init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(1);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} 


insertevent(p)
   struct event *p;
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


starttimer(AorB,increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

 struct event *q;
 struct event *evptr;

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}
