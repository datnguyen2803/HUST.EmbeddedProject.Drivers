/***************************************************************************//**
*  \file       gpio.c
*
*  \details    Simple GPIO device driver
*
*  \author     datnguyen
*
*  \Tested with Linux raspberrypi 5.10.90-v7+
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>

#define DRIVER_NAME     "Keypad Driver"
#define DRIVER_FILE_NAME "Keypad_Driver_File"


#define cols    3
#define rows    4

#define COL0 5          //keypad pin 5
#define COL1 6          //keypad pin 6
#define COL2 13         //keypad pin 7
#define ROW0 23         //keypad pin 1
#define ROW1 17         //keypad pin 2
#define ROW2 27         //keypad pin 3
#define ROW3 22         //keypad pin 4

#define LED_PIN 0

const char Cols_pin[cols] = {COL0, COL1, COL2};
const char Rows_pin[rows] = {ROW0, ROW1, ROW2, ROW3};
const char Keymap[rows][cols] = 
{
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};
unsigned int Cols_irqNumber[cols];

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

extern unsigned long volatile jiffies;
unsigned long old_jiffie = 0;

static char key_pressed = '\0';
const int time_interval = 20;
int keypad_init(void);
void keypad_release(void);
char keypad_scan(void);
static irqreturn_t col0_irq_handler(int irq,void *dev_id) ;
static irqreturn_t col1_irq_handler(int irq,void *dev_id) ;
static irqreturn_t col2_irq_handler(int irq,void *dev_id) ;

//Function for Keypad
int keypad_init(void)
{
        int i = 0;
        for(i = 0; i < rows; i++)
        {
                if(gpio_is_valid(Rows_pin[i]) == false)
                {
                        pr_err("GPIO %d is not valid\n", Rows_pin[i]);
                        return -1;
                }
                //Requesting the GPIO
                if(gpio_request(Rows_pin[i],"rows_pin") < 0){
                        pr_err("ERROR: GPIO %d request\n", Rows_pin[i]);
                        return -1;
                }
                //configure the GPIO as output
                gpio_direction_output(Rows_pin[i], 1);  
        }
        for(i = 0; i < cols; i++)
        {
                //Checking the BUTTON is valid or not
                if(gpio_is_valid(Cols_pin[i]) == false){
                        pr_err("GPIO %d is not valid\n", Cols_pin[i]);
                        return -1;
                }
                //Requesting the GPIO
                if(gpio_request(Cols_pin[i],"BUTTONPIN") < 0){
                        pr_err("ERROR: GPIO %d request\n", Cols_pin[i]);
                        return -1;
                }
                //configure the GPIO as input
                gpio_direction_input(Cols_pin[i]);
                
                //Get the IRQ number for our GPIO
                Cols_irqNumber[i] = gpio_to_irq(Cols_pin[i]);
                pr_info("Cols_irqNumber[%d] = %d\n", i, Cols_irqNumber[i]);
                
        }
        if (request_irq(Cols_irqNumber[0],             //IRQ number
                                (void *)col0_irq_handler,   //IRQ handler
                                IRQF_TRIGGER_RISING,        //Handler will be called in raising edge
                                DRIVER_FILE_NAME,               //used to identify the device name using this IRQ
                                NULL)) {                    //device id for shared IRQ
                        pr_err("GPIO_device: cannot register IRQ ");
                        return -1;
                }
        if (request_irq(Cols_irqNumber[1],             //IRQ number
                                (void *)col1_irq_handler,   //IRQ handler
                                IRQF_TRIGGER_RISING,        //Handler will be called in raising edge
                                DRIVER_FILE_NAME,               //used to identify the device name using this IRQ
                                NULL)) {                    //device id for shared IRQ
                        pr_err("GPIO_device: cannot register IRQ ");
                        return -1;
                }
        if (request_irq(Cols_irqNumber[2],             //IRQ number
                                (void *)col2_irq_handler,   //IRQ handler
                                IRQF_TRIGGER_RISING,        //Handler will be called in raising edge
                                DRIVER_FILE_NAME,               //used to identify the device name using this IRQ
                                NULL)) {                    //device id for shared IRQ
                        pr_err("GPIO_device: cannot register IRQ ");
                        return -1;
                }
        return 1;
}

void keypad_release(void)
{
        int i = 0;
        for(i = 0; i < cols; i++)
        {
                free_irq(Cols_irqNumber[i], NULL);
                gpio_free(Cols_pin[i]);
        }
        for(i = 0; i < rows; i++)
        {
                gpio_free(Rows_pin[i]);
        }
  
}

char keypad_scan(void)
{
        char key_pressed = '\0';
                
        uint8_t col_state = 0;
        int i, j;
        for(i = 0; i < rows; i++)
        {
                for(j = 0; j < rows; j++)
                {
                        gpio_set_value(Rows_pin[j], 0);
                }
                gpio_set_value(Rows_pin[i], 1);
                for(j = 0; j < cols; j++)
                {
                        col_state = gpio_get_value(Cols_pin[j]);
                        if(col_state == 1)
                        {
                                key_pressed = Keymap[i][j];
                                break;
                        }
                }
        }  
        
        return key_pressed;
}

//Interrupt handler for Cols_pin. This will be called whenever there is a raising edge detected. 
static irqreturn_t col0_irq_handler(int irq,void *dev_id) 
{
        static unsigned long flags = 0;
        //char key_pressed = '\0';
        int i = 0;

        unsigned long diff = jiffies - old_jiffie;
        if (diff < time_interval)
        {
                return IRQ_HANDLED;
        }
        
        old_jiffie = jiffies;

        local_irq_save(flags);
        key_pressed = keypad_scan();
        //led_toggle = (0x01 ^ led_toggle);                        // toggle the old value
        //gpio_set_value(LEDPIN, led_toggle);                      // toggle the GPIO_21_OUT
        pr_info("Interrupt Occurred : Key pressed : %c ", key_pressed);
        for(i = 0; i < rows; i++)
        {
                gpio_set_value(Rows_pin[i], 1);
        }
        local_irq_restore(flags);
        return IRQ_HANDLED;
}
static irqreturn_t col1_irq_handler(int irq,void *dev_id) 
{
        static unsigned long flags = 0;
        //char key_pressed = '\0';
        int i = 0;

        unsigned long diff = jiffies - old_jiffie;
        if (diff < time_interval)
        {
                return IRQ_HANDLED;
        }
        
        old_jiffie = jiffies;

        local_irq_save(flags);
        key_pressed = keypad_scan();
        //led_toggle = (0x01 ^ led_toggle);                        // toggle the old value
        //gpio_set_value(LEDPIN, led_toggle);                      // toggle the GPIO_21_OUT
        pr_info("Interrupt Occurred : Key pressed : %c ", key_pressed);
        for(i = 0; i < rows; i++)
        {
                gpio_set_value(Rows_pin[i], 1);
        }
        local_irq_restore(flags);
        return IRQ_HANDLED;
}
static irqreturn_t col2_irq_handler(int irq,void *dev_id) 
{
        static unsigned long flags = 0;
        //char key_pressed = '\0';
        int i = 0;

        unsigned long diff = jiffies - old_jiffie;
        if (diff < time_interval)
        {
                return IRQ_HANDLED;
        }
        
        old_jiffie = jiffies;

        local_irq_save(flags);
        key_pressed = keypad_scan();
        //led_toggle = (0x01 ^ led_toggle);                        // toggle the old value
        //gpio_set_value(LEDPIN, led_toggle);                      // toggle the GPIO_21_OUT
        pr_info("Interrupt Occurred : Key pressed : %c ", key_pressed);
        for(i = 0; i < rows; i++)
        {
                gpio_set_value(Rows_pin[i], 1);
        }
        local_irq_restore(flags);
        return IRQ_HANDLED;
}

/*
** Function Prototypes
*/
static int      __init etx_driver_init(void);
static void     __exit etx_driver_exit(void);
static int      etx_open(struct inode *inode, struct file *file);
static int      etx_release(struct inode *inode, struct file *file);
static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);

