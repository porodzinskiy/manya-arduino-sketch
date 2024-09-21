    //Настройки библиотек
  //Настройки библиотеки EncButton (подробнее смотреть в документации)
#define EB_NO_FOR
#define EB_NO_COUNTER
#define EB_NO_BUFFER

    //Подключение библиотек
#include <AnalogKey.h> //Библиотека Гайвера для работы с аналоговыми клавиатурами (нужно для считывания кнопок с руля)
#include <EncButton.h> //Библиотека Гайвера для работы с кнопками
#include "manyaLED.h" //Моя библиотека для работы с диммерами и подсветками
#include <SoftwareSerial.h> //Библиотека для работы с hc (bluetooth)
#include "mcp_can.h" //Библиотека для работы с CAN

    //Настройки, которые можно трогать
//#define DEBUG //Отладка в консоль
#define DMX_CMD_LEN	40 //Буфер данных для DMX. Выбирая размер буфера, выделять по 8 байт на каждый диммер
  //Пины
#define trans_vol_up_pin 7 //Пин подключения транзистора для резистора 13.5-18.5 кОм //Коричневый
#define trans_vol_down_pin 8 //Пин подключения транзистора для резистора 19-29 кОм //коричневый-желтый
#define wheel_btn_pin A6 //Пин подключения делимтеля напряжения для замера кнопок на руле
#define stock_btn_pin 5//Пин подключения чтения напряжения на диммеры
const int cs_pin = 10; //Пин CS на MS2515
const uint8_t bt_rx_pin = 3; //Пин bluetooth RX
const uint8_t bt_tx_pin = 4; //Пин bluetooth TX
  //Инициализация портов для DMX
#define DMX_OUT_HIGH()  PORTD |= (1<<1)
#define DMX_OUT_LOW() PORTD &=~ (1<<1)
#define DMX_DIR_TX()  PORTB |= (1<<1)
#define DMX_DIR_RX()  PORTB &=~ (1<<1)
  //Другие настройки
int16_t wheel_btn_sigs[6] = {688, 523, 370, 239, 131, 51}; //Массив значений с делителя напряжения (лучше замерять самому при помощи wheel_btn_test)
//Элементы массива: 0 - mute; 1 - mode; 2 - seek-(prev); 3 - seek+(next); 4- vol+; 5- vol-;

    //Настройки, которые лучше не трогать
AnalogKey <wheel_btn_pin, 6, wheel_btn_sigs> wheel_btn_keys; //Указание пина, количества кнопок и значений 
#define ID_DISP1 0x290 //ID первой части дисплея (отправляем)
#define ID_DISP2 0x291 //ID второй части дисплея (отправляем)
#define ID_INFO_SYMBOLS 0x28F //ID кнопки INFO и символов на дисплее (отправляем)
const uint16_t CAN_period = 250; //Период отправки посылок в CAN
  //Настройка UART
#define UART_BUF_SIZE 64
char UART_str[UART_BUF_SIZE] = {0};
unsigned char UART_counter = 0;
unsigned char UART_ready_flag = 0;
void DMX512_send_buf(unsigned char *data, unsigned int len);
    //Переменные
  //Кнопки
VirtButton wheel_btn_mute; //Кнопка Mute
VirtButton wheel_btn_mode; //Кнопка Mode
VirtButton wheel_btn_prev; //Кнопка Seek- (Prev)
VirtButton wheel_btn_next; //Кнопка Seek+ (Next)
VirtButton wheel_btn_vol_up; //Кнопка Vol+
VirtButton wheel_btn_vol_down; //Кнопка Vol-
  //Инициализация
SoftwareSerial bluetooth(bt_rx_pin, bt_tx_pin); //БТ
MCP_CAN CAN(cs_pin); //Инициализация CAN для отправки
  //Счетчики
int8_t mazda_gui_menu_counter; //Счетчик для меню
int8_t mazda_gui_settings_counter; // Счетчик для меню настроек
uint8_t bort_comp_counter; //Счетчик для борткомпа
uint8_t running_stroke_counter; //Счетчик для бегущей строки
int8_t effect_counter; //Счетчик для меню эффектов
  //Массивы
uint8_t stock_mode_before[2]; //Массив со значениями яркости до сток-мода
uint8_t dmx_buf[DMX_CMD_LEN] = {}; //Массив содержимого буфера DMX
  //Флаги
bool mazda_gui_menu_flag = false; //Флаг нахождения в меню
bool stock_mode_flag = false; //Флаг сток-мода
bool bort_comp_reset_flag = false; //Флаг для сброса БК
bool bort_comp_change_flag = false; //Флаг для изменения БК
bool new_song_name = true; //Флаг, если новая песня
bool stock_by_program = false; //Флаг программного вхождения в сток-мод
bool need_to_stock_delay_flag = false; //Флаг для переключения в сток мод через задержку
  //Время
uint32_t wheel_btn_mode_time; //Время нажатия на кнопку Mode
uint32_t dmx_send_time; //Время отправки содержимого буфера в DMX
uint32_t can_send_time; //Время отправки в CAN
uint32_t led_effect_time; //Время выполнения эффекта
uint32_t stock_delay_time; //Время задержки перехода из сток-мода
uint32_t stock_by_btn_time; //Время для задержки опроса кнопки
  //Яркость итд
uint8_t eyes_on_start_brightness = 0; //Яркость глазок при старте
  //Группы диодов
ManyaLed neon(0); //Неон в салоне
ManyaLed trunk(1); //Подсветка багажника
ManyaLed angel(2); //Ангельские глазки
ManyaLed demon(3); //Дъявольские глазки

  //То, что отправляем в CAN
