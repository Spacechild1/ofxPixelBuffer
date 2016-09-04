#include "ofxPixelBuffer.h"


/// ofxPixelBuffer classes

/// ofxPixelBuffer

ofxPixelBuffer::ofxPixelBuffer(){
    myWidth = 0;
    myHeight = 0;
    myChannels = 0;
    mySize = 0;
    myFrameSize = 0;
    bAllocated = false;
    myLoader = nullptr;
    bThreaded = false;
}

ofxPixelBuffer::ofxPixelBuffer(int width, int height, int channels, int frames)
    : ofxPixelBuffer() {
    allocate(width, height, channels, frames);
}

ofxPixelBuffer::ofxPixelBuffer(const ofPixels& pix, int frames)
    : ofxPixelBuffer() {
    allocate(pix, frames);
}

ofxPixelBuffer::ofxPixelBuffer(const ofxPixelBuffer& mom){
    if (&mom == this){
        return;
    } else {
        if (mom.bAllocated){
            myWidth = mom.myWidth;
            myHeight = mom.myHeight;
            myChannels = mom.myChannels;
            mySize = mom.mySize;
            myFrameSize = mom.myFrameSize;
            myBuffer = mom.myBuffer;
            lerpPixels = mom.lerpPixels;
            bAllocated = true;
        } else {
        cout << "couldn't copy: mom not allocated!\n";
        }
    }
}

ofxPixelBuffer& ofxPixelBuffer::operator= (const ofxPixelBuffer& mom){
    if (&mom == this){
        return *this;
    } else {
        if (mom.bAllocated){
            myWidth = mom.myWidth;
            myHeight = mom.myHeight;
            myChannels = mom.myChannels;
            mySize = mom.mySize;
            myFrameSize = mom.myFrameSize;
            myBuffer = mom.myBuffer;
            lerpPixels = mom.lerpPixels;
            bAllocated = true;
        } else {
            cout << "couldn't copy: mom not allocated!\n";
        }
        return *this;
    }
}

ofxPixelBuffer::ofxPixelBuffer(ofxPixelBuffer&& mom){
    if (mom.bAllocated){
        myWidth = mom.myWidth;
        myHeight = mom.myHeight;
        myChannels = mom.myChannels;
        mySize = mom.mySize;
        myFrameSize = mom.myFrameSize;
        myBuffer = move(mom.myBuffer);
        lerpPixels = move(mom.lerpPixels);
        bAllocated = true;
    } else {
        cout << "couldn't move: mom not allocated!\n";
    }
}

ofxPixelBuffer& ofxPixelBuffer::operator= (ofxPixelBuffer&& mom){
    if (mom.bAllocated){
        myWidth = mom.myWidth;
        myHeight = mom.myHeight;
        myChannels = mom.myChannels;
        mySize = mom.mySize;
        myFrameSize = mom.myFrameSize;
        myBuffer = move(mom.myBuffer);
        lerpPixels = move(mom.lerpPixels);
        bAllocated = true;
    } else {
        cout << "couldn't move: mom not allocated!\n";
    }
    return *this;
}

void ofxPixelBuffer::allocate(int width, int height, int channels, int frames){
    width = std::max(0, width);
    height = std::max(0, height);
    if (channels < 1 || channels > 4){
        channels = 0;
    }

    if (width*height*channels > 0 && frames > 0) {
        lerpPixels.allocate(width, height, channels);
        myWidth = width;
        myHeight = height;
        myChannels = channels;
        myFrameSize = width * height * channels;
        myBuffer.clear();
        lerpPixels.allocate(width, height, channels);
        bAllocated = true;
		// resize buffer and allocate ofPixels
        resize(frames);
    } else {
        cout << "couldn't allocate - bad dimensions!\n";
        return;
    }
}

void ofxPixelBuffer::allocate(const ofPixels& pix, int frames){
    allocate(pix.getWidth(), pix.getHeight(), pix.getNumChannels(), frames);
}

void ofxPixelBuffer::resize(int newSize){
    if (bAllocated){
        int oldSize = mySize;
        int diff = newSize - oldSize;
        myBuffer.resize(newSize);
        mySize = newSize;
		// allocate ofPixels if new size is larger than old size
        if (diff > 0){
            for(int i = 0; i<diff; ++i){
                ofPixels& pixRef = myBuffer[oldSize+i];
                pixRef.allocate(myWidth, myHeight, myChannels);

                unsigned char * pix = pixRef.getData();
                for (uint32_t i = 0; i < myFrameSize; ++i){
                    pix[i] = 0;
                }
            }
        }
    }
    else {
        cout << "not allocated yet!\n";
    }
}

