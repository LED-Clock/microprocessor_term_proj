#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define CW 1
#define CCW 0
#define STOP -1
volatile unsigned int illumi_result = 0xffff,ILL_THRESH = 0x00ff, light_onoff; //�������� ���� �������Ʈ ������
volatile unsigned char light_angle = 0; //�������Ʈ ���ϰ��� ����
volatile unsigned int DIRECTION = 0, COUNT=0; //0 ����, 1��ȸ��, 2��ȸ��, 3�ֹ��� //�����ð��Ŀ� �������õ� �ڵ�����
uint8_t FND_ARROW[] = {0x79, 0x40, 0x4f}; //��� : E - 3  //7seg-LED�� �������


void illumi() //F��Ʈ�� adc��⿬��, G��Ʈ�� LED 2���� �������Ʈ�� ǥ��
{
	DDRG = 0x03;
	PORTG = 0x00;
	
	DDRF = 0xFB; //2���ε��� ���������� �Է�����
	ADMUX = 0x02;
	ADCSRA = 0xAF; //enabling, freerunning prescaling 128
	ADCSRA |= 0x40; //ADC start conversion

}

ISR(ADC_vect) //���������� ��ȯ�� ������
{
	illumi_result = ADCL & 0xFF;
	illumi_result |= (ADCH << 8); //�������� ���� ��ȯ�� ���
	
	if(illumi_result < ILL_THRESH) light_onoff = 1;	//��ο�� �������Ʈ Ŵ
	else light_onoff = 0;		//�������Ʈ ��
	
}

void FND()
{
	
		if(DIRECTION == 1 && PINB & 0x20) //��ȸ��
		{
			
			PORTC = 0b11111101;
			PORTA = FND_ARROW[1];
			_delay_ms(1);

			PORTC = 0b11111110;
			PORTA = FND_ARROW[0];
			_delay_ms(1);
			
		}

		if(DIRECTION == 2 && PINB & 0x20) //��ȸ��
		{
			
			PORTC = 0b11111011;
			PORTA = FND_ARROW[1];
			_delay_ms(1);

			PORTC = 0b11110111;
			PORTA = FND_ARROW[2];
			_delay_ms(1);
			
		}

		if(DIRECTION == 0) //���õ� ����
		{
			PORTC = 0xff;
			PORTA = 0x00;
		}

		if(DIRECTION == 3 && PINB & 0x20) //�ֹ��� ���õ�
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

void Signal_light() //8��¥�� ����ġ����� PORTD�� ����, E��Ʈ�� LED�ΰ��� �������õ�
{	

	if(PINB & 0x20) //PB5 high�̸� LEDŰ��
	{
		if(DIRECTION == 1) //��ȸ��
			PORTE = 0b11000000;

		else if(DIRECTION == 2) //��ȸ��
			PORTE = 0b00000011;

		else if(DIRECTION == 0) //���õ� ����
			PORTE = 0x00;

		else if(DIRECTION == 3) //�ֹ��� ���õ�
			PORTE = 0b11000011;
	}

	else PORTE = 0x00; //PB5 low�̸� LED����
		
}

void init_timer(void)
{
	TIMSK |= (1 << OCIE1A);
	TCCR1A |= (1 << COM1A0);
	TCCR1B |= (1 << WGM12); // CTC4 ���
	TCCR1B |= (4 << CS10); // 256 ����
	TCNT1 = 0x0000;
	OCR1A = 25000; //�񱳰�, CTC��忡�� TOP�� OCR1A 0.4s

}

ISR(TIMER1_COMPA_vect)
{
	if((++COUNT) >= 15 && !(DIRECTION == 3)) DIRECTION = 0; //�ֹ����� Ư���� ���� ��� Ŵ
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
	DDRB = 0x2f; // OC1A ��� at PB5, motor ��� PB0 ��PB1
	DDRA = 0xFF; // FND data
	DDRC = 0x0f; // FND select
	DDRD = 0x00; PORTD = 0x00; //����ġ���
	DDRE = 0xff; PORTE = 0x00; //��������LED
	
	SREG |= 0x80; //��� ���ͷ�Ʈ enabling
	
	init_timer(); //0.4s���� high low
	illumi(); //DDRF , DDRG

	char light_onoff_butt=0;

	while(1)
	{	
		
		if(PIND & 0b10000000) { COUNT = 0; DIRECTION = 1; }//��ȸ�� ��ȣ
		if(PIND & 0b01000000) { COUNT = 0; DIRECTION = 0; }
		if(PIND & 0b00100000) { COUNT = 0; DIRECTION = 2; }//��ȸ����ȣ
		if(PIND & 0b00010000) { COUNT = 0; DIRECTION = 3; }//�ֹ���
		if(PIND & 0b00000010) { Motor1(CW); }//�����
		if(PIND & 0b00000001) { Motor1(CCW);}//�����, default

		
		FND();
		Signal_light();
		
		if(PING & 0b00000100) light_onoff_butt = 1;
		else if(PING & 0b00001000) light_onoff_butt = 0;

		if(light_onoff) { PORTG = 0x03; light_onoff_butt = 0;} //��ο�� ��ư�� ����
		else if(light_onoff_butt)  PORTG = 0x03;
		else PORTG = 0x00;

	}

}

