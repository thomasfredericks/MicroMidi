

class TinyMidi {

    void (*noteOnCallback)(byte channel, byte note, byte velocity);
    void (*noteOffCallback)(byte channel, byte note);
    void (*controlChangeCallback) (byte channel, byte controller, byte value);

  private:
    byte  midiType;
    byte midiChannel;

    uint8_t midiMessageLength = 0;
    byte midiMessage[3];
    Stream * stream;

  const static byte TINY_MIDI_NOTE_OFF = 0x08; // 0x80 >> 4 == 0x08 == 8 == B1000
    const static byte TINY_MIDI_NOTE_ON = 0x09; // 0x90 >> 4 == 0x09 == 9 == B1001
    const static byte TINY_MIDI_CTL = 0x0B;

  public:
   
    const static int BAUD = 31250;
    /*
      #define MM_NOTE_OFF 0x08
      #define MM_NOTE_ON 0x09
      #define MM_CTL 0x0B
      #define MM_BAUD 31250
    */
    TinyMidi( Stream * stream) {
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
  



        if (data < 0x80) { // 0x80 == 128 == B10000000
          if ( midiType ) {
            midiMessage[midiMessageLength] = data;
            midiMessageLength++;
            if ( midiMessageLength == 2 ) {
             // dd(indata);
              if ( midiType == TINY_MIDI_NOTE_ON    ) {
                if ( midiMessage[1] > 0  ) {
                  
                  if ( noteOnCallback) noteOnCallback(midiChannel, midiMessage[0], midiMessage[1]);
                } else {
                  
                  if ( noteOffCallback ) noteOffCallback(midiChannel, midiMessage[0] );
                }
                
              } else if ( midiType == TINY_MIDI_NOTE_OFF && noteOffCallback   ) {
               
                noteOffCallback(midiChannel, midiMessage[0] );

              } else if ( midiType == TINY_MIDI_CTL && controlChangeCallback  ) {
               
                controlChangeCallback(midiChannel, midiMessage[0] , midiMessage[1]);

              }

              midiType = 0;
            } 
            /*
            else {
              dd(indata);
            }
            */
          }
        } else {
          // GET CHANNEL DATA AND OFFSET IT BY 1
          midiChannel = (data & B00001111)+1;
          // GET RID OF CHANNEL DATA
          data = data >> 4;
          // ONLY HANDLE KNOWN MESSAGES
          if ( data == TINY_MIDI_NOTE_OFF || data == TINY_MIDI_NOTE_ON || data == TINY_MIDI_CTL ) {
            midiType = data;
          } else {
            midiType = 0;
          }
          midiMessageLength = 0;
        }
        
        //dd(indata);
      }
    }
};
