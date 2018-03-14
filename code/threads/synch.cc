// synch.cc
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks
//   	and condition variables.
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Once we'e implemented one set of higher level atomic operations,
// we can implement others using that implementation.  We illustrate
// this by implementing locks and condition variables on top of
// semaphores, instead of directly enabling and disabling interrupts.
//
// Locks are implemented using a semaphore to keep track of
// whether the lock is held or not -- a semaphore value of 0 means
// the lock is busy; a semaphore value of 1 means the lock is free.
//
// The implementation of condition variables using semaphores is
// a bit trickier, as explained below under Condition::Wait.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "main.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List<Thread *>;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
  // Here are the bits from the simulator that you will need: the
  // interrupt handler and a reference to the current thread running.
    Interrupt *interrupt = kernel->interrupt;
    Thread *currentThread = kernel->currentThread;

    // disable interrupts
  IntStatus oldValue = interrupt -> SetLevel(IntOff);

    while (value == 0){//the process cannot enter the critical region
	queue -> Append(currentThread);//The process appends to the queue so it could be woken up when it is possible for it to run
	currentThread -> Sleep(false);// The process goes to sleep
}
	value--;//Decrement the value of the semaphore
    // re-enable interrupts
    interrupt -> SetLevel(oldValue);
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that interrupts
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
  // Here is the bit from the simulator that you will need.
    Interrupt *interrupt = kernel->interrupt;
     Thread*threadValue;
    // disable interrupts
	IntStatus oldValue = interrupt -> SetLevel(IntOff);
    // if the queue is not empty, take the first thread from the queue, wake it from sleep, and put it on the ready list


if (!queue -> IsEmpty()) {
    threadValue = queue->RemoveFront();
    kernel->scheduler->ReadyToRun(threadValue);
}
    value++;
    // re-enable interrupts
	interrupt -> SetLevel(oldValue);
}

//----------------------------------------------------------------------
// Semaphore::SelfTest, SelfTestHelper
// 	Test the semaphore implementation, by using a semaphore
//	to control two threads ping-ponging back and forth.
//----------------------------------------------------------------------

static Semaphore *ping;
static void
SelfTestHelper (Semaphore *pong)
{
    for (int i = 0; i < 10; i++) {
        ping->P();
	pong->V();
    }
}

void
Semaphore::SelfTest()
{
  Thread *helper = new Thread((char*)"ping");

    ASSERT(value == 0);		// otherwise test won't work!
    ping = new Semaphore((char*)"ping", 0);
    helper->Fork((VoidFunctionPtr) SelfTestHelper, this);
    for (int i = 0; i < 10; i++) {
        ping->V();
	this->P();
    }

   // professor_student_synch_test();
   cout<<" "<<endl;
    producer_consumer_test();
    delete ping;
}

//----------------------------------------------------------------------
// Lock::Lock
// 	Initialize a lock, so that it can be used for synchronization.
//	Initially, unlocked.
//
//	"debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------

Lock::Lock(char* debugName)
{
    name = debugName;
    semaphore = new Semaphore ((char*)"locksem", 1);
    lockHolder = NULL;
}

//----------------------------------------------------------------------
// Lock::~Lock
// 	Deallocate a lock
//----------------------------------------------------------------------
Lock::~Lock()
{
    delete semaphore;
}

//----------------------------------------------------------------------
// Lock::Acquire
//	Atomically wait until the lock is free, then set it to busy.
//	Equivalent to Semaphore::P(), with the semaphore value of 0
//	equal to busy, and semaphore value of 1 equal to free.
//----------------------------------------------------------------------


void Lock::Acquire()
{
//When a thred wants to acquire a lock, if the lock is busy, the thread has to wait  until the lock is free to get it
	semaphore -> P();
        lockHolder = kernel -> currentThread;


}