unsigned char mazda_gui_left [8]; //То, что по ID_DISP1
unsigned char mazda_gui_right [8]; //То, что по ID_DISP2
unsigned char mazda_gui_symbols [8]; //То, что по ID_INFO_SYMBOLS
//Эмуляция кнопок родной магнитолы
const unsigned char info_push_btn [8] = {0x80,0x00,0x00,0x00,0xA8,0x00,0x00,0x00}; //Кнопка INFO с родной магнитолы
const unsigned char info_pull_btn [8] = {0x80,0x00,0x00,0x00,0xA0,0x00,0x00,0x00}; //Отпускаем кнопку INFO на родной магнитоле
//Далее идут родные ненужные символы, которые я использую для GUI (графический интерфейс)
const unsigned char null_gui [8] = {0x80,0x00,0x00,0x00,0x20,0x00,0x00,0x00}; //Ничего не горит
const unsigned char first_gui [8] = {0xE0,0x00,0x00,0x00,0x20,0x00,0x00,0x00}; //Первый выбор в меню (cd in, md in)
const unsigned char second_gui [8] = {0x98,0x00,0x00,0x00,0x20,0x00,0x00,0x00}; //Второй выбор в меню (st, dolby)
const unsigned char third_gui [8] = {0x81,0x80,0x00,0x00,0x20,0x00,0x00,0x00}; //Третий выбор в меню (ad, pty)
const unsigned char fourth_gui [8] = {0x86,0x00,0x00,0x00,0x20,0x00,0x00,0x00}; //Четвертый выбор в меню (rpt, rdm)
const unsigned char fifth_gui [8] = {0x80,0x60,0x00,0x00,0x20,0x00,0x00,0x00}; //Пятый выбор в меню (ta, tp)
const unsigned char sixth_gui [8] = {0x80,0x10,0x00,0x00,0x20,0x00,0x00,0x00}; //Шестой выбор в меню (auto-m)
//Далее необходимо воспользоваться либо таблицей ASCII кодов, либо онлайн переводом сиволов в HEX. Первый байт для левой части всегда C0, для правой- 85
//Меню на ЖК дисплее
const unsigned char left_menu_gui [8] = {0xC0,0x4D,0x75,0x42,0x63,0x4E,0x62,0x4E}; //Левая часть меню GUI "MuBcNbN"
const unsigned char right_menu_gui [8] = {0x85,0x62,0x4E,0x63,0x45,0x66,0x53,0x65}; //Правая часть меню GUI "bNcEfSe"
//Сообщение о названии трека 
unsigned char song_name [64] = ""; //Переменная, которая хранит имя трека
unsigned char left_running_stroke [8] = {0xC0,0x42,0x54,0x20,0x64,0x69,0x73,0x61}; //Левая часть "BT disa"
unsigned char right_running_stroke [8] = {0x85,0x73,0x61,0x62,0x6C,0x65,0x64,0x20}; //Правая часть "sabled "
//Сток-мод
const unsigned char left_stock_gui [8] = {0xC0,0x53,0x74,0x6F,0x63,0x6B,0x20,0x6D}; //Левая часть "Stock m"
const unsigned char right_stock_gui [8] = {0x85,0x20,0x6D,0x6F,0x64,0x65,0x20,0x20}; //Правая часть " mode  "
//Меню борт компьютера
const unsigned char left_bort_comp_gui [8] = {0xC0,0xED,0x53,0x77,0x61,0x70,0x20,0xEE}; //Левая часть "↑Swap ↓"
const unsigned char right_bort_comp_gui [8] = {0x85,0x20,0xEE,0x52,0x65,0x73,0x65,0x74}; //Правая часть " ↓Reset"
//Сообщение об обновлении
const unsigned char left_reseting_gui [8] = {0xC0,0x52,0x65,0x73,0x65,0x74,0x69,0x6E}; //Левая часть "Resetin"
const unsigned char right_reseting_gui [8] = {0x85,0x69,0x6E,0x67,0x2E,0x2E,0x2E,0x20}; //Правая часть "ing... "
//Меню яркости
const unsigned char left_100_bright_gui [8] = {0xC0,0x7C,0x7C,0x7C,0x7C,0x7C,0x7C,0x7C}; //Левая часть
const unsigned char right_100_bright_gui [8] = {0x85,0x7C,0x7C,0x7C,0x7C,0x4D,0x41,0x58}; //Правая часть
const unsigned char left_80_bright_gui [8] = {0xC0,0x7C,0x7C,0x7C,0x7C,0x7C,0x7C,0x7C}; //Левая часть
const unsigned char right_80_bright_gui [8] = {0x85,0x7C,0x7C,0x7C,0x20,0x38,0x30,0x25}; //Правая часть
const unsigned char left_60_bright_gui [8] = {0xC0,0x7C,0x7C,0x7C,0x7C,0x7C,0x7C,0x20}; //Левая часть
const unsigned char right_60_bright_gui [8] = {0x85,0x7C,0x20,0x20,0x20,0x36,0x30,0x25}; //Правая часть
const unsigned char left_40_bright_gui [8] = {0xC0,0x7C,0x7C,0x7C,0x7C,0x20,0x20,0x20}; //Левая часть
const unsigned char right_40_bright_gui [8] = {0x85,0x20,0x20,0x20,0x20,0x34,0x30,0x25}; //Правая часть
const unsigned char left_20_bright_gui [8] = {0xC0,0x7C,0x7C,0x20,0x20,0x20,0x20,0x20}; //Левая часть
const unsigned char right_20_bright_gui [8] = {0x85,0x20,0x20,0x20,0x20,0x32,0x30,0x25}; //Правая часть
const unsigned char left_0_bright_gui [8] = {0xC0,0x20,0x20,0x20,0x20,0x20,0x20,0x20}; //Левая часть
const unsigned char right_0_bright_gui [8] = {0x85,0x20,0x20,0x20,0x20,0x4F,0x46,0x46}; //Правая часть
//Меню цветов
const unsigned char left_colour_gui [8] = {0xC0,0x57,0x68,0x52,0x65,0x47,0x72,0x42}; //Левая часть "WhReGrB"
const unsigned char right_colour_gui [8] = {0x85,0x72,0x42,0x6C,0x4D,0x31,0x4D,0x32}; //Правая часть "rBlM1M2"
//Меню эффектов
const unsigned char left_effect_gui [8] = {0xC0,0x4F,0x66,0x46,0x6C,0x50,0x6F,0x42}; //Левая часть "OfFlPoB"
const unsigned char right_effect_gui [8] = {0x85,0x6F,0x42,0x72,0x53,0x6D,0x52,0x61}; //Правая часть "oBrSmRa"
//Настройки выход
const unsigned char left_settings_0_gui [8] = {0xC0,0xDB,0x65,0x78,0x69,0x74,0x20,0xEF}; //Левая часть "Оexit >"
const unsigned char right_settings_0_gui [8] = {0x85,0x20,0xEF,0x73,0x77,0x61,0x70,0x20}; //Правая часть " >swap "
//Настройки плавность
const unsigned char left_smooth_0_gui [8] = {0xC0,0x53, 0x6D,0x6F,0x6F,0x74,0x68,0x20}; //Левая часть "Smooth "
const unsigned char right_smooth_0_gui [8] = {0x85,0x68,0x20,0x30,0x20,0x20,0x20,0x20}; //Правая часть "h 0    "
const unsigned char left_smooth_33_gui [8] = {0xC0,0x53, 0x6D,0x6F,0x6F,0x74,0x68,0x20}; //Левая часть "Smooth "
const unsigned char right_smooth_33_gui [8] = {0x85,0x68,0x20,0x33,0x33,0x20,0x20,0x20}; //Правая часть "h 33   "
const unsigned char left_smooth_67_gui [8] = {0xC0,0x53, 0x6D,0x6F,0x6F,0x74,0x68,0x20}; //Левая часть "Smooth "
const unsigned char right_smooth_67_gui [8] = {0x85,0x68,0x20,0x36,0x37,0x20,0x20,0x20}; //Правая часть "h 67   "
const unsigned char left_smooth_100_gui [8] = {0xC0,0x53, 0x6D,0x6F,0x6F,0x74,0x68,0x20}; //Левая часть "Smooth "
const unsigned char right_smooth_100_gui [8] = {0x85, 0x68,0x20,0x31,0x30,0x30,0x20,0x20}; //Правая часть "h 100  "
//Настройки яркость глазок при старте
const unsigned char left_eyes_start_0_gui [8] = {0xC0,0x45,0x79,0x65,0x73,0x53,0x74,0x61}; //Левая часть "EyesSta"
const unsigned char right_eyes_start_0_gui [8] = {0x85,0x74,0x61,0x72,0x74,0x20,0x20,0x30}; //Правая часть "tart  0"
const unsigned char left_eyes_start_50_gui [8] = {0xC0,0x45,0x79,0x65,0x73,0x53,0x74,0x61}; //Левая часть "EyesSta"
const unsigned char right_eyes_start_50_gui [8] = {0x85,0x74,0x61,0x72,0x74,0x20,0x35,0x30}; //Правая часть "tart 50"
const unsigned char left_eyes_start_100_gui [8] = {0xC0,0x45,0x79,0x65,0x73,0x53,0x74,0x61}; //Левая часть "EyesSta"
const unsigned char right_eyes_start_100_gui [8] = {0x85,0x74,0x61,0x72,0x74,0x31,0x30,0x30}; //Правая часть "tart100"
//Приветственное сообщение
const unsigned char left_hello_gui [8] = {0xC0,0x4D,0x41,0x4E,0x59,0x41,0x20,0x56}; //Левая часть "MANYA "
const unsigned char right_hello_gui [8] = {0x85,0x20,0x56,0x32,0x31,0x20,0x62,0x79}; //Правая часть " V21 by"
const unsigned char left_name_gui [8] = {0xC0,0x50,0x6F,0x72,0x6F,0x64,0x7A,0x69}; //Левая часть "Porodzi"
const unsigned char right_name_gui [8] = {0x85,0x7A,0x69,0x6E,0x73,0x6B,0x69,0x79}; //Правая часть "zinskiy"
const unsigned char left_name_ft_gui [8] = {0xC0,0x66,0x74,0x20,0x44,0x75,0x62,0x69}; //Левая часть "ft Dubi"
const unsigned char right_name_ft_gui [8] = {0x85,0x62,0x69,0x73,0x68,0x6B,0x69,0x6E}; //Правая часть "bishkin"
  //Адреса для посылок блютуз (домашний json)
