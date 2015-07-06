/**
 * Author: I Putu Susila
 * E-Mail: susila{.}ip{@}gmail{.}com
 * Date  : 17.06.2015
 * 
 * Hardware: Pocket-Geiger
 *   http://www.radiation-watch.org/p/english.html
 *   
 * PubSubClient packet size must be modified, e.g. to 180 bytes
 */

#include <PubSubClient.h>
#include <WizClientAuto.h>

//various constant
const int ledPin = 13;
const int tempPin = 1;
const int humPin = 2;
const int pirPin = 3;
const int signPin = 5;        //T1
const int noisePin = 2;       //External interrupt 0
const double NTICKSEC = 0.2;  //Tick in seconds (don't change this)
const int NCPS  = 10;         //cps buffer count (4 seconds)
const int NCPM  = 30;         //cpm buffer count (2 minutes)
const int NTEMP = 25;         //temp buffer count
const int NHUM = 25;          //hum buffer count
const int NPIR = 5;           //pir buffer count
const int TOKEN = 0x0001;     //token to identify endian

const double alphaCPM = 53.032;           //cpm = uSv x alpha (don't change this)
const double alphaCPS = alphaCPM / 60;    //cps = (uSv x alpha) / 60
const unsigned long MAXMILLIS = 4294967295UL;

unsigned int tempBuf[NTEMP];
int tempPos, tempEnd;

unsigned int humBuf[NHUM];
int humPos, humEnd;

byte pirBuf[NPIR];
byte pirPos, pirEnd;

unsigned int nsend, sendTick; //sending interval
unsigned int cpmBuf[NCPM];    //cpm count history
int cpmPos, cpmEnd;           //cpm index and size

unsigned int cpsBuf[NCPS];    //cps count buffer
int cpsPos, cpsEnd;           //cps index and size
byte conf;                    //configure/run

//timer related
double second;
byte minute;
unsigned long hour;
unsigned long prevTime;
double cntCPS, vCPS, uSvs, errSvs;
double cntCPM, vCPM, uSvm, errSvm;
double temp, humidity, pir;
double vTemp, vHumidity, vPir;

//interrupt related
volatile byte isReady;
volatile byte tick;
volatile byte cntReaded;
volatile unsigned int signCount;
volatile unsigned int noiseCount;

unsigned int radCnt, nsCnt;

//server and mqtt related
//change SSID, WPA, server address and Client-ID
const char * ssid   = "MonLink";
const char * wpa    = "monLink2015";
byte mqServer[]     = {192, 168, 1, 10};
const int mqPort    = 1883;
const char * cidStr = "rad";
const int cid       = 1;

WizClientAuto wiz;
PubSubClient mqClient(mqServer, mqPort, NULL, wiz);

//send buffer
byte mqData[80];
char mqBuf[20];

//macro for storing data (d) of type (t) to buffer (p) as binary
#define WRITEB(t, p, d)   ((*(t *)(p)) = d, p += sizeof(t))

/**
 * Various initialisation.
 * - Timer, counter, ext interrupt, etc
 */
