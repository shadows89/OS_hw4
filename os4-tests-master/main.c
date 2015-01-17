#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <pthread.h>

#define HW4_MAGIC '4'
#define HW4_SET_KEY _IOW(HW4_MAGIC,0,int)

// Provided crypto code
enum {DECRYPT,ENCRYPT};

void encrypt_block(char* in, char* out,int key,int op) {
	unsigned long long value = *((unsigned long long*)(in));
	unsigned long long tmp = value;
	for( ; tmp ; tmp = tmp >> 1) {
		key += tmp%2;
	}

	int shift = key % (sizeof(value)*8);

	if ( op == DECRYPT) {
		shift = 64 - shift;
	}

	value = (value << shift) | (value >> (sizeof(value) * 8 - shift));
	memcpy(out,&value,sizeof(value));
}

void encryptor(char* in, char* out,int n,int key,int op) {
	int i=0;
	while (i<n) {
		encrypt_block(in+i,out+i,key,op);
		i+=8;
	}
}

struct test_def {
	int (*func)();
	const char *name;
};

#define BUFFER_SIZE	(1 << 12)

static int get_dec(int mode) {
	return open("dec", mode);
}

static int get_enc(int mode) {
	return open("enc", mode);
}

static int with_fd(int fd, int (*func)(int)) {
	int ret = func(fd);
	close(fd);
	return ret;
}

#define TEST_IMPL_WITH_FD(name, fd, test_func)		\
	static int name() {				\
		return with_fd((fd), (test_func));	\
	}