const uint8_t bt_address_version = 0;
const uint8_t bt_value_version = 0;
const uint8_t bt_address_auto_colour_red = 1;
const uint8_t bt_value_auto_colour_red = 6;
const uint8_t bt_address_auto_colour_green = 2;
const uint8_t bt_value_auto_colour_green = 77;
const uint8_t bt_address_auto_colour_blue = 3;
const uint8_t bt_value_auto_colour_blue = 107;
const uint8_t bt_address_neon = 20;
const uint8_t bt_address_trunk = 30;
const uint8_t bt_address_angel = 40;
const uint8_t bt_address_demon = 50;

void setup() {
  pinMode(wheel_btn_pin, INPUT); //Указание пинов
  pinMode(trans_vol_down_pin, OUTPUT);
  pinMode(trans_vol_up_pin, OUTPUT);
  pinMode(stock_btn_pin, INPUT);
  wheel_btn_keys.setWindow(30); //Установление окна значений для кнопок с руля
  
  randomSeed(analogRead(0));

  #ifdef DEBUG 
  Serial.begin(9600);
  Serial.println("DEBUG start");
  #else
  // Конфигурация используемых портов ввода-вывода
  DDRB |= (1<<5);
  PORTB &=~ (1<<5);
  DDRB |= (1<<1);   // pin 9 - прием/передача
  DMX_DIR_RX();   // выкл  передатчик

  _delay_ms(10);

  DDRD |= (1<<1);   // pin 1 (TX)
  // Инициализация UART
  UBRR0H = 0;
  UBRR0L = 3;
  UCSR0A = (0<<U2X0)|(0<<MPCM0);
  UCSR0B = (0<<RXCIE0)|(0<<TXCIE0)|(0<<UDRIE0)|(0<<RXEN0)|(0<<TXEN0)|(0<<UCSZ02);
  UCSR0C = (0<<UMSEL01)|(0<<UMSEL00)|(0<<UPM01)|(0<<UPM00)|(1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00)|(0<<UCPOL0);
  _delay_ms(10);
  #endif

  EEPROM.get(513, eyes_on_start_brightness);
  angel.setBrightness(eyes_on_start_brightness);
  angel.setColour(255, 255, 255);
  angel.setSmooth(128);

  demon.setBrightness(0);

  trunk.setBrightness(255);
  trunk.setColour(255, 255, 255);

  EEPROM.get(512, stock_mode_flag); //Проверка, не было ли предыдущее выключении со сток-модом
  if (stock_mode_flag == true) {
    stock_mode_flag = false;
    stock_mode();
    stock_by_program = true;
  }

  //Приветствие
  if(CAN.begin(CAN_125KBPS, MCP_8MHz) == CAN_OK){ //Проверка работы CAN
    for(int i = 0; i < 8; i++){
      CAN.sendMsgBuf(ID_INFO_SYMBOLS, 0, 8, null_gui);
      CAN.sendMsgBuf(ID_DISP1, 0, 8, left_hello_gui);
      CAN.sendMsgBuf(ID_DISP2, 0, 8, right_hello_gui);
      delay(CAN_period);
    }
    DMX_write();
    for(int i = 0; i < 6; i++){
      CAN.sendMsgBuf(ID_INFO_SYMBOLS, 0, 8, null_gui);
      CAN.sendMsgBuf(ID_DISP1, 0, 8, left_name_gui);
      CAN.sendMsgBuf(ID_DISP2, 0, 8, right_name_gui);
      delay(CAN_period);
    }
    
    for(int i = 0; i < 6; i++){
      CAN.sendMsgBuf(ID_INFO_SYMBOLS, 0, 8, null_gui);
      CAN.sendMsgBuf(ID_DISP1, 0, 8, left_name_ft_gui);
      CAN.sendMsgBuf(ID_DISP2, 0, 8, right_name_ft_gui);
      delay(CAN_period);
    }  
  }
  
  bluetooth.begin(9600);
  neon.smoothAfterStart();
}