void setup() {
  int idx;                  //index

  isReady = 0;
  //init other variable
  cli();
  
  nsend = 1;                //1 tick (0.2 seconds)
  sendTick = 0;             //tick for data sending mark
  conf = 0;                 //running mode (not configuration)

  second = 0.0;
  minute = 0;
  hour = 0;

  tick = 0;
  cntReaded = 0;
  signCount = 0;
  noiseCount = 0;

  cntCPM = 0.0;
  uSvm = 0.0;
  errSvm = 0.0;
  vCPM = 0.0;
  cpmPos = 0;
  cpmEnd = 0;
  for (idx = 0; idx < NCPM; idx++) {
    cpmBuf[idx] = 0;
  }

  cntCPS = 0.0;
  uSvs = 0.0;
  errSvs = 0.0;
  vCPS = 0.0;
  cpsPos = 0;
  cpsEnd = 0;
  for (idx = 0; idx < NCPS; idx++) {
    cpsBuf[idx] = 0;
  }

  tempPos = 0;
  tempEnd = 0;
  for (idx = 0; idx < NTEMP; idx++) {
    tempBuf[idx] = 0;
  }

  humPos = 0;
  humEnd = 0;
  for(idx = 0; idx < NHUM; idx++) {
    humBuf[idx] = 0;
  }

  pirPos = 0;
  pirEnd = 0;
  for(idx = 0; idx < NPIR; idx++) {
    pirBuf[idx] = 0;
  }
  temp = 0.0;
  vTemp = 0.0;
  humidity = 0.0;
  vHumidity = 0.0;
  pir = 0.0;
  vPir = 0.0;

  //LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  
  //init signal pin
  pinMode(signPin, INPUT);
  digitalWrite(signPin, HIGH);

  //init noise pin
  pinMode(noisePin, INPUT);
  digitalWrite(noisePin, HIGH);

  //init pir pin
  pinMode(pirPin, INPUT);

  //external interrupt 0 (noise)
  EICRA |= (1 << ISC01);    // set INT0 to trigger on falling edge
  EIMSK |= (1 << INT0);     // Turns on INT0

  //setup counter 1, count on falling edge
  TCCR1B = (1 << CS12) | (1 << CS11);
  TCNT1 = 0;
  
  //setup timer2
  //ctc mode
  TCCR2A = (1 << WGM21);
  //prescaler 1024
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
  //8 ms, 16 MHz, prescaler 1024
  OCR2A = 124;  
  //compare match
  TIMSK2 = (1 << OCIE2A);
  TCNT2 = 0;

  //enable interrupt
  sei();

  //init wiz and mqtt client
  delay(500);
  wiz.setup(115200, ssid, wpa);
  sprintf(mqBuf, "%s%d", cidStr, cid);
  if(mqClient.connect(mqBuf)) {
    sprintf(mqBuf, "bsensors/%d", cid);
    mqClient.publish(mqBuf, "{}");
  }
  else {
    mqBuf[0] = 0;
  }
  
  digitalWrite(ledPin, LOW);
  isReady = 1;

  //save time stamp
  prevTime = millis();
}

/**
 * Calculate timing since modul started.
 */
void calculateTiming() {
  //Calculate time in second
  unsigned long curTime = millis();
  double dsec;

  //take care overflow
  if (curTime > prevTime) {
    dsec = curTime - prevTime;
  } else {
    dsec = (MAXMILLIS - prevTime) + 1;
    dsec += curTime;
  }
  dsec /= 1000.0;
  second += dsec;
  prevTime = curTime; 

  //second to minute
  if (second >= 60.0) {
    second = 0.0;
    minute++;
  }

  //minute to hour
  if (minute >= 60) {
    minute = 0;
    hour++;
  } 
}

/**
 * Calculate doserate.
 */
void calculateDoserate() {
  //ignore noise
  if (nsCnt == 0) {  
    //accumulate cps
    cntCPS += radCnt;
    cntCPS -= cpsBuf[cpsPos];
    cpsBuf[cpsPos++] = radCnt;

    //accumulate cpm
    cntCPM += radCnt;

    //reset cps pos (circullar buffer)
    if (cpsPos >= NCPS) {
      cpsPos = 0;

      //shift cpm buffer, and subtract total cpm
      cntCPM -= cpmBuf[cpmPos];
      cpmBuf[cpmPos++] = (unsigned int)cntCPS;

      //reset circular buffer pos
      if (cpmPos >= NCPM) {
        cpmPos = 0;
      }
      if (cpmEnd < NCPM) {
        cpmEnd++;
      }
    }
    if (cpsEnd < NCPS) {
      cpsEnd++;
    }    
   
    //calculate cpm, uSv/h, error. correct second fraction
    double accTime = ((NCPS * NTICKSEC) * cpmEnd + cpsPos * NTICKSEC) / 60;
    vCPM = cntCPM / accTime;    
    //calculate uSv/h and error
    uSvm = vCPM / alphaCPM;
    errSvm = (sqrt(cntCPM) / accTime) / alphaCPM;  

    //calculate cps, uSv/h, error
    accTime = NTICKSEC * cpsEnd;
    vCPS = cntCPS / accTime;
    //calculate uSv/h and error
    uSvs = vCPS / alphaCPS;
    errSvs = (sqrt(cntCPS) / accTime) / alphaCPS;  
  }
}

