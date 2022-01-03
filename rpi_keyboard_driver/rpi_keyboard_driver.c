#include <linux/module.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define DRIVER_NAME     "rpi_keyboard_driver"

#define COL0			17
#define COL1			27
#define COL2			22
#define ROW0			18
#define ROW1			23
#define ROW2			24
#define ROW3			25
#define LED_PIN			0

#define cols 			3
#define rows 			4

uint8_t Rows_pin[rows] = {ROW0, ROW1, ROW2, ROW3};
uint8_t Cols_pin[cols] = {COL0, COL1, COL2};
const char keymap[rows][cols] = 
{
	{'1', '2', '3'},
	{'4', '5', '6'},
	{'7', '8', '9'},
	{'*', '0', '#'}
};
static char key_pressed = '\0';
char Keypad_Read(void)
{
	//bool Rows_state[rows] = {0, 0, 0, 0};
	//bool Cols_state[cols] = {0, 0, 0};
	char temp_key_pressed = '\0';
	//i = index of row, j = index of column
	uint8_t i = 0;
	for(i = 0; i < cols; i++)
	{
		gpio_set_value(Rows_pin[0], 1);
		gpio_set_value(Rows_pin[1], 0);
		gpio_set_value(Rows_pin[2], 0);
		gpio_set_value(Rows_pin[3], 0);
		if(gpio_get_value(Cols_pin[i]) == 1)	temp_key_pressed = keymap[0][i];

		gpio_set_value(Rows_pin[0], 0);
		gpio_set_value(Rows_pin[1], 1);
		gpio_set_value(Rows_pin[2], 0);
		gpio_set_value(Rows_pin[3], 0);
		if(gpio_get_value(Cols_pin[i]) == 1)	temp_key_pressed = keymap[1][i];

		gpio_set_value(Rows_pin[0], 0);
		gpio_set_value(Rows_pin[1], 0);
		gpio_set_value(Rows_pin[2], 1);
		gpio_set_value(Rows_pin[3], 0);
		if(gpio_get_value(Cols_pin[i]) == 1)	temp_key_pressed = keymap[2][i];

		gpio_set_value(Rows_pin[0], 0);
		gpio_set_value(Rows_pin[1], 0);
		gpio_set_value(Rows_pin[2], 0);
		gpio_set_value(Rows_pin[3], 1);
		if(gpio_get_value(Cols_pin[i]) == 1)	temp_key_pressed = keymap[3][i];
	}
	return temp_key_pressed;
}

/*************** Driver functions **********************/
static int driver_open(struct inode *device_file, struct file *instance);
static int driver_release(struct inode *device_file, struct file *instance);
static ssize_t driver_read(struct file *File, char *user_buffer, size_t size, loff_t *offs);
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t size, loff_t *offs);
/******************************************************/

static struct proc_dir_entry *driver_proc = NULL;

static struct proc_ops fops = 
{
    //.owner      = THIS_MODULE,
    .proc_open       = driver_open,
    .proc_release    = driver_release,
    .proc_read       = driver_read,
    .proc_write      = driver_write
};

//This used for storing the IRQ number for the GPIO
unsigned int Cols_irqNumber[cols];

//Interrupt handler for GPIO 25. This will be called whenever there is a raising edge detected. 
static irqreturn_t col0_irq_handler(int irq,void *dev_id) 
{
	static unsigned long flag0 = 0;

	local_irq_save(flag0);
	key_pressed = Keypad_Read();
	if (key_pressed == '1') 
    {
        //set the GPIO value to HIGH
        gpio_set_value(LED_PIN, 1);
    } else if (key_pressed == '0') 
    {
        //set the GPIO value to LOW
        gpio_set_value(LED_PIN, 0);
    } else 
    {
        //pr_err("Unknown command : Please provide either 1 or 0 \n");
    }
	local_irq_restore(flag0);
	return IRQ_HANDLED;
}
static irqreturn_t col1_irq_handler(int irq,void *dev_id) 
{
	static unsigned long flag1 = 0;

	local_irq_save(flag1);
	key_pressed = Keypad_Read();
	if (key_pressed == '1') 
    {
        //set the GPIO value to HIGH
        gpio_set_value(LED_PIN, 1);
    } else if (key_pressed == '0') 
    {
        //set the GPIO value to LOW
        gpio_set_value(LED_PIN, 0);
    } else 
    {
        //pr_err("Unknown command : Please provide either 1 or 0 \n");
    }
	local_irq_restore(flag1);
	return IRQ_HANDLED;
}
static irqreturn_t col2_irq_handler(int irq,void *dev_id) 
{
	static unsigned long flag2 = 0;

	local_irq_save(flag2);
	key_pressed = Keypad_Read();
	if (key_pressed == '1') 
    {
        //set the GPIO value to HIGH
        gpio_set_value(LED_PIN, 1);
		pr_info("Turn on the LED");
    } else if (key_pressed == '0') 
    {
        //set the GPIO value to LOW
        gpio_set_value(LED_PIN, 0);
		pr_info("Turn off the LED");
    } else 
    {
        //pr_err("Unknown command : Please provide either 1 or 0 \n");
    }
	local_irq_restore(flag2);
	return IRQ_HANDLED;
}

/*
** This function will be called when we open the Device file
*/ 
static int driver_open(struct inode *device_file, struct file *instance)
{
  pr_info("Device File of %s Opened...!!!\n", DRIVER_NAME);
  return 0;
}

/*
** This function will be called when we close the Device file
*/
static int driver_release(struct inode *device_file, struct file *instance)
{
  pr_info("Device File of %s Closed...!!!\n", DRIVER_NAME);
  return 0;
}