void loop() {
  wheel_btn();
  bluetooth_input_software();
  CAN_send();
  //led_effects();
  stock_mode_by_btn();
}

void wheel_btn() { //Функция для чтений и обработки кнопок с руля
  //Инициализация виртуальных кнопок
  wheel_btn_mute.tick(wheel_btn_keys.status(0));
  wheel_btn_mode.tick(wheel_btn_keys.status(1));
  wheel_btn_prev.tick(wheel_btn_keys.status(2));
  wheel_btn_next.tick(wheel_btn_keys.status(3));
  wheel_btn_vol_up.tick(wheel_btn_keys.status(4));
  wheel_btn_vol_down.tick(wheel_btn_keys.status(5));

  //Обработка нажатий
  if (wheel_btn_vol_down.press()) { //Если нажали одиночно на кнопку Vol+
    #ifdef DEBUG //Отладка в консоль
    Serial.println("Vol- pressed");
    #endif
    digitalWrite(trans_vol_down_pin, HIGH); //Подаем питание на транзистор
  } else if (wheel_btn_vol_down.release()) { //Если отпустили кнопку Vol+
    #ifdef DEBUG //Отладка в консоль
    Serial.println("Vol- released");
    #endif
    digitalWrite(trans_vol_down_pin, LOW); //Отключаем питание на транзистор
  } 
  if (wheel_btn_vol_up.press()) { //Если нажали одиночно на кнопку Vol+
    #ifdef DEBUG //Отладка в консоль
    Serial.println("Vol+ pressed");
    #endif
    digitalWrite(trans_vol_up_pin, HIGH); //Подаем питание на транзистор
  } else if (wheel_btn_vol_up.release()) { //Если отпустили кнопку Vol+
    #ifdef DEBUG //Отладка в консоль
    Serial.println("Vol+ released");
    #endif
    digitalWrite(trans_vol_up_pin, LOW); //Отключаем питание на транзистор
  }
  if (wheel_btn_next.press() || wheel_btn_next.step()) { //Одиночное нажатие или удержание кнопки Seek+
    #ifdef DEBUG //Отладка в консоль
    Serial.println("Seek+ press/step");
    #endif
    if (stock_mode_flag == false){
      if (mazda_gui_menu_flag == true) { //Находимся в меню
        mazda_gui_menu_counter++; //Сдвигаем на один
        if (mazda_gui_menu_counter == 6) mazda_gui_menu_counter = 0; //Циклическое меню
      } else if (mazda_gui_menu_counter == 0){ //Находимся в переключении треков
        bt_send(12, 0);
      } else if (mazda_gui_menu_counter == 1){ //Находимся в борткомпе
        bort_comp_change_flag = true;
      } else if (mazda_gui_menu_counter == 2){ //Находимся в переключении яркости неона
        effect_counter = 0;
        
        neon.brightnessAdd(); //Добавляем 20 процентов яркости
        bt_send_manyaled(bt_address_neon, neon.getBrightness(), neon.red(), neon.green(), neon.blue(), neon.getSmooth());
        DMX_write(); //Отправляем в DMX
      } else if (mazda_gui_menu_counter == 3){ //Находимся в переключении цвета неона
        effect_counter = 0;
        
        neon.colourAdd(); //Переключаем на следующий цвет
        bt_send_manyaled(bt_address_neon, neon.getBrightness(), neon.red(), neon.green(), neon.blue(), neon.getSmooth());
        DMX_write(); //Отправляем в DMX
      } else if (mazda_gui_menu_counter == 4){ //Находимся в переключении режима неона
        effect_counter = effect_counter + 1;
        
        if (effect_counter > 5) effect_counter = 0;
      } else if (mazda_gui_menu_counter == 5){ //Находимся в настройках
        mazda_gui_settings_counter++;
        if (mazda_gui_settings_counter > 2) mazda_gui_settings_counter = 0; //Циклическое меню
      }
    }
  }
  if (wheel_btn_prev.press() || wheel_btn_prev.step()) { //Одиночное нажатие или удержание кнопки Seek-
    #ifdef DEBUG //Отладка в консоль
    Serial.println("Seek- press/step");
    #endif
    if (stock_mode_flag == false){
      if (mazda_gui_menu_flag == true) { //Находимся в меню
        mazda_gui_menu_counter--; //Сдвигаем на один
        if (mazda_gui_menu_counter == -1) {
          mazda_gui_menu_counter = 5; //Циклическое меню
        } 
      } else if (mazda_gui_menu_counter == 0){ //Находимся в переключении треков
        bt_send(10, 0);
      } else if (mazda_gui_menu_counter == 1){ //Находимся в борткомпе
        bort_comp_reset_flag = true;
      } else if (mazda_gui_menu_counter == 2){ //Находимся в переключении яркости неона
        effect_counter = 0;
        
        neon.brightnessReduce(); //Убавляем 20 процентов яркости
        bt_send_manyaled(bt_address_neon, neon.getBrightness(), neon.red(), neon.green(), neon.blue(), neon.getSmooth());
        DMX_write(); //Отправляем в DMX
      } else if (mazda_gui_menu_counter == 3){ //Находимся в переключении цвета неона
        effect_counter = 0;
        
        neon.colourReduce(); //Переключаем на предыдущий цвет
        bt_send_manyaled(bt_address_neon, neon.getBrightness(), neon.red(), neon.green(), neon.blue(), neon.getSmooth());
        DMX_write(); //Отправляем в DMX
      } else if (mazda_gui_menu_counter == 4){ //Находимся в переключении режима неона
        
        effect_counter = effect_counter - 1;
        if (effect_counter < 0) effect_counter = 5;
        
      } else if (mazda_gui_menu_counter == 5){ //Находимся в настройках
        mazda_gui_settings_counter--;
        if (mazda_gui_settings_counter == -1) mazda_gui_settings_counter = 2; //Циклическое меню
      }
    }
    
  }
  if (wheel_btn_mode.press()) { //Одиночное нажатие кнопки Mode
    #ifdef DEBUG //Отладка в консоль
    Serial.println("Mode press");
    #endif
    wheel_btn_mode_time = millis(); //Запоминаем время нажатия
    if (stock_mode_flag == false) {
      if (mazda_gui_menu_counter != 5 || (mazda_gui_menu_counter == 5 && mazda_gui_menu_flag == true)) { //Не в настройках
        mazda_gui_menu_flag = !mazda_gui_menu_flag; //Входим/выходим из меню
      } else { //Если в настройках
        if (mazda_gui_settings_counter == 0) { //На выходе
        mazda_gui_menu_flag = true;
        } else if (mazda_gui_settings_counter == 1) { //Настройка плавности
            effect_counter = 0;
            
            neon.smoothChange();
            bt_send_manyaled(bt_address_neon, neon.getBrightness(), neon.red(), neon.green(), neon.blue(), neon.getSmooth());
        } else if (mazda_gui_settings_counter == 2) { //Настройка глазок на старте
          if (eyes_on_start_brightness == 0) eyes_on_start_brightness = 128;
          else if (eyes_on_start_brightness == 128) eyes_on_start_brightness = 255;
          else if (eyes_on_start_brightness == 255) eyes_on_start_brightness = 0;
          EEPROM.put(513, eyes_on_start_brightness);
        } 
      }
    }
  }
  if (wheel_btn_mute.press()) { //Одиночное нажатие кнопки Mute
    #ifdef DEBUG //Отладка в консоль
    Serial.println("Mute press");
    #endif
    if ((millis() - wheel_btn_mode_time <= 500)) {
      stock_mode(); //Входим/выходим из сток-мода
      stock_by_program = true;
    } 
    else { //Play/Pause
      bt_send(11, 0);
    }
  }
}
void stock_mode_by_btn() { //Функция для чтения кнопки сток мода
  if ((millis() - stock_by_btn_time) >= 100){
    stock_by_btn_time = millis();
    bool dimmer_voltage = digitalRead(stock_btn_pin);
    #ifdef DEBUG
    Serial.print ("Dimmer voltage = ");
    Serial.println(dimmer_voltage);
    #endif
    if (stock_mode_flag == false && dimmer_voltage == false) { //Входим в сток мод
      stock_mode();
    } else if (stock_mode_flag == true && dimmer_voltage == true && stock_by_program == false && need_to_stock_delay_flag == false) { //Выходим из сток-мода
      need_to_stock_delay_flag = true;
      stock_delay_time = millis();
    }
    if (need_to_stock_delay_flag == true && ((millis() - stock_delay_time) >= 200)) {
      need_to_stock_delay_flag = false;
      stock_mode();
    }
  }
}
void stock_mode() { //Функция для сток-мода
  stock_mode_flag = !stock_mode_flag;
  if (stock_by_program == true) {
    EEPROM.put(512, stock_mode_flag);
  } 
  if (stock_mode_flag == true) { //Входим в сток-мод
    effect_counter = 0;
    stock_mode_before[0] = neon.getBrightness();
    stock_mode_before[1] = angel.getBrightness();
    stock_mode_before[2] = neon.getSmooth();
    stock_mode_before[3] = angel.getSmooth();
    neon.setBrightness(0);
    angel.setBrightness(0);
    demon.setBrightness(0);
    neon.setSmooth(0);
    angel.setSmooth(0);
  } else if (stock_mode_flag == false) { //Выходим из сток-мода
    mazda_gui_menu_flag = false;
    stock_by_program = false;
    mazda_gui_menu_counter = 0;
    
    neon.setBrightness(stock_mode_before[0]);
    angel.setBrightness(stock_mode_before[1]);
    neon.setSmooth(stock_mode_before[2]);
    angel.setSmooth(stock_mode_before[3]);
  }
  DMX_write();
}
void bluetooth_input_software() { //Функция для принятия по bluetooth
  if (bluetooth.available()) { //Пришло что-то с телефона
    uint8_t bt_address = bluetooth.read(); //Считываем адрес
    bt_delay();
    //Serial.println(bt_address);
    #ifdef DEBUG //Отладка в консоль
    Serial.print("Address = ");
    Serial.println(bt_address);
    #endif
    
    if (1 == 1) { //Принимаем данные
      if (bt_address == 0) { //Запросы
        int bt_value = bluetooth.read();
        if (bt_value == 100) { //Запросы на старте
          bt_send(bt_address_auto_colour_red, bt_value_auto_colour_red);
          bt_send(bt_address_auto_colour_green, bt_value_auto_colour_green);
          bt_send(bt_address_auto_colour_blue, bt_value_auto_colour_blue);
        }
      } else if (bt_address == 10) { //Название трека
        memset(song_name, 0, sizeof(song_name)); //Чистка предыдущего названия трека
        uint8_t song_name_size = bluetooth.read(); //Чтение размера названия трека
        bluetooth.readBytes(song_name, song_name_size); //Запись названия трека
        running_stroke_counter = 0;
      } 
      else if (bt_address == 20) { //Неон яркость
        effect_counter = 0;
        
        uint8_t msg [5];
        bluetooth.readBytes(msg, 5);
        neon.setBrightness(msg[0]);
        neon.setColourRed(msg[1]);
        neon.setColourGreen(msg[2]);
        neon.setColourBlue(msg[3]);
        neon.setSmooth(msg[4]);
        DMX_write();
        bt_send_manyaled(bt_address_neon, neon.getBrightness(), neon.red(), neon.green(), neon.blue(), neon.getSmooth());
      } else if (bt_address == 25) { //Загрузить из памяти 1
        effect_counter = 0;
        
        bt_delay();
        uint8_t bt_value = bluetooth.read();
        neon.setFromM1();
        DMX_write();
        bt_send_manyaled(bt_address_neon, neon.getBrightness(), neon.red(), neon.green(), neon.blue(), neon.getSmooth());
      } else if (bt_address == 26) { //Сохранить в память 1
        effect_counter = 0;
        
        bt_delay();
        uint8_t bt_value = bluetooth.read();
        neon.saveToM1();
      } else if (bt_address == 27) { //Загрузить из памяти 2
        effect_counter = 0;
        
        bt_delay();
        uint8_t bt_value = bluetooth.read();
        neon.setFromM2();
        DMX_write();
        bt_send_manyaled(bt_address_neon, neon.getBrightness(), neon.red(), neon.green(), neon.blue(), neon.getSmooth());
      } else if (bt_address == 28) { //Сохранить в память 2
        effect_counter = 0;
        
        bt_delay();
        uint8_t bt_value = bluetooth.read();
        neon.saveToM2();
      }
      else if (bt_address == 30) { //Неон яркость
        effect_counter = 0;
        
        uint8_t msg [5];
        bluetooth.readBytes(msg, 5);
        trunk.setBrightness(msg[0]);
        trunk.setColourRed(msg[1]);
        trunk.setColourGreen(msg[2]);
        trunk.setColourBlue(msg[3]);
        trunk.setSmooth(msg[4]);
        DMX_write();
        bt_send_manyaled(bt_address_trunk, trunk.getBrightness(), trunk.red(), trunk.green(), trunk.blue(), trunk.getSmooth());
      }
      else if (bt_address == 40) { //Неон яркость
        effect_counter = 0;
        
        uint8_t msg [5];
        bluetooth.readBytes(msg, 5);
        angel.setBrightness(msg[0]);
        angel.setColourRed(msg[1]);
        angel.setColourGreen(msg[2]);
        angel.setColourBlue(msg[3]);
        angel.setSmooth(msg[4]);
        DMX_write();
        bt_send_manyaled(bt_address_angel, angel.getBrightness(), angel.red(), angel.green(), angel.blue(), angel.getSmooth());
      }
      else if (bt_address == 50) { //Неон яркость
        effect_counter = 0;
        
        uint8_t msg [5];
        bluetooth.readBytes(msg, 5);
        demon.setBrightness(msg[0]);
        demon.setColourRed(msg[1]);
        demon.setColourGreen(msg[2]);
        demon.setColourBlue(msg[3]);
        angel.setSmooth(msg[4]);
        DMX_write();
        bt_send_manyaled(bt_address_demon, demon.getBrightness(), demon.red(), demon.green(), demon.blue(), angel.getSmooth());
      }
    }
  }
}
void led_effects() { //Функция для обработки эффектов
  if (effect_counter != 0) { //Если активен какой-либо эффект на неоне
    if (effect_counter == 4 && ((millis() - led_effect_time) >= 75)){
      #ifdef DEBUG
      Serial.println(effect_counter);
      #endif
      led_effect_time = millis();
      for(int i = 0; i < 6; i++) {
        dmx_buf[i] = 143 + sin(2*3.14*i/6)*112;
      }
      dmx_buf[6] = 25;
      dmx_buf[7] = 0xFF;
      for(int x = 0; x < 45; x++) {
        dmx_buf[8] = dmx_buf[0];
        for(int i = 0; i < 5; i++) {
          dmx_buf[i] = dmx_buf[i+1];
        }
        dmx_buf[5] = dmx_buf[8];
        Send_channels_to_all(); 
        //_delay_ms(75);
      }
    }
    
  }
}
void Send_channels_to_all() {
  for(int i = 1; i < 6; i++) {
    for(int j = 0; j < 8; j++) {
      dmx_buf[i*8+j] = dmx_buf[j];
    }
  }
  DMX_send(dmx_buf, DMX_CMD_LEN);
}