/**
 * Calculate other sensor values.
 */
void calculateSensors() {
  //temp ain1
  //INTERNAL
  //reading = analogRead(tempPin);
  //tempC = reading / 9.31;

  //5V
  //temp = (5.0 * analogRead(tempPin) * 100.0) / 1024;
  
  int ain = analogRead(tempPin);
  temp += ain;
  temp -= tempBuf[tempPos];
  tempBuf[tempPos++] = ain;
  if (tempPos >= NTEMP) {
    tempPos = 0;
  }
  if (tempEnd < NTEMP) {
    tempEnd++;
  }
  vTemp = temp / tempEnd;
  vTemp = (500.0 * vTemp) / 1024;

  ain = analogRead(humPin);
  humidity += ain;
  humidity -= humBuf[humPos];
  humBuf[humPos++] = ain;
  if (humPos >= NHUM) {
    humPos = 0;
  }
  if (humEnd < NHUM) {
    humEnd++;
  }
  vHumidity = humidity / humEnd;
  vHumidity = ((4.882814 * vHumidity) - 800) / 31;

  //Humidity sensor http://vellamy.blogspot.com/p/sensorssocketprocessing.html
  //5V=1024;5000mV/1024=4.882814mv por unidad
  //float humidity=analogRead(humidityPin)* 4.882814;
  //humidity=(humidity-800)/31;//Conversion of milivolts to humidity
  
  //pir
  int dig = digitalRead(pirPin);
  pir += dig;
  pir -= pirBuf[pirPos];
  pirBuf[pirPos++] = dig;
  if (pirPos >= NPIR) {
    pirPos = 0;
  }
  if (pirEnd < NPIR) {
    pirEnd++;
  }
  vPir = pir / pirEnd;
}

/**
 * Send result if needed
 */
void sendResult() {
  //send result
  sendTick++;
  if (sendTick >= nsend) {
    sendTick = 0;

    byte * ptr = &mqData[1];
    //token
    const byte len = sizeof(byte) + 3*sizeof(int) + 2*sizeof(long) + 10 * sizeof(double);
    mqData[0] = len - sizeof(byte);
    WRITEB(int, ptr, TOKEN);
    WRITEB(long, ptr, (long)cid);
    WRITEB(long, ptr, hour);
    WRITEB(unsigned int, ptr, minute);
    WRITEB(double, ptr, second);
    WRITEB(unsigned int, ptr, radCnt);
    WRITEB(double, ptr, vCPS);
    WRITEB(double, ptr, uSvs);
    WRITEB(double, ptr, errSvs);
    WRITEB(double, ptr, vCPM);
    WRITEB(double, ptr, uSvm);
    WRITEB(double, ptr, errSvm);
    WRITEB(double, ptr, vPir);
    WRITEB(double, ptr, vTemp);
    WRITEB(double, ptr, vHumidity);
    
    //mqtt publish
    mqClient.publish(mqBuf, mqData, len);
  }
}

//repeat processing
void loop() {
  if (!isReady)
    return;
    
  if (cntReaded) {
    mqClient.loop();  //loop mqtt
    cntReaded = 0;
    radCnt = signCount;
    nsCnt = noiseCount;
    noiseCount = 0;

    //calculate timing
    calculateTiming();
    //calculate doserate
    calculateDoserate();
    //calculate other sensor values
    calculateSensors();

    //time needed 200 us
    
    //send result
    sendResult();   

    //time needed 3.5 ms
  }
}

/**
 * interrupt on timer 2 every 8 ms
 */
ISR (TIMER2_COMPA_vect)
{
    if (isReady) {
      tick++;
      if (tick >= 25) {
        //every 25*8 ms = 200 ms
        tick = 0;
        cntReaded = 1;
        signCount = TCNT1;
        TCNT1 = 0;
      }
    }
}

/**
 * interrupt if noise present
 */
ISR (PCINT0_vect)
{
  if (isReady)
    noiseCount++;
}
