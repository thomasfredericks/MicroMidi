#ifndef _MICRO_MIDI_
#define _MICRO_MIDI_


/*
THE FOLLOWING DOCUMENTATION WAS EXTREMILY HELPFUL IN MAKING THIS LIBRARY
- http://midi.teragonaudio.com/tech/midispec/run.htm
- https://learn.sparkfun.com/tutorials/midi-tutorial/introduction
*/


#ifndef __MICRO_LOG__
#define LOG(...)
#endif






class MicroMidi {



public:

  const static int  PITCH_BEND_CENTER = 8192;
  typedef enum  {
    START = 0xFA,
    CLOCK = 0xF8,
    STOP = 0xFC,
    CONTINUE = 0XFB
  } REALTIME;


  void (*noteOnCallback)(byte channel, byte note, byte velocity);
  void (*noteOffCallback)(byte channel, byte note);
  void (*controlChangeCallback) (byte channel, byte controller, byte value);
  void (*pitchBendCallback) (byte channel, int pitch);
  void (*realtimeCallback) (REALTIME type);
  void (*channelPressureCallback) (byte channel, byte pressure);
private:
  //byte midiChannel;
  byte runningStatusIn;

  uint8_t midiMessageLength = 0;
  byte midiMessage[3];
  Stream * stream;

  const static byte MICRO_MIDI_NOTE_OFF = 0x80;
  const static byte MICRO_MIDI_NOTE_ON = 0x90;
  const static byte MICRO_MIDI_CTL = 0xB0;
  const static byte MICRO_MIDI_BEND = 0xE0;
  const static byte MICRO_MIDI_CHANNEL_PRESSURE = 0xD0;


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

  void setMidiPitchBendCallback(void (*fptr)(byte channel, int pitch)) {
    pitchBendCallback = fptr;
  }

  void setMidiRealtimeCallback(void (*fptr)(REALTIME type)) {
    realtimeCallback = fptr;
  }

  void setMidiChannelPressureCallback(void (*fptr)(byte channel, byte pressure)) {
    channelPressureCallback = fptr;
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

      //Serial.print("RAW ");
      //Serial.println(data);

      if (data < 0x80) { // 0x80 == 128 == B10000000
        if ( runningStatusIn  ) {

          midiMessage[midiMessageLength] = data;
          midiMessageLength++;

          if ( midiMessageLength == 1 ) {
            // GET RID OF CHANNEL DATA
            byte messageType = data & B11110000;
            if ( messageType == MICRO_MIDI_CHANNEL_PRESSURE  ) {
              // GET CHANNEL DATA AND OFFSET IT BY 1
              byte midiChannel = (data & B00001111) + 1;
              if ( channelPressureCallback) channelPressureCallback(midiChannel, midiMessage[0] );
              midiMessageLength = 0;
            }
          } else if ( midiMessageLength == 2 ) {
            // GET RID OF CHANNEL DATA
            byte messageType = data & B11110000;
            byte midiChannel = (data & B00001111) + 1;
            switch ( messageType ) {
            case MICRO_MIDI_NOTE_ON :
              if ( midiMessage[1] > 0  ) {
                //LOG("MicroMidi:NoteOn", midiChannel, midiMessage[0], midiMessage[1]);
                if ( noteOnCallback) noteOnCallback(midiChannel, midiMessage[0], midiMessage[1]);
              } else {
                //LOG("MicroMidi:NoteOff", midiChannel, midiMessage[0]);
                if ( noteOffCallback ) noteOffCallback(midiChannel, midiMessage[0] );
              }
              break;
            case MICRO_MIDI_NOTE_OFF :
              //LOG("MicroMidi:NoteOff", midiChannel, midiMessage[0]);
              if ( noteOffCallback ) noteOffCallback(midiChannel, midiMessage[0] );
              break;
            case MICRO_MIDI_CTL :
              if ( controlChangeCallback ) controlChangeCallback(midiChannel, midiMessage[0] , midiMessage[1]);
              break;
            case MICRO_MIDI_BEND :
              if ( pitchBendCallback) pitchBendCallback(midiChannel, midiMessage[0] + (midiMessage[1] << 7));
              break;
            }

            // SHOULD HAVE MATCHED, RESETTING
            midiMessageLength = 0;
          }
        } else {
          // NO RUNNING STATUS, IGNORING
          midiMessageLength = 0;
        }

      } else {

        // realtime messages
        if ( data >= 0xF8) {

          if ( realtimeCallback ) realtimeCallback((REALTIME)data);

          // system common
        } if ( data >= 0xF0 ) {
          runningStatusIn = 0;
          midiMessageLength = 0;

          // voice messages
        } else {
          runningStatusIn = data;
          /*
          // GET RID OF CHANNEL DATA
          runningStatusIn = data & B11110000;
          // GET CHANNEL DATA AND OFFSET IT BY 1
          midiChannel = (data & B00001111) + 1;
          */
          midiMessageLength = 0;
        }

      }

      //dd(indata);
    }
  }

  // Copied from https://github.com/little-scale/mtof created by Sebastian Tomczak, 25 March 2017
  static float midiToFrequency( float note , float baseFrequency, float baseNote) {
    return baseFrequency * pow (2.0, (note - baseNote) / 12.0);
  }
  static float midiToFrequency(float note) {
    return midiToFrequency(note, 440.0, 69.0);
  }

  // Copied from https://github.com/little-scale/mtof created by Sebastian Tomczak, 25 March 2017
  static float frequencyToMidi(float frequency, float baseFrequency, float baseNote) {
    return baseNote + (12.0 * log(frequency / baseFrequency) / log(2));
  }
  static float frequencyToMidi(float frequency) {
    return frequencyToMidi(frequency, 440.0, 69.0);
  }



  // NESTED CLASS
  template <int L>
  class LastNoteHeldTable {
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
        data[L - 1] = note;
      }

    }

    void clear() {
      count = 0;
    }

    int getCount() {
      return count;
    }

    byte getActiveNote() {
      if ( count ) return data[count - 1];
      else return 0;
    }

  };

};
#endif