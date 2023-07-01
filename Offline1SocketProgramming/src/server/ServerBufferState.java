package server;

import util.BufferChunk;

import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class ServerBufferState {
    volatile long occupiedSize;
    final Map<Integer, List<BufferChunk>> currentBuffer;  // map between fileId and list of byte arrays

    public ServerBufferState() {
        currentBuffer = new ConcurrentHashMap<>();
        occupiedSize = 0;
    }

    void addChunk(int fileId, byte[] bytes, int chunkSize) {
        // check whether first transmission, if yes, initialize the list
        if (!currentBuffer.containsKey(fileId)) {
            currentBuffer.put(fileId, new ArrayList<>());
        }

        currentBuffer.get(fileId).add(new BufferChunk(bytes, chunkSize));
        synchronized (this) {
            occupiedSize += chunkSize;
        }
    }

    boolean isFileComplete(int fileId, long fileSize) {
        if(!currentBuffer.containsKey(fileId)) {
            return false;
        }

        long totalSize = 0;
        for(BufferChunk bufferChunk : currentBuffer.get(fileId)) {
            totalSize += bufferChunk.chunkSize;
        }

        return totalSize == fileSize;
    }

    void writeFileToOutputBuffer(int fileId, FileOutputStream fos) throws IOException {
        for(BufferChunk bufferChunk : currentBuffer.get(fileId)) {
            fos.write(bufferChunk.bytes, 0, bufferChunk.chunkSize);
        }
        fos.close();
        removeFileFromBuffer(fileId);
    }

    void removeFileFromBuffer(int fileId) {
        if(currentBuffer.containsKey(fileId)) {
            long totalBytesRemoved = 0;
            for(BufferChunk bufferChunk : currentBuffer.get(fileId)) {
                totalBytesRemoved += bufferChunk.chunkSize;
            }

            synchronized (this) {
                occupiedSize -= totalBytesRemoved;
            }
            currentBuffer.remove(fileId);
        }
    }
}