//----------------------------------------------------------------------
// Lock::Release
//	Atomically set lock to be free, waking up a thread waiting
//	for the lock, if any.
//	Equivalent to Semaphore::V(), with the semaphore value of 0
//	equal to busy, and semaphore value of 1 equal to free.
//
//	By convention, only the thread that acquired the lock
// 	may release it.
//---------------------------------------------------------------------

void Lock::Release()
{ Thread*currentThread = kernel -> currentThread;
 if (lockHolder == currentThread){ //if the current thread is the same as the one which acquired the lock, it can release that lock
    semaphore -> V();
    lockHolder = NULL;
}
}


//----------------------------------------------------------------------
// Condition::Condition
// 	Initialize a condition variable, so that it can be
//	used for synchronization.  Initially, no one is waiting
//	on the condition.
//
//	"debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------
Condition::Condition(char* debugName)
{
    name = debugName;
    waitQueue = new List<Semaphore*>;
}

//----------------------------------------------------------------------
// Condition::Condition
// 	Deallocate the data structures implementing a condition variable.
//----------------------------------------------------------------------

Condition::~Condition()
{
  delete waitQueue;
}

//----------------------------------------------------------------------
// Condition::Wait
// 	Atomically release monitor lock and go to sleep.
//	Our implementation uses semaphores to implement this, by
//	allocating a semaphore for each waiting thread.  The signaller
//	will V() this semaphore, so there is no chance the waiter
//	will miss the signal, even though the lock is released before
//	calling P().
//
//	Note: we assume Mesa-style semantics, which means that the
//	waiter must re-acquire the monitor lock when waking up.
//
//	"conditionLock" -- lock protecting the use of this condition
//----------------------------------------------------------------------

void Condition::Wait(Lock* conditionLock)
{
     Semaphore *waiter;
     if ( conditionLock -> IsHeldByCurrentThread()){
      waiter = new Semaphore((char*) "conditionsem", 0);
      waitQueue -> Append(waiter); //Append the waiter semaphore to the list of waiting threads so it can be woken up later
      conditionLock->Release();
      waiter->P();// The semaphore waiter is put to sleep, until it is possible to it to run
      conditionLock->Acquire();

     }

}

//----------------------------------------------------------------------
// Condition::Signal
// 	Wake up a thread waiting on this condition, if any.
//
//	Note: we assume Mesa-style semantics, which means that the
//	signaller doesn't give up control immediately to the thread
//	being woken up (unlike Hoare-style).
//
//	Also note: we assume the caller holds the monitor lock
//	(unlike what is described in Birrell's paper).  This allows
//	us to access waitQueue without disabling interrupts.
//
//	"conditionLock" -- lock protecting the use of this condition
//----------------------------------------------------------------------

void Condition::Signal(Lock* conditionLock)
{
    Semaphore *waiter;
   if (!conditionLock->IsHeldByCurrentThread() ) return; // check if the conditionLock is held by the current thread
   if (!waitQueue->IsEmpty()){
   waiter = waitQueue->RemoveFront(); // wake the waiter semaphore up because it can now run
   waiter->V(); // waiter is awake and will be put on the ready to run list of the scheduler
}
  conditionLock -> Release();
}

//----------------------------------------------------------------------
// Condition::Broadcast
// 	Wake up all threads waiting on this condition, if any.
//
//	"conditionLock" -- lock protecting the use of this condition
//----------------------------------------------------------------------

void Condition::Broadcast(Lock* conditionLock)
{
while (!waitQueue->IsEmpty()) {
        Signal(conditionLock);//wake all the semaphores in the waiting list
}
}
// Exercise 2: Synchronization Test
    Semaphore*askSemaphore;
    Semaphore*answerSemaphore;

