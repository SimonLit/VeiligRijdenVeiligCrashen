void checkAndAcktOnSerialMessage(void)
{
  bool receivedMessage = getIncommingString(&stringFromSerial);
  if (receivedMessage)
  {
    if (stringFromSerial == CONTROLLER_VALUE_PROTOCOL_REQUEST_SEND)
    {
      String controllerValues = "";
      int controllerReceiveSate = getControllerValues(&controllerValues);
      if (controllerValues.indexOf(CONTROLLER_VALUES) > -1)
      {
        Serial.println(controllerValues);
      }
      else
      {
        makeProtocolString("ControllerDisconnected");
        Serial.println(protocolStringToSend);
      }
    }
  }
}

