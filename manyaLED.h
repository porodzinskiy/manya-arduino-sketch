#pragma once
#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_size 11
//1 - br, 2 - r, 3 - g, 4 - b, 5 - rm1, 6 - gm1, 7 - bm1, 8 - rm2, 9 - gm2, 10 - bm2, 11 - smooth,
class ManyaLed {
public:
	ManyaLed(uint8_t EEPROM_counter);
	
	void setBrightness(uint8_t Brightness0_255);
	uint8_t getBrightness();
	void brightnessAdd();
	void brightnessReduce();
	
	void setColour(uint8_t Red, uint8_t Green, uint8_t Blue);
  void setColourRed(uint8_t Red);
  void setColourGreen(uint8_t Green);
  void setColourBlue(uint8_t Blue);
	void saveToM1();
	void saveToM2();
	void setFromM1();
	void setFromM2();
	void colourAdd();
	void colourReduce();
	void loadFromMem();

	void setSmooth(uint8_t smooth);
	uint8_t getSmooth();
	void smoothChange();
	void smoothAfterStart();
	
	uint8_t red();
	uint8_t green();
	uint8_t blue();
	
	uint8_t positionMazdaGuiCheck();
	
private:
	uint16_t _EEPROM_counter;
	uint16_t _first_adress;
	uint16_t _last_adress;
	
	void colourMazdaGuiCheck();
	
	uint8_t _smooth;
	uint8_t _brightness;
	uint8_t _red;
	uint8_t _green;
	uint8_t _blue;
	uint8_t _colour_m1_red;
	uint8_t _colour_m1_green;
	uint8_t _colour_m1_blue;
	uint8_t _colour_m2_red;
	uint8_t _colour_m2_green;
	uint8_t _colour_m2_blue;

	int8_t _colour_counter;
};

ManyaLed::ManyaLed(uint8_t EEPROM_counter) {
	_EEPROM_counter = EEPROM_counter;

	EEPROM.get(_EEPROM_counter*EEPROM_size, _brightness);
	EEPROM.get((_EEPROM_counter*EEPROM_size+1), _red);
	EEPROM.get((_EEPROM_counter*EEPROM_size+2), _green);
	EEPROM.get((_EEPROM_counter*EEPROM_size+3), _blue);
	EEPROM.get((_EEPROM_counter*EEPROM_size+4), _colour_m1_red);
	EEPROM.get((_EEPROM_counter*EEPROM_size+5), _colour_m1_green);
	EEPROM.get((_EEPROM_counter*EEPROM_size+6), _colour_m1_blue);
	EEPROM.get((_EEPROM_counter*EEPROM_size+7), _colour_m2_red);
	EEPROM.get((_EEPROM_counter*EEPROM_size+8), _colour_m2_green);
	EEPROM.get((_EEPROM_counter*EEPROM_size+9), _colour_m2_blue);
	_smooth = 255;
  _colour_counter = positionMazdaGuiCheck();

}
void ManyaLed::loadFromMem() {
  EEPROM.get(_EEPROM_counter*EEPROM_size, _brightness);
	EEPROM.get((_EEPROM_counter*EEPROM_size+1), _red);
	EEPROM.get((_EEPROM_counter*EEPROM_size+2), _green);
	EEPROM.get((_EEPROM_counter*EEPROM_size+3), _blue);
  _smooth = 255;
}
void ManyaLed::setBrightness(uint8_t Brightness0_255) {
	_brightness = Brightness0_255;
	EEPROM.put(_EEPROM_counter*EEPROM_size, _brightness);

}
uint8_t ManyaLed::getBrightness() {
	return _brightness;
}
void ManyaLed::brightnessAdd() {
  if (_brightness > 204){
    _brightness = 255;
  } else {
    _brightness = _brightness + 51;
  }
	EEPROM.put(_EEPROM_counter*EEPROM_size, _brightness);

}
void ManyaLed::brightnessReduce() {
	if (_brightness < 51){
    _brightness = 0;
  } else {
    _brightness = _brightness - 51;
  }
	EEPROM.put(_EEPROM_counter*EEPROM_size, _brightness);

}
void ManyaLed::setColourRed(uint8_t Red) {

	_red = Red;
	EEPROM.put((_EEPROM_counter*EEPROM_size+1), _red);
}
void ManyaLed::setColourGreen(uint8_t Green) {

	_green = Green;
	EEPROM.put((_EEPROM_counter*EEPROM_size+2), _green);
}
void ManyaLed::setColourBlue(uint8_t Blue) {
	_blue = Blue;
	EEPROM.put((_EEPROM_counter*EEPROM_size+3), _blue);

}
void ManyaLed::setColour(uint8_t Red, uint8_t Green, uint8_t Blue) {
	_red = Red;
	_green = Green;
	_blue = Blue;
	EEPROM.put((_EEPROM_counter*EEPROM_size+1), _red);
	EEPROM.put((_EEPROM_counter*EEPROM_size+2), _green);
	EEPROM.put((_EEPROM_counter*EEPROM_size+3), _blue);

}
void ManyaLed::saveToM1() {
	_colour_m1_red = _red;
	_colour_m1_green = _green;
	_colour_m1_blue = _blue;
	EEPROM.put((_EEPROM_counter*EEPROM_size+4), _colour_m1_red);
	EEPROM.put((_EEPROM_counter*EEPROM_size+5), _colour_m1_green);
	EEPROM.put((_EEPROM_counter*EEPROM_size+6), _colour_m1_blue);
}
void ManyaLed::saveToM2() {
	_colour_m2_red = _red;
	_colour_m2_green = _green;
	_colour_m2_blue = _blue;
	EEPROM.put((_EEPROM_counter*EEPROM_size+7), _colour_m2_red);
	EEPROM.put((_EEPROM_counter*EEPROM_size+8), _colour_m2_green);
	EEPROM.put((_EEPROM_counter*EEPROM_size+9), _colour_m2_blue);
}
void ManyaLed::setFromM1() {
	_red = _colour_m1_red;
	_blue = _colour_m1_blue;
	_green = _colour_m1_green;
  EEPROM.put((_EEPROM_counter*EEPROM_size+1), _red);
	EEPROM.put((_EEPROM_counter*EEPROM_size+2), _green);
	EEPROM.put((_EEPROM_counter*EEPROM_size+3), _blue);
}
void ManyaLed::setFromM2() {
	_red = _colour_m2_red;
	_green = _colour_m2_green;
	_blue = _colour_m2_blue;
  EEPROM.put((_EEPROM_counter*EEPROM_size+1), _red);
	EEPROM.put((_EEPROM_counter*EEPROM_size+2), _green);
	EEPROM.put((_EEPROM_counter*EEPROM_size+3), _blue);
}
uint8_t ManyaLed::red() {
	return _red;
}
uint8_t ManyaLed::green() {
	return _green;
}
uint8_t ManyaLed::blue() {
	return _blue;
}
void ManyaLed::colourAdd(){

	_colour_counter = _colour_counter + 1;
	if (_colour_counter == 5) {
		_colour_counter = 0;
	}
	colourMazdaGuiCheck();
}
void ManyaLed::colourReduce(){

	_colour_counter = _colour_counter - 1;
	if (_colour_counter == -1) {
		_colour_counter = 5;
	}
	colourMazdaGuiCheck();
}