void ofxPixelBuffer::clearBuffer(){
    myBuffer.clear();
    myWidth = 0;
    myHeight = 0;
    myChannels = 0;
    myFrameSize = 0;
    mySize = 0;
    bAllocated = false;
    lerpPixels.clear();
}

void ofxPixelBuffer::clearPixels(){
    for(int i = 0; i < mySize; ++i){
        unsigned char * pix = myBuffer[i].getData();
        for (uint32_t i = 0; i < myFrameSize; ++i){
            pix[i] = 0;
        }
    }
}

bool ofxPixelBuffer::loadImage(const string filePath, int bufferIndex){
    if (!bAllocated){
        cout << "not allocated!\n";
        return false;
    }
    if (mySize == 0){
        cout << "buffer has no frames!\n";
        return false;
    }

    ofImage image;
    image.setUseTexture(false);

    if (image.load(filePath)){
        int width = image.getWidth();
        int height = image.getHeight();
        int channels = image.getPixels().getNumChannels();

        if ((width != myWidth)||(height != myHeight)||(channels != myChannels)){
            cout << "wrong dimension!\n";
            return false;
        }

        bufferIndex = max(0, min(mySize-1, bufferIndex));
        myBuffer[bufferIndex] = image.getPixels();

        return true;
    } else {
        return false;
    }
}


int ofxPixelBuffer::loadMultiImage(const string filePath, int numFiles, int startIndex, int bufferOnset){
    ofImage image;
    // we don't need to load the image into a texture.
    image.setUseTexture(false);

    auto position = filePath.find("*");

    if (position == string::npos){
        cout << "path must contain wildcard [*]!\n";
        return -1; // couldn't find wildcard
    }

    // counter for actually loaded images.
    // all images have to have the same dimension.
    // images with wrong dimensions are skipped.
    // if numFiles = -1, the function will keep loading images until it can't find any more files

    int k = 0; // counting actually loaded images

    // special case: buffer is empty
    // load images and push_back
    if (mySize == 0){
        myBuffer.clear();

        startIndex = (startIndex < 0) ? 0 : startIndex;
        int endIndex = (numFiles < 0) ? 1000000 : numFiles + startIndex;

        for (int i = startIndex; i < endIndex; ++i){
            string newPath = filePath;
            newPath.replace(position, 1, ofToString(i));

            // try to load image
            if (image.load(newPath)){
                int width = image.getWidth();
                int height = image.getHeight();
                int channels = image.getPixels().getNumChannels();
                // first image determines the dimensions
                if (i == startIndex){
                    myWidth = width;
                    myHeight = height;
                    myChannels = channels;
                    myFrameSize = width*height*channels;
                    lerpPixels.allocate(width, height, channels);
                    bAllocated = true;
                    myBuffer.push_back(image.getPixels());
                    k++;
                }
                // compare with dimensions of the first image
                else if ((width != myWidth)||(height != myHeight)||(channels != myChannels)){
                    cout << "skip " << newPath << " - wrong dimension!\n";
                }
                else {
                    myBuffer.push_back(image.getPixels());
                    k++;
                }
            } else {
                // couldn't find any more images, stop looping
                break;
            }
        }
        // update buffer size variable
        mySize = k;
    }
    // default case: buffer is not empty, so we can write images to it.
    else {
        if (!bAllocated){
            cout << "buffer not allocated!\n";
            return -1;
        }

        bufferOnset = max(0, min(mySize-1, bufferOnset));
        numFiles = min(mySize - bufferOnset, numFiles);
        if (numFiles < 0){
            numFiles = mySize - bufferOnset;
        }

        startIndex = (startIndex < 0) ? 0 : startIndex;
        int endIndex = numFiles + startIndex;

        for (int i = startIndex; i < endIndex; ++i){
            string newPath = filePath;

            newPath.replace(position, 1, ofToString(i));

            if (image.load(newPath)){
                if ((image.getWidth() != myWidth)||(image.getHeight() != myHeight)||(image.getPixels().getNumChannels() != myChannels)){
                    cout << "skip " << newPath << " - wrong dimension!\n";
                }
                else {
                    myBuffer[k + bufferOnset] = image.getPixels();
                    k++;
                }
            } else {
                // just skip path, continue looping
                cout << "failed to load " << newPath << "!\n";
            }
        }
    }

    return k; // return number of successfully loaded images

}