static struct file_operations fops =
{
    .owner      = THIS_MODULE,
    .read       = etx_read,
    .write      = etx_write,
    .open       = etx_open,
    .release    = etx_release,
};

/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file)
{
        pr_info("%s Open Function Called...!!!\n", DRIVER_NAME);
        return 0;
}

/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
        pr_info("%s Release Function Called...!!!\n", DRIVER_NAME);
        return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        //char key_pressed = '\0';
        //reading GPIO value
        key_pressed = keypad_scan();
        
        if(key_pressed)
        {
                //write to user
                len = 1;
                if( copy_to_user(buf, &key_pressed, len) > 0) {
                        pr_err("ERROR: Not all the bytes have been copied to user\n");
                }
                pr_info("Read function: Key pressed = %c\n", key_pressed);
        }
        return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        uint8_t rec_buf[10] = {0};
  
        if( copy_from_user( rec_buf, buf, len ) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
        }
        
        pr_info("Write Function : LED Set = %c\n", rec_buf[0]);
        
        if (rec_buf[0]=='1') {
                //set the GPIO value to HIGH
                gpio_set_value(LED_PIN, 1);
        } else if (rec_buf[0]=='0') {
                //set the GPIO value to LOW
                gpio_set_value(LED_PIN, 0);
        } else {
                pr_err("Unknown command : Please provide either 1 or 0 \n");
        }
        return len;
}

