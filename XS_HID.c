#include "XS_HID.h"
/* BIT MANIPULATION MACRO */
#define bit_set(p,m) ((p) |= (m))
#define bit_clear(p,m) ((p) &= ~(m))
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define bit_check(value, bit) (((value) >> (bit)) & 0x01)

USB_JoystickReport_Input_t ReportData;
USB_JoystickReport_XInput_t XInput_ReportData;

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void) {
  // We need to disable watchdog if enabled by bootloader/fuses.
  MCUSR &= ~(1 << WDRF);
  wdt_disable();
  // We need to disable clock division before initializing the USB hardware.
  clock_prescale_set(clock_div_1);
  // We can then initialize our hardware and peripherals, including the USB stack.
  // The USB stack should be initialized last.
  USB_Init();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void) {
  // We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void) {
  // We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void) {

  if (!xinput) Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
  else Endpoint_ConfigureEndpoint((ENDPOINT_DIR_IN | 3), EP_TYPE_INTERRUPT, 32, 1);
  
  Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, xinput? JOYSTICK_EPSIZE_XINPUT: JOYSTICK_EPSIZE, 1);

}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void) {
  //No controlRequest received from the switch, so only handled in xinput mode 
 if (xinput){
  /* Handle HID Class specific requests */
  switch (USB_ControlRequest.bRequest) {
  case HID_REQ_GetReport:
    if (USB_ControlRequest.bmRequestType
        == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
      Endpoint_ClearSETUP();

      /* Write the report data to the control endpoint */
      Endpoint_Write_Control_Stream_LE(&XInput_ReportData, 20);
      Endpoint_ClearOUT();
    }
    break;
  }
 }
}

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void) {
    /* Device must be connected and configured for the task to run */
if (USB_DeviceState != DEVICE_STATE_Configured)
    return;

void* Address = &XInput_ReportData;
uint16_t    Size    = 20;

//no OUT endpoint for xinput in this firmware
if (!xinput){
  Address = &ReportData;
  Size = sizeof(ReportData);
  
   // We'll start with the OUT endpoint.
  Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
  // We'll check to see if we received something on the OUT endpoint.
  if (Endpoint_IsOUTReceived())
  {
    // If we did, and the packet has data, we'll react to it.
    if (Endpoint_IsReadWriteAllowed())
    {
      // We'll create a place to store our data received from the host.
      USB_JoystickReport_Output_t JoystickOutputData;
      // We'll then take in that data, setting it up in our storage.
      Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL);
      // At this point, we can react to this data.
      // However, since we're not doing anything with this data, we abandon it.
    }
    // Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
    Endpoint_ClearOUT();
  }
}
/* Select the Joystick Report Endpoint */
  Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);

  /* Check to see if the host is ready for another packet */
  if (Endpoint_IsINReady()) {
    /* Write Joystick Report Data */
    Endpoint_Write_Stream_LE(Address, Size, NULL);

    /* Finalize the stream transfer to send the last packet */
    Endpoint_ClearIN();
    /* Clear the report data afterwards */
    memset(Address, 0, Size);
    if (xinput) (*((USB_JoystickReport_XInput_t *) Address)).rsize = 20;
  }
}

void send_pad_state(void) {
  HID_Task();
  USB_USBTask();
}

