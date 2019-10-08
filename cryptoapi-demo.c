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
#include <linux/param.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <crypto/skcipher.h>





#define PFX "cryptoapi-demo: "

#define CRYPTO_skcipher_MODE_CBC		0 // adicionado 0x00000002
#define CRYPTO_skcipher_MODE_MASK		0 // adicionado 0x000000ff


MODULE_AUTHOR("Michal Ludvig <michal@logix.cz>");
MODULE_DESCRIPTION("Simple CryptoAPI demo");
MODULE_LICENSE("GPL");

/* ====== CryptoAPI ====== */

#define DATA_SIZE       16 // acredito que o tamnho desejado seja 32 bits

#define FILL_SG(sg,ptr,len)     do { (sg)->page = virt_to_page(ptr); (sg)->offset = offset_in_page(ptr); (sg)->length = len; } while (0)

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
        int mode = CRYPTO_skcipher_MODE_CBC;

	int mask = CRYPTO_skcipher_MODE_MASK; // adicionado

        char key[DATA_SIZE], iv[DATA_SIZE]; // alterado, estava com valor 16
        

        /* local variables */
        struct crypto_skcipher *skcipher; // utilzamos skcipher ao inves de tfm
        struct scatterlist sg[8]; // entender o q eh scatterlist
        int ret;
        char *input, *encrypted, *decrypted;

        memset(key, 0, sizeof(key));// recebe chave
        memset(iv, 0, sizeof(iv));// recebe iv

        skcipher = crypto_alloc_skcipher(algo, mode, mask);

        if (skcipher == NULL) {
                printk("failed to load transform for %s %s\n", algo, mode == CRYPTO_skcipher_MODE_CBC ? "CBC" : "");
                return;
        }

        ret = crypto_skcipher_setkey(skcipher, key, sizeof(key));

        if (ret) {
                printk(KERN_ERR PFX "setkey() failed flags= \n"); // FALTA FALAR SOBRE ESSE ERRO DE FLAG
                goto out;
        }

        input = kmalloc(GFP_KERNEL, DATA_SIZE);
        if (!input) {
                printk(KERN_ERR PFX "kmalloc(input) failed\n");
                goto out;
        }

        encrypted = kmalloc(GFP_KERNEL, DATA_SIZE);
        if (!encrypted) {
                printk(KERN_ERR PFX "kmalloc(encrypted) failed\n");
                kfree(input);
                goto out;
        }

        decrypted = kmalloc(GFP_KERNEL, DATA_SIZE);
        if (!decrypted) {
                printk(KERN_ERR PFX "kmalloc(decrypted) failed\n");
                kfree(encrypted);
                kfree(input);
                goto out;
        }

        memset(input, 0, DATA_SIZE);

        FILL_SG(&sg[0], input, DATA_SIZE);
        FILL_SG(&sg[1], encrypted, DATA_SIZE);
        FILL_SG(&sg[2], decrypted, DATA_SIZE);

        crypto_skcipher_set_iv(skcipher, iv, crypto_skcipher_ivsize(skcipher)); //*
        ret = crypto_skcipher_encrypt(skcipher, &sg[1], &sg[0], DATA_SIZE);
        if (ret) {
                printk(KERN_ERR PFX "encryption failed, flags= \n");// MODIFICAR
                goto out_kfree;
        }

        crypto_skcipher_set_iv(skcipher, iv, crypto_skcipher_ivsize(skcipher));
        ret = crypto_skcipher_decrypt(skcipher, &sg[2], &sg[1], DATA_SIZE);
        if (ret) {
                printk(KERN_ERR PFX "decryption failed, flags= \n"); // modificado
                goto out_kfree;
        }

        printk(KERN_ERR PFX "IN: "); hexdump(input, DATA_SIZE);
        printk(KERN_ERR PFX "EN: "); hexdump(encrypted, DATA_SIZE);
        printk(KERN_ERR PFX "DE: "); hexdump(decrypted, DATA_SIZE);

        if (memcmp(input, decrypted, DATA_SIZE) != 0)
                printk(KERN_ERR PFX "FAIL: input buffer != decrypted buffer\n");
        else
                printk(KERN_ERR PFX "PASS: encryption/decryption verified\n");

out_kfree:
        kfree(decrypted);
        kfree(encrypted);
        kfree(input);

out:
        crypto_free_skcipher(skcipher);
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