bool ofxPixelBuffer::loadMovie(const string filePath, int numFrames, int frameOnset, int bufferOnset){
    if (!myLoader){          // loader has been set
        cout << "set movie loader first!\n";
        return false;
    }
    // check if loader is ofVideoPlayer
    if (auto* v = dynamic_cast<ofVideoPlayer*>(myLoader)){
        v->setUseTexture(false);
    }

    if (myLoader->load(filePath)){
        int width = myLoader->getWidth();
        int height = myLoader->getHeight();
        int channels;
        switch (myLoader->getPixelFormat()){
            case OF_PIXELS_GRAY:
                channels = 1;
                break;
            case OF_PIXELS_RGB:
                channels = 3;
                break;
            case OF_PIXELS_RGBA:
                channels = 4;
                break;
        }

        frameOnset = max(0, min(myLoader->getTotalNumFrames()-1, frameOnset));

        // negative numFrames -> till end of video
        if (numFrames < 0) {
            numFrames = max(1, myLoader->getTotalNumFrames() - frameOnset);
        }
        else {
            numFrames = max(1, min(numFrames, myLoader->getTotalNumFrames() - frameOnset));
        }

        // special case: buffer is empty, therefore resize the buffer to the number of frames and load everything
        if (mySize == 0) {
            allocate(width, height, channels, numFrames);
            // necessary on some threaded players (DS)
            if (bThreaded){
                myLoader->setSpeed(0);
                myLoader->play();
            }
            myLoader->setFrame(frameOnset);

            for (int i = 0; i < numFrames; ++i){
                // if threaded, wait till new frame has been loaded
                if (bThreaded){
                    while (true){
                        myLoader->update();
                        if (myLoader->isFrameNew()){
                            cout << "loaded frame " << myLoader->getCurrentFrame() << "\n";
                            break;
                        }
                    }
                }
                myBuffer[i] = myLoader->getPixels();
                myLoader->nextFrame();
            }
        }
        // default case: buffer is not empty, so we can write frames to it.
        else {
            if (!bAllocated){
                cout << "buffer not allocated!\n";
                return false;
            }

            if ((width != myWidth)||(height != myHeight)||(channels != myChannels)){
                cout << "wrong dimension!";
                return false;
            }

            bufferOnset = max(0, min(mySize-1, bufferOnset));
            int length = min(mySize - bufferOnset, numFrames);
            // necessary on some threaded players (DS)
            if (bThreaded){
                myLoader->setSpeed(0);
                myLoader->play();
            }
            myLoader->setFrame(frameOnset);

            for (int i = 0; i < length; ++i){
                // if threaded, wait till new frame has been loaded
                if (bThreaded){
                    while (true){
                        myLoader->update();
                        if (myLoader->isFrameNew()){
                            cout << "loaded frame " << myLoader->getCurrentFrame() << "\n";
                            break;
                        }
                    }
                }
                myBuffer[i+bufferOnset] = myLoader->getPixels();
                myLoader->nextFrame();
            }
        }
        // movie was successfully written into the buffer
        return true;
    } else {
        // couldn't load movie
        cout << "couldn't load movie!\n";
        return false;
    }
}

void ofxPixelBuffer::setMovieLoader(ofBaseVideoPlayer &loader, bool isThreaded){
    myLoader = &loader;
    bThreaded = isThreaded;
}

void ofxPixelBuffer::write(int index, const ofPixels& myPixels){
    if (mySize == 0){
        cout << "buffer has no frames!\n";
        return;
    }

    if ((myPixels.getWidth() != myWidth)||
        (myPixels.getHeight() != myHeight)||
        (myPixels.getNumChannels() != myChannels)
        ){
        cout << "wrong dimension!\n";
        return;
    }

    index = max(0, min(mySize-1, index));
    myBuffer[index] = myPixels;
}


const ofPixels& ofxPixelBuffer::read (int index) const {
    if (mySize > 0){
        index = max(0, min(mySize-1, index));
        return myBuffer[index];
    }
    else {
        cout << "buffer is empty!\n";
        return dummy;
    }
}

