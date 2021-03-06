/* 
 * *******************************************************
 * *** Inclueds needed for FrSkySPortTelemetry         ***
 * *******************************************************
 */
#include "Time.h"
#include "FrSkySportSensor.h"
#include "FrSkySportSensorFas.h"
#include "FrSkySportSensorFuel.h"
#include "FrSkySportSensorFlvss.h"
#include "FrSkySportSensorGps.h"
#include "FrSkySportSensorRpm.h"
#include "FrSkySportSensorAcc.h"
#include "FrSkySportSensorSp2uart.h"
#include "FrSkySportSensorVario.h"
#include "FrSkySportSingleWireSerial.h"
#include "FrSkySportTelemetry.h"

/* 
 * *******************************************************
 * *** Define FrSkySPortTelemetry Sensors:             ***
 * *******************************************************
 */
FrSkySportSensorFas fas;                               // Create FAS sensor with default ID
FrSkySportSensorFuel fuel;                             // Create FUEL sensor with default ID
#ifdef USE_SINGLE_CELL_MONITOR
  FrSkySportSensorFlvss flvss1;                          // Create FLVSS sensor with default ID
  #if (MAXCELLS > 6) 
    FrSkySportSensorFlvss flvss2(FrSkySportSensor::ID15);  // Create FLVSS sensor with given ID
  #endif
#endif
FrSkySportSensorGps gps;                               // Create GPS sensor with default ID
FrSkySportSensorRpm rpm;                               // Create RPM sensor with default ID
FrSkySportSensorAcc acc;                               // Create ACC sensor with default ID
FrSkySportSensorSp2uart sp2uart;                       // Create SP2UART Type B sensor with default ID
FrSkySportSensorVario vario;                           // Create Variometer sensor with default ID
FrSkySportTelemetry telemetry;                         // Create Variometer telemetry object


/* 
 * *******************************************************
 * *** Define some Variables:                          ***
 * *******************************************************
 */
float FASCurrent = 0.0;
float FASVoltage = 0.0;

unsigned long FAS_timer = 0;
int8_t transmit = 0;

// Scale factor for roll/pitch:
// We need to scale down 360 deg to fit when max value is 256, and 256 equals 362 deg
float scalefactor = 360.0/((362.0/360.0)*256.0);

uint32_t handle_A2_A3_value(uint32_t value)
{
  return (value *330-165)/0xFF;
}

unsigned long GPS_debug_time = 500;
/* 
 * *******************************************************
 * *** Initialize FrSkySPortTelemetry                  ***
 * *******************************************************
 */
void FrSkySPort_Init()
{
  // Configure the telemetry serial port and sensors (remember to use & to specify a pointer to sensor)
  #ifdef USE_SINGLE_CELL_MONITOR
    #if (MAXCELLS <= 6)
      telemetry.begin(FrSkySportSingleWireSerial::SERIAL_1, &fas, &flvss1, &gps, &sp2uart, &rpm, &vario, &fuel, &acc);
    #else
      telemetry.begin(FrSkySportSingleWireSerial::SERIAL_1, &fas, &flvss1, &flvss2, &gps, &sp2uart, &rpm, &vario, &fuel, &acc);
    #endif
  #else
      telemetry.begin(FrSkySportSingleWireSerial::SERIAL_1, &fas, &gps, &sp2uart, &rpm, &vario, &fuel, &acc);
  #endif

}

/* 
 * *******************************************************
 * *** Process and Transmit FrSkySPortTelemetry Data   ***
 * *******************************************************
 */
