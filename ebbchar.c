#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function
#include <linux/mutex.h>	  /// Required for the mutex functionality
#include<linux/moduleparam.h>
#include<linux/init.h>
#include<linux/mm.h>
#include <linux/scatterlist.h> // modificado de "asm" para "linux" 
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/kmod.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <crypto/skcipher.h>
#include <linux/stat.h>  
 
            


MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality

//MODIFICADO
#define  DEVICE_NAME "cryptoDevice"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "criptografia"        ///< The device class -- this is a character device driver
#define DATA_SIZE 32
#define CRYPTO_skcipher_MODE_CBC 0
#define CRYPTO_skcipher_MODE_MASK 0
 
static char key[DATA_SIZE];
module_param_string(key,key,DATA_SIZE,0);

static char iv[DATA_SIZE];
module_param_string(iv,iv,DATA_SIZE,0);


 
static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[513] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer

        /* config options */
        char *algo = "cbc(aes)"; // modificado: anterior "aes"
        int   mode = CRYPTO_skcipher_MODE_CBC;
	int   mask = CRYPTO_skcipher_MODE_MASK; // adicionado
   

        /* local variables */
static struct crypto_skcipher *tfm; // utilzamos skcipher ao inves de tfm
static struct scatterlist sg[8]; // entender o q eh scatterlist
static struct skcipher_request *req = NULL; // necessario apra criptografar
        int    ret;

        char   *input, *encrypted, *decrypted;



static DEFINE_MUTEX(ebbchar_mutex);  /// A macro that is used to declare a new mutex that is visible in this file
                                     /// results in a semaphore variable ebbchar_mutex with value 1 (unlocked)
                                     /// DEFINE_MUTEX_LOCKED() results in a variable with value 0 (locked)


// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
 
/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};
 
/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */

static void hexdump(unsigned char *buf, unsigned int len)
{
        while (len--)
                printk("%02x", *buf++);

        printk("\n");
}

static int __init ebbchar_init(void){
   printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");
 
   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "EBBChar failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);
 
   // Register the device class
   ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "EBBChar: device class registered correctly\n");
 
   // Register the device driver
   ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
      class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ebbcharDevice);
   }

	///////////////////////////////////




	tfm = crypto_alloc_skcipher(algo, mode, mask);

        if (IS_ERR(tfm)) {
                printk("failed to load transform for %s %s\n", algo, mode == CRYPTO_skcipher_MODE_CBC ? "CBC" : "");
                //return; ***
        }
        
        ret = crypto_skcipher_setkey(tfm, key, sizeof(key));

        if (ret) {
                //printk(KERN_ERR PFX "setkey() failed flags= \n"); // FALTA FALAR SOBRE ESSE ERRO DE FLAG
                //goto out;
        }

        req = skcipher_request_alloc(tfm, GFP_KERNEL); // o que é GFP_KERNEL????
    	if (!req) {
        	printk("could not allocate tfm request\n");
        	////goto out;
    	}


        input = kmalloc(DATA_SIZE, GFP_KERNEL); // Parametros estavam invertidos
        if (!input) {
               // printk(KERN_ERR PFX "kmalloc(input) failed\n");
                ///goto out;
        }

        encrypted = kmalloc(DATA_SIZE, GFP_KERNEL); // Parametros estavam invertidos
        if (!encrypted) {
                //printk(KERN_ERR PFX "kmalloc(encrypted) failed\n");
                kfree(input);
                ///goto out;
        }

        decrypted = kmalloc(DATA_SIZE, GFP_KERNEL); // Parametros estavam invertidos
        if (!decrypted) {
                //printk(KERN_ERR PFX "kmalloc(decrypted) failed\n");
                kfree(encrypted);
                kfree(input);
                ///goto out;
        }



	 sg_init_one(&sg[0], input, DATA_SIZE);          //
        sg_init_one(&sg[1], encrypted, DATA_SIZE);      //      entrou no lugar do FILL_SG();
        sg_init_one(&sg[2], decrypted, DATA_SIZE);      //


/////////////////////

   mutex_init(&ebbchar_mutex);//Start mutex

   printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized
   return 0;
}


 
/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit ebbchar_exit(void){

   mutex_destroy(&ebbchar_mutex);        /// destroy the dynamically-allocated mutex

   device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(ebbcharClass);                          // unregister the device class
   class_destroy(ebbcharClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
}
 
/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){

   if(!mutex_trylock(&ebbchar_mutex)){    /// Try to acquire the mutex (i.e., put the lock on/down)
                                          /// returns 1 if successful and 0 if there is contention
      printk(KERN_ALERT "EBBChar: Device in use by another process");
      return -EBUSY;
   }

   numberOpens++;
   printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}
 
/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success

 
	
   error_count = copy_to_user(buffer, encrypted, size_of_message);
 
   if (error_count==0){            // if true then have success
      printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}
 
/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	int i=0;   
	
	sprintf(message, "%s(%zu letters)", buffer, len);   // appending received string with its length
   	printk("EBBCHAR dado de entrada = %s\n",message);
	

	for(i = 0; i < len-2; i++){
		message[i] = message[i+2];
	}
	message[len-2] = '\0';

	printk("EBBCHAR dado de entrada = %s\n",message);

  if(message[0]=='c'){

	skcipher_request_set_crypt(req, &sg[0], &sg[1], DATA_SIZE, iv); // ordem ods parametros: requisição, origem, destino, tamanho, iv;
  }
	

  else if(message[0] == 'd'){
	
	 skcipher_request_set_crypt(req, &sg[1], &sg[0], DATA_SIZE, iv); //Descriptar usa a ordem inversa	
  }
   	
   size_of_message = strlen(message);                 // store the length of the stored message
   printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);
   return len;
}
 
/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){

   mutex_unlock(&ebbchar_mutex);          /// Releases the mutex (i.e., the lock goes up)

   printk(KERN_INFO "EBBChar: Device successfully closed\n");
   return 0;
}
 
/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(ebbchar_init);
module_exit(ebbchar_exit);
