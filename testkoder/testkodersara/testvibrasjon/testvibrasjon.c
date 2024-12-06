#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>
int main ( void )
    {
    /* Configure SW0 as input */
    PORTB.DIRCLR = PIN2_bm ;
    /* Configure LED0 pin as output */
    PORTB.DIRSET = PIN3_bm ;
    /* Enable the internal pull -up for SW0 */
    PORTB.PIN2CTRL |= PORT_PULLUPEN_bm ;
    
    while (1){
    /* Check the status of SW0 */
    /* 0: Pressed */
        if (!( PORTB.IN & ( PIN2_bm )))
        {
        /* LED0 on */
        PORTB.OUTSET = PIN3_bm ;
        }
        /* 1: Released */
        else
        {
        /* LED0 off */
        PORTB.OUTCLR = PIN3_bm ;
        }
    }
}