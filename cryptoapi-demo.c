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

#define DATA_SIZE       32 // acredito que o tamnho desejado seja 32 bits
#define PFX "cryptoapi-demo: "

#define CRYPTO_skcipher_MODE_CBC		0 // adicionado 0x00000002
#define CRYPTO_skcipher_MODE_MASK		0 // adicionado 0x000000ff

static char key[DATA_SIZE];
module_param_string(key,key,DATA_SIZE,0);

static char iv[DATA_SIZE];
module_param_string(iv,iv,DATA_SIZE,0);
/*
int input;
module_param(input,int,0);*/

MODULE_AUTHOR("Eu, você e dois Caue");
MODULE_DESCRIPTION("Simple CryptoAPI demo");
MODULE_LICENSE("GPL");

/* ====== CryptoAPI ====== */




static void hexdump(unsigned char *buf, unsigned int len)
{
        while (len--)
                printk("%02x", *buf++);

        printk("\n");
}

static void cryptoapi_demo(void)
{
        /* config options */
        char *algo = "cbc(aes)"; // modificado: anterior "aes"
        int   mode = CRYPTO_skcipher_MODE_CBC;
	int   mask = CRYPTO_skcipher_MODE_MASK; // adicionado
   

        /* local variables */
        struct crypto_skcipher *tfm; // utilzamos skcipher ao inves de tfm
        struct scatterlist sg[8]; // entender o q eh scatterlist
        struct skcipher_request *req = NULL; // necessario apra criptografar
        int    ret;
        char   *input, *encrypted, *decrypted;

     
        tfm = crypto_alloc_skcipher(algo, mode, mask);

        if (IS_ERR(tfm)) {
                printk("failed to load transform for %s %s\n", algo, mode == CRYPTO_skcipher_MODE_CBC ? "CBC" : "");
                return;
        }
        
        ret = crypto_skcipher_setkey(tfm, key, sizeof(key));

        if (ret) {
                printk(KERN_ERR PFX "setkey() failed flags= \n"); // FALTA FALAR SOBRE ESSE ERRO DE FLAG
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

        encrypted = kmalloc(DATA_SIZE, GFP_KERNEL); // Parametros estavam invertidos
        if (!encrypted) {
                printk(KERN_ERR PFX "kmalloc(encrypted) failed\n");
                kfree(input);
                goto out;
        }

        decrypted = kmalloc(DATA_SIZE, GFP_KERNEL); // Parametros estavam invertidos
        if (!decrypted) {
                printk(KERN_ERR PFX "kmalloc(decrypted) failed\n");
                kfree(encrypted);
                kfree(input);
                goto out;
        }

        memset(input, 1, DATA_SIZE);

        sg_init_one(&sg[0], input, DATA_SIZE);          //
        sg_init_one(&sg[1], encrypted, DATA_SIZE);      //      entrou no lugar do FILL_SG();
        sg_init_one(&sg[2], decrypted, DATA_SIZE);      //

	printk(KERN_ERR PFX "IN: "); hexdump(input, DATA_SIZE);
        skcipher_request_set_crypt(req, &sg[0], &sg[1], DATA_SIZE, iv); // ordem ods parametros: requisição, origem, destino, tamanho, iv;
 	skcipher_request_set_crypt(req, &sg[1], &sg[0], DATA_SIZE, iv); //Descriptar usa a ordem inversa

        
        printk(KERN_ERR PFX "EN: "); hexdump(encrypted, DATA_SIZE);
        printk(KERN_ERR PFX "DE: "); hexdump(input, DATA_SIZE); //enquanto não descripitar, não printar
	


        if (memcmp(input, decrypted, DATA_SIZE) != 0) // mudei aki para saber se esta fazendo a criptografia
                printk(KERN_ERR PFX "FAIL: input buffer != decrypted buffer\n");
        else
                printk(KERN_ERR PFX "PASS: encryption/decryption verified\n");

	//out_kfree:
        	kfree(decrypted);
        	kfree(encrypted);
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