void FrSkySPort_Process()
{
  /* 
   * *****************************************************
   * *** Send the telemetry data                       ***
   * *****************************************************
   * Note that the data will only be sent for sensors
   * that are being polled at given moment
   */
  telemetry.send();
  /* 
   * *****************************************************
   * *** Set current/voltage sensor (FAS) data         ***
   * *****************************************************
   */
  FrSkySportTelemetry_FAS();

  /* 
   * *****************************************************
   * *** Set current/voltage sensor (FAS) data         ***
   * *****************************************************
   */
  FrSkySportTelemetry_FLVSS();
  
  /* 
   * *****************************************************
   * *** Set GPS data                                  ***
   * *****************************************************
   */
  FrSkySportTelemetry_GPS();
  
  /* 
   * *****************************************************
   * *** Set RPM/Temperature sensor data               ***
   * *****************************************************
   */
  FrSkySportTelemetry_RPM();
  
  /* 
   * *****************************************************
   * *** Set SP2UART sensor data ( A3 & A4 )           ***
   * *****************************************************
   */
  FrSkySportTelemetry_A3A4();
  
  /* 
   * *****************************************************
   * *** Set variometer data                           ***
   * *****************************************************
   */
  FrSkySportTelemetry_VARIO();
  
  /* 
   * *****************************************************
   * *** Set Accelerometer data                        ***
   * *****************************************************
   */
  FrSkySportTelemetry_ACC();
  
  /* 
   * *****************************************************
   * *** Set Fuel sensor data (Flight Mode)            ***
   * *****************************************************
   */
  FrSkySportTelemetry_FUEL();
  /* 
   * *****************************************************
   * *** Send the telemetry data                       ***
   * *****************************************************
   * Note that the data will only be sent for sensors
   * that are being polled at given moment
   */
//  telemetry.send();
}



/* 
 * *******************************************************
 * *** Set current/voltage sensor (FAS) data           ***
 * *******************************************************
 * set Voltage source to FAS in menu to use this data for battery voltage,
 * set Current source to FAS in menu to use this data for current readings
 */
void FrSkySportTelemetry_FAS() {
  FASVoltage = readAndResetAverageVoltage();
  if ( FASVoltage > 0 ) {                                 // only progress if we have a Battery Voltage
    //FASCurrent = readAndResetAverageCurrent();            // read Average Current
    #ifdef DEBUG_FrSkySportTelemetry_FAS
      debugSerial.print(millis());
      debugSerial.println("FrSkySportTelemetry_FAS:");
      debugSerial.print("\tVFAS (0x0210): ");
      debugSerial.print(FASVoltage / 10.0 );
      debugSerial.print("\tCurr (0x0200): ");
      //debugSerial.print(FASCurrent);
      debugSerial.print(ap_current_battery / 10.0);
      debugSerial.println();
    #endif
    fas.setData(ap_current_battery / 10.0,    // Current consumption in amps
                FASVoltage / 10.0);           // Battery voltage in volts
  }
}

/* 
 * *******************************************************
 * *** Set current/voltage sensor (FAS) data           ***
 * *******************************************************
 * set LiPo voltage sensor (FLVSS) data (we use two sensors to simulate 8S battery)
 * set Voltage source to Cells in menu to use this data for battery voltage
 */
