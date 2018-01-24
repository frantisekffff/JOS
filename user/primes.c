// Concurrent version of prime sieve of Eratosthenes.
// Invented by Doug McIlroy, inventor of Unix pipes.
// See http://swtch.com/~rsc/thread/.
// The picture halfway down the page and the text surrounding it
// explain what's going on here.
//
// Since NENV is 1024, we can print 1022 primes before running out.
// The remaining two environments are the integer generator at the bottom
// of main and user/idle.

#include <inc/lib.h>

unsigned
primeproc(void)
{
	int i, id, p;
	envid_t envid;

	//*********challenge***********
	int msg_id = 0;
	//*********challenge***********

	// fetch a prime from our left neighbor
top:
	p = ipc_recv(&envid, 0, 0);
	//cprintf("CPU %d: %d ", thisenv->env_cpunum, p);
	
	//*********challenge***********
	cprintf("CPU %d: %d \n", thisenv->env_cpunum, p&0xffff);
	//*********challenge***********	

	// fork a right neighbor to continue the chain
	if ((id = fork()) < 0)
		panic("fork: %e", id);
	if (id == 0)
		goto top;

	// filter out multiples of our prime
	while (1) {
		//*********challenge***********	
		i = ipc_recv(&envid, 0, 0) & 0xffff;
		//*********challenge***********	

		if (i % p)
		
			//*********challenge***********	
			ipc_send(id, i | ((msg_id++) << 16), 0, 0);
			//*********challenge***********	
	}
}

void
umain(int argc, char **argv)
{
	int i, id;

	// fork the first prime process in the chain
	if ((id = fork()) < 0)
		panic("fork: %e", id);
	if (id == 0)
		primeproc();

	// feed all the integers through
	for (i = 2; ; i++)
		ipc_send(id, i, 0, 0);
}