uint8_t ManyaLed::positionMazdaGuiCheck() {
	if (_red == 255 && _green == 255 && _blue == 255) return 0;
	else if (_red == 255 && _green == 0 && _blue == 0) return 1;
	else if (_red == 0 && _green == 255 && _blue == 0) return 2;
	else if (_red == 0 && _green == 0 && _blue == 255) return 3;
	else if (_red == _colour_m1_red && _green == _colour_m1_green && _blue == _colour_m1_blue) return 4;
	else if (_red == _colour_m2_red && _green == _colour_m2_green && _blue == _colour_m2_blue) return 5;
	else return -1;
}
void ManyaLed::colourMazdaGuiCheck() {
	if (_colour_counter == 0) {
		_red = 255;
		_green = 255;
		_blue = 255;
	}
	else if (_colour_counter == 1) {
		_red = 255;
		_green = 0;
		_blue = 0;
	}
	else if (_colour_counter == 2) {
		_red = 0;
		_green = 255;
		_blue = 0;
	}
	else if (_colour_counter == 3) {
		_red = 0;
		_green = 0;
		_blue = 255;
	}
	else if (_colour_counter == 4) {
		_red = _colour_m1_red;
		_green = _colour_m1_green;
		_blue = _colour_m1_blue;
	}
	else if (_colour_counter == 5) {
		_red = _colour_m2_red;
		_green = _colour_m2_green;
		_blue = _colour_m2_blue;
	}
	EEPROM.put((_EEPROM_counter*EEPROM_size+1), _red);
	EEPROM.put((_EEPROM_counter*EEPROM_size+2), _green);
	EEPROM.put((_EEPROM_counter*EEPROM_size+3), _blue);
}

void ManyaLed::setSmooth(uint8_t smooth){
	_smooth = smooth;
	EEPROM.put((_EEPROM_counter*EEPROM_size + 10), _smooth);
}
void ManyaLed::smoothAfterStart(){
	EEPROM.get((_EEPROM_counter*EEPROM_size+10), _smooth);
}
uint8_t ManyaLed::getSmooth(){
	return _smooth;
}
void ManyaLed::smoothChange(){
	if ((_smooth >= 0) && (_smooth < 85)) {
		_smooth = 85;
	}
	else if ((_smooth >= 85) && (_smooth < 170)) {
		_smooth = 170;
	}
	else if ((_smooth >= 170) && (_smooth < 255)) {
		_smooth = 255;
	}
	else if (_smooth == 255) {
		_smooth = 0;
	}
	EEPROM.put((_EEPROM_counter*EEPROM_size + 10), _smooth);
}