void FrSkySportTelemetry_FLVSS() {
  #ifdef USE_SINGLE_CELL_MONITOR
    #ifdef DEBUG_FrSkySportTelemetry_FLVSS
        debugSerial.print(millis());
        debugSerial.print("\tmaxCells: ");
        debugSerial.println(MAXCELLS);
      for (int i=0; i < MAXCELLS; i++) {
        debugSerial.print(millis());
        debugSerial.print("\tZelle[");
        debugSerial.print(i);
        debugSerial.print("]: ");
        debugSerial.print((lscm.getCellVoltageAsUint32_T(i) / 1000.0);
        debugSerial.println("Volt");
        
      }
    #endif
    
    switch(lscm.getCellsInUse()) {
      case 1:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0);
        break;
      case 2:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0);
        break;
      case 3:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0, lscm.getCellVoltageAsUint32_T(2) / 1000.0);
        break;
      case 4:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0, lscm.getCellVoltageAsUint32_T(2) / 1000.0, lscm.getCellVoltageAsUint32_T(3) / 1000.0);
        break;
      case 5:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0, lscm.getCellVoltageAsUint32_T(2) / 1000.0, lscm.getCellVoltageAsUint32_T(3) / 1000.0, lscm.getCellVoltageAsUint32_T(4) / 1000.0);
        break;
      case 6:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0, lscm.getCellVoltageAsUint32_T(2) / 1000.0, lscm.getCellVoltageAsUint32_T(3) / 1000.0, lscm.getCellVoltageAsUint32_T(4) / 1000.0, lscm.getCellVoltageAsUint32_T(5) / 1000.0);
        break;
      #if (MAXCELLS > 6)
      case 7:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0, lscm.getCellVoltageAsUint32_T(2) / 1000.0, lscm.getCellVoltageAsUint32_T(3) / 1000.0, lscm.getCellVoltageAsUint32_T(4) / 1000.0, lscm.getCellVoltageAsUint32_T(5) / 1000.0);
        flvss2.setData(lscm.getCellVoltageAsUint32_T(6) / 1000.0);
        break;
      case 8:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0, lscm.getCellVoltageAsUint32_T(2) / 1000.0, lscm.getCellVoltageAsUint32_T(3) / 1000.0, lscm.getCellVoltageAsUint32_T(4) / 1000.0, lscm.getCellVoltageAsUint32_T(5) / 1000.0);
        flvss2.setData(lscm.getCellVoltageAsUint32_T(6) / 1000.0, lscm.getCellVoltageAsUint32_T(7) / 1000.0);
        break;
      case 9:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0, lscm.getCellVoltageAsUint32_T(2) / 1000.0, lscm.getCellVoltageAsUint32_T(3) / 1000.0, lscm.getCellVoltageAsUint32_T(4) / 1000.0, lscm.getCellVoltageAsUint32_T(5) / 1000.0);
        flvss2.setData(lscm.getCellVoltageAsUint32_T(6) / 1000.0, lscm.getCellVoltageAsUint32_T(7) / 1000.0, lscm.getCellVoltageAsUint32_T(8) / 1000.0);
        break;
      case 10:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0, lscm.getCellVoltageAsUint32_T(2) / 1000.0, lscm.getCellVoltageAsUint32_T(3) / 1000.0, lscm.getCellVoltageAsUint32_T(4) / 1000.0, lscm.getCellVoltageAsUint32_T(5) / 1000.0);
        flvss2.setData(lscm.getCellVoltageAsUint32_T(6) / 1000.0, lscm.getCellVoltageAsUint32_T(7) / 1000.0, lscm.getCellVoltageAsUint32_T(8) / 1000.0, lscm.getCellVoltageAsUint32_T(9) / 1000.0);
        break;
      case 11:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0, lscm.getCellVoltageAsUint32_T(2) / 1000.0, lscm.getCellVoltageAsUint32_T(3) / 1000.0, lscm.getCellVoltageAsUint32_T(4) / 1000.0, lscm.getCellVoltageAsUint32_T(5) / 1000.0);
        flvss2.setData(lscm.getCellVoltageAsUint32_T(6) / 1000.0, lscm.getCellVoltageAsUint32_T(7) / 1000.0, lscm.getCellVoltageAsUint32_T(8) / 1000.0, lscm.getCellVoltageAsUint32_T(9) / 1000.0, lscm.getCellVoltageAsUint32_T(10) / 1000.0);
        break;
      case 12:
        flvss1.setData(lscm.getCellVoltageAsUint32_T(0) / 1000.0, lscm.getCellVoltageAsUint32_T(1) / 1000.0, lscm.getCellVoltageAsUint32_T(2) / 1000.0, lscm.getCellVoltageAsUint32_T(3) / 1000.0, lscm.getCellVoltageAsUint32_T(4) / 1000.0, lscm.getCellVoltageAsUint32_T(5) / 1000.0);
        flvss2.setData(lscm.getCellVoltageAsUint32_T(6) / 1000.0, lscm.getCellVoltageAsUint32_T(7) / 1000.0, lscm.getCellVoltageAsUint32_T(8) / 1000.0, lscm.getCellVoltageAsUint32_T(9) / 1000.0, lscm.getCellVoltageAsUint32_T(10) / 1000.0, lscm.getCellVoltageAsUint32_T(11) / 1000.0);
        break;
      #endif
    
  }
  #endif
}

/* 
 * *******************************************************
 * *** Set GPS data                                    ***
 * *******************************************************
 */