const ofPixels& ofxPixelBuffer::operator[] (int index) const {
    if (mySize > 0){
        index = max(0, min(mySize-1, index));
        return myBuffer[index];
    }
    else {
        cout << "buffer is empty!\n";
        return dummy;
    }
}


const ofPixels& ofxPixelBuffer::readLinear (float index) const {
    if (mySize > 0){
        index = max(0.f, min(mySize-0.0001f, index));
        int intPart = static_cast<int>(index);
        float floatPart = index-intPart;

        const unsigned char* pix1 = myBuffer[intPart].getData();
        const unsigned char* pix2;

        if (intPart == (mySize-1)){
            pix2 = myBuffer[0].getData();
        }
        else {
            pix2 = myBuffer[intPart+1].getData();
        }

        unsigned char* result = lerpPixels.getData();

        float a = 1 - floatPart;
        float b = floatPart;

        for (uint32_t i = 0; i < myFrameSize; ++i){
            result[i] = static_cast<unsigned char>(pix1[i] * a + pix2[i] * b);
        }

        return lerpPixels;
    }
    else {
        cout << "buffer is empty!\n";
        return dummy;
    }
}

void ofxPixelBuffer::pushFront(const ofPixels& myPixels){
    if (bAllocated){
        if ((myPixels.getWidth() != myWidth)||(myPixels.getHeight() != myHeight)||(myPixels.getNumChannels() != myChannels)){
            cout << "wrong dimension!";
            return;
        }

        myBuffer.push_front(myPixels);
        mySize = myBuffer.size();
    }
    else {
        myWidth = myPixels.getWidth();
        myHeight = myPixels.getHeight();
        myChannels = myPixels.getNumChannels();
        myFrameSize = myWidth*myHeight*myChannels;
        bAllocated = true;
        myBuffer.push_front(myPixels);
        mySize = myBuffer.size();
    }
}

void ofxPixelBuffer::pushFront(ofPixels&& myPixels){
    if (bAllocated){
        if ((myPixels.getWidth() != myWidth)||(myPixels.getHeight() != myHeight)||(myPixels.getNumChannels() != myChannels)){
            cout << "wrong dimension!";
            return;
        }

        myBuffer.push_front(move(myPixels));
        mySize = myBuffer.size();
    }
    else {
        myWidth = myPixels.getWidth();
        myHeight = myPixels.getHeight();
        myChannels = myPixels.getNumChannels();
        myFrameSize = myWidth*myHeight*myChannels;
        bAllocated = true;
        myBuffer.push_front(move(myPixels));
        mySize = myBuffer.size();
    }
}

ofPixels ofxPixelBuffer::popFront(){
    if (!bAllocated){
        cout << "buffer not allocated!\n";
    }
    if (mySize == 0){
        cout << "buffer already empty!\n";
    }

    ofPixels popPixels = myBuffer.front();
    myBuffer.pop_front();

    mySize = myBuffer.size();

    return popPixels;
}

ofPixels ofxPixelBuffer::popBack(){
    if (!bAllocated){
        cout << "buffer not allocated!\n";
    }
    if (mySize == 0){
        cout << "buffer already empty!\n";
    }

    ofPixels popPixels = myBuffer.back();
    myBuffer.pop_back();

    mySize = myBuffer.size();

    return popPixels;
}

void ofxPixelBuffer::pushBack(const ofPixels& myPixels){
    if (bAllocated){
        if ((myPixels.getWidth() != myWidth)||(myPixels.getHeight() != myHeight)||(myPixels.getNumChannels() != myChannels)){
            cout << "wrong dimension!";
            return;
        }

        myBuffer.push_back(myPixels);
        mySize = myBuffer.size();
    }
    else {
        myWidth = myPixels.getWidth();
        myHeight = myPixels.getHeight();
        myChannels = myPixels.getNumChannels();
        myFrameSize = myWidth*myHeight*myChannels;
        bAllocated = true;
        myBuffer.push_back(myPixels);
        mySize = myBuffer.size();
    }
}

