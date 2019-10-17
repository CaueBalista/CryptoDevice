#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
 
#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM

               

int main(){
   	int ret, fd;
	char string[BUFFER_LENGTH+1], stringFinal[(BUFFER_LENGTH*2)+1];//257:256+1, 513:256*2+1
	int i, len;
	char stringEnvio[(BUFFER_LENGTH*2)+1];
	int op;

   printf("Starting testeebbchar...\n");
   fd = open("/dev/cryptoDevice", O_RDWR);             // Open the device with read/write access

   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }

	printf("Informe o formato da sua entrada\n1- Ascii\n2- Hexadecimal\nOpcao: ");
	scanf("%d", &op);

	printf("Digite sua string: ");
	__fpurge(stdin);
	scanf("%[^\n]%*c", string);

	if (op == 1){
		stringEnvio[0] = string[0];
		stringEnvio[1] = '\0';

		len = strlen(string);
		for(i = 0; i<len-2; i++){
			sprintf(stringFinal+i*2, "%02X", string[i+2]);
		}

		strcat(stringEnvio, " ");
		strcat(stringEnvio, stringFinal);
	}
	else {
		strcpy(stringEnvio, string);
	}
	
   	ret = write(fd, stringEnvio, strlen(stringEnvio));
   
   if (ret < 0){
	perror("Failed to write the message to the device.");
	return errno;
   }

   printf("Writing message to the device [%s].\n", string);
	
   printf("Press ENTER to read back from the device...\n");
   getchar();
 
   printf("Reading from the device...\n");
   ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
   if (ret < 0){
      perror("Failed to read the message from the device.");
      return errno;
   }
   printf("The received message is: [%02x]\n", receive);
   printf("End of the program\n");
   return 0;
}
