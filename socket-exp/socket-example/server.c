#include <stdio.h>
#include <string.h>
#include <sys/socket.h>			
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h> //ͷ�ļ�
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>


struct train{
	int _newfd;   		 //socketͨ��fd
	long _start;		 //task��ʼλ��
	long _end;
};
static int ans[26]={0};				//��������client���صĽ��
pthread_mutex_t output_lock;		//���� �̻߳��� �������߳�ͬʱ�������ս��д����

int countIP(){
	char buf[100]={0};
	int countIP=0;
	char* conf = "../workers.conf";
	int conf_fd = open(conf,O_RDONLY);
	read(conf_fd,buf,sizeof(buf));
	for(int i=0;i<strlen(buf);i++){
		if(buf[i]=='\n')
			countIP++;
	}
	printf("ReadIP_count = %d\n",countIP);
	return countIP;
}	
long startFile(int index,int allnum){          			//index�����0��ʼ��worker; allnum����worker����
	char* readfile="../war_and_peace.txt";
	struct stat statbuf;
	memset(&statbuf,0,sizeof(statbuf));
	stat(readfile,&statbuf);
	long len = statbuf.st_size;
	return (len/allnum)*index;
}
long endFile(int index,int allnum){          			
	char* readfile="../war_and_peace.txt";
	struct stat statbuf;
	memset(&statbuf,0,sizeof(statbuf));
	stat(readfile,&statbuf);
	long len = statbuf.st_size;
	return (len/allnum)*(index+1)-1;
}
long endFileFinal(){          			
	char* readfile="../war_and_peace.txt";
	struct stat statbuf;
	memset(&statbuf,0,sizeof(statbuf));
	stat(readfile,&statbuf);
	long len = statbuf.st_size;
	return len;
} 
void * handle_client(void * arg){ //void* arg��������intǿת����
	printf("pid: %lu\n",pthread_self());
	
	struct train *ptrain = (struct train*)arg;
	char* filename = "../war_and_peace.txt";
	size_t len = strlen(filename);
	if(send(ptrain->_newfd,filename,len,0)==-1){
		perror("send filename failed\n");
	}
	printf("filename send ok  ");
	send(ptrain->_newfd,(void*)&ptrain->_start,sizeof(long),0);				//& ȡ��ַ�����ȼ���
	send(ptrain->_newfd,(void*)&ptrain->_end,sizeof(long),0);
	printf("start,end send ok\n");
	int table[26];
	recv(ptrain->_newfd,table,26*sizeof(int),0);
	
	pthread_mutex_lock(&output_lock);
	for(int i=0;i<26;i++)
		ans[i] += table[i];
	pthread_mutex_unlock(&output_lock);
	
	close(ptrain->_newfd);
	//return NULL;								//��ʲô����
	pthread_exit(NULL);							//�߳�������ֹ
}

int main(int argc, const char *argv[])
{
	struct sockaddr_in server, client;
	int sfd,newfd;
	
	// Create socket
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Could not create socket");
		return -1;
    }
    printf("Socket created");
	// Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;  			//�Զ���䱾��IP ��ַ. 0.0.0.0
    server.sin_port = htons(8888);
    // Bind
    if (bind(sfd,(struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed. Error");
        return -1;
    }
    printf("bind done");
    // Listen
    listen(sfd, 3);
	
	int IPnum = countIP();
	pthread_t cli_thread[IPnum] ;
	bzero(cli_thread,sizeof(pthread_t)*IPnum);
	struct train t[IPnum];    				//��֤�˴��ݸ��̺߳����� �������ݲ��ἴ������
	pthread_mutex_init(&output_lock, NULL);
	
	for(int index=0;index<IPnum;index++){
		int socksize = sizeof(struct sockaddr_in);
		if( (newfd = accept(sfd,(struct sockaddr*)&client,(socklen_t *)&socksize)) == -1 ){			//accept����������
			perror("accept failed");
			return 1;
		}
		printf("\n");
		printf("----worker %d----\n",index);
		printf("client ip=%s , port=%d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
		
		//����һ���߳�����ɺͿͻ��˵ĻỰ�������̼�������
		
			bzero(&t[index],sizeof(struct train));
			t[index]._newfd = newfd;
			t[index]._start = startFile(index,IPnum);
			if(index!=IPnum-1)
				t[index]._end = endFile(index,IPnum);
			else
				t[index]._end = endFileFinal();
			printf("intdex:%d start:%ld end:%ld\n",index,t[index]._start,t[index]._end);				
			pthread_create(&cli_thread[index],NULL,handle_client,(void *)&t[index]);
			printf("----pid=%lu: thread doing\n",cli_thread[index]);		
		
	}
	
	int retvalue=0;
	//���û����仰 ��ô���һ���߳���δִ�����
	pthread_join(cli_thread[IPnum-1],(void**)&retvalue);				//�ȴ����һ���߳��˳� ��ȡ����ֵ; join��������  
	
	for(int k=0;k<26;k++)
		printf("%c = %d\n",'a'+k,ans[k]);
	close(sfd);
	return 0;
}

#if 0
ע�����������硢�����ֽ����ת�� 
�����ȫ��ʹ�������ֽ��� ���ͽ����ƺ�Ҳû�����⣿�Ͼ����Լ�����
����Ϊ�˿�ƽ̨������ ���Ի�����Ҫ����ֽ��������(��Ϊ����������������ֽ��򲢲���ͬ)
#endif  