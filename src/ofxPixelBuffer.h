#pragma once

#include "ofMain.h"

/// ofxPixelBuffer classes


class ofxPixelBuffer {
    protected:
        deque<ofPixels> myBuffer;
        int myWidth, myHeight, myChannels, mySize;
        uint32_t myFrameSize;
        bool bAllocated;
        ofBaseVideoPlayer* myLoader;
        bool bThreaded;
        ofPixels dummy;
    public:
        // constructors
        ofxPixelBuffer();
        ofxPixelBuffer(int width, int height, int channels, int frames);
        ofxPixelBuffer(const ofPixels& pix, int frames);
        // destructor
        virtual ~ofxPixelBuffer() {}
        // copy constructor and assignment:
        ofxPixelBuffer(const ofxPixelBuffer& mom);
        ofxPixelBuffer& operator= (const ofxPixelBuffer& mom);
        // move constructor and assignment:
        ofxPixelBuffer(ofxPixelBuffer&& mom);
        ofxPixelBuffer& operator= (ofxPixelBuffer&& mom);

        void allocate(int width, int height, int channels, int frames);
        void allocate(const ofPixels& pix, int frames);
        void resize(int size);
        void clearBuffer();
        void clearPixels();
        bool loadImage(const string filePath, int bufferOnset);
        int loadMultiImage(const string filePath, int numFiles = -1, int startIndex = 0, int bufferOnset = 0);
        bool loadMovie(const string filePath, int numFrames = -1, int frameOnset = 0, int bufferOnset = 0);
        void setMovieLoader(ofBaseVideoPlayer& loader, bool isThreaded = false);

        void write(int index, const ofPixels& myPixels);
        const ofPixels& read (int index) const;
        const ofPixels& operator[] (int index) const;
        // read with linear interpolation. returns new ofPixels object.
        ofPixels readLinear (float index) const;

        void pushFront(const ofPixels& myPixels);
        void pushFront(ofPixels&& myPixels);
        void pushBack(const ofPixels& myPixels); // could pushing trigger a deque resize and therefore invalidate references obtained through 'read'?
        void pushBack(ofPixels&& myPixels);
        ofPixels popFront();
        ofPixels popBack(); // could popping trigger a deque resize and therefore invalidate references obtained through 'read'?
        void replace(const ofxPixelBuffer& buffer, int index = 0);
        void replace(ofxPixelBuffer&& buffer, int index = 0);
        void insert(const ofxPixelBuffer& buffer, int index);
        void insert(ofxPixelBuffer&& buffer, int index);
        void remove(int index, int numFrames = -1); // -1 (negative) = till end of buffer
        ofxPixelBuffer getCopy(int index, int numFrames = -1); // -1 (negative) = till end of buffer

        int getWidth() const {return myWidth;}
        int getHeight() const {return myHeight;}
        int getNumChannels() const {return myChannels;}
        int size() const {return mySize;}
        bool isAllocated() const {return bAllocated;}
};

class ofxPixelBufferRecorder {
    protected:
        ofxPixelBuffer* myBufferPtr;
        int myCounter;
        int myOnset;
        int myFrames;
        bool bRecord;
    public:
        ofxPixelBufferRecorder() {myBufferPtr = nullptr; bRecord = false; myCounter = 0;}
        ofxPixelBufferRecorder(ofxPixelBuffer& buffer) {setBuffer(buffer);}

        void setBuffer(ofxPixelBuffer& buffer);
        ofxPixelBuffer& getBuffer() {return *myBufferPtr;}
        const ofxPixelBuffer& getBuffer() const {return *myBufferPtr;}

        void record(int onset = 0, int numFrames = -1);
        void stop();
        void resume();
        void in(const ofPixels& myPixels);
        int getRecordedFrames() const {return myCounter;}
        int getCurrentIndex() const {return myOnset + myCounter;}
};

class ofxPixelRingBuffer {
    protected:
        ofxPixelBuffer myBuffer;
        int myIndex;
    public:
        ofxPixelRingBuffer() {myIndex = 0;}
        ofxPixelRingBuffer(int width, int height, int channels, int frames){myBuffer.allocate(width, height, channels, frames); myIndex = 0;}