/*
** Module Init function
*/
static int __init etx_driver_init(void)
{
        /*Allocating Major number*/
        if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
                pr_err("Cannot allocate major number\n");
                goto r_unreg;
        }
        pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

        /*Creating cdev structure*/
        cdev_init(&etx_cdev,&fops);

        /*Adding character device to the system*/
        if((cdev_add(&etx_cdev,dev,1)) < 0){
            pr_err("Cannot add the device to the system\n");
            goto r_del;
        }

        /*Creating struct class*/
        if((dev_class = class_create(THIS_MODULE,"etx_class")) == NULL){
            pr_err("Cannot create the struct class\n");
            goto r_class;
        }

        /*Creating device*/
        if((device_create(dev_class,NULL,dev,NULL,DRIVER_FILE_NAME)) == NULL){
            pr_err("Cannot create the Device 1\n");
            goto r_device;
        }

        //Checking the GPIO is valid or not
        if(gpio_is_valid(LED_PIN) == false){
                pr_err("GPIO %d is not valid\n", LED_PIN);
                goto r_device;
        }
        
        //Requesting the GPIO
        if(gpio_request(LED_PIN,"LED_PIN") < 0){
                pr_err("ERROR: GPIO %d request\n", LED_PIN);
                goto r_led;
        }
        
        //configure the GPIO as output
        gpio_direction_output(LED_PIN, 1);

        if(keypad_init() < 0)
        {
                goto r_keypad;
        }

        pr_info("%s Insert...Done!!!\n", DRIVER_NAME);
        return 0;

r_keypad:
        keypad_release();
r_led:
        gpio_free(LED_PIN);
r_device:
        device_destroy(dev_class, dev);
r_class:
        class_destroy(dev_class);
r_del:
        cdev_del(&etx_cdev);
r_unreg:
        unregister_chrdev_region(dev,1);

        return -1;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void)
{
        keypad_release();
        gpio_free(LED_PIN);
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        pr_info("%s Remove...Done!!!\n", DRIVER_NAME);
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Datnguyen");
MODULE_DESCRIPTION("Sample Linux device driver");
MODULE_VERSION("1.0");