// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two with fork.

#include <inc/lib.h>

#define MAXNUM 100
void
umain(int argc, char **argv)
{
	envid_t who;

	if ((who = fork()) != 0) {       // fork nam vrati ID dietata
		cprintf("send 0 from env %x to env %x\n", sys_getenvid(), who);
		ipc_send(who, 0, 0, 0);   //posle 0 dietatu
	}

	while (1) {
		uint32_t i = ipc_recv(&who, 0, 0);   //prijatie spravy dietatom
		cprintf("%x got %d from %x\n", sys_getenvid(), i, who);
		if (i == MAXNUM)
			return;
		i++;
		ipc_send(who, i, 0, 0);
		cprintf("send %x from env %x to env %x\n", i, sys_getenvid(), who);
		if (i == MAXNUM)
			return;
	}

}
