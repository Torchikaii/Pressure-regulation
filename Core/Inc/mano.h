extern int counter;
extern int mux_counter;

// other code for examples, this is unused BEGIN
#define LED1On() HAL_GPIO_WritePin(RGB_LD1_GPIO_Port,RGB_LD1_Pin,GPIO_PIN_SET) 
#define LED1Off() HAL_GPIO_WritePin(RGB_LD1_GPIO_Port,RGB_LD1_Pin,GPIO_PIN_RESET)

#define GPIOOn() HAL_GPIO_WritePin(GPIO_OutPB15_GPIO_Port,GPIO_OutPB15_Pin,GPIO_PIN_SET) 
#define GPIOOff() HAL_GPIO_WritePin(GPIO_OutPB15_GPIO_Port,GPIO_OutPB15_Pin,GPIO_PIN_RESET)
// other code for examples, this is unused END

// each of 4 7 segment display's segments adresses
#define Aseg 0x40   // PC6
#define Bseg 0x10   // PC4
#define Cseg 0x08   // PC3
#define Dseg 0x02   // PC1
#define Eseg 0x04   // PC2
#define Fseg 0x20   // PC5
#define Gseg 0x01   // PC0
#define DPseg 0x80   // PC7

#define COM1 ((unsigned char)~0x08)
#define COM2 ((unsigned char)~0x04)
#define COM3 ((unsigned char)~0x02)
#define COM4 ((unsigned char)~0x01)

void DisplayDigit(void);


