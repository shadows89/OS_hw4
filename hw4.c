#include <linux/ctype.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>

#include "hw4.h"
#define MY_MODULE "hw4"
#define BUF_SIZE 4096

MODULE_LICENSE( "GPL" );

int iNumDevices = 0;
MODULE_PARM( iKey, "i" );

/* globals */
int my_major = 0; 			    /* will hold the major # of my device driver */
char buffer[BUF_SIZE];          /* will hold buffer for device */
int first_byte_writen = 0;      /* index of first byte writen to buffer */
int last_byte_writen = 0;       /* index of last byte writen to buffer */
sem_t sem;                      /* semaphore to lock the module */

struct file_operations my_fops = {
	.open=		my_open,
	.release=	my_release,
	.read=		my_read,
	.write=		my_write,
	.llseek=		NULL,
	.ioctl=		my_ioctl,
	.owner=		THIS_MODULE,
};

struct file_operations my_fops2 = {
	.open=		my_open,
	.release=	my_release,
	.read=		my_read2,
	.write=		my_write2,
	.llseek=		NULL,
	.ioctl=		my_ioctl,
	.owner=		THIS_MODULE,
};


int init_module( void ) {
	my_major = register_chrdev( my_major, MY_MODULE, &my_fops );
	if( my_major < 0 ) {
		return my_major;
	}
	//do_init();
	if(seminit(&sem, 0, 1) != 0){    /* binary semaphore */
		unregister_chrdev( my_major, MY_MODULE);
		return -1;
	}
	printk("HELLO!\n");
	return 0;
}


void cleanup_module( void ) {

	unregister_chrdev( my_major, MY_MODULE);

    //do clean_up();
	semdestroy(&sem);
	printk("SE'YA!\n");
	return;
}


int my_open( struct inode *inode, struct file *filp ) {

	filp->private_data = allocate_private_data();

	if( filp->f_mode & FMODE_READ )
                //handle read opening
	if( filp->f_mode & FMODE_WRITE )
                //handle write opening

	MOD_INC_USE_COUNT; /*no need in 2.4 or later*/

	// if( filp->f_flags & O_NONBLOCK ) {
 //                //example of additional flag
	// }

	if (MINOR( inode->i_rdev )==1){
        filp->f_op = &my_fops2;
    }

	return 0;
}


int my_release( struct inode *inode, struct file *filp ) {
	if( filp->f_mode & FMODE_READ )
                //handle read close\ing
	if( filp->f_mode & FMODE_WRITE )
                //handle write closing

	MOD_DEC_USE_COUNT; /*no need in 2.4 or later*/

	return 0;
}


ssize_t my_read( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {
	//read the data
	//copy the data to user
    //return the ammount of read data
}


ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    //copy the data from user
	//write the data
    // return the ammount of written data
}


ssize_t my_read2( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {
	//read the data
	//copy the data to user
    //return the ammount of read data
}


ssize_t my_write2(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
        //copy the data from user
	//write the data
    //return the ammount of written data
}




int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {

	switch( cmd ) {
		case MY_OP1:
            //handle op1;
			break;

		case MY_OP2:
			//handle op2;
			break;

		default: return -ENOTTY;
	}
	return 0;
}

2