void generate_report_xinput(){
  // HAT
    buttonStatus[BUTTONUP]    ? bit_set(XInput_ReportData.digital_buttons_1, XBOX_DPAD_UP)    : bit_clear(XInput_ReportData.digital_buttons_1, XBOX_DPAD_UP);
    buttonStatus[BUTTONDOWN]  ? bit_set(XInput_ReportData.digital_buttons_1, XBOX_DPAD_DOWN)  : bit_clear(XInput_ReportData.digital_buttons_1, XBOX_DPAD_DOWN);
    buttonStatus[BUTTONLEFT]  ? bit_set(XInput_ReportData.digital_buttons_1, XBOX_DPAD_LEFT)  : bit_clear(XInput_ReportData.digital_buttons_1, XBOX_DPAD_LEFT);
    buttonStatus[BUTTONRIGHT] ? bit_set(XInput_ReportData.digital_buttons_1, XBOX_DPAD_RIGHT) : bit_clear(XInput_ReportData.digital_buttons_1, XBOX_DPAD_RIGHT);
    
  // analogs    
    XInput_ReportData.l_x = buttonStatus[AXISLX] * 257 + -32768;
    XInput_ReportData.l_y = buttonStatus[AXISLY] * -257 + 32767;
    XInput_ReportData.r_x = buttonStatus[AXISRX] * 257 + -32768;
    XInput_ReportData.r_y = buttonStatus[AXISRY] * -257 + 32767;
    
  // buttons
  
    buttonStatus[BUTTONSTART]  ? bit_set(XInput_ReportData.digital_buttons_1, XBOX_START)       : bit_clear(XInput_ReportData.digital_buttons_1, XBOX_START);
    buttonStatus[BUTTONSELECT] ? bit_set(XInput_ReportData.digital_buttons_1, XBOX_BACK)        : bit_clear(XInput_ReportData.digital_buttons_1, XBOX_BACK);
    buttonStatus[BUTTONL3]     ? bit_set(XInput_ReportData.digital_buttons_1, XBOX_LEFT_STICK)  : bit_clear(XInput_ReportData.digital_buttons_1, XBOX_LEFT_STICK);
    buttonStatus[BUTTONR3]     ? bit_set(XInput_ReportData.digital_buttons_1, XBOX_RIGHT_STICK) : bit_clear(XInput_ReportData.digital_buttons_1, XBOX_RIGHT_STICK);

    buttonStatus[BUTTONA] ? bit_set(XInput_ReportData.digital_buttons_2, XBOX_A)    : bit_clear(XInput_ReportData.digital_buttons_2, XBOX_A);
    buttonStatus[BUTTONB] ? bit_set(XInput_ReportData.digital_buttons_2, XBOX_B)  : bit_clear(XInput_ReportData.digital_buttons_2, XBOX_B);
    buttonStatus[BUTTONX] ? bit_set(XInput_ReportData.digital_buttons_2, XBOX_X)  : bit_clear(XInput_ReportData.digital_buttons_2, XBOX_X);
    buttonStatus[BUTTONY] ? bit_set(XInput_ReportData.digital_buttons_2, XBOX_Y) : bit_clear(XInput_ReportData.digital_buttons_2, XBOX_Y);

    buttonStatus[BUTTONLB] ? bit_set(XInput_ReportData.digital_buttons_2, XBOX_LB)    : bit_clear(XInput_ReportData.digital_buttons_2, XBOX_LB);
    buttonStatus[BUTTONRB] ? bit_set(XInput_ReportData.digital_buttons_2, XBOX_RB)    : bit_clear(XInput_ReportData.digital_buttons_2, XBOX_RB);

    buttonStatus[BUTTONHOME] ? bit_set(XInput_ReportData.digital_buttons_2, XBOX_HOME) : bit_clear(XInput_ReportData.digital_buttons_2, XBOX_HOME);

    XInput_ReportData.lt = buttonStatus[BUTTONLT] * 0xFF;
    XInput_ReportData.rt = buttonStatus[BUTTONRT] * 0xFF;
    
}

void generate_report_switch(){
// HAT
    if ((buttonStatus[BUTTONUP]) && (buttonStatus[BUTTONRIGHT])){ReportData.HAT = DPAD_UPRIGHT_MASK_ON;}
    else if ((buttonStatus[BUTTONDOWN]) && (buttonStatus[BUTTONRIGHT])) {ReportData.HAT = DPAD_DOWNRIGHT_MASK_ON;} 
    else if ((buttonStatus[BUTTONDOWN]) && (buttonStatus[BUTTONLEFT])) {ReportData.HAT = DPAD_DOWNLEFT_MASK_ON;}
    else if ((buttonStatus[BUTTONUP]) && (buttonStatus[BUTTONLEFT])){ReportData.HAT = DPAD_UPLEFT_MASK_ON;}
    else if (buttonStatus[BUTTONUP]) {ReportData.HAT = DPAD_UP_MASK_ON;}
    else if (buttonStatus[BUTTONDOWN]) {ReportData.HAT = DPAD_DOWN_MASK_ON;}
    else if (buttonStatus[BUTTONLEFT]) {ReportData.HAT = DPAD_LEFT_MASK_ON;}
    else if (buttonStatus[BUTTONRIGHT]) {ReportData.HAT = DPAD_RIGHT_MASK_ON;}
    else{ReportData.HAT = DPAD_NOTHING_MASK_ON;}

// analogs
  ReportData.LX = buttonStatus[AXISLX];
  ReportData.LY = buttonStatus[AXISLY];
  ReportData.RX = buttonStatus[AXISRX];
  ReportData.RY = buttonStatus[AXISRY];

// Buttons
  if (buttonStatus[BUTTONA]) {ReportData.Button |= A_MASK_ON;}
  if (buttonStatus[BUTTONB]) {ReportData.Button |= B_MASK_ON;}
  if (buttonStatus[BUTTONX]) {ReportData.Button |= X_MASK_ON;}
  if (buttonStatus[BUTTONY]) {ReportData.Button |= Y_MASK_ON;}
  if (buttonStatus[BUTTONLB]) {ReportData.Button |= LB_MASK_ON;}
  if (buttonStatus[BUTTONRB]) {ReportData.Button |= RB_MASK_ON;}
  if (buttonStatus[BUTTONLT]) {ReportData.Button |= ZL_MASK_ON;}
  if (buttonStatus[BUTTONRT]) {ReportData.Button |= ZR_MASK_ON;}
  if (buttonStatus[BUTTONSTART]){ReportData.Button |= START_MASK_ON;}
  if (buttonStatus[BUTTONSELECT]){ReportData.Button |= SELECT_MASK_ON;}
  if (buttonStatus[BUTTONHOME]){ReportData.Button |= HOME_MASK_ON;}

}

void generate_report(){
  if (xinput) generate_report_xinput();
  else generate_report_switch();
}