void ofxPixelBuffer::pushBack(ofPixels&& myPixels){
    if (bAllocated){
        if ((myPixels.getWidth() != myWidth)||(myPixels.getHeight() != myHeight)||(myPixels.getNumChannels() != myChannels)){
            cout << "wrong dimension!";
            return;
        }

        myBuffer.push_back(move(myPixels));
        mySize = myBuffer.size();
    }
    else {
        myWidth = myPixels.getWidth();
        myHeight = myPixels.getHeight();
        myChannels = myPixels.getNumChannels();
        myFrameSize = myWidth*myHeight*myChannels;
        bAllocated = true;
        myBuffer.push_back(move(myPixels));
        mySize = myBuffer.size();
    }
}

void ofxPixelBuffer::replace(const ofxPixelBuffer& buffer, int index){
    if (!bAllocated){
        cout << "buffer not allocated!\n";
        return;
    }
    if (mySize == 0){
        cout << "buffer is empty!\n";
        return;
    }

    if ((buffer.myWidth != myWidth)||(buffer.myHeight != myHeight)||(buffer.myChannels != myChannels)){
        cout << "wrong dimension!";
        return;
    }

    index = max(0, min(mySize - 1, index));
    int length = min(mySize - index, buffer.mySize);

    for (int i = 0; i < length; ++i){
        myBuffer[i + index] = buffer.myBuffer[i];
    }
}

// only makes sense if ofPixels has move assignment
void ofxPixelBuffer::replace(ofxPixelBuffer&& buffer, int index){
    if (!bAllocated){
        cout << "buffer not allocated!\n";
        return;
    }
    if (mySize == 0){
        cout << "buffer is empty!\n";
        return;
    }

    if ((buffer.myWidth != myWidth)||(buffer.myHeight != myHeight)||(buffer.myChannels != myChannels)){
        cout << "wrong dimension!";
        return;
    }

    index = max(0, min(mySize - 1, index));
    int length = min(mySize - index, buffer.mySize);

    for (int i = 0; i < length; ++i){
        myBuffer[i + index] = move(buffer.myBuffer[i]);
    }
}

void ofxPixelBuffer::insert(const ofxPixelBuffer& buffer, int index){
    if (!bAllocated){
        cout << "buffer not allocated!\n";
        return;
    }
    if (mySize == 0){
        cout << "buffer is empty!\n";
        return;
    }

    if ((buffer.myWidth != myWidth)||(buffer.myHeight != myHeight)||(buffer.myChannels != myChannels)){
        cout << "wrong dimension!";
        return;
    }

    index = max(0, min(mySize - 1, index));
    myBuffer.insert(myBuffer.begin() + index, buffer.myBuffer.begin(), buffer.myBuffer.end());
    mySize = myBuffer.size();

}

// only makes sense if ofPixels have move assignment
void ofxPixelBuffer::insert(ofxPixelBuffer&& buffer, int index){
    if (!bAllocated){
        cout << "buffer not allocated!\n";
        return;
    }
    if (mySize == 0){
        cout << "buffer is empty!\n";
        return;
    }

    if ((buffer.myWidth != myWidth)||(buffer.myHeight != myHeight)||(buffer.myChannels != myChannels)){
        cout << "wrong dimension!";
        return;
    }

    index = max(0, min(mySize - 1, index));
    myBuffer.insert(myBuffer.begin() + index, buffer.myBuffer.begin(), buffer.myBuffer.end());
    mySize = myBuffer.size();

}

void ofxPixelBuffer::remove(int index, int numFrames){
    if (!bAllocated){
        cout << "buffer not allocated!\n";
        return;
    }
    if (mySize == 0){
        cout << "buffer is empty!\n";
        return;
    }

    index = max(0, min(mySize - 1, index));
    int length;
    // negative argument means 'till the end of buffer'
    if (numFrames < 0){
        length = mySize - index;
    } else {
        length = min(mySize - index, numFrames);
    }

    myBuffer.erase(myBuffer.begin() + index, myBuffer.begin() + index + numFrames);
    mySize = myBuffer.size();

}

ofxPixelBuffer ofxPixelBuffer::getCopy(int index, int numFrames){
    if (!bAllocated){
        cout << "buffer not allocated!\n";
    }
    if (mySize == 0){
        cout << "buffer is empty!\n";
    }

    ofxPixelBuffer newBuffer;
    newBuffer.myWidth = myWidth;
    newBuffer.myHeight = myHeight;
    newBuffer.myChannels = myChannels;
    newBuffer.myFrameSize = myFrameSize;
    newBuffer.bAllocated = true;

    index = max(0, min(mySize-1, index));
    int length;
    // negative argument means 'till the end of buffer'
    if (numFrames < 0){
        length = mySize - index;
    } else {
        length = min(mySize - index, numFrames);
    }

    newBuffer.myBuffer.resize(length);
    newBuffer.mySize = length;

    for (int i = 0; i < length; ++i){
        newBuffer.myBuffer[i] = myBuffer[i + index];
    }

    return newBuffer;

}


