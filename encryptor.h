
#ifndef HW4_ENCRYPTOR_H_
#define HW4_ENCRYPTOR_H_

#include <linux/string.h>

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

#endif /* HW4_H_ */
