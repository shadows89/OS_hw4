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
Buff* my_buff2;
sem_t writers_queue_dec;                      /* semaphore as a wait queue for writers dec */
sem_t readers_queue_dec;                      /* semaphore as a wait queue for readers dec */
sem_t writers_queue_enc;                      /* semaphore as a wait queue for writers enc */
sem_t readers_queue_enc;                      /* semaphore as a wait queue for readers enc */
int enc_num_of_writers = 0;
int enc_num_of_readers = 0;
int dec_num_of_writers = 0;
int dec_num_of_readers = 0;

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
	.owner=		THIS_MODULE,
};

struct file_operations my_fops2 = {
	.open=		my_open,
	.release=	my_release,
	.read=		my_read2,
	.write=		my_write2,
	.llseek=	my_llseek,
	.ioctl=		my_ioctl,
	.owner=		THIS_MODULE,
};


int init_module( void ) {
	SET_MODULE_OWNER(&my_fops);
	my_major = register_chrdev( my_major, MY_MODULE, &my_fops );
	if( my_major < 0 ) {
		return my_major;
	}
	//printk("Major: %d\n",my_major); // TODO Remove bleaaaat
	//do_init();
	init_MUTEX(&writers_queue_dec);    /* binary semaphore */
	init_MUTEX(&readers_queue_dec);    /* binary semaphore */
	init_MUTEX(&writers_queue_enc);    /* binary semaphore */
	init_MUTEX(&readers_queue_enc);    /* binary semaphore */
	down_interruptible(&readers_queue_dec);
	down_interruptible(&readers_queue_enc);
	my_buff = init_Buff(BUF_SIZE);
	if(my_buff == NULL)
		return -1;
	my_buff2 = init_Buff(BUF_SIZE);
	if(my_buff2 == NULL){
		free_Buff(my_buff2);
		return -1;
	}
	return 0;
}


void cleanup_module( void ) {

	unregister_chrdev( my_major, MY_MODULE);

    //do clean_up();
    free_Buff(my_buff);
    free_Buff(my_buff2);
	return;
}


