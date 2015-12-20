#include "stm32f0xx.h"

#define PRESCALER       24000-1;         
#define AUTO_RELOAD     10000-1;         

#define TEMP110_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7C2))
#define TEMP30_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7B8))
#define VDD_CALIB ((uint16_t) (330))
#define VDD_APPLI ((uint16_t) (300))

//--------- LED INIT ----------
void Led_ini() {
  RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
  GPIOC->MODER |= (GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0);
  GPIOC->OTYPER = 0;
  GPIOC->OSPEEDR = 0;
}

//--------- TIMER INIT ----------
void Timer_ini(){
  RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
  TIM6->ARR = AUTO_RELOAD;                      
  TIM6->PSC = PRESCALER;                     
  TIM6->DIER |= TIM_DIER_UIE;
  TIM6->CR1 |= TIM_CR1_ARPE;
  NVIC_EnableIRQ(TIM6_DAC_IRQn); 
  TIM6->CR1 |= TIM_CR1_CEN;       
}

//--------- TEMPERATURE SENSOR INIT ----------
void Temp_sensor_ini(void){
  RCC->APB2ENR = RCC_APB2ENR_ADCEN;
  RCC->CR2 |= RCC_CR2_HSI14ON;
  while ((RCC->CR2 & RCC_CR2_HSI14RDY) == 0);
 
  ADC1->CHSELR = ADC_CHSELR_CHSEL16;
  
  // Temperature sensor colibration
  if ((ADC1->CR & ADC_CR_ADEN) != 0)
  {
    ADC1->CR &= (uint32_t)(~ADC_CR_ADEN);  
  }
  ADC1->CR |= ADC_CR_ADCAL; 
  while ((ADC1->CR & ADC_CR_ADCAL) != 0);
  
  ADC1->SMPR |= ADC_SMPR_SMP_0 | ADC_SMPR_SMP_1 | ADC_SMPR_SMP_2;
  ADC1->CFGR1 |= ADC_CFGR1_CONT;
  ADC->CCR |= ADC_CCR_TSEN;
  ADC1->CR |= ADC_CR_ADEN; 
  while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
  ADC1->CR |= ADC_CR_ADSTART;
}

void Temperature_show(uint32_t degree){
  // LED9 (Blue) will show decades degres of temperature and
  // LED8 (Green) will show units degres of temperature
  uint8_t units = degree % 10;
  uint8_t decades = (degree - units) / 10;
  
  if (degrees > 0){
    if (decades){
      for (decades = decades * 2; decades > 0; decades--){
        GPIOC->ODR ^= GPIO_ODR_8;
        for (uint32_t i=3000000; i>0; i--);
      }
    }  
    if (units){
      for ( units = units * 2 ; units > 0; units-- ){
        GPIOC->ODR ^= GPIO_ODR_9;
        for (uint32_t i=3000000; i>0; i--);
      }
    }
  } else if (degrees < 0) {
    if (degrees < 0){
      if (decades){
        for (decades = decades * 2; decades > 0; decades--){
          GPIOC->ODR ^= GPIO_ODR_8;
          for (uint32_t i=3000000; i>0; i--);
        }
      }  
      if (units){
        for ( units = units * 2 ; units > 0; units-- ){
          GPIOC->ODR ^= GPIO_ODR_9;
          for (uint32_t i=3000000; i>0; i--);
        }
      }
    }
  } else {
    GPIOC->ODR ^= GPIO_ODR_8;
    GPIOC->ODR ^= GPIO_ODR_9;  
  } 
}

int32_t Temerature_calc (uint32_t raw_data){
  int32_t temperature;
  temperature = ((raw_data * VDD_APPLI / VDD_CALIB) - (int32_t) *TEMP30_CAL_ADDR ) ;	
  temperature = temperature * (int32_t)(110 - 30);                      
  temperature = temperature / (int32_t)(*TEMP110_CAL_ADDR - *TEMP30_CAL_ADDR);                 
  temperature = temperature + 30;
  return(temperature);
}

void TIM6_DAC_IRQHandler(void){
  if (ADC1->DR){
    Temperature_show(Temerature_calc(ADC1->DR));
  }
  TIM6->SR &= ~TIM_SR_UIF;
}

int main(){
  Led_ini();
  Temp_sensor_ini();
  Timer_ini();  
  
  while(1){  
  }
}
