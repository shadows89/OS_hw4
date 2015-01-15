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
#include <encryptor.h>


#include "hw4.h"
#define MY_MODULE "hw4"
#define BUF_SIZE 4096

MODULE_LICENSE( "GPL" );

int iKey = 0;
MODULE_PARM( iKey, "i" );

/**********************************************************************************************/
typedef struct buff{
	char* buff;
	int size_of_buff;
	int first;
	int last;
	int buff_full;
} Buff;

/* init buffer */
Buff* init_Buff(int size); 

/* destroy buffer */
void free_Buff(Buff* buff);

/* return size of available data to read */
int available_data_Buff(Buff* buff);

/* return empty space in buff  for writing*/
int available_space_Buff(Buff* buff);  

/* write to buff */
/* return amount of data writen */
int write_Buff(Buff* buff,char* source, int count);

/* read from buff */
/* return amount of data read */
int read_Buff(Buff* buff,char* target, int count);
/**********************************************************************************************/

typedef struct semaphore sem_t;

/* globals */
int my_major = 0; 			    /* will hold the major # of my device driver */
Buff* my_buff;
sem_t sem;                      /* semaphore to lock the module */
// int dec_num_of_readers = 0;
// int dec_num_of_writers = 0;
// int enc_num_of_readers = 0;
// int en_num_of_writers = 0;

typedef struct rw_count{
	int iKey;
	int num_of_writers;
	int num_of_readers; 
} RW_count;

