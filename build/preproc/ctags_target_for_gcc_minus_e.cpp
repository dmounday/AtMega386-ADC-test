# 1 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
/*
  ADC Testing
  Uses Noise reduction ADC to read the analogue voltage
  using Band Gap internal 1.1 volt reference.
*/

# 8 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 2
# 9 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 2
# 10 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 2


# 11 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
const uint16_t VCC {1100}; // Band Gap Reference voltage
const uint16_t RESOLUTION {1024}; // ADC resolution

static const uint8_t SAMPLE_CNT {8}; // ADC value is average of SAMPLE_CNT
volatile uint8_t adcIRQCnt;
volatile uint16_t samples[SAMPLE_CNT+1];
/////////////////////////////////////////////////////////////////////////
// Function: Interrupt routine for ADC
// Purpose : Fired when ADC interrupt occured (mainly end of conversion)
//
/////////////////////////////////////////////////////////////////////////

# 22 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
extern "C" void __vector_21 /* ADC Conversion Complete */ (void) __attribute__ ((signal,used, externally_visible)) ; void __vector_21 /* ADC Conversion Complete */ (void)

# 23 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
{
  // skip first conversion
  if ( adcIRQCnt < SAMPLE_CNT+1 ) { // Test we aren't out of bounds.
    uint8_t low, high;
    // read low first
    low = 
# 28 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
          (*(volatile uint8_t *)(0x78))
# 28 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
              ;
    high = 
# 29 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
          (*(volatile uint8_t *)(0x79))
# 29 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
              ;
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
  
# 45 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
 (*(volatile uint8_t *)(0x7A)) 
# 45 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
        |= 
# 45 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
           (1 << (3)) 
# 45 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
                       | 
# 45 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
                         (1 << (7))
# 45 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
                                  ;

  // Loop thru taking samples. adcIRQCnt is incremented in interrupt handler.
  for (adcIRQCnt = 0; adcIRQCnt<SAMPLE_CNT+1;)
  {
    // Enable Noise Reduction Sleep Mode -- ADC starts on CPU halt.
    
# 51 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
   do { (*(volatile uint8_t *)((0x33) + 0x20)) = (((*(volatile uint8_t *)((0x33) + 0x20)) & ~((1 << (1)) | (1 << (2)) | (1 << (3)))) | ((0x01<<1))); } while(0)
# 51 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
                                   ;
    
# 52 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
   do { (*(volatile uint8_t *)((0x33) + 0x20)) |= (uint8_t)(1 << (0)); } while(0)
# 52 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
                 ;
    
# 53 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
   __asm__ __volatile__ ("sei" ::: "memory")
# 53 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
        ; // enable interrupts
    // Sleep must be called after sei().
    
# 55 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
   do { __asm__ __volatile__ ( "sleep" "\n\t" :: ); } while(0)
# 55 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
              ;
    // Awake again following interrupt handler.
    
# 57 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
   do { (*(volatile uint8_t *)((0x33) + 0x20)) &= (uint8_t)(~(1 << (0))); } while(0)
# 57 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
                  ;
  }

  //Disable ADC, Auto Trigger Disable, Clear Interrup Flag, Disable ADC Interrupt.
  // Prescaler set to 128.
  
# 62 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
 (*(volatile uint8_t *)(0x7A)) 
# 62 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
        = 23;
  Serial.print("samples: "); Serial.print(samples[0]); Serial.print("  :");
  // skip first sample
  for (uint8_t i=1; i<SAMPLE_CNT+1; ++i){
    sum += samples[i];
    Serial.print( samples[i]); Serial.print(" ");
  }
  uint16_t avg = sum/SAMPLE_CNT;
  Serial.print("("); Serial.print(avg); Serial.print(")");
  Serial.print("\n");
  return ( avg ); // return average of samples.

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
  
# 90 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
 (*(volatile uint8_t *)(0x7C)) 
# 90 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
       = (1<<
# 90 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
             7
# 90 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
                  ) | (1<<
# 90 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
                          6
# 90 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
                               ) | (0<<
# 90 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
                                       5
# 90 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
                                            ) | (0<<
# 90 "/home/dmounday/vsworkspace/modenioADC/adc.ino" 3
                                                    3
# 90 "/home/dmounday/vsworkspace/modenioADC/adc.ino"
                                                        ) | mux;

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
  Serial.print( "Ax mv: "); Serial.print(pVcc, 10);
  // Apply the voltage divider multiplier.
  uint16_t vIN = (pVcc * bat_vd_multiplier())/1000;
  Serial.print("  Vin mv: "); Serial.print(vIN, 10);
  Serial.print("\n");

}

// Setup to ADC with VCC as reference and read voltage on A0 using LOw noise

void setup() {
  Serial.begin( 115200);
}
