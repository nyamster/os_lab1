#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#define A 56
#define B 0x440B7178
#define D 129
#define E 189
#define G 135
#define I 101


const int filenumbers = A / E + (A % E == 0 ? 0: 1);
sem_t sem1;
sem_t sem2;
sem_t sem3;

int loop = 1;

typedef struct {
	char* address;
	size_t size;
	FILE* file;
}thread_info;


typedef struct {
	char* address;
	int filenumber;
}writer_thread_info;

typedef struct {
	int number_thread;
}reader_thread_info;


char* allocate()
{
	return malloc(A * 1024 * 1024);
}

void* write_memory(void* data)
{
	thread_info* cur_data = (thread_info*) data;
	char* block = malloc(cur_data->size * sizeof(char));

	fread(block, 1, cur_data->size, cur_data->file);
	for (size_t i = 0; i < cur_data->size; i ++)
		cur_data->address[i] = block[i];
	free(block);
	pthread_exit(0);
}

void fill_space(char* address)
{
	char* address_offset = address;
	
	thread_info informations[D]; 
	pthread_t threads[D];

	size_t size_for_thread = A * 1024 * 1024 / D;
	FILE* filerandom = fopen("/dev/urandom", "rb");

	for (int i = 0; i < D; i ++)
	{
		informations[i].address = address_offset;
	    informations[i].size = size_for_thread;
	    informations[i].file = filerandom;
		address_offset += size_for_thread;
	}

	informations[D - 1].size += A * 1024 * 1024 % D;

	for (int i = 0; i < D; i ++) pthread_create(&threads[i], NULL, write_memory, &informations[i]);

	for (int i = 0; i < D; i ++) pthread_join(threads[i], NULL);

	fclose(filerandom);
}

void* generate_info(void* data)
{
	char* cur_data = (char*) data;

	while(loop == 1)
	{
		fill_space(cur_data);
	}
	
	pthread_exit(0);
}

sem_t* get_sem(int id)
{
	if (id == 0) return &sem1;
	if (id == 1) return &sem2;
	if (id == 2) return &sem3;
}


void write_file(writer_thread_info* data, int idFile)
{
	sem_t* cur_sem = get_sem(idFile);
	sem_wait(cur_sem);
	char* defaultname = malloc(sizeof(char) * 6);
	snprintf(defaultname, sizeof(defaultname) + 1, "labOS%d",++idFile);
	FILE* file = fopen(defaultname, "wb");
	size_t file_size = E * 1024 * 1024;
	size_t rest = 0;
	while (rest < file_size) 
	{
		long size = file_size - rest >= G ? G : file_size - rest;
		rest += fwrite(data->address + (rest % (A * 1024 * 1024)), 1, size, file);	
	}
	sem_post(cur_sem);
}

void* write_files(void* data)
{
	writer_thread_info* cur_data = (writer_thread_info*) data;
    while (loop == 1)
	{	
		write_file(cur_data, cur_data->filenumber);
	}
}


void read_file(int id_thread, int idFile)
{
	sem_t* cur_sem = get_sem(idFile);
	sem_wait(cur_sem);

	char* defaultname = malloc(sizeof(char) * 6);
	snprintf(defaultname, sizeof(defaultname) + 1, "labOS%d",++idFile);
	FILE* file = fopen(defaultname, "rb");

	unsigned char block[G];
	
	fread(&block, 1 , G, file);
	
	int min = 0;
	int flag = 1;
	
	size_t parts = E * 1024 * 1024 / G;

	for (size_t part = 0; part < parts; part ++)
	{
		int min_from_block = 0;
		int flag_for_block = 1;
		for (size_t i = 0; i < sizeof(block); i += 4)
		{
			unsigned int num = 0;
		
			for (int j = 0; j < 4; j ++) 
			{
				num = (num<<8) + block[i + j];
			}
			if (flag_for_block == 1)
			{
				flag_for_block = 0;
				min_from_block = num;
			}
			else
			{
				if (min_from_block < num) min_from_block = num;
			}
		}
		if (flag == 1) 
		{
			flag = 0;
			min = min_from_block;
		}
		else
		{
			if (min < min_from_block) min = min_from_block;
		}
	}
	fprintf(stdout,"\nMin from labOS%d from thread-%d is %d\n",idFile, id_thread, min );
	fflush(stdout);
	fclose(file);
	sem_post(cur_sem);
}

void* read_files(void* data)
{
	reader_thread_info* cur_data = (reader_thread_info*) data;
	while (loop) 
	{
		for (int i = 0; i < filenumbers; i ++) read_file(cur_data->number_thread, i);
	}
}



int main(void){
	printf("Before allocation");
	getchar();

	char* address = allocate();

	printf("After allocation");
	getchar();

	fill_space(address);
	
	printf("After filling");
	getchar();

	//munmap(address, A * 1024 * 1024);
	free(address);

	printf("After deallocation");
	getchar();

	pthread_t generate_thread;
	
	address = allocate();

	pthread_create(&generate_thread, NULL, generate_info, address);

	sem_init(&sem1, 0, 1);
	sem_init(&sem2, 0, 1);
	sem_init(&sem3, 0, 1);
	
	pthread_t writer_thread[filenumbers];
	writer_thread_info writer_information[filenumbers];

	for (int  i = 0; i < filenumbers; i++)
	{
		writer_information[i].filenumber = i;
		writer_information[i].address = address;
	}

	for (int i = 0; i < filenumbers; i ++) pthread_create(&writer_thread[i], NULL, write_files, &writer_information[i]);

	pthread_t reader_thread[I];
	reader_thread_info reader_information[I];

	for (int i = 0; i < I; i++)
	{
		reader_information[i].number_thread = i + 1;
		pthread_create(&reader_thread[i], NULL, read_files, &reader_information[i]);
	}



	printf("Pause");
	getchar();

	printf("End");
	getchar();

	loop = 0;

	for (int i = 0; i < I; i ++)
	{
		pthread_join(reader_thread[i], NULL);
	}

	for (int i = 0; i < filenumbers; i ++)
		pthread_join(writer_thread[i], NULL);
	
	pthread_join(generate_thread, NULL);

	sem_destroy(&sem1);
	sem_destroy(&sem2);
	sem_destroy(&sem3);

	free(address);
	return 0;
}
