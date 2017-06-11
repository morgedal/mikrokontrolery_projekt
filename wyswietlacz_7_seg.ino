
#define TEXT 0xFEC89B
#define DISP 500
#define BRIGHT 2110

#define START_DIGIT_1 (TEXT/0x100000)%0x10
#define START_DIGIT_2 (TEXT/0x10000)%0x10
#define START_DIGIT_3 (TEXT/0x1000)%0x10
#define START_DIGIT_4 (TEXT/0x100)%0x10
#define START_DIGIT_5 (TEXT/0x10)%0x10
#define START_DIGIT_6 TEXT%0x10

#define START_BRIGHTNESS_1 100+450*((BRIGHT/1000)%10)
#define START_BRIGHTNESS_2 100+450*((BRIGHT/100)%10)
#define START_BRIGHTNESS_3 100+450*((BRIGHT/10)%10)
#define START_BRIGHTNESS_4 100+450*(BRIGHT%10)

/* polaczenia segmentow */
#define SS (1<<PD7)
#define PG (1<<PD2)
#define PD (1<<PD3)
#define SRD (1<<PD4)
#define LD (1<<PD5)
#define LG (1<<PD6)
#define SG (1<<PB0)

const int pgm_digits[] PROGMEM = 
{
  ~(SG|PG|PD|SRD|LD|LG),  /* pgm_digits[0] = 0 ... */
  ~(PG|PD),
  ~(SG|PG|SS|LD|SRD),
  ~(SG|PG|PD|SRD|SS),
  ~(LG|SS|PG|PD),
  ~(SG|LG|SS|PD|SRD),
  ~(SG|LG|LD|SRD|PD|SS),
  ~(SG|PG|PD),
  ~(SG|PG|PD|SRD|LD|LG|SS),
  ~(SG|PG|PD|SRD|LG|SS),
  ~(LD|LG|SG|PG|PD|SS),
  ~(LG|LD|SS|SRD|PD),
  ~(SG|LG|LD|SRD),
  ~(PG|LD|SS|SRD|PD),
  ~(SG|SRD|LD|LG|SS),
  ~(SG|LD|LG|SS),
  0xFF  /* pgm_digits[16] = SPACE */
};

volatile uint8_t digit[10] = 
{
  16,16,16,16,     /* cztery spacje na poczatku sekwencji */
  START_DIGIT_1,
  START_DIGIT_2,
  START_DIGIT_3,
  START_DIGIT_4,
  START_DIGIT_5,
  START_DIGIT_6  
};

volatile uint8_t sequence_counter = 0;
int display_time = DISP;
volatile int pwm[5] = 
{
  START_BRIGHTNESS_1,
  START_BRIGHTNESS_2,
  START_BRIGHTNESS_3,
  0,                    //czwarty element( pwm[3] ) nigdy nie uzywany, wlozony do tablicy dla uproszczenia algorytmu
  START_BRIGHTNESS_4
};  

void setup( void ) 
{
  TCCR1A = 0;     // poczatkowe wyzerowanie rejestrow odpowiedzialnych za przerwania zeby 
  TCCR1B = 0;     // wewnetrzne ustawienia Arduino nam przerwan nie psuly
  TCNT1  = 0;
  
  TCCR1B |= (1<<WGM12);                     //tryb CTC na timerze 2
  TCCR1B |= (1<<CS12)|(1<<CS10);            //preskaler 1024
  OCR1A = 64;                               //dodatkowy podzial przez 65
  TIMSK1 |=(1<<OCIE1A);                     //zezwolenie na przerwania
  
  Serial.begin(9600);
   
  DDRD |= 0xFC;   //port D jako wyjscie
  PORTD = 0xFC;   //wygaszenie katod

  DDRB |= 0x01;   //jedno wyjscie brakujace w porcie D
  PORTB = 0x01;
  
  DDRC |= 0x0F;  
  PORTC = 0x0F;
  
  sei(); //wlaczenie przerwan
}

void loop( void ) 
{
  for( sequence_counter = 0 ; sequence_counter < 10 ; sequence_counter++ )
  {
    delay( display_time );
    check_serial();
  }  
}

void check_serial( void )
{
  char received_command;
  
  if( Serial.available() )
  {
      received_command = Serial.read();

      switch( received_command )
      {
        case 'T':
        case 't':
          change_digits();
          break;
        case 'D':
        case 'd':
          change_display_time();
          break;
        case 'B':
        case 'b':
          change_brightness();
          break;
        default:
          Serial.println("ERROR: Unknown command");
      }

     while( Serial.available() )    //wyczyszczenie bufora na nastepne komendy
       Serial.read();
  }
}