void FrSkySportTelemetry_GPS() {
  if(ap_fixtype==3) 
  {
    /*
    if(ap_longitude < 0)
      longitude=((abs(ap_longitude)/100)*6);//  | 0xC0000000;
    else
      longitude=((abs(ap_longitude)/100)*6);//  | 0x80000000;
    
      if(ap_latitude < 0 )
      latitude=((abs(ap_latitude)/100)*6);// | 0x40000000;
    else
      latitude=((abs(ap_latitude)/100)*6);
    */      
    gps.setData(ap_latitude / 1E7, ap_longitude / 1E7,    // Latitude and longitude in degrees decimal (positive for N/E, negative for S/W)
              ap_gps_altitude / 10.0,         // Altitude (AMSL, NOT WGS84), in meters * 1000 (positive for up). Note that virtually all GPS modules provide the AMSL altitude in addition to the WGS84 altitude.
              ap_gps_speed * 10.0,// / 100.0,            // GPS ground speed (m/s * 100). If unknown, set to: UINT16_MAX
              ap_heading ,                     // Heading, in degrees * 100, 0.0..359.99 degrees. If unknown, set to: UINT16_MAX
              ap_gps_hdop);                   // GPS HDOP horizontal dilution of position in cm (m*100)
//              ap_cog,                         // Course over ground (NOT heading, but direction of movement) in degrees * 100, 0.0..359.99 degrees. If unknown, set to: UINT16_MAX
  
    #ifdef DEBUG_FrSkySportTelemetry_GPS
      if (millis() > GPS_debug_time) {
        debugSerial.print(millis());
        debugSerial.print("\tAPM Latitude:\t");
        debugSerial.print(ap_latitude);
        debugSerial.print("\tAPM Longitude:\t\t");
        debugSerial.println(ap_longitude);
        debugSerial.print(millis());
        debugSerial.print("\tFrSky Latitude:\t");
        debugSerial.print(ap_latitude/1E7);
        debugSerial.print("\tFrSky Longitude:\t");
        debugSerial.println(ap_longitude/1E7);
        debugSerial.print(millis());
        debugSerial.print("\tGPSAlt: ");
        debugSerial.print(ap_gps_altitude / 10.0);
        debugSerial.print("cm");
        debugSerial.print("\tGPSSpeed: ");
        debugSerial.print((ap_gps_speed / 100.0 ));
        debugSerial.print("m/s");
        
        debugSerial.print("\tCog: ");
        debugSerial.print(ap_cog);
        debugSerial.print("°");
        
        debugSerial.print("\tHeading: ");
        debugSerial.print(ap_heading);
        debugSerial.print("°");
        debugSerial.print("\tHDOP (A2): ");
        debugSerial.print(ap_gps_hdop);
        /*
        debugSerial.print("\tDATE: ");
        debugSerial.print(year(ap_gps_time_unix_utc));
        debugSerial.print("-");
        debugSerial.print(month(ap_gps_time_unix_utc));
        debugSerial.print("-");
        debugSerial.print(day(ap_gps_time_unix_utc));
        debugSerial.print("\tTIME: ");
        debugSerial.print(hour(ap_gps_time_unix_utc));
        debugSerial.print(":");
        debugSerial.print(minute(ap_gps_time_unix_utc));
        debugSerial.print(":");
        debugSerial.print(second(ap_gps_time_unix_utc));
        */
        debugSerial.println();
        debugSerial.println();
        GPS_debug_time = millis() + 500;
      }
    #endif
  }
}

/* 
 * *******************************************************
 * *** Set RPM/Temperature sensor data                 ***
 * *******************************************************
 * 
 * 16 bit value: 
 * * bit 1     : armed
 * * bit 2 -  5: severity +1 (0 means no message)
 * * bit 6 - 15: number representing a specific text
 */