void DMX_send(unsigned char *data, unsigned int len) {
  if ((millis() - dmx_send_time) >= 50){
    dmx_send_time = millis();
  
    DMX_OUT_LOW();
    _delay_us(250);  //Break
   DMX_OUT_HIGH();
    _delay_us(16); //Mark-After-Break
  
    uint8_t s = 0x00;
    len++;

    UCSR0B |= (1<<TXEN0);
  
    while(len--) {
      while(!(UCSR0A & (1<<UDRE0))) {};
      UDR0 = s;
    
      s = *data++;
    }
  
    UCSR0B &=~ (1<<TXEN0);
    DMX_OUT_HIGH();
  }
}
void DMX_write() { //Функция для формирования буфера DMX
  //Диммер над магнитолой 1
  dmx_buf[2] = neon.red()*neon.getBrightness()/255;
  dmx_buf[1] = neon.green()*neon.getBrightness()/255;
  dmx_buf[0] = neon.blue()*neon.getBrightness()/255;
  dmx_buf[5] = neon.red()*neon.getBrightness()/255;
  dmx_buf[4] = neon.green()*neon.getBrightness()/255;
  dmx_buf[3] = neon.blue()*neon.getBrightness()/255;
  dmx_buf[6] = neon.getSmooth();
  dmx_buf[7] = 255;
  //Диммер над магнитолой 2
  dmx_buf[10] = neon.red()*neon.getBrightness()/255;
  dmx_buf[9] = neon.green()*neon.getBrightness()/255;
  dmx_buf[8] = neon.blue()*neon.getBrightness()/255;
  dmx_buf[13] = neon.red()*neon.getBrightness()/255;
  dmx_buf[12] = neon.green()*neon.getBrightness()/255;
  dmx_buf[11] = neon.blue()*neon.getBrightness()/255;
  dmx_buf[14] = neon.getSmooth();
  dmx_buf[15] = 255;
  //Диммер в багажнике
  dmx_buf[16] = 0;
  dmx_buf[17] = 0;
  if (trunk.red() == 255 && trunk.green() == 255 && trunk.blue() == 255){
    dmx_buf[18] = 0;
    dmx_buf[19] = 0;
    dmx_buf[20] = 0;
    dmx_buf[21] = trunk.getBrightness(); 
  } else {
    dmx_buf[18] = trunk.red()*trunk.getBrightness()/255;
    dmx_buf[19] = trunk.green()*trunk.getBrightness()/255;
    dmx_buf[20] = trunk.blue()*trunk.getBrightness()/255;
    dmx_buf[21] = 0; 
  }
  dmx_buf[22] = 128;
  dmx_buf[23] = 255;
  //Диммер в левой фаре
  dmx_buf[24] = angel.red()*angel.getBrightness()/255;
  dmx_buf[25] = angel.green()*angel.getBrightness()/255;
  dmx_buf[26] = angel.blue()*angel.getBrightness()/255;
  dmx_buf[27] = demon.red()*demon.getBrightness()/255;
  dmx_buf[28] = demon.green()*demon.getBrightness()/255;
  dmx_buf[29] = demon.blue()*demon.getBrightness()/255;
  dmx_buf[30] = angel.getSmooth();
  dmx_buf[31] = 255;
  //Диммер в правой фаре
  dmx_buf[32] = angel.red()*angel.getBrightness()/255;
  dmx_buf[33] = angel.green()*angel.getBrightness()/255;
  dmx_buf[34] = angel.blue()*angel.getBrightness()/255;
  dmx_buf[35] = demon.red()*demon.getBrightness()/255;
  dmx_buf[36] = demon.green()*demon.getBrightness()/255;
  dmx_buf[37] = demon.blue()*demon.getBrightness()/255;
  dmx_buf[38] = angel.getSmooth();
  dmx_buf[39] = 255;

  DMX_send(dmx_buf, DMX_CMD_LEN);
}
void CAN_send() { //Функция отправки в CAN
  //Отправка по адресам экрана и символов (Mazda_GUI)
  if ((millis() - can_send_time) >= CAN_period) {
    if (bort_comp_reset_flag == true) {
      if (bort_comp_counter < 4) {
        Mazda_GUI_CAN_package(left_reseting_gui, right_reseting_gui, -1);
        for (int i = 0; i < 8; i++){
          mazda_gui_symbols[i] = info_push_btn[i];
        }
        bort_comp_counter++;
      } else {
        Mazda_GUI_CAN_package(left_reseting_gui, right_reseting_gui, -1);
        for (int i = 0; i < 8; i++){
          mazda_gui_symbols[i] = info_pull_btn[i];
        }
        bort_comp_counter = 0;
        bort_comp_reset_flag = false;
      }
    } else if (bort_comp_change_flag == true){
      if (bort_comp_counter < 1) {
        Mazda_GUI_CAN_package(left_bort_comp_gui, right_bort_comp_gui, -1);
        for (int i = 0; i < 8; i++){
          mazda_gui_symbols[i] = info_push_btn[i];
        }
        bort_comp_counter++;
      } else {
        Mazda_GUI_CAN_package(left_bort_comp_gui, right_bort_comp_gui, -1);
        for (int i = 0; i < 8; i++){
          mazda_gui_symbols[i] = info_pull_btn[i];
        }
        bort_comp_counter = 0;
        bort_comp_change_flag = false;
      }
    } else if (stock_mode_flag == true) Mazda_GUI_CAN_package(left_stock_gui, right_stock_gui, -1);
    else if (mazda_gui_menu_flag == true) Mazda_GUI_CAN_package(left_menu_gui, right_menu_gui, mazda_gui_menu_counter);
    else if (mazda_gui_menu_flag == false && mazda_gui_menu_counter == 0) { //Название песни
      running_stroke();
      Mazda_GUI_CAN_package(left_running_stroke, right_running_stroke, -1); 
    } 
    else if (mazda_gui_menu_flag == false && mazda_gui_menu_counter == 1) Mazda_GUI_CAN_package(left_bort_comp_gui, right_bort_comp_gui, -1); //Борт компьютер
    else if (mazda_gui_menu_flag == false && mazda_gui_menu_counter == 2) {//Яркость неона
      if (neon.getBrightness() == 0) Mazda_GUI_CAN_package(left_0_bright_gui, right_0_bright_gui, -1);
      else if (neon.getBrightness() > 0 && neon.getBrightness() <= 76) Mazda_GUI_CAN_package(left_20_bright_gui, right_20_bright_gui, -1); //51
      else if (neon.getBrightness() > 76 && neon.getBrightness() <= 127) Mazda_GUI_CAN_package(left_40_bright_gui, right_40_bright_gui, -1); //102
      else if (neon.getBrightness() > 127 && neon.getBrightness() <= 178) Mazda_GUI_CAN_package(left_60_bright_gui, right_60_bright_gui, -1); //153
      else if (neon.getBrightness() > 178 && neon.getBrightness() < 255) Mazda_GUI_CAN_package(left_80_bright_gui, right_80_bright_gui, -1); //204
      else if (neon.getBrightness() == 255) Mazda_GUI_CAN_package(left_100_bright_gui, right_100_bright_gui, -1); //255
    } else if (mazda_gui_menu_flag == false && mazda_gui_menu_counter == 3) Mazda_GUI_CAN_package(left_colour_gui, right_colour_gui, neon.positionMazdaGuiCheck()); //Цвет неона
    else if (mazda_gui_menu_flag == false && mazda_gui_menu_counter == 4) Mazda_GUI_CAN_package(left_effect_gui, right_effect_gui, effect_counter); //Эффекты неона
    else if (mazda_gui_menu_flag == false && mazda_gui_menu_counter == 5) { //Настройки
      if (mazda_gui_settings_counter == 0) Mazda_GUI_CAN_package(left_settings_0_gui, right_settings_0_gui, -1); //Выход
      else if (mazda_gui_settings_counter == 1) { //Плавность
        if (neon.getSmooth() == 0) Mazda_GUI_CAN_package(left_smooth_0_gui, right_smooth_0_gui, -1);
        else if (neon.getSmooth() > 0 && neon.getSmooth() <= 128) Mazda_GUI_CAN_package(left_smooth_33_gui, right_smooth_33_gui, -1);
        else if (neon.getSmooth() > 128 && neon.getSmooth() < 255) Mazda_GUI_CAN_package(left_smooth_67_gui, right_smooth_67_gui, -1);
        else if (neon.getSmooth() == 255) Mazda_GUI_CAN_package(left_smooth_100_gui, right_smooth_100_gui, -1);
      }
      else if (mazda_gui_settings_counter == 2) { //Глазки на старте
        if (eyes_on_start_brightness == 0) Mazda_GUI_CAN_package(left_eyes_start_0_gui, right_eyes_start_0_gui, -1);
        else if (eyes_on_start_brightness == 128) Mazda_GUI_CAN_package(left_eyes_start_50_gui, right_eyes_start_50_gui, -1);
        else if (eyes_on_start_brightness == 255) Mazda_GUI_CAN_package(left_eyes_start_100_gui, right_eyes_start_100_gui, -1);
      } 
    }  
    CAN.sendMsgBuf(ID_DISP1, 0, 8, mazda_gui_left);
    CAN.sendMsgBuf(ID_DISP2, 0, 8, mazda_gui_right);
    CAN.sendMsgBuf(ID_INFO_SYMBOLS, 0, 8, mazda_gui_symbols);
    can_send_time = millis();
  }
}
void Mazda_GUI_CAN_package(const unsigned char* left, const unsigned char* right, int8_t position){
  for (int i = 0; i < 8; i++){
    mazda_gui_left[i] = *(left + i);
    mazda_gui_right[i] = *(right + i);
    if (position == -1) mazda_gui_symbols[i] = null_gui[i];
    else if (position == 0) mazda_gui_symbols[i] = first_gui[i];
    else if (position == 1) mazda_gui_symbols[i] = second_gui[i];
    else if (position == 2) mazda_gui_symbols[i] = third_gui[i];
    else if (position == 3) mazda_gui_symbols[i] = fourth_gui[i];
    else if (position == 4) mazda_gui_symbols[i] = fifth_gui[i];
    else if (position == 5) mazda_gui_symbols[i] = sixth_gui[i];
  }
}

