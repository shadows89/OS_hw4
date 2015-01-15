#include "Cyclic_Buff.h"
#include <assert.h>	//TODO remove before release
#define SIZE 64 	//TODO remove before release
Buff* init_Buff(int size) {
	Buff* buffer = malloc(sizeof(Buff)); //TODO: change to kmalloc
	if (buffer == NULL) {
		return NULL;
	}
	buffer->buff = malloc(sizeof(char) * size); //TODO: change to kmalloc
	if (buffer->buff == NULL) {
		free(buffer); //TODO: change to kfree
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
	free(buff->buff); //TODO: change to kfree
	free(buff); //TODO: change to kfree
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

//int main() { //TODO: Remove!!!!
//	Buff* buffer = init_Buff(SIZE);
//	int write_size = 0;
//	int read_size = 0;
//	char* target = malloc(sizeof(char) * SIZE);
//	assert(available_data_Buff(buffer) == 0);
//	assert(available_space_Buff(buffer) == SIZE);
//
//	write_size = write_Buff(buffer, "01234567abcdefgh", 16); // 01234567abcdefgh
//	assert(write_size==16);
//	assert(available_data_Buff(buffer) == 16);
//	assert(available_space_Buff(buffer) == SIZE - 16);
//	read_size = read_Buff(buffer, target, 8);
//	assert(read_size==8);
//	assert(available_data_Buff(buffer) == 8);
//	assert(available_space_Buff(buffer) == SIZE - 8);
//	printf("01234567 = %s\n", target);
//	free(target);
//
//	write_size = write_Buff(buffer, "ijklmnop", 8); // abcdefghijklmnop
//	assert(write_size==8);
//	assert(available_data_Buff(buffer) == 16);
//	assert(available_space_Buff(buffer) == SIZE - 16);
//
//	target = malloc(sizeof(char) * SIZE);
//	read_size = read_Buff(buffer, target, 8); // ijklmnop
//	assert(read_size==8);
//	assert(available_data_Buff(buffer) == 8);
//	assert(available_space_Buff(buffer) == SIZE - 8);
//	printf("abcdefgh = %s\n", target);
//	free(target);
//
//	target = malloc(sizeof(char) * SIZE);
//	read_size = read_Buff(buffer, target, 8); // ijklmnop
//	assert(read_size==8);
//	assert(available_data_Buff(buffer) == 0);
//	assert(available_space_Buff(buffer) == SIZE - 0);
//	printf("ijklmnop = %s\n", target);
//	free(target);
//
//	target = malloc(sizeof(char) * SIZE);
//
//	read_size = read_Buff(buffer, target, 8); // nothing
//	assert(read_size==0);
//	assert(available_data_Buff(buffer) == 0);
//	assert(available_space_Buff(buffer) == SIZE - 0);
//	printf("nothing = %s\n", target);
//	free(target);
//
//	free_Buff(buffer);
//	printf("END");
//	return 0;
//}