int my_open( struct inode *inode, struct file *filp );
int my_release( struct inode *inode, struct file *filp );
ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
ssize_t my_write2(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
ssize_t my_read( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t my_read2( struct file *filp, char *buf, size_t count, loff_t *f_pos );
int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
loff_t my_llseek(struct file *filp, loff_t f_pos,int i);

struct file_operations my_fops = {
	.open=		my_open,
	.release=	my_release,
	.read=		my_read,
	.write=		my_write,
	.llseek=	my_llseek,
	.ioctl=		my_ioctl,
};

struct file_operations my_fops2 = {
	.open=		my_open,
	.release=	my_release,
	.read=		my_read2,
	.write=		my_write2,
	.llseek=	my_llseek,
	.ioctl=		my_ioctl,
};


int init_module( void ) {
	SET_MODULE_OWNER(&my_fops);
	my_major = register_chrdev( my_major, MY_MODULE, &my_fops );
	if( my_major < 0 ) {
		return my_major;
	}
	printk("Major: %d\n",my_major);
	//do_init();
	init_MUTEX(&sem);    /* binary semaphore */
	my_buff = init_Buff(BUF_SIZE);
	if(my_buff == NULL)
		return -1;
	return 0;
}


void cleanup_module( void ) {

	unregister_chrdev( my_major, MY_MODULE);

    //do clean_up();
    free_Buff(my_buff);
	return;
}


int my_open( struct inode *inode, struct file *filp ) {
	int minor = MINOR(inode->i_rdev); 
	if(minor != 1 && minor != 0)
		return -EINVAL;
	if(filp->private_data == NULL){
		filp->private_data = kmalloc(sizeof(RW_count),GFP_KERNEL);
		((RW_count*)(filp->private_data))->iKey = iKey;
		((RW_count*)(filp->private_data))->num_of_writers = 0;
		((RW_count*)(filp->private_data))->num_of_readers = 0;
	}
	// if( filp->f_mode & FMODE_READ )
	// 	filp->private_data->num_of_readers++;
	// if( filp->f_mode & FMODE_WRITE )
	// 	filp->private_data->num_of_writers++;

    //printk("Open--> readers: %d , writers: %d\n",num_of_reader,num_of_writer);
	//MOD_INC_USE_COUNT; 
	/*no need in 2.4 or later*/

	// if( filp->f_flags & O_NONBLOCK ) {
 //                //example of additional flag
	// }

	if (minor == 1){
        filp->f_op = &my_fops2;
    }

	return 0;
}


int my_release( struct inode *inode, struct file *filp ) {
	int minor = MINOR(inode->i_rdev); 
	if(minor != 1 && minor != 0)
		return -EINVAL;
	// if( filp->f_mode & FMODE_READ )
	// 	filp->private_data->num_of_readers--;
	// if( filp->f_mode & FMODE_WRITE )
	// 	filp->private_data->num_of_writers--;

	//if(filp->private_data->num_of_writers)
	//printk("Close--> readers: %d , writers: %d\n",num_of_reader,num_of_writer);
	//MOD_DEC_USE_COUNT; /*no need in 2.4 or later*/

	return 0;
}


ssize_t my_read( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {  /* decryptor */
	if(((RW_count*)(filp->private_data))->num_of_writers == 0 || available_data_Buff(my_buff) == 0)
		return 0;
	if(count % 8 != 0)
		return -EINVAL;
	int data_read = 0;
	char* tmp_buff = NULL;
	char* decrypted_data = NULL;
	tmp_buff = kmalloc(sizeof(char)*count,GFP_KERNEL);
	if(tmp_buff == NULL)
		return -EFAULT;
	decrypted_data = kmalloc(sizeof(char)*count,GFP_KERNEL);
	if(decrypted_data == NULL){
		kfree(tmp_buff);
		return -EFAULT;
	}
	data_read = read_Buff(my_buff,tmp_buff,count);
	encryptor(tmp_buff, decrypted_data, count, ((RW_count*)(filp->private_data))->iKey, DECRYPT);
	copy_to_user(buf,decrypted_data,sizeof(char)*count);
	kfree(tmp_buff);
	kfree(decrypted_data);
    return data_read;
}


ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {  /* decryptor */
    //copy the data from user
	//write the data
    // return the ammount of written data
	if(((RW_count*)(filp->private_data))->num_of_readers == 0 || available_space_Buff(my_buff) == 0)
		return 0;
	if(count % 8 != 0)
		return -EINVAL;
	char* tmp_buff = NULL;
	int data_writen = 0;
	tmp_buff = kmalloc(sizeof(char)*count,GFP_KERNEL);
	if(tmp_buff == NULL)
		return -EFAULT;
	copy_from_user(tmp_buff,buf,sizeof(char)*count);
	data_writen = write_Buff(my_buff, tmp_buff, count);
	kfree(tmp_buff);
	return data_writen;
}


ssize_t my_read2( struct file *filp, char *buf, size_t count, loff_t *f_pos ) { /* encryptor */
	//read the data
	//copy the data to user
    //return the ammount of read data
	if(((RW_count*)(filp->private_data))->num_of_writers == 0 || available_data_Buff(my_buff) == 0)
		return 0;
	if(count % 8 != 0)
		return -EINVAL;
	char* tmp_buff = NULL;
	int data_read = 0;
	tmp_buff = kmalloc(sizeof(char)*count,GFP_KERNEL);
	if(tmp_buff == NULL)
		return -EFAULT;
	data_read = read_Buff(my_buff, tmp_buff, count);
	copy_to_user(buf,tmp_buff,sizeof(char)*count);
	kfree(tmp_buff);
	return data_read;
}


ssize_t my_write2(struct file *filp, const char *buf, size_t count, loff_t *f_pos) { /* encryptor */
        //copy the data from user
	//write the data
    //return the ammount of written data
	if(((RW_count*)(filp->private_data))->num_of_readers == 0 || available_space_Buff(my_buff) == 0)
		return 0;
	if(count % 8 != 0)
		return -EINVAL;
	int data_writen = 0;
	char* tmp_buff = NULL;
	char* encrypted_data = NULL;
	tmp_buff = kmalloc(sizeof(char)*count,GFP_KERNEL);
	if(tmp_buff == NULL)
		return -EFAULT;
	copy_to_user(tmp_buff,buf,sizeof(char)*count);
	encrypted_data = kmalloc(sizeof(char)*count,GFP_KERNEL);
	if(encrypted_data == NULL){
		kfree(tmp_buff);
		return -EFAULT;
	}
	encryptor(tmp_buff, encrypted_data, count, ((RW_count*)(filp->private_data))->iKey, ENCRYPT);
	data_writen = write_Buff(my_buff,tmp_buff,count);
	kfree(tmp_buff);
	kfree(encrypted_data);
    return data_writen;
}


loff_t my_llseek(struct file *filp, loff_t f_pos,int i){
	return -ENOSYS;
}

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) { 
	if(cmd != HW4_SET_KEY)
		return -ENOTTY;
	((RW_count*)(filp->private_data))->iKey = (int)arg;	
	return 0;
}


/**********************************************************************************************/
Buff* init_Buff(int size) {
	Buff* buffer = kmalloc(sizeof(Buff),GFP_KERNEL);
	if (buffer == NULL) {
		return NULL;
	}
	buffer->buff = kmalloc(sizeof(char)*size,GFP_KERNEL);
	if (buffer->buff == NULL) {
		kfree(buffer);
	}
	buffer->first = 0;
	buffer->last = 0;
	buffer->size_of_buff = size;
	buffer->buff_full = 0;
	return buffer;
}

void free_Buff(Buff* buff) {
	if (buff == NULL) {
		return;
	}
	kfree(buff->buff);
	kfree(buff);
}

int available_data_Buff(Buff* buff) {
	if (buff->first < buff->last) {
		return buff->last - buff->first;
	} else if (buff->first > buff->last) {
		return buff->size_of_buff - buff->first + buff->last;
	} else if ((buff->first == buff->last) && (buff->buff_full == 1)) {
		return buff->size_of_buff;
	} else if ((buff->first == buff->last) && (buff->buff_full == 0)) {
		return 0;
	}
	return -1; // We shouldn't get here!!!!!!!
}

int available_space_Buff(Buff* buff) {
	return buff->size_of_buff - available_data_Buff(buff);
}

int write_Buff(Buff* buff, char* source, int count) {
	int i, bytes_to_write;
	int bytes_available = available_space_Buff(buff);
	bytes_to_write = count > bytes_available ? bytes_available : count;
	if (bytes_available % 8 != 0 || bytes_to_write % 8 != 0) {
		return -1; // We shouldn't get here!!!!!!!
	}
	for (i = 0; i < bytes_to_write; i++) {
		buff->buff[buff->last] = source[i];
		buff->last = (buff->last + 1) % buff->size_of_buff;
	}
	return bytes_to_write;
}

int read_Buff(Buff* buff, char* target, int count) {
	int i, bytes_to_read;
	int bytes_available = available_data_Buff(buff);
	bytes_to_read = count > bytes_available ? bytes_available : count;
	if (bytes_available % 8 != 0 || bytes_to_read % 8 != 0) {
		return -1; // We shouldn't get here!!!!!!!
	}
	for (i = 0; i < bytes_to_read; i++) {
		target[i] = buff->buff[buff->first];
		buff->first = (buff->first + 1) % buff->size_of_buff;
	}
	return bytes_to_read;
}
/**********************************************************************************************/