//-------------------------------------------------------------------------


/// ofxPixelBufferRecorder

void ofxPixelBufferRecorder::setBuffer(ofxPixelBuffer& buffer){
    myBufferPtr = &buffer;
    myCounter = 0;
    bRecord = false;
}


void ofxPixelBufferRecorder::record(int onset, int numFrames){
    if (myBufferPtr == nullptr){
        cout << "\nset a buffer first!\n\n";
        return;
    }

    bRecord = true;
    myOnset = onset;
    myFrames = numFrames;
    myCounter = 0;

}

void ofxPixelBufferRecorder::stop(){
    if (myBufferPtr == nullptr){
        cout << "\nset a buffer first!\n\n";
        return;
    }

    bRecord = false;
}

void ofxPixelBufferRecorder::resume(){
    if (myBufferPtr == nullptr){
        cout << "\nset a buffer first!\n\n";
        return;
    }

    bRecord = true;
}

void ofxPixelBufferRecorder::in(const ofPixels& myPixels){
    if (myBufferPtr == nullptr){
        cout << "set a buffer first!\n\n";
        return;
    }

    if (bRecord){

        if((myPixels.getWidth()!= myBufferPtr->getWidth()) ||
            (myPixels.getHeight() != myBufferPtr->getHeight()) ||
            (myPixels.getNumChannels() != myBufferPtr->getNumChannels())
           ){
            cout << "wrong dimensions!\n";
            return;
        }

        int onset, length;

        onset = max(0, min(myBufferPtr->size()-1, myOnset));

        // if myFrames is negativ (e.g. -1) record till end of buffer
        if (myFrames < 0){
            length = myBufferPtr->size() - onset;
        }
        else {
            length = max(1, min(myBufferPtr->size() - onset, myFrames));
        }

        if (myCounter >= length){
            bRecord = false;
            return;
        }

        myBufferPtr->write(myCounter + onset, myPixels);

        myCounter++;

    }

}

//----------------------------------------------------------------------------

/// ofxPixelRingBuffer

void ofxPixelRingBuffer::in(const ofPixels& myPixels){
    myBuffer.write(myIndex, myPixels);
    myIndex--;
    if(myIndex < 0){
        myIndex = myBuffer.size() - 1;
    }
}

const ofPixels& ofxPixelRingBuffer::read(int index) const{
    int length = myBuffer.size();
    // limit index
    index = max(0, min(length - 1, index));
    // add 1 to compensate for decrementing the myIndex in ofxPixelRingBuffer::in()
    return myBuffer.read((index + myIndex + 1) % length);
}

ofPixels ofxPixelRingBuffer::readLinear(float index) const{
    float length = static_cast<float>(myBuffer.size());
    // limit index
    index = max(0.f, min(myBuffer.size() - 1.f, index));
    // add 1 to compensate for decrementing the myIndex in ofxPixelRingBuffer::in()
    float k = index + myIndex + 1.f;
    k = fmodf(k, length);
    return myBuffer.readLinear(k);
}


//------------------------------------------------------------------------------

/// ofxPixelBufferPlayer

ofxPixelBufferPlayer::ofxPixelBufferPlayer() {
    myBufferPtr = nullptr;
    lerpPixelsPtr = nullptr;
    bPlay = false;
    bLoop = false;
    bLoopNew = false;
    bPingPong = false;
    bLerp = false;
    myFrameRate = 30;
    myPosition = 0;
    myTime = 0;
    mySpeed = 1;
    myDirection = 1;
    myLoopOnset = 0;
    // hack
    myLoopSize = 1e+6;
    onset = myLoopOnset;
    size = myLoopSize;
    myLoopOnsetDev = 0;
    myLoopSizeDev = 0;
}

