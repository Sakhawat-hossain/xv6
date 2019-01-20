#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sock.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

//
// TODO: Create a structure to maintain a list of sockets
// Should it have locking?
//
struct {
  struct spinlock lock;
  struct sock sock[NSOCK];
} stable;
struct {
  struct spinlock lock;
  struct port port[NPORT];
} ptable;

void
sinit(void)
{
  //
  // TODO: Write any initialization code for socket API
  // initialization.
  //
  initlock(&stable.lock, "stable");
  initlock(&ptable.lock, "ptable");
}

int
listen(int lport) {

  //
  // TODO: Put the actual implementation of listen here.
  //
	acquire(&stable.lock);
	acquire(&ptable.lock);
	
	if(ptable.port[lport].state==BOUND){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}

	//
	int i;
	for (i = 0; i < NSOCK; ++i) {
    		if (stable.sock[i].state==CLOSED)
			break;
  	}
	if(i==NSOCK){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}

	ptable.port[lport].state=BOUND;
	stable.sock[i].state=LISTENING;
        stable.sock[i].localport=lport;
	stable.sock[i].owner=myproc()->pid;

	release(&ptable.lock);
	release(&stable.lock);

	return 0;
}

int
connect(int rport, const char* host) {
  //
  // TODO: Put the actual implementation of connect here.
  //
	acquire(&stable.lock);
	acquire(&ptable.lock);

	if(ptable.port[rport].state==UNBOUND){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}
	int i;
        for (i = 0; i < NSOCK; ++i) {
    		if (stable.sock[i].state==LISTENING && stable.sock[i].localport==rport)
			break;
  	}
	if(i==NSOCK){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}
	//int rspos=i;

	for (i = 0; i < NSOCK; ++i) {
    		if (stable.sock[i].state==CLOSED)
			break;
  	}
	if(i==NSOCK){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}
	int j=i;
	for (i = 0; i < NPORT; ++i) {
    		if (ptable.port[i].state==UNBOUND)
			break;
  	}
	if(i==NPORT){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}
	ptable.port[i].state=BOUND;

	stable.sock[j].state=CONNECTED;
        stable.sock[j].localport=i;
        stable.sock[j].remoteport=rport;
	stable.sock[j].owner=myproc()->pid;
	stable.sock[j].dataAvailable=0;

	release(&ptable.lock);
	release(&stable.lock);

	return i;
}

int
send(int lport, const char* data, int n) {
  //
  // TODO: Put the actual implementation of send here.
  //
	acquire(&stable.lock);
	acquire(&ptable.lock);

	//if(ptable.port[rport].state==CLOSED)
	//	return -1;
	int i;
        for (i = 0; i < NSOCK; ++i) {
    		if (stable.sock[i].state==CONNECTED && stable.sock[i].localport==lport)
			break;
  	}
	if(i==NSOCK){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}
	int lspos=i;

	int rport=stable.sock[i].remoteport;
	for (i = 0; i < NSOCK; ++i) {
    		if (stable.sock[i].state==CONNECTED && stable.sock[i].localport==rport)
			break;
  	}
	if(i==NSOCK){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}
	//int rspos=i;
	int f=0;
	while(1){
		if(stable.sock[i].dataAvailable==1){
			sleep(&stable.sock[lspos],&stable.lock);
			f=1;
		}
		else
			break;
	}
	strncpy(stable.sock[i].buf,data,n);
	stable.sock[lspos].dataAvailable=0;
	//stable.sock[i].dataAvailable=1;
	if(f==1)
		wakeup(&stable.sock[lspos]);

	release(&ptable.lock);
	release(&stable.lock);

  	return 0;
}


int
recv(int lport, char* data, int n) {
  //
  // TODO: Put the actual implementation of recv here.
  //
	acquire(&stable.lock);
	acquire(&ptable.lock);

	//if(ptable.port[rport].state==CLOSED)
	//	return -1;
	int i;
        for (i = 0; i < NSOCK; ++i) {
    		if (stable.sock[i].state==CONNECTED && stable.sock[i].localport==lport)
			break;
  	}
	if(i==NSOCK){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}
	int lspos=i;

	int rport=stable.sock[i].remoteport;
	for (i = 0; i < NSOCK; ++i) {
    		if (stable.sock[i].state==CONNECTED && stable.sock[i].localport==rport)
			break;
  	}
	if(i==NSOCK){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}
	//int rspos=i;
	int f=0;
	while(1){
		if(stable.sock[i].dataAvailable==0){
			sleep(&stable.sock[lspos],&stable.lock);
			f=1;
		}
		else
			break;
	}
	strncpy(data,stable.sock[i].buf,n);
	stable.sock[lspos].dataAvailable=1;
	if(f==1)
		wakeup(&stable.sock[lspos]);

	release(&ptable.lock);
	release(&stable.lock);

  	
  return 0;
}

int
disconnect(int lport) {
  //
  // TODO: Put the actual implementation of disconnect here.
  //
	acquire(&stable.lock);
	acquire(&ptable.lock);

	int i;
        for (i = 0; i < NSOCK; ++i) {
    		if (stable.sock[i].localport==lport)
			break;
  	}
	if(i==NSOCK){
		release(&ptable.lock);
		release(&stable.lock);
		return -1;
	}
	
	ptable.port[lport].state=UNBOUND;

	stable.sock[i].state=CLOSED;
        stable.sock[i].localport=0;
        stable.sock[i].remoteport=0;
	stable.sock[i].owner=0;
	stable.sock[i].dataAvailable=0;

	release(&ptable.lock);
	release(&stable.lock);

	return 0;
}