static ssize_t driver_read(struct file *File, char *user_buffer, size_t size, loff_t *offs)
{
    
    //write back to user
    size = 1;
    if(copy_to_user(user_buffer, &key_pressed, size) > 0)
    {
        pr_err("ERROR: Not all the bytes have been copied to user");
    }
	//key_pressed = Keypad_Read();
	if(key_pressed != '\0')
		pr_info("Key pressed = %c\n", key_pressed);
	
    return 0;
}

static ssize_t driver_write(struct file *File, const char *user_buffer, size_t size, loff_t *offs)
{
    char temp_buffer[10];

    if(copy_from_user(temp_buffer, user_buffer, size) > 0)
    {
        pr_err("ERROR: Not all the bytes have been copied from user");
    }
    pr_info("Write function: GPIO_0 set = %c\n", temp_buffer[0]);

/*
    if (key_pressed == '1') 
    {
        //set the GPIO value to HIGH
        gpio_set_value(LED_PIN, 1);
    } else if (key_pressed == '0') 
    {
        //set the GPIO value to LOW
        gpio_set_value(LED_PIN, 0);
    } else 
    {
        pr_err("Unknown command : Please provide either 1 or 0 \n");
    }
	*/
    return size;
}

static int rpi_keyboard_driver_init(void)
{
	pr_info("Hello kernel, this is %s", DRIVER_NAME);
	driver_proc = proc_create(DRIVER_NAME, 0666, NULL, &fops);
	if(driver_proc == NULL)	return -1;
	//Checking the GPIO is valid or not
	if(gpio_is_valid(LED_PIN) == false){
		pr_err("GPIO %d is not valid\n", LED_PIN);
		return -1;
	}
	
	//Requesting the GPIO
	if(gpio_request(LED_PIN,"LED_PIN") < 0){
		pr_err("ERROR: GPIO %d request\n", LED_PIN);
		gpio_free(LED_PIN);
		return -1;
	}
	gpio_direction_output(LED_PIN, 1);
	gpio_export(LED_PIN, false);
	
	uint8_t i = 0;
	//Rows = output, cols = input
	for(i = 0; i < rows; i++)
	{
		//Checking the GPIO is valid or not
		if(gpio_is_valid(Rows_pin[i]) == false){
			pr_err("GPIO %d is not valid\n", Rows_pin[i]);
			return -1;
		}
		
		//Requesting the GPIO
		if(gpio_request(Rows_pin[i],"Row_pin") < 0){
			pr_err("ERROR: GPIO %d request\n", Rows_pin[i]);
			gpio_free(LED_PIN);
			return -1;
		}
		gpio_direction_output(Rows_pin[i], 1);
		gpio_export(Rows_pin[i], false);
	}

	for(i = 0; i < cols; i++)
	{
		//Checking the GPIO is valid or not
		if(gpio_is_valid(Cols_pin[i]) == false){
			pr_err("GPIO %d is not valid\n", Cols_pin[i]);
			return -1;
		}
		
		//Requesting the GPIO
		if(gpio_request(Cols_pin[i],"Column_pin") < 0){
			pr_err("ERROR: GPIO %d request\n", Cols_pin[i]);
			gpio_free(LED_PIN);
			return -1;
		}
		gpio_direction_input(Cols_pin[i]);
		gpio_export(Cols_pin[i], false);			
	}	
	//Get the IRQ number for our GPIO
	for(i = 0; i < cols; i++)
	{
		Cols_irqNumber[i] = gpio_to_irq(Cols_pin[i]);
		pr_info("GPIO_irqNumber = %d\n", Cols_irqNumber[i]);
	}
	if (request_irq(Cols_irqNumber[0],             //IRQ number
					(void *)col0_irq_handler,   //IRQ handler
					IRQF_TRIGGER_HIGH,        //Handler will be called in raising edge
					DRIVER_NAME,               //used to identify the device name using this IRQ
					NULL)) 
					{                    //device id for shared IRQ
						pr_err("my_device: cannot register IRQ ");
						gpio_free(Cols_pin[0]);
						return -1;
					}
	if (request_irq(Cols_irqNumber[1],             //IRQ number
					(void *)col1_irq_handler,   //IRQ handler
					IRQF_TRIGGER_HIGH,        //Handler will be called in raising edge
					DRIVER_NAME,               //used to identify the device name using this IRQ
					NULL)) 
					{                    //device id for shared IRQ
						pr_err("my_device: cannot register IRQ ");
						gpio_free(Cols_pin[1]);
						return -1;
					}
	if (request_irq(Cols_irqNumber[2],             //IRQ number
					(void *)col2_irq_handler,   //IRQ handler
					IRQF_TRIGGER_HIGH,        //Handler will be called in raising edge
					DRIVER_NAME,               //used to identify the device name using this IRQ
					NULL)) 
					{                    //device id for shared IRQ
						pr_err("my_device: cannot register IRQ ");
						gpio_free(Cols_pin[2]);
						return -1;
					}
	pr_info("%s insert...Done!!!\n", DRIVER_NAME);
	return 0;
}

static void rpi_keybpard_driver_exit(void)
{
    gpio_unexport(LED_PIN);
    gpio_free(LED_PIN);
	uint8_t i = 0;
	for(i = 0; i < cols; i++)
	{
		free_irq(Cols_irqNumber[i], NULL);
	}
	for(i = 0; i < cols; i++)
	{
		gpio_free(Cols_pin[i]);
	}
	for(i = 0; i < rows; i++)
	{
		gpio_free(Rows_pin[i]);
	}
	proc_remove(driver_proc);
	pr_info("%s Remove...Done!!\n", DRIVER_NAME);
}

module_init(rpi_keyboard_driver_init);
module_exit(rpi_keybpard_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dat");
MODULE_DESCRIPTION("Keypad_3x4 Driver");
MODULE_VERSION("1.0");