        void allocate(int width, int height, int channels, int frames){myBuffer.allocate(width, height, channels, frames); myIndex = 0;}
        void in(const ofPixels& myPixels);
        const ofPixels& read(int index) const;
        ofPixels readLinear(float index) const;
        void resize(int size){myBuffer.resize(size);}
        void clearBuffer(){myBuffer.clearPixels();}
        const ofxPixelBuffer& getBuffer() const {return myBuffer;}
        ofxPixelBuffer& getBuffer() {return myBuffer;}
        int getBufferPosition() {return myIndex;}
        bool isAllocated() {return myBuffer.isAllocated();}
        int getWidth() const {return myBuffer.getWidth();}
        int getHeight() const {return myBuffer.getHeight();}
        int getNumChannels() const {return myBuffer.getNumChannels();}

};


class ofxPixelBufferPlayer {
    protected:
        ofxPixelBuffer* myBufferPtr;
        ofPixels lerpPixels; // pointer to internal lerpPixels of ofxPixelBuffer
        ofPixels dummy; // dummy ofPixels to return if something goes wrong

        int64_t oldTime;
        bool bPlay;
        bool bLoop;
        bool bLoopNew;
        bool bPingPong;
        bool bLerp;
        float myFrameRate;
        float myPosition;
        float myTime;
        float mySpeed;
        int myDirection;
        float myLoopOnset;
        float myLoopSize;
        float myLoopOnsetDev;
        float myLoopSizeDev;
        float onset;
        float size;

    public:
        ofxPixelBufferPlayer();
        ofxPixelBufferPlayer(ofxPixelBuffer& buffer);

        void setBuffer(ofxPixelBuffer& buffer);
        bool hasBuffer() const;
        ofxPixelBuffer* getBuffer() { return myBufferPtr; }
        const ofxPixelBuffer* getBuffer() const { return myBufferPtr; }

        void update();
        const ofPixels& getPixels() const;
        void setInterpolation(bool mode) {bLerp = mode;}
        bool getInterpolation() {return bLerp;}

        void play(float frameOnset = 0);
        void stop() {bPlay = false; myTime = 0;}
        void pause() {bPlay = false;}
        void resume(){bPlay = true; oldTime = ofGetElapsedTimeMicros();}
        bool isPlaying() const {return bPlay;}

        void resetLoop();
        void startLoop() {resetLoop(); resume(); }
        bool isLoopNew() const {return bLoopNew;}
        void setLoopState (bool state) {bLoop = state;}
        bool getLoopState() const {return bLoop;}
        void setLoopPingPong(bool mode);
        bool getLoopPingPong(){return bPingPong;}
        void setLoopOnset (float frames) {myLoopOnset = max(0.f, frames); onset = myLoopOnset;}
        float getLoopOnset() const {return min(myBufferPtr->size() - 1.f, myLoopOnset);}
        void setLoopSize (float frames) {myLoopSize = max(0.f, frames); size = myLoopSize;}
        float getLoopSize() const;
        void setLoopOnsetDeviation(float frames) {myLoopOnsetDev = max(0.f, frames);}
        float getLoopOnsetDeviation() {return myLoopOnsetDev;}
        void setLoopSizeDeviation(float frames) {myLoopSizeDev = max(0.f, frames);}
        float getLoopSizeDeviation() {return myLoopSizeDev;}

        float getTotalDuration() const;
        int getTotalNumFrames() const;
        float getPassedTime() const {return myTime;}
        void setFrameRate(float fps);
        float getFrameRate() const {return myFrameRate;}
        void setPosition(float frames);
        float getPosition() const;
        void setRelativePosition(float position);
        float getRelativePosition() const {return getPosition()/getTotalNumFrames();}
        void setSpeed (float speed) {mySpeed = speed;}
        float getSpeed() const {return mySpeed;}

        bool isAllocated() const;
        int getWidth() const;
        int getHeight() const;
        int getNumChannels() const;

};


