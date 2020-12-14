// 本ソースコードのオリジナルは下記の Ted Meyers 氏によるものです。
// More docs and example code for the VL53L0X LIDAR sensor
// the original code by Ted Meyers
// posted here: https://groups.google.com/d/msg/diyrovers/lc7NUZYuJOg/ICPrYNJGBgAJ

#include <Wire.h>

#define VL53L0X_REG_IDENTIFICATION_MODEL_ID         0xc0
#define VL53L0X_REG_IDENTIFICATION_REVISION_ID      0xc2
#define VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD   0x50
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD 0x70
#define VL53L0X_REG_SYSRANGE_START                  0x00
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS         0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS             0x14
#define VL53L0X_address 0x29

byte VL53L0X_gbuf[16];

/*
void setup() {
    // put your setup code here, to run once:
    Wire.begin();          // join i2c bus (VL53L0X_address optional for master)
    Serial.begin(9600);    // start serial for output
    Serial.println("VLX53LOX test started.");
}

void loop() {
    Serial.println("----- START TEST ----");
    VL53L0X_get();
    Serial.println("----- END TEST ----");
    Serial.println("");
    delay(4000);
}
*/

uint16_t VL53L0X_get(){
/*
    byte val1 = read_byte_data_at(VL53L0X_REG_IDENTIFICATION_REVISION_ID);
    Serial.print("Revision ID: "); Serial.println(val1);

    val1 = read_byte_data_at(VL53L0X_REG_IDENTIFICATION_MODEL_ID);
    Serial.print("Device ID: "); Serial.println(val1);

    val1 = read_byte_data_at(VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD);
    Serial.print("PRE_RANGE_CONFIG_VCSEL_PERIOD="); Serial.println(val1); 
    Serial.print(" decode: "); Serial.println(VL53L0X_decode_vcsel_period(val1));

    val1 = read_byte_data_at(VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD);
    Serial.print("FINAL_RANGE_CONFIG_VCSEL_PERIOD="); Serial.println(val1);
    Serial.print(" decode: "); Serial.println(VL53L0X_decode_vcsel_period(val1));
*/
    write_byte_data_at(VL53L0X_REG_SYSRANGE_START, 0x01);

    byte val = 0;
    int cnt = 0;
    while (cnt < 100) { // 1 second waiting time max
        delay(10);
        val = read_byte_data_at(VL53L0X_REG_RESULT_RANGE_STATUS);
        if (val & 0x01) break;
        cnt++;
    }
    if (val & 0x01){
        // Serial.println("ready");
    }else{
        Serial.println("VL53L0X is not ready");
        return 0;
    }

    read_block_data_at(0x14, 12);
    uint16_t acnt = makeuint16(VL53L0X_gbuf[7], VL53L0X_gbuf[6]);
    uint16_t scnt = makeuint16(VL53L0X_gbuf[9], VL53L0X_gbuf[8]);
    uint16_t dist = makeuint16(VL53L0X_gbuf[11], VL53L0X_gbuf[10]);
    byte DeviceRangeStatusInternal = ((VL53L0X_gbuf[0] & 0x78) >> 3);
/*
    Serial.print("ambient count: "); Serial.println(acnt);
    Serial.print("signal count: ");  Serial.println(scnt);
    Serial.print("distance ");       Serial.println(dist);
    Serial.print("status: ");        Serial.println(DeviceRangeStatusInternal);
*/
    return dist;
}

uint16_t bswap(byte b[]) {
    // Big Endian unsigned short to little endian unsigned short
    uint16_t val = ((b[0] << 8) & b[1]);
    return val;
}

uint16_t makeuint16(int lsb, int msb) {
        return ((msb & 0xFF) << 8) | (lsb & 0xFF);
}

void write_byte_data(byte data) {
    Wire.beginTransmission(VL53L0X_address);
    Wire.write(data);
    Wire.endTransmission();
}

void write_byte_data_at(byte reg, byte data) {
    // write data word at address and register
    Wire.beginTransmission(VL53L0X_address);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

void write_word_data_at(byte reg, uint16_t data) {
    // write data word at address and register
    byte b0 = (data &0xFF);
    byte b1 = ((data >> 8) && 0xFF);
        
    Wire.beginTransmission(VL53L0X_address);
    Wire.write(reg);
    Wire.write(b0);
    Wire.write(b1);
    Wire.endTransmission();
}

byte read_byte_data() {
    Wire.requestFrom(VL53L0X_address, 1);
    delay(10);
    // while (Wire.available() < 1) delay(1);
    if(Wire.available() >= 1){
        byte b = Wire.read();
        return b;
    }else Serial.println("ERROR: read_byte_data");
    return 0;
}

byte read_byte_data_at(byte reg) {
    //write_byte_data((byte)0x00);
    write_byte_data(reg);
    Wire.requestFrom(VL53L0X_address, 1);
    delay(10);
    // while (Wire.available() < 1) delay(1);
    if(Wire.available() >= 1){
        byte b = Wire.read();
        return b;
    }else Serial.println("ERROR: read_byte_data_at");
    return 0;
}

uint16_t read_word_data_at(byte reg) {
    write_byte_data(reg);
    Wire.requestFrom(VL53L0X_address, 2);
    delay(10);
    // while (Wire.available() < 2) delay(1);
    if(Wire.available() >= 2){
        VL53L0X_gbuf[0] = Wire.read();
        VL53L0X_gbuf[1] = Wire.read();
        return bswap(VL53L0X_gbuf); 
    }else Serial.println("ERROR: read_word_data_at");
    return 0;
}

void read_block_data_at(byte reg, int sz) {
    int i = 0;
    write_byte_data(reg);
    Wire.requestFrom(VL53L0X_address, sz);
    delay(10);
    if(sz > 15) sz = 15;
    for (i=0; i<sz; i++) {
        // while (Wire.available() < 1) delay(1);
        if(Wire.available() >= 1){
            VL53L0X_gbuf[i] = Wire.read();
        }else Serial.println("ERROR: read_block_data_at");
    }
}


uint16_t VL53L0X_decode_vcsel_period(short vcsel_period_reg) {
    // Converts the encoded VCSEL period register value into the real
    // period in PLL clocks
    uint16_t vcsel_period_pclks = (vcsel_period_reg + 1) << 1;
    return vcsel_period_pclks;
}
