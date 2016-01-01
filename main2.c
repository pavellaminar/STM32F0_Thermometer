#include "stm32f0xx.h"

#define PRESCALER       48000-1;         
#define AUTO_RELOAD     10000;         

#define TEMP110_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7C2))
#define TEMP30_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7B8))
#define VDD_CALIB ((uint16_t) (330))
#define VDD_APPLI ((uint16_t) (300))

//--------- LED INIT ----------
void Led_ini() {
  RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
  GPIOC->MODER |= GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;
  GPIOC->OTYPER = 0;
  GPIOC->OSPEEDR = 0;
}

//--------- TIMER INIT ----------
void Timer_ini(){
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
  TIM3->PSC = PRESCALER;
  TIM3->ARR = AUTO_RELOAD;
  TIM3->CR2 |= TIM_CR2_MMS_1;
  TIM3->DIER |= TIM_DIER_TIE;
  TIM3->CR1 |= TIM_CR1_ARPE;
  TIM3->CR1 |= TIM_CR1_CEN;       
}

//--------- TEMPERATURE SENSOR INIT ----------
void Temp_sensor_ini(void){
  ADC1->CFGR2 |= ADC_CFGR2_CKMODE_1; 
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;  //
  ADC1->CFGR1 |= ADC_CFGR1_EXTEN_0;
  ADC1->CFGR1 |= ADC_CFGR1_EXTSEL_0 | ADC_CFGR1_EXTSEL_1;
  ADC1->CHSELR = ADC_CHSELR_CHSEL16;
  ADC1->SMPR |= ADC_SMPR_SMPR_0 | ADC_SMPR_SMPR_1; // 
  ADC1->CR |= ADC_CR_ADCAL;                       // Temperature sensor colibration
   while ((ADC1->CR & ADC_CR_ADCAL));
  ADC->CCR |= ADC_CCR_TSEN;  
  ADC1->CR |= ADC_CR_ADEN; 
   while (!(ADC1->ISR & ADC_ISR_ADRDY));
  ADC1->CR |= ADC_CR_ADSTART;
}

void Temperature_show(uint32_t degrees){
  // LED9 (Blue) will show decades of temperature and
  // LED8 (Green) will show units of temperature
  int8_t units = degrees % 10;
  int8_t decades = (degrees - units) / 10;
  
    if (decades){
      for (decades = decades * 2; decades > 0; decades--){
        GPIOC->ODR ^= GPIO_ODR_8;
        for (uint32_t i = 2500000; i > 0; i--);
      }
    }  
    if (units){
      for ( units = units * 2 ; units > 0; units-- ){
        GPIOC->ODR ^= GPIO_ODR_9;
        for (uint32_t i = 2500000; i > 0; i--);
      }
    }
}

int32_t Temerature_calc (int32_t raw_data){
  int32_t temperature;
  temperature = ((raw_data * VDD_APPLI / VDD_CALIB) - (int32_t) *TEMP30_CAL_ADDR ) ;	
  temperature = temperature * (int32_t)(110 - 30);                      
  temperature = temperature / (int32_t)(*TEMP110_CAL_ADDR - *TEMP30_CAL_ADDR);                 
  temperature = temperature + 30;
  return(temperature);
}


int main(){
  Led_ini();
  Temp_sensor_ini();
  Timer_ini(); 
  
  while(1){
    if ((ADC1->ISR & ADC_ISR_EOC)){      
      Temperature_show(Temerature_calc(ADC1->DR));
      //ADC1->ISR |= ADC_ISR_EOC;
    }
  }
}
