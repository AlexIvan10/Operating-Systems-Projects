#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define RESP_PIPE "RESP_PIPE_68275"
#define REQ_PIPE "REQ_PIPE_68275"
#define shared_mem_name "1PqFbUK"
#define memory_align 1024

void problem(){
	int fd_resp_pipe = -1;
	int fd_req_pipe = -1;
	int i = 0;
	int shared_mem_fd;
	char *shared_mem;
	char *map_file;
	unsigned int size;
	int file_size = 0;

	if(mkfifo(RESP_PIPE, 0666) != 0){
		printf("ERROR\ncannot create the response pipe\n");
		return;
	}

	fd_req_pipe = open(REQ_PIPE, O_RDONLY);
	if(fd_req_pipe == -1){
		printf("ERROR\ncannot open the request pipe\n");
		return;
	}

	fd_resp_pipe = open(RESP_PIPE, O_WRONLY);

	char connection[] = "CONNECT!";
	write(fd_resp_pipe, &connection, strlen(connection));
	printf("SUCCESS\n");

	for(;;){
		char read_message[255];

		i = 0;
		do{
			read(fd_req_pipe, &read_message[i], sizeof(char));
			i ++;
		}while(read_message[i - 1] != '!');
		read_message[i] = 0;

		if(strncmp(read_message, "EXIT", strlen("EXIT")) == 0){
			close(shared_mem_fd);
			close(fd_req_pipe);
			close(fd_resp_pipe);

			munmap(map_file, file_size);
			shm_unlink(shared_mem_name);
			unlink(RESP_PIPE);
			break;
		}

		else if(strncmp(read_message, "PING", strlen("PING")) == 0){
			char response[] = "PING!PONG!";
			unsigned int number = 68275;

			write(fd_resp_pipe, &response, strlen(response));
			write(fd_resp_pipe, &number, sizeof(unsigned int));
		}

		else if(strncmp(read_message, "CREATE_SHM", strlen("CREATE_SHM")) == 0){
			read(fd_req_pipe, &size, sizeof(unsigned int));
			shared_mem_fd = shm_open(shared_mem_name, O_CREAT | O_RDWR, 0664);
			if(shared_mem_fd < 0){
				char error_message[] = "CREATE_SHM!ERROR!";
				write(fd_resp_pipe, &error_message, strlen(error_message));
			}
			else{
				ftruncate(shared_mem_fd, size);
				char success_message[] = "CREATE_SHM!SUCCESS!";
				write(fd_resp_pipe, &success_message, strlen(success_message));
			}
		}

		else if(strncmp(read_message, "WRITE_TO_SHM", strlen("WRITE_TO_SHM")) == 0){
			unsigned int offset;
			unsigned int value;

			read(fd_req_pipe, &offset, sizeof(unsigned int));
			read(fd_req_pipe, &value, sizeof(unsigned int));

			int can_write = 1;
			if(offset <= 0 || offset >= size || offset + sizeof(unsigned int) > size)
				can_write = 0;

			if(can_write){
                shared_mem = (char*) mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
				if(shared_mem == MAP_FAILED){
					char error_message[] = "WRITE_TO_SHM!ERROR!";
					write(fd_resp_pipe, error_message, strlen(error_message));
				}
				else{
					*((unsigned int*)(shared_mem + offset)) = value;
                    char success_message[] = "WRITE_TO_SHM!SUCCESS!";
                    write(fd_resp_pipe, success_message, strlen(success_message));
                    munmap(shared_mem, size);
                    shared_mem = NULL;
                }
			}
			else{
				char error_message[] = "WRITE_TO_SHM!ERROR!";
				write(fd_resp_pipe, &error_message, strlen(error_message));
			}
		}

		else if(strncmp(read_message, "MAP_FILE", strlen("MAP_FILE")) == 0){
			char file_name[255];
			int fd = -1;

			i = 0;
			do{
				read(fd_req_pipe, &file_name[i], sizeof(char));
				i ++;
			}while(file_name[i - 1] != '!');

			file_name[i - 1] = 0;

			fd = open(file_name, O_RDONLY);
			if(fd == -1){
				char error_message[] = "MAP_FILE!ERROR!";
				write(fd_resp_pipe, error_message, strlen(error_message));
			}
			else{
				file_size = lseek(fd, 0, SEEK_END);
				lseek(fd, 0, SEEK_SET);

				map_file = (char*)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
				if(map_file == (void*)-1){
					char error_message[] = "MAP_FILE!ERROR!";
					write(fd_resp_pipe, error_message, strlen(error_message));
				}
				else{
					char success_message[] = "MAP_FILE!SUCCESS!";
                    write(fd_resp_pipe, success_message, strlen(success_message));
				}
				close(fd);
			}
		}

		else if(strncmp(read_message, "READ_FROM_FILE_OFFSET", strlen("READ_FROM_FILE_OFFSET")) == 0){
			unsigned int offset;
			unsigned int no_of_bytes;

			read(fd_req_pipe, &offset, sizeof(unsigned int));
			read(fd_req_pipe, &no_of_bytes, sizeof(unsigned int));

			shared_mem = (char*) mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
            if(shared_mem == MAP_FAILED){
                char error_message[] = "READ_FROM_FILE_OFFSET!ERROR!";
                write(fd_resp_pipe, error_message, strlen(error_message));
            }
            else{
            	int can_read = 1;
            	if(shared_mem == NULL || map_file == NULL || offset + no_of_bytes > file_size)
                	can_read = 0;

				if(can_read){
                	memcpy(shared_mem, map_file + offset, no_of_bytes);
                	char success_message[] = "READ_FROM_FILE_OFFSET!SUCCESS!";
                	write(fd_resp_pipe, success_message, strlen(success_message));
                	munmap(shared_mem, size);
                	shared_mem = NULL;
               
            	}
            	else{
                	char error_message[] = "READ_FROM_FILE_OFFSET!ERROR!";
                	write(fd_resp_pipe, error_message, strlen(error_message));
            	}
			}
		}

		else if(strncmp(read_message, "READ_FROM_FILE_SECTION", strlen("READ_FROM_FILE_SECTION")) == 0){
			unsigned int section_no;
			unsigned int offset;
			unsigned int no_of_bytes;

			read(fd_req_pipe, &section_no, sizeof(unsigned int));
			read(fd_req_pipe, &offset, sizeof(unsigned int));
			read(fd_req_pipe, &no_of_bytes, sizeof(unsigned int));

			shared_mem = (char*) mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
            if(shared_mem == MAP_FAILED){
                char error_message[] = "READ_FROM_FILE_SECTION!ERROR!";
                write(fd_resp_pipe, error_message, strlen(error_message));
            }
            else{
            	short header_size;
            	int section_offset;
            	int section_size;
            	memcpy(&header_size, map_file + file_size - 3, 2);
            	memcpy(&section_offset, map_file + file_size - header_size + 2 + (section_no - 1) * 24 + 16, 4);
            	memcpy(&section_size, map_file + file_size - header_size + 2 + (section_no - 1) * 24 + 20, 4);
            	
            	if(offset + no_of_bytes > section_size){
            		char error_message[] = "READ_FROM_FILE_SECTION!ERROR!";
                	write(fd_resp_pipe, error_message, strlen(error_message));
            	}
            	else{
            		memcpy(shared_mem, map_file + section_offset + offset, no_of_bytes);
                	char success_message[] = "READ_FROM_FILE_SECTION!SUCCESS!";
                	write(fd_resp_pipe, success_message, strlen(success_message));
                	munmap(shared_mem, size);
                	shared_mem = NULL;
            	}
            }
		}

		else if(strncmp(read_message, "READ_FROM_LOGICAL_SPACE_OFFSET", strlen("READ_FROM_LOGICAL_SPACE_OFFSET")) == 0){
			unsigned int logical_offset;
			unsigned int no_of_bytes;

			read(fd_req_pipe, &logical_offset, sizeof(unsigned int));
			read(fd_req_pipe, &no_of_bytes, sizeof(unsigned int));


			shared_mem = (char*) mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem_fd, 0);
			if(shared_mem == MAP_FAILED){
                char error_message[] = "READ_FROM_LOGICAL_SPACE_OFFSET!ERROR!";
                write(fd_resp_pipe, error_message, strlen(error_message));
            }
            else{
            	short header_size;
            	char no_of_sections;
            	int section_offset;
            	int section_size;
            	int logical_position = 0;

            	memcpy(&header_size, map_file + file_size - 3, 2);
            	memcpy(&no_of_sections, map_file + file_size - header_size + 1, 1);

            	int found = 0;
            	for (int i = 0; i < no_of_sections; ++i){
            		memcpy(&section_offset, map_file + file_size - header_size + 2 + i * 24 + 16, 4);
            		memcpy(&section_size, map_file + file_size - header_size + 2 + i * 24 + 20, 4);
            		
            		int logical_start = logical_position;
            		int logical_end = logical_start + section_size;

            		if(logical_offset >= logical_start && logical_offset < logical_end){
            			int section_start_offset = logical_offset - logical_start;
                		section_offset += section_start_offset;

                		memcpy(shared_mem, map_file + section_offset, no_of_bytes);

                		char success_message[] = "READ_FROM_LOGICAL_SPACE_OFFSET!SUCCESS!";
                		write(fd_resp_pipe, success_message, strlen(success_message));
                		found = 1;
                		break;
            		}

            		logical_position = logical_end;
            		if(logical_position % memory_align != 0) {
                		logical_position = ((logical_position / memory_align) + 1) * memory_align;
            		}
            	}

            	if(!found){
            		char error_message[] = "READ_FROM_LOGICAL_SPACE_OFFSET!ERROR!";
                	write(fd_resp_pipe, error_message, strlen(error_message));
            	}

            	munmap(shared_mem, size);
        		shared_mem = NULL;
            }
		}

		else 
			break;
	}
}

int main(){
	problem();

	return 0;
}