ofxPixelBufferPlayer::ofxPixelBufferPlayer(ofxPixelBuffer& buffer) {
    myBufferPtr = &buffer;
    lerpPixelsPtr = nullptr;
    bPlay = false;
    bLoop = false;
    bLoopNew = false;
    bPingPong = false;
    bLerp = false;
    myFrameRate = 30;
    myPosition = 0;
    myTime = 0;
    mySpeed = 1;
    myDirection = 1;
    myLoopOnset = 0;
    myLoopSize = getTotalNumFrames();
    onset = myLoopOnset;
    size = myLoopSize;
    myLoopOnsetDev = 0;
    myLoopSizeDev = 0;

}

void ofxPixelBufferPlayer::setBuffer(ofxPixelBuffer& buffer) {
    myBufferPtr = &buffer;
}

bool ofxPixelBufferPlayer::hasBuffer() const {
    return (myBufferPtr != nullptr);
}

void ofxPixelBufferPlayer::update(){
    if (myBufferPtr == nullptr){
        cout << "set buffer first!";
        return;
    }

    if (!myBufferPtr->isAllocated()){
        cout << "buffer not allocated!\n";
        return;
    }

    float length = myBufferPtr->size() - 1.f;

    if (bPlay){
        int64_t newTime = ofGetElapsedTimeMicros();
        float delta = (newTime - oldTime)/1000000.f;
        oldTime = newTime;
        myTime += delta;

        if (bLoop){
            // take myDirection into account
            myPosition += delta*mySpeed*myDirection*myFrameRate;

            onset = max(0.f, min(length, onset));
            size = max(0.f, min(length - onset, size));

            // if speed is positive, we only care if we go beyond the 'right' bound of the loop
            // because we might enter it from the 'left' side.
            if (mySpeed * myDirection >= 0) {
                if (myPosition > (onset + size)){

                    bLoopNew = true;
                    // this has to go here (and not after setting the position), otherwise the loops will be chaotic
                    if (myLoopOnsetDev > 0) {
                        onset = myLoopOnset + ofRandomf() * myLoopOnsetDev;
                    } else {
                        onset = myLoopOnset;
                    }
                    if (myLoopSizeDev > 0) {
                        size = myLoopSize + ofRandomf() * myLoopSizeDev;
                    } else {
                        size = myLoopSize;
                    }

                    // set new position
                    if (bPingPong){
                        myDirection *= -1;
                        myPosition = onset + size;
                    } else {
                        myPosition = onset;
                    }

                } else {
                    bLoopNew = false;
                }
            } else {
            // if speed is negative, we only care if we go beyond the 'left' bound of the loop
            // because we might enter it from the 'right' side.
                if (myPosition < onset){

                    bLoopNew = true;
                    // this has to go here (and not after setting the position), otherwise the loops will be chaotic
                    if (myLoopOnsetDev > 0) {
                        onset = myLoopOnset + ofRandomf() * myLoopOnsetDev;
                    } else {
                        onset = myLoopOnset;
                    }
                    if (myLoopSizeDev > 0) {
                        size = myLoopSize + ofRandomf() * myLoopSizeDev;
                    } else {
                        size = myLoopSize;
                    }

                    // set new position
                    if (bPingPong){
                        myDirection *= -1;
                        myPosition = onset;
                    } else {
                        myPosition = onset + size;
                    }

                } else {
                    bLoopNew = false;
                }
            }
        }
        else {
            // ignore myDirection if not looping!
            myPosition += delta*mySpeed*myFrameRate;

            if ((myPosition < 0)||(myPosition >= length)){
                bPlay = false;
                myTime = 0;
            }
        }
        // check for boundaries
        myPosition = max(0.f, min(length, myPosition));
        // update lerpPixels if linear interpolation is turned on
        if (bLerp){
            // ofxPixelBuffer::readLinear() calculates the interpolated pixels and returns a const reference
            // to its internal lerpPixels member - which we want to keep a pointer to.
            lerpPixelsPtr = &(myBufferPtr->readLinear(myPosition));
        }
    } else {
        // only check for boundaries:
        myPosition = max(0.f, min(length, myPosition));
    }


}


const ofPixels& ofxPixelBufferPlayer::getPixels() const {
    if (myBufferPtr == nullptr){
        cout << "set buffer first!\n";
        return dummy;
    }

    if (bLerp && lerpPixelsPtr){
        return *lerpPixelsPtr;
    } else {
        return myBufferPtr->read(static_cast<int>(myPosition + 0.5f)); // round to frame
    }
}