void running_stroke(){ //Функция бегущей строки 
  uint8_t size = strlen(song_name);
  if (size > 0){
    if (strlen(song_name) <= 12) {
      for (uint8_t i = (size); i < 12; i++){
        song_name[i] = 0x20;
      }
      for (uint8_t i = 0; i < 7; i++){
        left_running_stroke[i + 1] = song_name[i];
        right_running_stroke[i + 1] = song_name[i+5];
      }
    } else {
      if (running_stroke_counter < 4){
        for (uint8_t i = 0; i < 7; i++){
          left_running_stroke[i + 1] = song_name[i];
          right_running_stroke[i + 1] = song_name[i+5];
        }
        running_stroke_counter += 1;
      } else if (running_stroke_counter < (size - 12 + 4)) {
        for (uint8_t i = 0; i < 7; i++){
          left_running_stroke[i + 1] = song_name[i + running_stroke_counter - 4];
          right_running_stroke[i + 1] = song_name[i+5 + running_stroke_counter - 4];
        }
        running_stroke_counter += 1;
      } else if (running_stroke_counter == (size - 12 + 4)){
        for (uint8_t i = 0; i < 7; i++){
          left_running_stroke[i + 1] = song_name[i + running_stroke_counter - 4];
          right_running_stroke[i + 1] = song_name[i+5 + running_stroke_counter - 4];
        }
        running_stroke_counter = 0;
      }
    }
  }
}

void bt_send_manyaled (const uint8_t start_address, uint8_t brightness, uint8_t red, uint8_t green, uint8_t blue, uint8_t smooth) { //Функция для формирования пакетов при помощи библиотеки
  uint8_t values[5] = {brightness, red, green, blue, smooth};
  for (uint32_t i = 0; i < sizeof(values); i++) {
    bt_send (i + start_address, values[i]);
  }
}
void bt_send (uint8_t adr, uint8_t msg) { //Функция для отправки по БТ
  bluetooth.write(adr);
  bluetooth.write(msg);
}
void bt_delay(){
  uint8_t timer = 0;
  while (!bluetooth.available() && (timer < 250)){
    _delay_us(100);
    timer++;
  }
}