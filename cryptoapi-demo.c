/* 
 * Simple demo explaining usage of the Linux kernel CryptoAPI.
 * By Michal Ludvig <michal@logix.cz>
 *    http://www.logix.cz/michal/
 */

#include <linux/module.h> 
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/scatterlist.h> // modificado de "asm" para "linux" 
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <crypto/skcipher.h>
#include <linux/stat.h>  
#include <linux/device.h>   
#include <linux/fs.h>             
#include <linux/uaccess.h>   

#define DATA_SIZE      16 // acredito que o tamnho desejado seja 32 bits
#define PFX "cryptoapi-demo: "

#define CRYPTO_skcipher_MODE_CBC		0 // adicionado 0x00000002
#define CRYPTO_skcipher_MODE_MASK		0 // adicionado 0x000000ff

static char keyPega[17];
module_param_string(keyPega,keyPega,17,0);

static char ivPega[17];
module_param_string(ivPega,ivPega,17,0);

static char inputPega[17];
module_param_string(inputPega,inputPega,17,0);

static int escolha;
module_param(escolha, int, 0);

static char   key[16];
static char   iv[16];

/*
int input;
module_param(input,int,0);*/

MODULE_AUTHOR("Eu, você e dois Caue");
MODULE_DESCRIPTION("Simple CryptoAPI demo");
MODULE_LICENSE("GPL");

/* ====== CryptoAPI ====== */




static void hexdump(unsigned char *buf, unsigned int len)
{
        while (len--){
		//printk(" %02x", *buf++);
		printk(" %c", *buf++);
	}
        //printk("\n");
}

static void cryptoapi_demo(void)
{
        /* config options */
        char *algo = "cbc(aes)"; // modificado: anterior "aes"
        int   mode = CRYPTO_skcipher_MODE_CBC;
	int   mask = CRYPTO_skcipher_MODE_MASK; // adicionado
   	int  i;	

        /* local variables */
        struct crypto_skcipher *tfm; // utilzamos skcipher ao inves de tfm
        struct scatterlist sg[2]; // entender o q eh scatterlist
        struct skcipher_request *req = NULL; // necessario apra criptografar
        int    ret;
        char   *input, *output, *saida;
	
			
	for(i=0; i < 16; i++){
		if(keyPega[i] == '\0')
			key[i] = 0;
		else
			key[i] =  keyPega[i];
	
		if(ivPega[i] == '\0')
			iv[i] = 0;
		else
			iv[i] = ivPega[i];

	}
	

        tfm = crypto_alloc_skcipher(algo, mode, mask);

        if (IS_ERR(tfm)) {
                printk(" Erro ao alocar tfm");
                return;
        }
        
        ret = crypto_skcipher_setkey(tfm, key, 16); // estava errado, o correto eh strlen, n sizeof

        if (ret) {
                printk(KERN_ERR PFX "erro no ret \n"); // FALTA FALAR SOBRE ESSE ERRO DE FLAG
                goto out;
        }

        req = skcipher_request_alloc(tfm, GFP_KERNEL); // o que é GFP_KERNEL????
    	if (!req) {
        	printk("could not allocate tfm request\n");
        	goto out;
    	}

        input = kmalloc(DATA_SIZE, GFP_KERNEL); // Parametros estavam invertidos
        if (!input) {
                printk(KERN_ERR PFX "kmalloc(input) failed\n");
                goto out;
        }

        output = kmalloc(DATA_SIZE, GFP_KERNEL); // Parametros estavam invertidos
        if (!output) {
                printk(KERN_ERR PFX "kmalloc(output) failed\n");
                kfree(input);
                goto out;
        }
	
		
	
	for(i=0; i < 16; i++){

		if(inputPega[i] == '\0')
			input[i] = 0;
		else
			input[i] = inputPega[i];

	};

        sg_init_one(&sg[0], input, DATA_SIZE);          //
        sg_init_one(&sg[1], output, DATA_SIZE);      	//      entrou no lugar do FILL_SG();

 	skcipher_request_set_crypt(req, &sg[0], &sg[1], DATA_SIZE, iv); // ordem ods parametros: requisição, origem, destino, tamanho, iv;
 	

	if(escolha == 1){

		printk("Encrypt Correct:\n\n\n");

		crypto_skcipher_encrypt(req);	
		
		saida = sg_virt(&sg[1]);	

		printk(KERN_ERR PFX "Input: "); hexdump(input, DATA_SIZE);
		printk(KERN_ERR PFX "Output: "); hexdump(saida, DATA_SIZE+1);
	}
	else{
		printk("Descrypt Correct:\n\n\n");
	 	
		inputPega[0] = 223;
		inputPega[1] = 70;
		inputPega[2] = 92;
		inputPega[3] = 216;
		inputPega[4] = 61;
		inputPega[5] = 248;
		inputPega[6] = 4;
		inputPega[7] = 166;
		inputPega[8] = 28;
		inputPega[9] = 108;
		inputPega[10] = 12;
		inputPega[11] = 196;
		inputPega[12] = 127;
		inputPega[13] = 218;
		inputPega[14] = 103;
		inputPega[15] = 148;
	
			
		for(i=0; i < 16; i++){

		if(inputPega[i] == '\0')
			input[i] = 0;
		else
			input[i] = inputPega[i];

		}

		crypto_skcipher_decrypt(req);
	
		saida = sg_virt(&sg[1]);
	
		printk(KERN_ERR PFX "Input: "); hexdump(input, DATA_SIZE);
		printk(KERN_ERR PFX "Output: "); hexdump(saida, DATA_SIZE+1);	
	}											
	//out_kfree:
 
        kfree(output);
        kfree(input);

	out:
        crypto_free_skcipher(tfm);
        skcipher_request_free(req);
	
   }

/* ====== Module init/exit ====== */

static int __init init_cryptoapi_demo(void)
{
        cryptoapi_demo();

        return 0;
}

static void __exit exit_cryptoapi_demo(void)
{
}

module_init(init_cryptoapi_demo);
module_exit(exit_cryptoapi_demo);
