/*
  ADC Testing
  Uses Noise reduction ADC to read the analogue voltage
  using Band Gap internal 1.1 volt reference.
*/

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/sleep.h>

const uint16_t VCC {1100};          // Band Gap Reference voltage
const uint16_t RESOLUTION {1024};   // ADC resolution

static const uint8_t SAMPLE_CNT {8}; // ADC value is average of SAMPLE_CNT
volatile uint8_t  adcIRQCnt;
volatile uint16_t samples[SAMPLE_CNT+1];
/////////////////////////////////////////////////////////////////////////
// Function: Interrupt routine for ADC
// Purpose : Fired when ADC interrupt occured (mainly end of conversion)
//
/////////////////////////////////////////////////////////////////////////
ISR(ADC_vect)  
{ 
  // skip first conversion
  if ( adcIRQCnt < SAMPLE_CNT+1 ) { // Test we aren't out of bounds.
    uint8_t low, high;
    // read low first
    low  = ADCL;
    high = ADCH;
    samples[adcIRQCnt] = (high<<8) | low;  
  }
  adcIRQCnt++;
}

/////////////////////////////////////////////////////////////////////////
// Function: adcNoiseReduction
// Purpose : ADC with Sleep Mode of "ACD Noise Reduction" 
// Output  : average SAMPLE_CNT conversions.
/////////////////////////////////////////////////////////////////////////

uint16_t adcNoiseReduction()
{
  uint16_t sum = 0;
  // Set ADC Interrupt Flag, ADC Enable.
  ADCSRA |= _BV( ADIE ) | _BV(ADEN);
  
  // Loop thru taking samples. adcIRQCnt is incremented in interrupt handler.
  for (adcIRQCnt = 0; adcIRQCnt<SAMPLE_CNT+1;)
  {
    // Enable Noise Reduction Sleep Mode -- ADC starts on CPU halt.
    set_sleep_mode( SLEEP_MODE_ADC );
    sleep_enable();
    sei(); // enable interrupts
    // Sleep must be called after sei().
    sleep_cpu();
    // Awake again following interrupt handler.
    sleep_disable();
  }

  //Disable ADC, Auto Trigger Disable, Clear Interrup Flag, Disable ADC Interrupt.
  // Prescaler set to 128.
  ADCSRA = B00010111;
  Serial.print("samples: "); Serial.print(samples[0]); Serial.print("  :");
  // skip first sample
  for (uint8_t i=1; i<SAMPLE_CNT+1; ++i){
    sum += samples[i];
    Serial.print( samples[i]); Serial.print(" ");
  }
  uint16_t avg = sum/SAMPLE_CNT;
  Serial.print("("); Serial.print(avg); Serial.print(")");
  Serial.print("\n");
  return ( avg );  // return average of samples.
  
}

/////////////////////////////////////////////////////////////////////////
// Function: readVcc ( analogue_pin )
// Purpose : ADC conversion using acdNoiseReducton mode.
// Input   : Analogue pin number (limited to 0..7)
// Return  : voltage value in mV
/////////////////////////////////////////////////////////////////////////

uint16_t readVcc(int apin) 
{
  uint32_t value; // need 32 bits for millivolt range (1024*3200).
  
  // Limit mux to 0..7
  int mux = apin&0x07;
  // Voltage reference is 1.1 internal Band Gap. Right adjust conversion. 
  ADMUX = (1<<REFS1) | (1<<REFS0) | (0<<ADLAR) | (0<<MUX3) | mux;

  // read value
  value = adcNoiseReduction();
  //  
  return ( (( value * VCC) / RESOLUTION) ); 
}
// Battery monitor thru voltage divider.
// Voltage divider R1=2.2Mohm, R2=680Kohm
// If Vin = 4.5V Vout = 1.06V
// R1 voltage drop (2.2M/2.88M = .764 of Vin)
// R2 voltage drop .236 of Vin (6//80K/2.88M)
// In millivolts Vin = pVcc x (1000/.236) or pVcc*4237
// Let the compiler calculate the multipler for pVcc.
constexpr long bat_vd_multiplier(){
  const float R1 {2200000};
  const float R2 {680000}; 
  return 1000/(R2/(R2+R1));
}


void loop(){
  delay( 1000 );

  long pVcc = readVcc(1);
  Serial.print( "Ax mv: "); Serial.print(pVcc, DEC);
  // Apply the voltage divider multiplier.
  uint16_t vIN = (pVcc * bat_vd_multiplier())/1000;
  Serial.print("  Vin mv: "); Serial.print(vIN, DEC);
  Serial.print("\n");

}

// Setup to ADC with VCC as reference and read voltage on A0 using LOw noise

void setup() {
  Serial.begin( 115200);
}