#define TEST_BOTH_WITH_FD(name, flags, test_func)			\
	TEST_IMPL_WITH_FD(name ## _dec, get_dec((flags)), (test_func))	\
	TEST_IMPL_WITH_FD(name ## _enc, get_enc((flags)), (test_func))


int _test_open_success(int fd) {
	return fd != -1;
}
TEST_BOTH_WITH_FD(test_open_ro, O_RDONLY, _test_open_success);
TEST_BOTH_WITH_FD(test_open_wo, O_WRONLY, _test_open_success);
TEST_BOTH_WITH_FD(test_open_rw, O_RDWR, _test_open_success);

static int test_open_invalid() {
	return open("inv", O_RDONLY) == -1;
}

static int _test_read_empty(int fd) {
	char buffer[8];
	return read(fd, buffer, 8) == 0;
}
TEST_BOTH_WITH_FD(test_read_empty, O_RDONLY, _test_read_empty);

static int _test_read_unaligned(int fd) {
	char buffer[6];
	return read(fd, buffer, 6) == -1 && errno == EINVAL;
}
TEST_BOTH_WITH_FD(test_read_unaligned, O_RDONLY, _test_read_unaligned);

static int _test_write_aligned(int fd) {
	char buffer[8];
	return write(fd, buffer, 8) == 8;
}
TEST_BOTH_WITH_FD(test_write_aligned, O_WRONLY, _test_write_aligned);

static int _test_write_unaligned(int fd) {
	char buffer[7];
	return write(fd, buffer, 7) == -1 && errno == EINVAL;
}
TEST_BOTH_WITH_FD(test_write_unaligned, O_WRONLY, _test_write_unaligned);

static int _test_write_full(int fd) {
	char buffer[BUFFER_SIZE];
	if (write(fd, buffer, BUFFER_SIZE - 8) != (BUFFER_SIZE - 8))
		return 0;
	return write(fd, buffer, BUFFER_SIZE) == 8;
}
TEST_BOTH_WITH_FD(test_write_full, O_WRONLY, _test_write_full);

static int _test_no_lseek(int fd) {
	return lseek(fd, 0, SEEK_SET) == -1 && errno == ENOSYS;
}
TEST_BOTH_WITH_FD(test_no_lseek, O_RDONLY, _test_no_lseek);

// static unsigned int reader_timestamp;
// static void *reader_thread(void *data) {
// 	int fd = *((int *) &data);
// 	int ret;
// 	char buffer[8];
// 	ret = read(fd, buffer, 8);
// 	reader_timestamp = time(NULL);
// 	return *(int **) &ret;
// }

// static unsigned int writer_timestamp;
// static void *writer_thread(void *data) {
// 	int fd = *((int *) &data);
// 	int ret;
// 	char buffer[8];
// 	ret = write(fd, buffer, 8);
// 	writer_timestamp = time(NULL);
// 	return *(int **) &ret;
// }

// static int with_two_fds(int fd1, int fd2, int (*func)(int, int)) {
// 	int ret = func(fd1, fd2);
// 	close(fd1);
// 	close(fd2);
// 	return ret;
// }
#define TEST_IMPL_WITH_TWO_FDS(name, fd1, fd2, test_func)		\
	static int name() {						\
		return with_two_fds((fd1), (fd2), (test_func));		\
	}

#define TEST_BOTH_WITH_TWO_FDS(name, flags1, flags2, test_func)		\
	TEST_IMPL_WITH_TWO_FDS(						\
		name ## _dec,						\
		get_dec((flags1)), 					\
		get_dec((flags2)), 					\
		(test_func)						\
	)								\
	TEST_IMPL_WITH_TWO_FDS(						\
		name ## _enc,						\
		get_enc((flags1)), 					\
		get_enc((flags2)), 					\
		(test_func)						\
	)


// static int _test_write_blocks_until_read(int fd_read, int fd_write) {
// 	pthread_t writer;
// 	char buffer[BUFFER_SIZE];
// 	int reader_ret, writer_ret;

// 	if (write(fd_write, buffer, BUFFER_SIZE) < BUFFER_SIZE)
// 		return 0;

// 	if (pthread_create(&writer, NULL, writer_thread, *(int **) &fd_write))
// 		return 0;
// 	sleep(2);
// 	reader_ret = read(fd_read, buffer, 8);
// 	reader_timestamp = time(NULL);

// 	if (pthread_join(writer, (void **) &writer_ret))
// 		return 0;

// 	return reader_ret == 8 && writer_ret == 8 &&
// 		reader_timestamp <= writer_timestamp;
// }
// TEST_BOTH_WITH_TWO_FDS(
// 	test_write_blocks_until_read,
// 	O_RDONLY,
// 	O_WRONLY,
// 	_test_write_blocks_until_read
// );

// static int _test_write_blocks_until_no_readers(int fd_read, int fd_write) {
// 	pthread_t writer;
// 	char buffer[BUFFER_SIZE];
// 	int writer_ret;
// 	int close_timestamp;

// 	if (write(fd_write, buffer, BUFFER_SIZE) < BUFFER_SIZE)
// 		return 0;

// 	if (pthread_create(&writer, NULL, writer_thread, *(int **) &fd_write))
// 		return 0;
// 	sleep(2);
// 	close_timestamp = time(NULL);
// 	close(fd_read);

// 	if (pthread_join(writer, (void **) &writer_ret))
// 		return 0;

// 	return writer_ret == 0 && close_timestamp <= writer_timestamp;
// }
// TEST_BOTH_WITH_TWO_FDS(
// 	test_write_blocks_until_no_readers,
// 	O_RDONLY,
// 	O_WRONLY,
// 	_test_write_blocks_until_no_readers
// );

// static int _test_write_blocks_until_signal(int fd_read, int fd_write) {
// 	char buffer[BUFFER_SIZE];
// 	int writer_ret;
// 	int child;

// 	if (write(fd_write, buffer, BUFFER_SIZE) < BUFFER_SIZE)
// 		return 0;

// 	child = fork();
// 	if (child == -1)
// 		return 0;

// 	if (!child) {
// 		writer_ret = write(fd_write, buffer, 8);
// 		if (writer_ret == -1 && errno == EINTR)
// 			exit(22);
// 		exit(0);
// 	} else {
// 		sleep(1);
// 		kill(child, SIGUSR1);
// 		waitpid(child, &writer_ret, 0);
// 	}

// 	return WEXITSTATUS(writer_ret) == 22;
// }
// TEST_BOTH_WITH_TWO_FDS(
// 	test_write_blocks_until_signal,
// 	O_RDONLY,
// 	O_WRONLY,
// 	_test_write_blocks_until_signal
// );

// static int _test_read_blocks_until_write(int fd_read, int fd_write) {
// 	pthread_t reader, writer;
// 	int reader_ret, writer_ret;

// 	if (pthread_create(&reader, NULL, reader_thread, *(int **) &fd_read))
// 		return 0;
// 	sleep(2);

// 	if (pthread_create(&writer, NULL, writer_thread, *(int **) &fd_write))
// 		return 0;

// 	if (pthread_join(writer, (void **) &writer_ret))
// 		return 0;

// 	if (pthread_join(reader, (void **) &reader_ret))
// 		return 0;

// 	return reader_ret == 8 && writer_ret == 8 &&
// 		reader_timestamp >= writer_timestamp;
// }
// TEST_BOTH_WITH_TWO_FDS(
// 	test_read_blocks_until_write,
// 	O_RDONLY,
// 	O_WRONLY,
// 	_test_read_blocks_until_write
// );

// static int _test_read_blocks_until_no_writers(int fd_read, int fd_write) {
// 	pthread_t reader;
// 	int reader_ret;
// 	int close_timestamp;

// 	if (pthread_create(&reader, NULL, reader_thread, *(int **) &fd_read))
// 		return 0;
// 	sleep(2);
// 	close_timestamp = time(NULL);
// 	close(fd_write);

// 	if (pthread_join(reader, (void **) &reader_ret))
// 		return 0;

// 	return reader_ret == 0 && close_timestamp <= reader_timestamp;
// }
// TEST_BOTH_WITH_TWO_FDS(
// 	test_read_blocks_until_no_writers,
// 	O_RDONLY,
// 	O_WRONLY,
// 	_test_read_blocks_until_no_writers
// );

// static int _test_read_blocks_until_signal(int fd_read, int fd_write) {
// 	char buffer[8];
// 	int reader_ret;
// 	int child;

// 	child = fork();
// 	if (child == -1)
// 		return 0;

// 	if (!child) {
// 		reader_ret = read(fd_read, buffer, 8);
// 		if (reader_ret == -1 && errno == EINTR)
// 			exit(22);
// 		exit(0);
// 	} else {
// 		sleep(1);
// 		kill(child, SIGUSR1);
// 		waitpid(child, &reader_ret, 0);
// 	}

// 	return WEXITSTATUS(reader_ret) == 22;
// }
// TEST_BOTH_WITH_TWO_FDS(
// 	test_read_blocks_until_signal,
// 	O_RDONLY,
// 	O_WRONLY,
// 	_test_read_blocks_until_signal
// );

static int set_key(int fd, int key) {
	return ioctl(fd, HW4_SET_KEY, key);
}

static int _test_set_key(int fd) {
	return set_key(fd, 2345) == 0;
}
TEST_BOTH_WITH_FD(test_set_key, O_RDONLY, _test_set_key);

static int _test_invalid_ioctl(int fd) {
	return ioctl(fd, HW4_SET_KEY + 1, 0) == -1 && errno == ENOTTY;
}
TEST_BOTH_WITH_FD(test_invalid_ioctl, O_RDONLY, _test_invalid_ioctl);

// static int do_encrypt(char *dst, const char *src, size_t len, int key) {
// 	int fd = get_enc(O_RDWR);
// 	int ret = 0;

// 	set_key(fd, key);
// 	if ((write(fd, src, len) != len) ||
// 	    (read(fd, dst, len) != len))
// 		ret = 1;
// 	close(fd);
// 	return ret;
// }

// static int do_decrypt(char *dst, const char *src, size_t len, int key) {
// 	int fd = get_dec(O_RDWR);
// 	int ret = 0;
// 	set_key(fd, key);

// 	if ((write(fd, src, len) != len) ||
// 	    (read(fd, dst, len) != len))
// 		ret = 1;
// 	close(fd);
// 	return ret;
// }


// static int test_encryption() {
// 	char buffer_kernel[1024];
// 	char buffer_user[1024];
// 	int i;

// 	for (i = 0; i < 1024; i++) {
// 		buffer_kernel[i] = buffer_user[i] = i % 234;
// 	}
// 	do_encrypt(buffer_kernel, buffer_kernel, 1024, 1234);
// 	encryptor(buffer_user, buffer_user, 1024, 1234, ENCRYPT);

// 	return memcmp(buffer_user, buffer_kernel, 1024) == 0;
// }

// static int test_decryption() {
// 	char buffer_kernel[1024];
// 	char buffer_user[1024];
// 	int i;

// 	for (i = 0; i < 1024; i++) {
// 		buffer_kernel[i] = buffer_user[i] = i % 234;
// 	}
// 	do_decrypt(buffer_kernel, buffer_kernel, 1024, 1234);
// 	encryptor(buffer_user, buffer_user, 1024, 1234, DECRYPT);

// 	return memcmp(buffer_user, buffer_kernel, 1024) == 0;
// }

// static int test_sanity() {
// 	char buffer_pre[1024];
// 	char buffer_post[1024];
// 	int i;

// 	for (i = 0; i < 1024; i++) {
// 		buffer_pre[i] = i % 234;
// 	}

// 	do_encrypt(buffer_post, buffer_pre, 1024, 123);
// 	do_decrypt(buffer_post, buffer_post, 1024, 123);

// 	return memcmp(buffer_pre, buffer_post, 1024) == 0;
// }

// static int test_key_not_shared() {
// 	char buffer_pre[1024];
// 	char buffer_post[1024];
// 	int i;

// 	for (i = 0; i < 1024; i++) {
// 		buffer_pre[i] = i % 234;
// 	}

// 	do_encrypt(buffer_post, buffer_pre, 1024, 123);
// 	do_decrypt(buffer_post, buffer_post, 1024, 321);

// 	return memcmp(buffer_pre, buffer_post, 1024) != 0;
// }

#define DEFINE_TEST(func) { func, #func }
#define DEFINE_TEST_ON_BOTH(func)	\
	DEFINE_TEST(func ## _dec),	\
	DEFINE_TEST(func ## _enc)

struct test_def tests[] = {
	DEFINE_TEST_ON_BOTH(test_open_ro),
	DEFINE_TEST_ON_BOTH(test_open_wo),
	DEFINE_TEST_ON_BOTH(test_open_rw),
	DEFINE_TEST(test_open_invalid),
	DEFINE_TEST_ON_BOTH(test_read_empty),
	DEFINE_TEST_ON_BOTH(test_read_unaligned),
	DEFINE_TEST_ON_BOTH(test_write_aligned),
	DEFINE_TEST_ON_BOTH(test_write_unaligned),
	DEFINE_TEST_ON_BOTH(test_write_full),
	DEFINE_TEST_ON_BOTH(test_no_lseek),
	DEFINE_TEST_ON_BOTH(test_set_key),
	DEFINE_TEST_ON_BOTH(test_invalid_ioctl),
	// DEFINE_TEST_ON_BOTH(test_write_blocks_until_read),
	// DEFINE_TEST_ON_BOTH(test_write_blocks_until_no_readers),
	// DEFINE_TEST_ON_BOTH(test_write_blocks_until_signal),
	// DEFINE_TEST_ON_BOTH(test_read_blocks_until_write),
	// DEFINE_TEST_ON_BOTH(test_read_blocks_until_no_writers),
	// DEFINE_TEST_ON_BOTH(test_read_blocks_until_signal),
	// DEFINE_TEST(test_encryption),
	// DEFINE_TEST(test_decryption),
	// DEFINE_TEST(test_sanity),
	// DEFINE_TEST(test_key_not_shared),
	{ NULL, "The end" },
};

static int drain_devices() {
	int dec_fd = get_dec(O_RDONLY);
	int enc_fd = get_enc(O_RDONLY);
	char buffer[1024];

	while (read(dec_fd, buffer, 1024) == 1024)
		;
	while (read(enc_fd, buffer, 1024) == 1024)
		;

	close(dec_fd);
	close(enc_fd);
	return 0;
}

static void sigusr1_handler(int signum) {
}

int main() {
	struct test_def *current = &tests[0];
	int passed = 0, failed = 0;

	signal(SIGUSR1, sigusr1_handler);

	while (current->func) {
		int ret;

		printf("%-40s:\t", current->name);
		fflush(stdout);
		ret = current->func();
		printf("%s\n", ret ? "PASS" : "FAIL");
		current++;
		drain_devices();
		ret ? passed++ : failed++;
	};

	puts("----------------------------------------------------");
	printf(
		"Summary:\nPassed: %d/%d, Failed: %d/%d\n",
	       	passed,
		passed + failed,
		failed,
		passed + failed
	);

	return !!failed;
}
