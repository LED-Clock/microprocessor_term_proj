#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define CW 1
#define CCW 0
#define STOP -1
volatile unsigned int illumi_result = 0xffff,ILL_THRESH = 0x00ff, light_onoff; //조도센서 관련 전방라이트 변수들
volatile unsigned char light_angle = 0; //전방라이트 상하각도 조절
volatile unsigned int DIRECTION = 0, COUNT=0; //0 끄기, 1좌회전, 2우회전, 3쌍방향 //일정시간후에 방향지시등 자동끄기
uint8_t FND_ARROW[] = {0x79, 0x40, 0x4f}; //모양 : E - 3  //7seg-LED에 방향출력


void illumi() //F포트에 adc모듈연결, G포트에 LED 2개를 전방라이트로 표현
{
	DDRG = 0x03;
	PORTG = 0x00;
	
	DDRF = 0xFB; //2번인덱스 조도센서만 입력으로
	ADMUX = 0x02;
	ADCSRA = 0xAF; //enabling, freerunning prescaling 128
	ADCSRA |= 0x40; //ADC start conversion

}

ISR(ADC_vect) //조도센서의 변환이 끝나면
{
	illumi_result = ADCL & 0xFF;
	illumi_result |= (ADCH << 8); //조도센서 값을 변환한 결과
	
	if(illumi_result < ILL_THRESH) light_onoff = 1;	//어두우면 전방라이트 킴
	else light_onoff = 0;		//전방라이트 끔
	
}

void FND()
{
	
		if(DIRECTION == 1 && PINB & 0x20) //좌회전
		{
			
			PORTC = 0b11111101;
			PORTA = FND_ARROW[1];
			_delay_ms(1);

			PORTC = 0b11111110;
			PORTA = FND_ARROW[0];
			_delay_ms(1);
			
		}

		if(DIRECTION == 2 && PINB & 0x20) //우회전
		{
			
			PORTC = 0b11111011;
			PORTA = FND_ARROW[1];
			_delay_ms(1);

			PORTC = 0b11110111;
			PORTA = FND_ARROW[2];
			_delay_ms(1);
			
		}

		if(DIRECTION == 0) //지시등 끄기
		{
			PORTC = 0xff;
			PORTA = 0x00;
		}

		if(DIRECTION == 3 && PINB & 0x20) //쌍방향 지시등
		{
			PORTC = 0b11111101;
			PORTA = FND_ARROW[1];
			_delay_ms(1);

			PORTC = 0b11111110;
			PORTA = FND_ARROW[0];
			_delay_ms(1);

			PORTC = 0b11111011;
			PORTA = FND_ARROW[1];
			_delay_ms(1);

			PORTC = 0b11110111;
			PORTA = FND_ARROW[2];
			_delay_ms(1);
			
		}
	
	PORTA = 0x00;
}

void Signal_light() //8개짜리 스위치모듈을 PORTD와 연결, E포트의 LED두개씩 방향지시등
{	

	if(PINB & 0x20) //PB5 high이면 LED키고
	{
		if(DIRECTION == 1) //좌회전
			PORTE = 0b11000000;

		else if(DIRECTION == 2) //우회전
			PORTE = 0b00000011;

		else if(DIRECTION == 0) //지시등 끄기
			PORTE = 0x00;

		else if(DIRECTION == 3) //쌍방향 지시등
			PORTE = 0b11000011;
	}

	else PORTE = 0x00; //PB5 low이면 LED끄고
		
}

void init_timer(void)
{
	TIMSK |= (1 << OCIE1A);
	TCCR1A |= (1 << COM1A0);
	TCCR1B |= (1 << WGM12); // CTC4 모드
	TCCR1B |= (4 << CS10); // 256 분주
	TCNT1 = 0x0000;
	OCR1A = 25000; //비교값, CTC모드에서 TOP은 OCR1A 0.4s

}

ISR(TIMER1_COMPA_vect)
{
	if((++COUNT) >= 15 && !(DIRECTION == 3)) DIRECTION = 0; //쌍방향은 특수한 경우라 계속 킴
}

void M1B(int onoff)
{
	if(onoff) PORTB |= 0x01; //set
	else PORTB &= 0xFE; //reset
}

void M1A(int onoff)
{
	if(onoff) PORTB |= 0x02; //set
	else PORTB &= 0xFD; //reset
}

void Motor1(int mode)
{
	if(mode == CW && light_angle < 4)
	{ 
		M1A(0); M1B(1);
		_delay_ms(30);
		light_angle++;
	}
	
	else if(mode == CCW && light_angle > 0 )
	{
		M1A(1); M1B(0);
		_delay_ms(30);
		light_angle--;
	}

	M1A(1); M1B(1);
}

int main(void)
{
	cli();
	DDRB = 0x2f; // OC1A 출력 at PB5, motor 출력 PB0 과PB1
	DDRA = 0xFF; // FND data
	DDRC = 0x0f; // FND select
	DDRD = 0x00; PORTD = 0x00; //스위치모듈
	DDRE = 0xff; PORTE = 0x00; //방향지시LED
	
	SREG |= 0x80; //모든 인터럽트 enabling
	
	init_timer(); //0.4s마다 high low
	illumi(); //DDRF , DDRG

	char light_onoff_butt=0;

	while(1)
	{	
		
		if(PIND & 0b10000000) { COUNT = 0; DIRECTION = 1; }//좌회전 신호
		if(PIND & 0b01000000) { COUNT = 0; DIRECTION = 0; }
		if(PIND & 0b00100000) { COUNT = 0; DIRECTION = 2; }//우회전신호
		if(PIND & 0b00010000) { COUNT = 0; DIRECTION = 3; }//쌍방향
		if(PIND & 0b00000010) { Motor1(CW); }//상향등
		if(PIND & 0b00000001) { Motor1(CCW);}//하향등, default

		
		FND();
		Signal_light();
		
		if(PING & 0b00000100) light_onoff_butt = 1;
		else if(PING & 0b00001000) light_onoff_butt = 0;

		if(light_onoff) { PORTG = 0x03; light_onoff_butt = 0;} //어두우면 버튼값 무시
		else if(light_onoff_butt)  PORTG = 0x03;
		else PORTG = 0x00;

	}

}

