/*********************************
PCF8574_HD44780_I2C
Library for PCF8574 Breakout Board
by Testato on ArduinoForum

based on code from:
- Mario H.
**********************************/


#include "PCF8574_HD44780_I2C.h"
#include <inttypes.h>
#include "Wire.h"
#include "Arduino.h"


// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

PCF8574_HD44780_I2C::PCF8574_HD44780_I2C(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows)
{
  _Addr = lcd_Addr;
  _cols = lcd_cols;
  _rows = lcd_rows;
  _backlightval = LCD_NOBACKLIGHT;
}

void PCF8574_HD44780_I2C::init(){
	init_priv();
}

void PCF8574_HD44780_I2C::init_priv()
{
	Wire.begin();
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	begin(_cols, _rows);  
}

void PCF8574_HD44780_I2C::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
	if (lines > 1) {
		_displayfunction |= LCD_2LINE;
	}
	_numlines = lines;

	// for some 1 line displays you can select a 10 pixel high font
	if ((dotsize != 0) && (lines == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	delay(50); 
  
	// !NOT NECESSARY! //
	// Now we pull both RS and R/W low to begin commands
	//expanderWrite(_backlightval);	// reset expanderand turn backlight off (Bit 8 =1) - _backlightval = LCD_NOBACKLIGHT;
	//delay(1000);

  	// put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46
	
	// LCD automatically start in 8bit mode, but we manually repeat
	// the 8bit initialization instructions for safety
	write4bits(_BV(P4) | _BV(P5));	
	delay(5); // wait min 4.1ms
	write4bits(_BV(P4) | _BV(P5));	
	delayMicroseconds(150); // wait min 100us
	write4bits(_BV(P4) | _BV(P5));	
	
	// !NOT NECESSARY! //
	//delayMicroseconds(150);
	
	// Set interface to be 4 bits long
	write4bits(_BV(P5));


	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);  
	
	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();
	
	// clear it off
	clear();
	
	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	
	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);
	
	home();
  
}



/********** high level commands, for the user! */
void PCF8574_HD44780_I2C::clear(){
	command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	delayMicroseconds(2000);  // this command takes a long time!
}

void PCF8574_HD44780_I2C::home(){
	command(LCD_RETURNHOME);  // set cursor position to zero
	delayMicroseconds(2000);  // this command takes a long time!
}

void PCF8574_HD44780_I2C::setCursor(uint8_t col, uint8_t row){
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };				// LCD Addressing
	if ( row > _numlines ) {
		row = _numlines-1;    // we count rows starting w/0
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void PCF8574_HD44780_I2C::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void PCF8574_HD44780_I2C::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void PCF8574_HD44780_I2C::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void PCF8574_HD44780_I2C::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void PCF8574_HD44780_I2C::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void PCF8574_HD44780_I2C::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void PCF8574_HD44780_I2C::scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void PCF8574_HD44780_I2C::scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void PCF8574_HD44780_I2C::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void PCF8574_HD44780_I2C::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void PCF8574_HD44780_I2C::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void PCF8574_HD44780_I2C::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void PCF8574_HD44780_I2C::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
}

// Turn the (optional) backlight off/on
void PCF8574_HD44780_I2C::noBacklight(void) {
	_backlightval=LCD_NOBACKLIGHT;
	expanderWrite(0);
}

void PCF8574_HD44780_I2C::backlight(void) {
	_backlightval=LCD_BACKLIGHT;
	expanderWrite(0);
}



/*********** mid level commands, for sending data/cmds */

inline void PCF8574_HD44780_I2C::command(uint8_t value) {
	send(value, 0);
}

inline size_t PCF8574_HD44780_I2C::write(uint8_t value) {
	send(value, Rs);
	return 0;
}



/************ low level data pushing commands **********/

// write either command or data 
void PCF8574_HD44780_I2C::send(uint8_t value, uint8_t mode) {
	uint8_t highnib=value & 0xF0;	
	uint8_t lownib=value << 4;		
	write4bits((highnib)|mode);		
	write4bits((lownib)|mode);
}

void PCF8574_HD44780_I2C::write4bits(uint8_t value) {
	expanderWrite(value);
	pulseEnable(value);
}

void PCF8574_HD44780_I2C::expanderWrite(uint8_t _data){                                        
	Wire.beginTransmission(_Addr);
	Wire.write((int)(_data) | _backlightval);
	Wire.endTransmission();   
}

void PCF8574_HD44780_I2C::pulseEnable(uint8_t _data){
	expanderWrite(_data | En);	// En high
	delayMicroseconds(1);		// enable pulse must be >450ns
	
	expanderWrite(_data & ~En);	// En low
	delayMicroseconds(50);		// commands need > 37us to settle
} 


// Alias functions

void PCF8574_HD44780_I2C::cursor_on(){
	cursor();
}

void PCF8574_HD44780_I2C::cursor_off(){
	noCursor();
}

void PCF8574_HD44780_I2C::blink_on(){
	blink();
}

void PCF8574_HD44780_I2C::blink_off(){
	noBlink();
}

void PCF8574_HD44780_I2C::load_custom_character(uint8_t char_num, uint8_t *rows){
		createChar(char_num, rows);
}

void PCF8574_HD44780_I2C::setBacklight(uint8_t new_val){
	if(new_val){
		backlight();		// turn backlight on
	}else{
		noBacklight();		// turn backlight off
	}
}

void PCF8574_HD44780_I2C::printstr(const char c[]){
	//This function is not identical to the function used for "real" I2C displays
	//it's here so the user sketch doesn't have to be changed 
	print(c);
}


// unsupported API functions
void PCF8574_HD44780_I2C::off(){}
void PCF8574_HD44780_I2C::on(){}
void PCF8574_HD44780_I2C::setDelay (int cmdDelay,int charDelay) {}
uint8_t PCF8574_HD44780_I2C::status(){return 0;}
uint8_t PCF8574_HD44780_I2C::keypad (){return 0;}
uint8_t PCF8574_HD44780_I2C::init_bargraph(uint8_t graphtype){return 0;}
void PCF8574_HD44780_I2C::draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end){}
void PCF8574_HD44780_I2C::draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_row_end){}
void PCF8574_HD44780_I2C::setContrast(uint8_t new_val){}

	