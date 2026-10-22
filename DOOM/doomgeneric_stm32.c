#include "main.h"
#include "doomgeneric.h"
#include "doomkeys.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_dma.h"

#define frame ((uint16_t*)0x90640000)
extern void DrawFrame(void);


static uint16_t scale_x[240];
static uint16_t scale_y[240];

void DG_Init(void){

	 for(int i=0;i<240;i++){
		scale_x[i] = (i*320)/240;
	 }
	 for(int i=0;i<240;i++){
	 		scale_y[i] = (i*200)/240;
	 }

}

void DG_DrawFrame(void) {

	while (LL_DMA_IsEnabledStream(DMA1, LL_DMA_STREAM_0));

	for(int i=0;i<240;i++){
		uint32_t *src = &DG_ScreenBuffer[scale_y[i] * 320];
		uint16_t *dst = &frame[i*240];

		for(int j=0;j<240;j++){

			uint32_t c = src[scale_x[j]];

			  dst[j] = (uint16_t)( (((c >> 19) & 0x1F) << 11) | (((c >> 10) & 0x3F) << 5) | ((c >> 3)  & 0x1F) );

		}
	}

	DrawFrame();

}

void DG_SleepMs(uint32_t ms)
{
    HAL_Delay(ms);
}

uint32_t DG_GetTicksMs(void)
{
    return HAL_GetTick();
}



typedef struct {
	GPIO_TypeDef *port;
	uint32_t pin;
	unsigned char key;
	uint8_t last_state;
}button;


static button buttons[] = {
    {GPIOB, LL_GPIO_PIN_9, KEY_UPARROW,   1},
    {GPIOB, LL_GPIO_PIN_8, KEY_DOWNARROW, 1},
    {GPIOB, LL_GPIO_PIN_7, KEY_LEFTARROW, 1},
    {GPIOB, LL_GPIO_PIN_6, KEY_RIGHTARROW,1},
    {GPIOE, LL_GPIO_PIN_0, KEY_RCTRL,     1}, // Выстрел
    {GPIOE, LL_GPIO_PIN_1, KEY_ENTER,     1}
};

#define num_buttons (sizeof(buttons)/sizeof(buttons[0]))



int DG_GetKey(int* pressed, unsigned char* doomKey){

	for(int i=0;i< num_buttons; i++){

		uint8_t current_state = LL_GPIO_IsInputPinSet(buttons[i].port, buttons[i].pin);


	if(current_state != buttons[i].last_state){

		buttons[i].last_state = current_state;

		*pressed = (current_state == 0) ? 1 : 0;
		*doomKey = buttons[i].key;

		return 1;

	}


}
	return 0;
}


// 1. Заглушка для заголовка окна (на экране ST7789 заголовков нет)
void DG_SetWindowTitle(const char *title)
{
    (void)title;
}

// 2. Заглушки для джойстика ПК (у нас свои 6 кнопок!)
void I_InitJoystick(void)
{
}

void I_BindJoystickVariables(void)
{
}

// 3. Заглушка для создания папки на диске ПК (делаем вид, что папка создана)
int mkdir(const char *pathname, unsigned int mode)
{
    (void)pathname;
    (void)mode;
    return 0; // Возвращаем успех!
}


