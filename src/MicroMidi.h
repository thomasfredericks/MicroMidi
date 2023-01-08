


class MicroMidi {

    void (*noteOnCallback)(byte channel, byte note, byte velocity);
    void (*noteOffCallback)(byte channel, byte note);
    void (*controlChangeCallback) (byte channel, byte controller, byte value);

  private:
    byte  midiType;
    byte midiChannel;

    uint8_t midiMessageLength = 0;
    byte midiMessage[3];
    Stream * stream;

    const static byte TINY_MIDI_NOTE_OFF = 0x80; 
    const static byte TINY_MIDI_NOTE_ON = 0x90; 
    const static byte TINY_MIDI_CTL = 0xB0;


  public:
   
    const static unsigned long MIDI_SERIAL_BAUD = 31250;
    const static int MIDI_SERIAL_CONFIG = SERIAL_8N1;


    /*
      #define MM_NOTE_OFF 0x08
      #define MM_NOTE_ON 0x09
      #define MM_CTL 0x0B
      #define MM_BAUD 31250
    */
    MicroMidi( Stream * stream) {
      this->stream = stream;
    }

    void setControlChangeCallback(void (*fptr)(byte channel, byte controller, byte value)) {
      controlChangeCallback = fptr;
    }

    void setMidiNoteOnCallback(void (*fptr)(byte channel, byte note, byte velocity)) {
      noteOnCallback = fptr;
    }

    void setMidiNoteOffCallback(void (*fptr)(byte channel, byte note)) {
      noteOffCallback = fptr;
    }
/*
    void dd(int indata) {
        Serial.print(indata);
        Serial.print("->");
        Serial.print(midiType);
        Serial.print(" ");
        Serial.print(midiMessageLength);
        Serial.print(" ");
        Serial.print(midiMessage[0]);
        Serial.print(" ");
        Serial.print(midiMessage[1]);
        Serial.println();
    }
*/
    void receiveMessages() {
      while (stream->available() ) {
        int data = stream->read();

        int indata= data;
        //Serial.print("RAW ");
        //Serial.println(data);

        if (data < 0x80) { // 0x80 == 128 == B10000000

          if ( midiType >= 0x80 && midiType < 0xF0  ) {

            midiMessage[midiMessageLength] = data;
            midiMessageLength++;

            /*
            Serial.print("midiBuffer ");
            Serial.print(midiMessage[0]);
            Serial.print(" ");

            if (midiMessageLength> 1) {
              Serial.println(midiMessage[1]);
            } else {
              Serial.println();
            }

 */

            

            if ( midiMessageLength == 2 ) {
             // dd(indata);
              
              if ( midiType == TINY_MIDI_NOTE_ON    ) {
                if ( midiMessage[1] > 0  ) {
                  //Serial.println("NOTE ON");
                  if ( noteOnCallback) noteOnCallback(midiChannel, midiMessage[0], midiMessage[1]);
                } else {
                 // Serial.println("NOTE OFF");
                  if ( noteOffCallback ) noteOffCallback(midiChannel, midiMessage[0] );
                }
                
              } else if ( midiType == TINY_MIDI_NOTE_OFF && noteOffCallback   ) {
               //Serial.println("NOTE ON");
                noteOffCallback(midiChannel, midiMessage[0] );

              } else if ( midiType == TINY_MIDI_CTL && controlChangeCallback  ) {
               //Serial.println("CTRL");
                controlChangeCallback(midiChannel, midiMessage[0] , midiMessage[1]);

              }
              // KEEP MIDI TYPE (AND CHANNEL) FOR RUNNING STATUS
              //midiType = 0;
             midiMessageLength = 0;
            }  
          } else {
            midiMessageLength = 0;
            //Serial.println("NSL");
          }
            /*
            else {
              dd(indata);
            }
            */
            //midiMessageLength = 0;

        } else {

          // GET RID OF CHANNEL DATA
          int incommingDataType = data & B11110000;
          
          //if ( data == TINY_MIDI_NOTE_OFF || data == TINY_MIDI_NOTE_ON || data == TINY_MIDI_CTL ) {
          /*if (data >= 8 && data < 15  )
            midiType = data;

          } else {
            midiType = 0;  
          }*/
          // VOICE MESSAGES
          if ( incommingDataType >= 0x80 && incommingDataType < 0xF0 ) {
            midiType = data;
            // GET CHANNEL DATA AND OFFSET IT BY 1
            midiChannel = (data & B00001111)+1;
          }            
          midiMessageLength = 0;
        }
        
        //dd(indata);
      }
    }
    };

template <int L>
class MicroMidiLastNoteHeldTable {
  byte data[L];
  int count = 0;
  void shiftNotes(int index) {
    count = count - 1;
    for (int i = index; i < count; i++) {
      data[i] = data[i + 1];
    }
  }
public:


  void removeNote(byte note) {
    for (int i = 0; i < count; i++) {
      if (data[i] == note) {
        data[i] = 0;
        shiftNotes(i);
        i--;
      }
    }
  }

  void addNote(byte note) {
    if (count < L) {
      data[count] = note;
      count++;
    } else {
      shiftNotes(0);
      data[L-1] = note;
    }
  
  }

  void clear(){
    count =0;
  }

  int getCount() {
    return count;
  }

  byte getActiveNote() {
    if ( count ) return data[count-1];
    else return 0;
  }


};