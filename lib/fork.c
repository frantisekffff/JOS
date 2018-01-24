// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//

// Obsulha stranok vypadku
extern void _pgfault_upcall(void);
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	// Kontrola chyboveho kodu err ci ide o chybu zapisu a ci je zaznam v tabulke perms COW , poradene na stranke cvika
	if (!((err & FEC_WR) && (uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_COW)))
		panic("not copy-on-write");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	// Alokovanie novej stranky na docasne miesto vo virtualnom adresnom priestore

	if (sys_page_alloc(0, PFTEMP, PTE_W|PTE_U|PTE_P) < 0)
		panic("sys_page_alloc");

	// Zaokruhlenie adresy nadol pre spravne pracovanie so strankami, neberie sa zaznam fault_va preto dole
	addr = ROUNDDOWN(addr,PGSIZE);

	// Prekopirovanie udaje z addr do docasnej na adresu PFTEMP
	memcpy(PFTEMP, addr, PGSIZE);

	// Namapujem novu stranku na staru adresu , tam kde ma byt s uz Write opravnenim...
	if (sys_page_map(0, PFTEMP, 0, addr, PTE_W|PTE_U|PTE_P)<0)
		panic("sys_page_map");

	// Odmapuje sa pomocna stranka zpamate
	if (sys_page_unmap(0, PFTEMP) < 0)
		panic("sys_page_unmap");

	return;



}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	//*********challenge***********

	int r;
	// LAB 4: Your code here.
	void *va = (void*)(pn<<PGSHIFT);
	if (uvpt[pn] & PTE_W || uvpt[pn] & PTE_COW){
		r = sys_page_map(0, va, envid, va, PTE_P | PTE_U | PTE_COW);
		if (r) return r;
		// if (uvpt[pn] & PTE_COW) return 0;
		r = sys_page_map(0, va, 0, va, PTE_P | PTE_U | PTE_COW);
	}else
		r = sys_page_map(0, va, envid, va, PTE_P | PTE_U);
	return r;
	
	//*********challenge***********

}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.

	envid_t envid;
	int ret;
	set_pgfault_handler(pgfault);
	envid = sys_exofork();
	if (envid < 0)
		return envid;
	if (!envid){
		// child
		thisenv = &envs[ENVX(sys_getenvid())];
		// cprintf("I'm a new process!! [..%x..]\n", thisenv->env_id);
		return 0;
	}
	// parent
	// scan through the address space
	int pgnum;
	for (pgnum = 0; pgnum < PGNUM(UXSTACKTOP-PGSIZE); ++pgnum){
		if (!(uvpd[pgnum>>(PDXSHIFT-PTXSHIFT)] & PTE_P) || !(uvpt[pgnum] & PTE_P))
			continue;	// skip empty entries
		if ((ret = duppage(envid, pgnum)) < 0)
			return ret;
	}
	
	// allocate child's exception stack and page fault handler
	if ((ret = sys_page_alloc(envid, (void*)(UXSTACKTOP-PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
		return ret;
	extern void _pgfault_upcall (void);
	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	if ((ret = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		return ret;
	return envid;

}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