void ofxPixelBufferPlayer::play(float frameOnset){
    if (myBufferPtr == nullptr){
        cout << "set buffer first!\n";
        return;
    }

    myPosition = frameOnset;
    oldTime = ofGetElapsedTimeMicros();
    myTime = 0;
    // necessary so that jumping out of a ping pong loop works as expected
    myDirection = 1;
    bPlay = true;

}

void ofxPixelBufferPlayer::resetLoop(){
    if (myBufferPtr == nullptr){
        cout << "set buffer first!\n";
        return;
    }
    if (bLoop){
        float length = myBufferPtr->size()-1.f;

        if (myLoopOnsetDev > 0) {
            onset = max(0.f, (myLoopOnset + ofRandomf() * myLoopOnsetDev));
        } else {
            onset = myLoopOnset;
        }
        if (myLoopSizeDev > 0) {
            size = max(0.f, (myLoopSize + ofRandomf() * myLoopSizeDev));
        } else {
            size = myLoopSize;
        }

        // ignore myDirection
        if (mySpeed >= 0){
            // actual checking of onset is done in update() and in 'get' member functions
            myPosition = onset;
        } else {
            // actual checking for onset is done in update() and in 'get' member functions
            myPosition = onset + size;
        }

        if (!bPlay && bLerp){
            // here we should check
            myPosition = max(0.f, min(length, myPosition));
            lerpPixelsPtr = &(myBufferPtr->readLinear(myPosition));
        }
    }

}

void ofxPixelBufferPlayer::setLoopPingPong(bool mode){
    if (bPingPong != mode){
        bPingPong = mode;
        myDirection = 1;
    }
}

float ofxPixelBufferPlayer::getLoopSize() const {
    if (myBufferPtr == nullptr){
        cout << "set buffer first!\n";
        return 0;
    }

    return max(0.f, min(myBufferPtr->size() - 1.f - myLoopOnset, myLoopSize));
}

int ofxPixelBufferPlayer::getTotalNumFrames() const {
    if (myBufferPtr == nullptr){
        cout << "set buffer first!\n";
        return 0;
    }

    return myBufferPtr->size();
}

float ofxPixelBufferPlayer::getTotalDuration() const {
    if (myBufferPtr == nullptr){
        cout << "set buffer first!\n";
        return 0;
    }

    return myBufferPtr->size()/myFrameRate;
}

void ofxPixelBufferPlayer::setFrameRate(float fps) {
    if (fps > 0) {
        myFrameRate = fps;
    } else {
        cout << "bad framerate!\n";
    }
}

void ofxPixelBufferPlayer::setPosition(float frames){
    if (myBufferPtr == nullptr){
        cout << "set buffer first!\n";
        return;
    }

    if (!bPlay && bLerp){
        // check for boundaries
        float length = myBufferPtr->size()-1.f;
        myPosition = max(0.f, min(length, frames));
        // update lerpPixels
        lerpPixelsPtr = &(myBufferPtr->readLinear(myPosition));
    } else {
        // checking is not necessary
        myPosition = frames;
    }
    // necessary so that jumping out of a ping pong loop works as expected
    myDirection = 1;
}

float ofxPixelBufferPlayer::getPosition() const {
    if (myBufferPtr == nullptr){
        cout << "set buffer first!\n";
        return 0;
    }

    return max(0.f, min(myBufferPtr->size() - 1.f, myPosition));
}

void ofxPixelBufferPlayer::setRelativePosition(float position){
    setPosition(position * getTotalNumFrames());
}

bool ofxPixelBufferPlayer::isAllocated() const {
    if (myBufferPtr != nullptr){
        return myBufferPtr->isAllocated();
    } else {
        return false;
    }
}

int ofxPixelBufferPlayer::getWidth() const {
     if (myBufferPtr != nullptr){
        return myBufferPtr->getWidth();
    } else {
        return -1;
    }
}

int ofxPixelBufferPlayer::getHeight() const {
     if (myBufferPtr != nullptr){
        return myBufferPtr->getHeight();
    } else {
        return -1;
    }
}

int ofxPixelBufferPlayer::getNumChannels() const {
     if (myBufferPtr != nullptr){
        return myBufferPtr->getNumChannels();
    } else {
        return -1;
    }
}