void professor_student_synch_test(){
    askSemaphore = new Semaphore((char*) "ask Semaphore", 1); //Initialized to 1 because a student needs to be awake from start
	answerSemaphore = new Semaphore((char*)"answer Semaphore", 0); //Initialized to 0 because a student needs to ask a question first before the semaphore needs to wake up

	/*Creating threads for 3 students to ask questions and forking those threads with the student function to run in parallel
	with the main thread which will run the professor function. */
	Thread *student1 = new Thread((char*)"student1");
	student1->Fork((VoidFunctionPtr) student_function, (void *) 1);

    Thread *student2 = new Thread((char*)"student2");
	student2->Fork((VoidFunctionPtr) student_function, (void *) 2);

    Thread *student3 = new Thread((char*)"student3");
	student3->Fork((VoidFunctionPtr) student_function, (void *) 3);

    for(int i = 0; i < 12; i++){
        professor_function(); //
    }
	delete askSemaphore;
	delete answerSemaphore;

}


void answer_start(){
     Thread *currentThread = kernel->currentThread;
     answerSemaphore -> P();
    cout <<"The professor starts answering  a question.\n"<<endl;
	currentThread->Yield();
}


void answer_done(){
     Thread *currentThread = kernel->currentThread;
	askSemaphore -> V();
    cout <<"The professor is finished with the answer.\n"<<endl;
	currentThread->Yield();
}

void question_start(int studentID){
     Thread *currentThread = kernel->currentThread;
      askSemaphore -> P();
       cout <<"The student "<<studentID<<" is ready to ask a question.\n"<<endl;


     currentThread->Yield();
}


void question_done(int studentID){
     Thread *currentThread = kernel->currentThread;
     cout <<"The student "<<studentID<<" is finished asking the question.\n"<<endl;
     answerSemaphore -> V();

     currentThread->Yield();
}

void student_function(int studentID){
    for (int i = 0; i < 4 ; i++){
                question_start(studentID);
                question_done(studentID);
}

}
void professor_function(){
		answer_start();
		answer_done();
}

// Exercise: Synchronization Exercise 3
int buffsize = 3;
string buffer[3];
string word[9] = {"I", " ", "d","i","d"," ","i","t","!"};
Semaphore*producerSemaphore;
Semaphore*consumerSemaphore;
Lock*producerLock;
Lock*consumerLock;
int consumerIndex = 0;
int producerIndex = 0;

void producer_consumer_test(){
    producerSemaphore = new Semaphore((char*)"Producer Semaphore", 3);
    consumerSemaphore = new Semaphore((char*) "Consumer Semaphore", 0);

    producerLock = new Lock((char*)"Producer Lock");
    consumerLock = new Lock((char*)"Consumer Lock");

    Thread*consumerThread = new Thread((char*)"consumer thread");
    consumerThread->Fork((VoidFunctionPtr)consumer, (void*)1);

    Thread *consumerTwo = new Thread((char*)"consumer two");
	consumerTwo->Fork((VoidFunctionPtr) consumer, (void *) 2);

	//Thread*producerTwo = new Thread((char*)"producer two");
	//producerTwo->Fork((VoidFunctionPtr)producer, (void*)2);

    producer(1);
}
void producer(int producerID){
    for (int i = 0; i < 9; i++){
        producerSemaphore -> P();
        //producerLock ->Acquire();
        buffer[producerIndex] = word[i]; // adds characters from the word array into the buffer one at a time
        cout <<"Producer " << producerID<< "-> An element was produced: "<<buffer[producerIndex]<<endl;
        producerIndex = (producerIndex+1)%buffsize; //incrementing the producer index in a way to prevent buffer overflow
        producerLock -> Release();
        consumerSemaphore -> V();
        kernel->currentThread -> Yield();
    }
}
void consumer(int consumerID){
    for (int i = 0; i < 9; i++){
        consumerSemaphore -> P();
        consumerLock ->Acquire();
        cout<<"Consumer "<<consumerID<<"->Element consumed:" << buffer[consumerIndex]<<endl;
        cout<<" "<<endl;
        consumerIndex = (consumerIndex+1)%buffsize;
        consumerLock -> Release();
        producerSemaphore -> V();
        kernel -> currentThread -> Yield();

    }

}

/*For this assignment, we only got the chance to synchronize multiple consumers.
We would really appreciate an extension to implement multiple producers and finish the last section.*/