void change_digits( void )
{
  uint8_t buffer[6];
  if( read_hex( buffer ) == -1 )
    return;

  if( check_if_args_in_range( buffer , 6 , 15 ) == -1 )
     return;

  for( uint8_t i=0 ; i<6 ; i++ )
    digit[i+4] = buffer[i];
}

int read_hex( uint8_t * buffer )
{
  if( read_ints( buffer , 6 ) == -1 )
    return -1;
  
  for( uint8_t i=0 ; i<6 ; i++ )
    if( buffer[i] > 16 && buffer[i] < 23 )  //dla wpisanych A-F doprowadzamy do ich wartosci dziesietnej
      buffer[i] -= 7;
    else if( buffer[i] > 48 && buffer[i] < 55 )   //to samo dla a-f
      buffer[i] -= 39;

  return 0; 
}

int read_ints( uint8_t * buffer , uint8_t arg_length )
{
   for( uint8_t i=0 ; i < arg_length ; i++ )
     buffer[i] = Serial.read() - '0';

   if( Serial.peek() != 10 )
   {
     Serial.println( "ERROR:Argument too long" );
     return -1;
   }

   return 0;
}

int check_if_args_in_range( uint8_t * args , uint8_t number_of_args , uint8_t max_value )
{
  for( uint8_t i=0 ; i < number_of_args ; i++ )
    if( args[i] > max_value )
    {
      Serial.println( "ERROR:Wrong argument" );
      return -1;
    }
  
  return 0;
}

void change_display_time( void )
{
   uint8_t buffer[4];
   int temp;
   
   if( read_ints( buffer , 4 ) == -1 )
     return;

   if( check_if_args_in_range( buffer , 4 , 9 ) == -1 )
     return; 

   if( ( temp = 1000*buffer[0] + 100*buffer[1] + 10*buffer[2] + buffer[3] ) == 0 )
     return;
     
   display_time = temp;
}

void change_brightness( void )
{
   uint8_t buffer[4];
   if( read_ints( buffer , 4 ) == -1 )
     return;

   if( check_if_args_in_range( buffer , 4 , 2 ) == -1 )
     return;
   
   pwm[0] = 100 + 450 * buffer[0];
   pwm[1] = 100 + 450 * buffer[1];
   pwm[2] = 100 + 450 * buffer[2];
   pwm[4] = 100 + 450 * buffer[3];
}

ISR(TIMER1_COMPA_vect)
{
  static uint8_t counter = 0x01;
 
  switch( counter )
  {
    case 0x01:
      PORTD = pgm_read_byte( &pgm_digits[ digit[ sequence_counter % 10 ] ] );   
      PORTB = pgm_read_byte( &pgm_digits[ digit[ sequence_counter % 10 ] ] ) & 0x01;     
      break;
    case 0x02:
      PORTD = pgm_read_byte( &pgm_digits[ digit[ ( sequence_counter + 1 ) % 10 ] ] ) & 0xFC;   
      PORTB = pgm_read_byte( &pgm_digits[ digit[ ( sequence_counter + 1 ) % 10 ] ] ) & 0x01;      
      break;
    case 0x04:
      PORTD = pgm_read_byte( &pgm_digits[ digit[ ( sequence_counter + 2 ) % 10 ] ] ) & 0xFC;   
      PORTB = pgm_read_byte( &pgm_digits[ digit[ ( sequence_counter + 2 ) % 10 ] ] ) & 0x01;      
      break;
    default:
      PORTD = pgm_read_byte( &pgm_digits[ digit[ ( sequence_counter + 3 ) % 10 ] ] ) & 0xFC;   
      PORTB = pgm_read_byte( &pgm_digits[ digit[ ( sequence_counter + 3 ) % 10 ] ] ) & 0x01;
  }

  for( int i = 0 ; i < 1000 ; i++ )
    if( i < pwm[ counter/2 ] )        //counter przez 2 aby obsłużyć elementy 0,1,2,4 tablicy, pomijamy 3, bo to latwiejsze obliczeniowo
      PORTC = ~counter ;
    else
      PORTC = 0xFF;
 
  counter <<= 1;
  if( counter > 0x08 )
    counter = 0x01;
}