int my_open( struct inode *inode, struct file *filp ) {
	int minor = MINOR(inode->i_rdev); 
	if(minor != 1 && minor != 0)
		return -EINVAL;
	//if(filp->private_data == NULL){
	filp->private_data = kmalloc(sizeof(int),GFP_KERNEL);
		*((int*)(filp->private_data)) = iKey;

	
	if( filp->f_mode & FMODE_READ )
		minor == 1 ? enc_num_of_readers++ : dec_num_of_readers++ ;
	if( filp->f_mode & FMODE_WRITE )
		minor == 1 ? enc_num_of_writers++ : dec_num_of_writers++ ;
	// else if(filp->f_mode & O_RDWR){
	// 	num_of_writers++;
	// 	num_of_readers++;
	// }
	//printk("Opened. R: %d, W: %d\n", num_of_readers, num_of_writers);

    //printk("Open--> readers: %d , writers: %d\n",num_of_readers,num_of_writers);
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


int my_release(struct inode *inode, struct file *filp ) {
	int minor = MINOR(inode->i_rdev);
	int writers = 0;
	int readers = 0;
	sem_t* q_write;
	sem_t* q_read;
	if(minor != 1 && minor != 0)
		return -EINVAL;
	if( filp->f_mode & FMODE_READ )
		minor == 1 ? enc_num_of_readers-- : dec_num_of_readers-- ;
	if( filp->f_mode & FMODE_WRITE )
		minor == 1 ? enc_num_of_writers-- : dec_num_of_writers-- ;


	if(minor == 0){
		readers = dec_num_of_readers;
		writers = dec_num_of_writers;
		q_write = &writers_queue_dec;
		q_read = &readers_queue_dec;
	}
	else{
		readers = enc_num_of_readers;
		writers = enc_num_of_writers;
		q_write = &writers_queue_enc;
		q_read = &readers_queue_enc;
	}
	if(filp->f_mode & FMODE_WRITE && writers == 0 && readers != 0){
		down_trylock(q_read);
		up(q_read);
	}
	if(filp->f_mode & FMODE_READ && readers == 0 && writers != 0){
		down_trylock(q_write);
		up(q_write);
	}
	// else if(filp->f_mode & O_RDWR){
	// 	num_of_writers--;
	// 	num_of_readers--;
	// }
	//printk("Released. R: %d, W: %d\n", num_of_readers, num_of_writers);

	kfree(filp->private_data);
	//printk("Close--> readers: %d , writers: %d\n",num_of_readers,num_of_writers);
	//MOD_DEC_USE_COUNT; /*no need in 2.4 or later*/

	return 0;
}


ssize_t my_read( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {  /* decryptor */
	if(filp->f_mode & FMODE_WRITE && !(filp->f_mode & FMODE_READ))
		return -EINVAL;
	if(count % 8 != 0)
		return -EINVAL;
	int available_space_on_start = available_space_Buff(my_buff);
	//printk("Read R: %d W: %d\n", num_of_readers, num_of_writers);
	if(dec_num_of_writers == 0 && available_data_Buff(my_buff) == 0)
		return 0;
	if(down_interruptible(&readers_queue_dec) != 0)
		return -EINTR;
	if(available_data_Buff(my_buff) == 0){
		if(dec_num_of_writers == 0){
			up(&readers_queue_dec);
			return 0;
		}
		else if(down_interruptible(&readers_queue_dec) != 0)
		return -EINTR; 
	}
	// if(!(filp->f_mode & FMODE_WRITE) && available_data_Buff(my_buff) == 0)
	// 	return 0;
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
	encryptor(tmp_buff, decrypted_data, count, *((int*)(filp->private_data)), DECRYPT);
	copy_to_user(buf,decrypted_data,sizeof(char)*data_read);
	kfree(tmp_buff);
	kfree(decrypted_data);
	up(&readers_queue_dec);
	if(available_data_Buff(my_buff) == 0)
		if(down_interruptible(&readers_queue_dec) != 0) /* writer will up readers_queue */
			return -EINTR;
	if(available_space_on_start == 0 && data_read > 0){
		down_trylock(&writers_queue_dec);
		up(&writers_queue_dec);  /* we allow writer to work */
	}
    return data_read;
}


ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {  /* decryptor */
    //copy the data from user
	//write the data
    // return the ammount of written data
	if(filp->f_mode & FMODE_READ && !(filp->f_mode & FMODE_WRITE))
		return -EINVAL;
	if(count % 8 != 0)
		return -EINVAL;
	//printk("Write R: %d W: %d\n", num_of_readers, num_of_writers);
	int available_data_on_start = available_data_Buff(my_buff);
	if(dec_num_of_readers == 0 && available_space_Buff(my_buff) == 0)
		return 0;
	if(down_interruptible(&writers_queue_dec) != 0)
		return -EINTR;
	if(dec_num_of_readers == 0 && available_space_Buff(my_buff) == 0){
		up(&writers_queue_dec);
		return 0;
	}
	// if(!(filp->f_mode & FMODE_READ) && available_space_Buff(my_buff) == 0)
	// 	return 0;
	char* tmp_buff = NULL;
	int data_writen = 0;
	tmp_buff = kmalloc(sizeof(char)*count,GFP_KERNEL);
	if(tmp_buff == NULL)
		return -EFAULT;
	copy_from_user(tmp_buff,buf,sizeof(char)*count);
	data_writen = write_Buff(my_buff, tmp_buff, count);
	kfree(tmp_buff);
	up(&writers_queue_dec);
	if(available_space_Buff(my_buff) == 0)
		if(down_interruptible(&writers_queue_dec) != 0) /* reader will up writers_queue */
			return -EINTR;
	if(available_data_on_start == 0 && data_writen > 0){
		down_trylock(&readers_queue_dec);
		up(&readers_queue_dec);  /* we allow reader to work */
	}
	return data_writen;
}


ssize_t my_read2( struct file *filp, char *buf, size_t count, loff_t *f_pos ) { /* encryptor */
	//read the data
	//copy the data to user
    //return the ammount of read data
	if(filp->f_mode & FMODE_WRITE && !(filp->f_mode & FMODE_READ))
		return -EINVAL;
	if(count % 8 != 0)
		return -EINVAL;
	int available_space_on_start = available_space_Buff(my_buff2);
	//printk("Read2 R: %d W: %d\n", num_of_readers, num_of_writers);
	if(enc_num_of_writers == 0 && available_data_Buff(my_buff2) == 0)
		return 0;
	//printk("readers_queue_enc = %d\n", readers_queue_enc.count);
	if(down_interruptible(&readers_queue_enc) != 0)
		return -EINTR;
	if(available_data_Buff(my_buff2) == 0){
		if(enc_num_of_writers == 0){
			up(&readers_queue_enc);
			return 0;
		} 
		else if(down_interruptible(&readers_queue_enc) != 0)
			return -EINTR;
	}
	// if(!(filp->f_mode & FMODE_WRITE) && available_data_Buff(my_buff) == 0)
	// 	return 0;
	char* tmp_buff = NULL;
	int data_read = 0;
	tmp_buff = kmalloc(sizeof(char)*count,GFP_KERNEL);
	if(tmp_buff == NULL)
		return -EFAULT;
	data_read = read_Buff(my_buff2, tmp_buff, count);
	copy_to_user(buf,tmp_buff,sizeof(char)*count);
	kfree(tmp_buff);
	up(&readers_queue_enc);
	if(available_data_Buff(my_buff2) == 0)
		if(down_interruptible(&readers_queue_enc) != 0)  /* writer will up readers_queue */
			return -EINTR;
	if(available_space_on_start == 0 && data_read > 0){
		down_trylock(&writers_queue_enc);
		up(&writers_queue_enc);  /* we allow writer to work */
	}
	return data_read;
}


ssize_t my_write2(struct file *filp, const char *buf, size_t count, loff_t *f_pos) { /* encryptor */
        //copy the data from user
	//write the data
    //return the ammount of written data
	if(filp->f_mode & FMODE_READ && !(filp->f_mode & FMODE_WRITE))
		return -EINVAL;
	if(count % 8 != 0)
		return -EINVAL;
	//printk("Write2 R: %d W: %d\n", num_of_readers, num_of_writers);
	int available_data_on_start = available_data_Buff(my_buff2);
	if(enc_num_of_readers == 0 && available_space_Buff(my_buff2) == 0)
		return 0;
	if(down_interruptible(&writers_queue_enc) != 0)
		return -EINTR;
	if(enc_num_of_readers == 0 && available_space_Buff(my_buff2) == 0){
		up(&writers_queue_enc);
		return 0;
	}
	// if(!(filp->f_mode & FMODE_READ) && available_space_Buff(my_buff) == 0)
	// 	return 0;
	int data_writen = 0;
	char* tmp_buff = NULL;
	char* encrypted_data = NULL;
	tmp_buff = kmalloc(sizeof(char)*count,GFP_KERNEL);
	if(tmp_buff == NULL)
		return -EFAULT;
	copy_from_user(tmp_buff,buf,sizeof(char)*count);
	encrypted_data = kmalloc(sizeof(char)*count,GFP_KERNEL);
	if(encrypted_data == NULL){
		kfree(tmp_buff);
		return -EFAULT;
	}
	encryptor(tmp_buff, encrypted_data, count, *((int*)(filp->private_data)), ENCRYPT);
	data_writen = write_Buff(my_buff2,encrypted_data,count);
	kfree(tmp_buff);
	kfree(encrypted_data);
	up(&writers_queue_enc);
	if(available_space_Buff(my_buff2) == 0)
		if(down_interruptible(&writers_queue_enc) != 0)  /* reader will up writers_queue */
			return -EINTR;
	if(available_data_on_start == 0 && data_writen > 0){
		down_trylock(&readers_queue_enc);
		up(&readers_queue_enc);  /* we allow reader to work */
	}
    return data_writen;
}


loff_t my_llseek(struct file *filp, loff_t f_pos,int i){
	return -ENOSYS;
}

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) { 
	if(cmd != HW4_SET_KEY)
		return -ENOTTY;
	*((int*)(filp->private_data)) = (int)arg;	
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
		return (buff->size_of_buff - buff->first + buff->last);
	} else if ((buff->first == buff->last) && (buff->buff_full == 1)) {
		return buff->size_of_buff;
	} else if ((buff->first == buff->last) && (buff->buff_full == 0)) {
		return 0;
	}
	return -1; // We shouldn't get here!!!!!!!
}

int available_space_Buff(Buff* buff) {
	if(buff->buff_full == 1)
		return 0;
	return buff->size_of_buff - available_data_Buff(buff);
}

int write_Buff(Buff* buff, char* source, int count) {
	int i, bytes_to_write;
	int bytes_available = available_space_Buff(buff);
	bytes_to_write = count > bytes_available ? bytes_available : count;
	if (bytes_available % 8 != 0 || bytes_to_write % 8 != 0) {
		return -1; // We shouldn't get here!!!!!!!
	}
	printk("bytes_to_write %d\n",bytes_to_write);
	for (i = 0; i < bytes_to_write; i++) {
		buff->buff[buff->last] = source[i];
		buff->last = (buff->last + 1) % buff->size_of_buff;
	}
	if(buff->first == buff->last){
		buff->buff_full = 1;
		printk("buff->buff_full = 1, first: %d, last:%d\n",buff->first, buff->last);
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
	if(bytes_to_read != 0){
		buff->buff_full = 0;
		printk("buff->buff_full = 0, first: %d, last:%d\n",buff->first, buff->last);
	}
	return bytes_to_read;
}
/**********************************************************************************************/