void FrSkySportTelemetry_RPM() {
  uint32_t ap_status_value = ap_base_mode&0x01;
  // If we have a message-text to report (we send it multiple times to make sure it arrives even on telemetry glitches)
  if(ap_status_send_count > 0 && ap_status_text_id > 0)
  {
    // Add bits 2-15
    ap_status_value |= (((ap_status_severity+1)&0x0F)<<1) |((ap_status_text_id&0x3FF)<<5);
    ap_status_send_count--;
    if(ap_status_send_count == 0)
    {
       // Reset severity and text-message after we have sent the message
       ap_status_severity = 0; 
       ap_status_text_id = 0;
    }          
  }
  #ifdef DEBUG_FrSkySportTelemetry_RPM
    debugSerial.print(millis());
    debugSerial.print("\tRPM (Throttle/battery_remain): ");
    debugSerial.print(ap_throttle * 200+ap_battery_remaining*2);
    debugSerial.print("\tT1 ((ap_sat_visible * 10) + ap_fixtype): ");
    debugSerial.print(gps_status);
    debugSerial.print("\tT2 (Armed Status + Severity + Statustext): ");
    debugSerial.print(ap_status_value);
    debugSerial.println();
  #endif
  rpm.setData(ap_throttle * 200+ap_battery_remaining*2,    // * 2 if number of blades on Taranis is set to 2 + First 4 digits reserved for battery remaining in %
              gps_status,         // (ap_sat_visible * 10) + ap_fixtype eg. 83 = 8 sattelites visible, 3D lock 
              ap_status_value);   // Armed Status + Severity + Statustext


}

/* 
 * *****************************************************
 * *** Set SP2UART sensor data ( A3 & A4 )           ***
 * *****************************************************
 */
void FrSkySportTelemetry_A3A4() {
  #ifdef DEBUG_FrSkySportTelemetry_A3A4
    debugSerial.print(millis());
    debugSerial.print("\tRoll Angle (A3): ");
    debugSerial.print(handle_A2_A3_value((ap_roll_angle+180)/scalefactor));
    debugSerial.print("\tPitch Angle (A4): ");
    debugSerial.print(handle_A2_A3_value((ap_pitch_angle+180)/scalefactor));
    debugSerial.println();
  #endif
  sp2uart.setData(handle_A2_A3_value((ap_roll_angle+180)/scalefactor),     // Roll Angle
                  handle_A2_A3_value((ap_pitch_angle+180)/scalefactor));   // Pitch Angle

}

/* 
 * *******************************************************
 * *** Set variometer data                             ***
 * *******************************************************
 * set Variometer source to VSpd in menu to use the vertical speed data from this sensor for variometer.
 */
void FrSkySportTelemetry_VARIO() {
  #ifdef DEBUG_FrSkySportTelemetry_VARIO
    debugSerial.print(millis());
    debugSerial.print("\tCurrent altitude: ");
    debugSerial.print(ap_bar_altitude / 100.0);
    debugSerial.print("m\tCurrent climb rate in meters/second: ");
    debugSerial.print(ap_climb_rate);
    debugSerial.print("m/s");
    debugSerial.println();
  #endif
  vario.setData(ap_bar_altitude,  // Current altitude (MSL), in meters
                ap_climb_rate);   // Current climb rate in meters/second
}

/* 
 * *******************************************************
 * *** Set Accelerometer data                          ***
 * *******************************************************
 */
void FrSkySportTelemetry_ACC() {
  #ifdef DEBUG_FrSkySportTelemetry_ACC
    debugSerial.print(millis());
    debugSerial.print("\tX acceleration (raw): ");
    debugSerial.print(fetchAccX());
    debugSerial.print("\tY acceleration (raw): ");
    debugSerial.print(fetchAccY());
    debugSerial.print("\tZ acceleration (raw): ");
    debugSerial.print(fetchAccZ());
    debugSerial.println();
  #endif
  acc.setData(fetchAccX(),        // X acceleration (raw)
              fetchAccY(),        // Y acceleration (raw)
              fetchAccZ());       // Z acceleration (raw)
}

/* 
 * *****************************************************
 * *** Set Fuel sensor data                          ***
 * *****************************************************
 * Used for Flight Mode
 */
void FrSkySportTelemetry_FUEL() {
  #ifdef DEBUG_FrSkySportTelemetry_FLIGHTMODE
    debugSerial.print(millis());
    debugSerial.print("\tFlightmode: ");
    debugSerial.print(ap_custom_mode);
    debugSerial.println();
  #endif
  if(ap_custom_mode >= 0) {
    fuel.setData(ap_custom_mode);
  }
}

