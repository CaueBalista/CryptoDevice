#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
 
#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM
 
void criptografia(int fd){
	int op,ret;
	char string[BUFFER_LENGTH];

	do {
		printf("Escolha a opcao:\n");
		printf("1 - Caracteres\n2 - Hexadecimal\n3 - Sair\nOpcao: ");
		scanf("%d", &op);
	
		switch (op){

			case 1: //Receber Caracteres

				printf("Digite sua string: ");
				scanf("%[^\n]%*c", string);  

				ret = write(fd, string, strlen(string));
				if (ret < 0){
				      	perror("Failed to write the message to the device.");
					return errno;
				}

				printf("Writing message to the device [%s].\n", string);
			break;

			case 2: //Receber valor Hexa

				printf("Digite um valor em Hexa: ");
				scanf("%[^\n]%*c", string);

				int i;
                char text[BUFFER_LENGTH/2];
                char ch, high, low;
                
                //convertendo valores hexadecimais em string
                for (i = 0; i < strlen(string); i += 2)
                {
                    high = string[i];
                    high -= 0x30;
                    if (high > 9) high -= 7;
                    high <<= 4;

                    low = string[i+1];
                    low -= 0x30;
                    if (low > 9) low -= 7;

                    ch = high | low;
                            
                    text[i/2] = ch;
                }

                text[i/2] = 0; // para delimitar string

                ret = write(fd, text, strlen(text));
               if (ret < 0){
                        perror("Failed to write the message to the device.");
                  return errno;
               }

               printf("Writing message to the device [%s].\n", text);
               
			break;

			default: break;
		}
	}while(op == 1 || op == 2);
}

void descriptografia(){

}

void calculoHash(){

}

int main(){
   int ret, fd;
   char stringToSend[BUFFER_LENGTH];
   printf("Starting device test code example...\n");
   fd = open("/dev/ebbchar", O_RDWR);             // Open the device with read/write access
   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }

   //menu de escolha caractere/hex
	int op1;
   	do{
		printf("Escolha a opcao:\n");
		printf("1 - Criptografar\n2 - Descriptografar\n3 - Calcular Hash\nOpcao: ");
		scanf("%d", &op1);
	
		switch (op1){

			case 1: criptografia(fd);
			break;

			case 2: descriptografia();
			break;

			case 3: calculoHash();
			break;

			default: break;
		}
				
	}while(op1 == 1 || op1 == 2 || op1 == 3);
	
   printf("Type in a short string to send to the kernel module:\n");
   scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
   
   ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
   if (ret < 0){
      perror("Failed to write the message to the device.");
      return errno;
   }
 
   printf("Press ENTER to read back from the device...\n");
   getchar();
 
   printf("Reading from the device...\n");
   ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
   if (ret < 0){
      perror("Failed to read the message from the device.");
      return errno;
   }
   printf("The received message is: [%s]\n", receive);
   printf("End of the program\n");
   return 0;
}