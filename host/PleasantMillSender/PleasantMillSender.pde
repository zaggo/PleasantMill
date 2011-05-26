/*
 * Copyright 2011 by Eberhard Rensch <http://pleasantsoftware.com/developer/3d>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */
  
import processing.serial.*; // serial library
import controlP5.*; // controlP5 library
import javax.swing.*;

Serial serial;
ControlP5 controlP5;
Button buttonChoose, buttonAgain, buttonAbort;
ListBox commListbox;
Textlabel txtlblWhichcom, version, statusline; 

boolean init_com=false;
boolean sendData=false;
int currentLineIndex=0;
String linesToSend[];
boolean waitForResponse = true;
StringBuffer inBuffer=new StringBuffer();

color green_ = color(30, 120, 30);
color red_ = color(120, 30, 30);
color bkgcolor = color(80, 80, 80);

void setup() {
    size(400,300);
    smooth();
    frameRate(30);
    
    controlP5 = new ControlP5(this); // initialize the GUI controls
    
    // make a listbox and populate it with the available comm ports
    commListbox = controlP5.addListBox("portComList",5,49,110,180); //addListBox(name,x,y,width,height)
    commListbox.captionLabel().set("PORT COM");
    commListbox.setColorBackground(red_);
    for(int i=0;i<Serial.list().length;i++)
       commListbox.addItem(shortifyPortName(Serial.list()[i], 20),i); // addItem(name,value)
  
    // text label for which comm port selected
    txtlblWhichcom = controlP5.addTextlabel("txtlblWhichcom","No Port Selected",7,28); // textlabel(name,text,x,y)
    version = controlP5.addTextlabel("version","v0.1",5,285); // textlabel(name,text,x,y)
    statusline = controlP5.addTextlabel("statusline","",150,70); // textlabel(name,text,x,y)

    buttonChoose = controlP5.addButton("SEND", 1, 150, 39, 30, 19);
    buttonAgain = controlP5.addButton("AGAIN", 1, 190, 39, 30, 19);
    buttonAbort = controlP5.addButton("ABORT", 1, 230, 39, 30, 19);
    buttonChoose.setColorBackground(red_);
    buttonAgain.setColorBackground(red_);
    buttonAbort.setColorBackground(red_);
}

void draw() {
  background(bkgcolor);
  if(sendData && linesToSend!=null)
  {
    if(waitForResponse)
    {
      while(serial.available()>0)
      {
        char inChar = (char)serial.readChar();
        if(inChar=='\n')
        {
          if(inBuffer.toString().startsWith("ok"))
          {
            waitForResponse=false;
            currentLineIndex++;
          }
          else if(inBuffer.toString().startsWith("start"))
          {
            waitForResponse=false;
            statusline.setValue("Ready");
          }
          else
          {
            statusline.setValue(inBuffer.toString());
            sendData = false;
            buttonChoose.setColorBackground(green_);
            buttonAgain.setColorBackground(green_);
            buttonAbort.setColorBackground(red_);
            waitForResponse=false;
          }
          inBuffer.setLength(0);  
          break;
        }
        else
          inBuffer.append(inChar);
      }
    }
    else
    {
      if(currentLineIndex<linesToSend.length)
      {
        String currentLine = linesToSend[currentLineIndex];
        statusline.setValue("Line "+currentLineIndex+": "+currentLine);
        serial.write(currentLine);
        serial.write(0x0d);
        waitForResponse = true;
      }
      else
      {
        sendData = false;
        statusline.setValue("Send complete");
        buttonChoose.setColorBackground(green_);
        buttonAgain.setColorBackground(green_);
        buttonAbort.setColorBackground(red_);
      }
    }
  }
}

public void controlEvent(ControlEvent theEvent) {
  if (theEvent.isGroup())
    if (theEvent.name()=="portComList") InitSerial(theEvent.group().value()); // initialize the serial port selected
}

public void SEND() {
  if(init_com && !sendData)
  {
    try { 
      UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName()); 
    } catch (Exception e) { 
      e.printStackTrace();  
    } 
     
    // create a file chooser 
    final JFileChooser fc = new JFileChooser(); 
    fc.setFileFilter(new javax.swing.filechooser.FileFilter()
      {
          public boolean accept(File f) { if(f.getName().endsWith(".gcode")) return true; return false; }
          public String getDescription() { return "GCode Files"; }
      }
    );
    
    // in response to a button click: 
    int returnVal = fc.showOpenDialog(this);  
    if (returnVal == JFileChooser.APPROVE_OPTION) { 
      File file = fc.getSelectedFile(); 
      linesToSend = loadStrings(file); 
      currentLineIndex = 0;
      waitForResponse = false;
      sendData=true;
      buttonChoose.setColorBackground(red_);
      buttonAgain.setColorBackground(red_);
      buttonAbort.setColorBackground(green_);
    }
  }
}

public void AGAIN() {
    if(init_com && linesToSend!=null)
    {
      currentLineIndex = 0;
      waitForResponse = false;
      sendData=true;
      buttonChoose.setColorBackground(red_);
      buttonAgain.setColorBackground(red_);
      buttonAbort.setColorBackground(green_);
    }
}

public void ABORT() {
    if(init_com && sendData)
    {
      sendData=false;
      serial.write("M18");
      serial.write(0x0d);
      statusline.setValue("Aborted");

      buttonChoose.setColorBackground(green_);
      buttonAgain.setColorBackground(green_);
      buttonAbort.setColorBackground(red_);
    }
}

// Truncates a long port name for better (readable) display in the GUI
String shortifyPortName(String portName, int maxlen)
{
  String shortName = portName;
  if(shortName.startsWith("/dev/"))
    shortName = shortName.substring(5);
    
  if(portName.length()>maxlen)
  {
     shortName = shortName.substring(0,(maxlen-1)/2) + "~" +shortName.substring(shortName.length()-(maxlen-(maxlen-1)/2));
  }
  return shortName;
}

// initialize the serial port selected in the listBox
void InitSerial(float portValue) {
  String portPos = Serial.list()[int(portValue)];
  txtlblWhichcom.setValue("COM = " + shortifyPortName(portPos, 16));
  serial = new Serial(this, portPos, 115200);
  init_com=true;
  buttonChoose.setColorBackground(green_);
  commListbox.setColorBackground(green_);
}
