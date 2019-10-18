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
                printk(" %02x", *buf++);
	}
        //printk("\n");
}

static void cryptoapi_demo(void)
{
        /* config options */
        char *algo = "cbc(aes)"; // modificado: anterior "aes"
        int   mode = CRYPTO_skcipher_MODE_CBC;
	int   mask = CRYPTO_skcipher_MODE_MASK; // adicionado
   	int i;	

        /* local variables */
        struct crypto_skcipher *tfm; // utilzamos skcipher ao inves de tfm
        struct scatterlist sg[4]; // entender o q eh scatterlist
        struct skcipher_request *req = NULL; // necessario apra criptografar
        struct skcipher_request *reqTeste = NULL;
        int    ret;
        char   *input, *output, *teste, *inputTeste,*saida1, *saida2;
	

	for(i=0; i < 16; i++){
		if(keyPega[i])
			key[i] = keyPega[i];
		else
			key[i] = 0;
	
		if(ivPega[i])
			iv[i] = ivPega[i];
		else
			iv[i] = 0;
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

	
	reqTeste = skcipher_request_alloc(tfm, GFP_KERNEL); // o que é GFP_KERNEL????
    	if (!reqTeste) {
        	printk("could not allocate tfm request\n");
        	goto out;
    	}
	
	teste = kmalloc(DATA_SIZE, GFP_KERNEL); // Parametros estavam invertidos
        if (!teste) {
                printk(KERN_ERR PFX "kmalloc(input) failed\n");
                goto out;
        }

	inputTeste = kmalloc(DATA_SIZE, GFP_KERNEL); // Parametros estavam invertidos
        if (!inputTeste) {
                printk(KERN_ERR PFX "kmalloc(input) failed\n");
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

        memset(input, 1, DATA_SIZE);
	memset(inputTeste, 1, DATA_SIZE);

        sg_init_one(&sg[0], input, DATA_SIZE);          //
        sg_init_one(&sg[1], output, DATA_SIZE);      //      entrou no lugar do FILL_SG();
 	sg_init_one(&sg[2], teste, DATA_SIZE);      //      entrou no lugar do FILL_SG();
 	sg_init_one(&sg[3], inputTeste, DATA_SIZE);

	printk(KERN_ERR PFX "IN 1: "); hexdump(input, DATA_SIZE);
	printk(KERN_ERR PFX "EN 1: "); hexdump(output, DATA_SIZE);

	//printk(KERN_ERR PFX "Teste 1: "); hexdump(teste, DATA_SIZE);	

        //skcipher_request_set_crypt(reqTeste, &sg[3], &sg[2], DATA_SIZE, iv); // ordem ods parametros: requisição, origem, destino, tamanho, iv;
 	skcipher_request_set_crypt(req, &sg[0], &sg[1], DATA_SIZE, iv); // ordem ods parametros: requisição, origem, destino, tamanho, iv;
 	
	//crypto_skcipher_encrypt(reqTeste);

	crypto_skcipher_encrypt(req);	
        
	saida1 = sg_virt(&sg[1]);	

	printk(KERN_ERR PFX "IN 2: "); hexdump(input, DATA_SIZE);
	printk(KERN_ERR PFX "EN 2: "); hexdump(saida1, DATA_SIZE);

	/*printk(KERN_ERR PFX "Teste 2: "); hexdump(teste, DATA_SIZE);
	
	strcpy(input,teste);
	
	if(input == NULL || teste == NULL){
		goto out_kfree;
	}

	printk("Descrypt Teste:\n\n\n");

	crypto_skcipher_decrypt(reqTeste);

	printk(KERN_ERR PFX "IN 3: "); hexdump(input, DATA_SIZE);
	printk(KERN_ERR PFX "Teste 3: "); hexdump(teste, DATA_SIZE);	
        */


	printk("Descrypt Certo:\n\n\n");
	
	strcpy(teste,saida1);

	if(input == NULL || output == NULL){
		goto out_kfree;
	}

	skcipher_request_set_crypt(reqTeste, &sg[2], &sg[3], DATA_SIZE, iv); // ordem ods parametros: requisição, origem, destino, tamanho, iv;
 	
	crypto_skcipher_decrypt(reqTeste);
	
	saida2 = sg_virt(&sg[3]);
	
	printk(KERN_ERR PFX "IN 3: "); hexdump(teste, DATA_SIZE);
	printk(KERN_ERR PFX "EN 3: "); hexdump(saida2, DATA_SIZE);	
												
	out_kfree:
 
        kfree(output);
        kfree(input);
	kfree(teste);
	kfree(inputTeste);

	out:
        crypto_free_skcipher(tfm);
        skcipher_request_free(req);
	skcipher_request_free(reqTeste);


	
	
		
	
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
