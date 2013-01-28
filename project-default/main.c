#include "stm32f0xx_conf.h"
// #include "snd1.h"

// const uint32_t snd_a[8] = {0xb8f, 0xbe3, 0xbc0, 0xb44, 0xbff, 0xb44, 0xbe3, 0xbc0};
// const uint16_t snd_a[32] = {
//   2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056, 4095, 4056,
//   3939, 3750, 3495, 3185, 2831, 2447, 2047, 1647, 1263, 909,
//   599, 344, 155, 38, 0, 38, 155, 344, 599, 909, 1263, 1647
// };

uint32_t get_random() {
  static uint32_t z = 19, w = 13;
  z = 36969 * (z & 65535) + (z >> 16);
  w = 18000 * (w & 65535) + (w >> 16);
  return (z << 16) + w;  /* 32-bit result */
}

/*
  Big endian bits

  0         1         2         3
  01234567890123456789012345678901
  P0  P2  P4  P6  P8  P10 P12 P14
    P1  P3  P5  P7  P9  P11 P13 P15
*/
void GPIO_Init_Pin(GPIO_TypeDef *reg, uint16_t pin, GPIOMode_TypeDef mode,
  GPIOSpeed_TypeDef speed) {

  GPIO_InitTypeDef ginit;
  ginit.GPIO_Mode = mode;
  ginit.GPIO_OType = GPIO_OType_PP;
  ginit.GPIO_Pin = pin;
  ginit.GPIO_PuPd = GPIO_PuPd_NOPULL;
  ginit.GPIO_Speed = speed;
  GPIO_Init(reg, &ginit);
}

void GPIO_Toggle_Bit(GPIO_TypeDef *reg, uint16_t pin) {
  BitAction current = GPIO_ReadInputDataBit(reg, pin);
  GPIO_WriteBit(reg, pin, !current);
}

void SysTick_Handler(void) {
  static uint16_t tick = 0;
  static uint16_t max_ticks = 2000;
  static int8_t state = 0, next_state = 0;

  tick++;
  if (tick >= max_ticks) {
    tick = 0;
    // On-board blue LED
    // GPIO_Toggle_Bit(GPIOC, GPIO_Pin_8);
    if (0 == state) {
      // GPIO_Toggle_Bit(GPIOB, GPIO_Pin_9);
    }

    GPIO_Toggle_Bit(GPIOB, GPIO_Pin_9);
  }

  // On-board User (blue) button
  if (RESET == GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)) {
    next_state = !state;
  } else if (next_state != state) {
    state = next_state;

    switch (state) {
    case 0:
      // On-board blue LED
      GPIO_ResetBits(GPIOC, GPIO_Pin_8);
      DAC_Cmd(DAC_Channel_1, DISABLE);
      break;
    case 1:
      // On-board blue LED
      GPIO_SetBits(GPIOC, GPIO_Pin_8);
      DAC_Cmd(DAC_Channel_1, ENABLE);
      break;
    }
  }
}

void TIM6_DAC_IRQHandler(void) {
  static uint32_t t = 0;

  TIM6->SR &= ~TIM_SR_UIF;
  t++;

  uint32_t x = t*(t>>5|t>>8)>>(t>>16);
  DAC->DHR12R1 = x << 4;
  // DAC->DHR12R1 = get_random();

  // DAC->DHR8R1 = snd_a[t] << 4;
  // if (t >= sizeof(snd_a)) {
  //   t = 0;
  //     DAC_Cmd(DAC_Channel_1, DISABLE);
  // }
}

int main(void) {
  // Enable clock to GPIO ports
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
  // RCC_APB1PeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

  // Configure GPIO pins
  GPIO_Init_Pin(GPIOA, GPIO_Pin_0, GPIO_Mode_IN, GPIO_Speed_2MHz);
  GPIO_Init_Pin(GPIOA, GPIO_Pin_4, GPIO_Mode_AN, GPIO_Speed_2MHz);
  GPIO_Init_Pin(GPIOB, GPIO_Pin_9, GPIO_Mode_OUT, GPIO_Speed_50MHz);
  GPIO_Init_Pin(GPIOC, GPIO_Pin_8, GPIO_Mode_OUT, GPIO_Speed_50MHz);
  GPIO_Init_Pin(GPIOC, GPIO_Pin_9, GPIO_Mode_OUT, GPIO_Speed_50MHz);

  // Turn off yellow LED on back
  GPIO_SetBits(GPIOB, GPIO_Pin_9);

  TIM6->PSC = 0x0;
  TIM6->ARR = 6000;
  TIM6->DIER |= TIM_DIER_UIE; // interrupt on overflow
  // TIM6->DIER |= TIM_DIER_UDE; // enable ???
  TIM6->CR2 |= TIM_CR2_MMS_1;
  TIM6->CR1 |= TIM_CR1_CEN;
  NVIC_EnableIRQ(TIM6_DAC_IRQn);

  DAC_InitTypeDef dac_init;
  dac_init.DAC_Trigger = DAC_Trigger_None;
  dac_init.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  // dac_init.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
  DAC_Init(DAC_Channel_1, &dac_init);

  // DMA1_Channel3->CCR |= DMA_CCR_MSIZE_0; // будем передавать 16 бит данных
  // DMA1_Channel3->CCR |= DMA_CCR_PSIZE_0;
  // DMA1_Channel3->CCR |= DMA_CCR_CIRC;    // цыклическая передача
  // DMA1_Channel3->CCR |= DMA_CCR_DIR;     // чтение с памяти
  // DMA1_Channel3->CNDTR = 1;              // для того чтобы работало
  // DMA1_Channel3->CPAR = (uint32_t) &DAC->DHR12R1;
  // DMA1_Channel3->CMAR = (uint32_t) &a;
  // DMA1_Channel3->CCR |= DMA_CCR_EN;      // вкл. ПДП

  // DAC_Cmd(DAC_Channel_1, ENABLE);
  SysTick_Config(SystemCoreClock / 1000);

  while(1) {
    PWR_EnterSleepMode(PWR_SLEEPEntry_WFE);
  }
}
