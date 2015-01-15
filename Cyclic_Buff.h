
typedef sturct buff{
	
	char* buff;
	int size_of_buff;
	int first_writen;
	int last_writen;
	int buff_full;

} Buff;

/* init buffer */
Buff* init_Buff(int size); 

/* destroy buffer */
void free_Buff(Buff* buff);

/* return size of available data to read */
int available_data_Buff(Buff* buff,char* buf,int size);

/* return empty space in buff  for writing*/
int available_space_Buff(Buff* buff,char* buf,int size);  

/* write to buff */
/* return amount of data writen */
int write_Buff(Buff* buff,char* source, int count);

/* read from buff */
/* return amount of data read */
int read_Buff(Buff* buff,char* target, int count);